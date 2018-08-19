

#include "ConnectionCheck.h"

ConnectionCheck *ConnectionCheck::instance;

retained ConnectionCheckRetainedData ConnectionCheck::connectionCheckRetainedData;

ConnectionCheck::ConnectionCheck()  {
	instance = this;
	if (connectionCheckRetainedData.magic != CONNECTION_CHECK_MAGIC) {
		connectionCheckRetainedData.magic = CONNECTION_CHECK_MAGIC;
		connectionCheckRetainedData.numFailures = 0;
	}
}
ConnectionCheck::~ConnectionCheck() {

}

void ConnectionCheck::setup() {

}

void ConnectionCheck::loop() {
	// Check cellular status - used for event logging mostly
	bool temp = Cellular.ready();
	if (temp != isCellularReady) {
		// Cellular state changed
		isCellularReady = temp;
		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_CELLULAR_READY, isCellularReady);

		Log.info("cellular %s", isCellularReady ? "up" : "down");
	}

	// Check cloud connection status
	temp = Particle.connected();
	if (temp != isCloudConnected) {
		// Cloud connection state changed
		isCloudConnected = temp;
		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_CLOUD_CONNECTED, isCloudConnected);
		Log.info("cloud connection %s", isCloudConnected ? "up" : "down");

		if (isCloudConnected) {
			// Upon succesful connection, clear the failure count
			connectionCheckRetainedData.numFailures = 0;
		}
		else {
			// Cloud just disconnected, start measuring how long we've been disconnected
			cloudCheckStart = millis();
		}
	}

	if (!isCloudConnected) {
		// Not connected to the cloud - check to see if we've spent long enough in this state to reboot
		if (cloudWaitForReboot != 0 && millis() - cloudCheckStart >= cloudWaitForReboot) {
			// The time to wait to connect to the cloud has expired, reboot

			if (isCellularReady) {
				// Generate events about the state of the connection before rebooting.
				cloudConnectDebug();
			}

			// Keep the number of failures in a retained variable
			connectionCheckRetainedData.numFailures++;

			if (failureSleepSec > 0 && connectionCheckRetainedData.numFailures > 1) {
				// failureSleepSec has been set to a non-zero value, so sleep for that
				// many seconds after the first fullModemReset.
				// This is useful when battery powered if the SIM has been paused or something
				// is wrong with the SIM data or cloud such that a connection can be made.
				// It sleeps for some period (maybe 10 - 15 minutes?) before trying again to
				// avoid draining the battery continuously trying and failing to connect.
				ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_FAILURE_SLEEP);
				System.sleep(SLEEP_MODE_DEEP, failureSleepSec);
			}

			// Reboot
			ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_REBOOT_NO_CLOUD);
			fullModemReset();
		}
	}


	if (Cellular.listening()) {
		// Entered listening mode (blinking blue). Could be from holding down the MODE button, or
		// by repeated connection failures, see: https://github.com/spark/firmware/issues/687
		if (listeningStart == 0) {
			listeningStart = millis();
			ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_LISTENING_ENTERED);

			Log.info("entered listening mode");
		}
		else {
			if (listenWaitForReboot != 0 && millis() - listeningStart >= listenWaitForReboot) {
				// Reboot
				ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_REBOOT_LISTENING);
				fullModemReset();
			}
		}
	}
}


// This is called when timing out connecting to the cloud. It adds some debugging events to
// help log the current state for debugging purposes.
// It returns true to force a modem reset immediately, false to use the normal logic for whether to reset the modem.
bool ConnectionCheck::cloudConnectDebug() {
	int res = Cellular.command(pingTimeout, "AT+UPING=\"8.8.8.8\"\r\n");
	ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_PING_DNS, res);

	res = Cellular.command(pingTimeout, "AT+UPING=\"api.particle.io\"\r\n");
	ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_PING_API, res);

	// If pinging api.particle.io does not succeed, then reboot the modem right away
	return (res != RESP_OK);
}


// reason is the reason code, one of the ConnectionEvents::CONNECTION_EVENT_* constants
// forceResetMode will reset the modem even immediately instead of waiting for multiple failures
void ConnectionCheck::fullModemReset() {

	Log.info("resetting modem");

	ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_MODEM_RESET);

	// Disconnect from the cloud
	Particle.disconnect();

	// Wait up to 15 seconds to disconnect
	unsigned long startTime = millis();
	while(Particle.connected() && millis() - startTime < 15000) {
		delay(100);
	}

	// Reset the modem and SIM card
	// 16:MT silent reset (with detach from network and saving of NVM parameters), with reset of the SIM card
	Cellular.command(30000, "AT+CFUN=16\r\n");

	delay(1000);

	// Go into deep sleep for 10 seconds to try to reset everything. This turns off the modem as well.
	System.sleep(SLEEP_MODE_DEEP, 10);
}


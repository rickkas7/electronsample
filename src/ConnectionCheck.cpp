

#include "ConnectionCheck.h"

ConnectionCheck *ConnectionCheck::instance;

ConnectionCheck::ConnectionCheck()  {
	instance = this;
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

		if (!isCloudConnected) {
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


// Electron Sample Application for fault tolerance and problem debugging techniques
// Requires system firmware 0.5.0 or later!
//
// Original code location:
// https://github.com/rickkas7/electronsample

#include "Particle.h"

// If you are using a 3rd party SIM card, put your APN here. See also
// the call to Particle.keepAlive in setup()
// STARTUP(cellular_credentials_set("YOUR_APN_GOES_HERE", "", "", NULL));

// We use retained memory keep track of connection events; these are saved and later uploaded
// to the cloud even after rebooting
STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

// System threaded mode is not required here, but it's a good idea with 0.5.0 and later.
// https://docs.particle.io/reference/firmware/electron/#system-thread
SYSTEM_THREAD(ENABLED);

// SEMI_AUTOMATIC mode or system thread enabled is required here, otherwise we can't
// detect a failure to connect
// https://docs.particle.io/reference/firmware/electron/#semi-automatic-mode
SYSTEM_MODE(SEMI_AUTOMATIC);


// Various settings for how long to wait before rebooting and so forth
const unsigned long LISTEN_WAIT_FOR_REBOOT = 30000; // milliseconds, 0 = don't reboot on entering listening mode
const unsigned long CLOUD_WAIT_FOR_REBOOT = 180000; // milliseconds, 0 = don't reboot
const unsigned long PUBLISH_RATE_LIMIT = 1010; // milliseconds; to avoid sending events too rapidly
const unsigned long PING_TIMEOUT = 30000; // milliseconds
const unsigned long APP_WATCHDOG_TIMEOUT = 60000; // milliseconds
const uint8_t NUM_REBOOTS_BEFORE_RESETTING_MODEM = 2; //
const size_t MAX_TESTERFN_ARGS = 5; // Used to split out arguments to testerFn Particle.function
const int SLEEP_TEST_PIN = D2; // Used for testing sleep with pin modes

// This code is used to track connection events, used mainly for debugging
// and making sure this code works properly.
const size_t CONNECTION_EVENT_MAX = 32; // Maximum number of events to queue, 16 bytes each
const unsigned long CONNECTION_EVENT_MAGIC = 0x5c39d415;

// This is the event that's used to publish data to your event log. It's sent in PRIVATE mode.
const char *CONNECTION_EVENT_NAME = "connEventStats";

// These are the defined event codes. Instead of a string, they're sent as an integer to make
// the output more compact, saving retained memory and allowing more events to fit in a Particle.publish/
enum ConnectionEventCode {
	CONNECTION_EVENT_SETUP_STARTED = 0,		// 0
	CONNECTION_EVENT_CELLULAR_READY,		// 1
	CONNECTION_EVENT_CLOUD_CONNECTED,		// 2
	CONNECTION_EVENT_LISTENING_ENTERED,		// 3
	CONNECTION_EVENT_MODEM_RESET,			// 4
	CONNECTION_EVENT_REBOOT_LISTENING,		// 5
	CONNECTION_EVENT_REBOOT_NO_CLOUD,		// 6
	CONNECTION_EVENT_PING_DNS,				// 7
	CONNECTION_EVENT_PING_API,				// 8
	CONNECTION_EVENT_APP_WATCHDOG,			// 9
	CONNECTION_EVENT_TESTERFN_REBOOT,		// 10
	CONNECTION_EVENT_TESTERFN_APP_WATCHDOG,	// 11
	CONNECTION_EVENT_SLEEP					// 12
};

// Data for each event is stored in this structure
typedef struct { // 16 bytes per event
	unsigned long tsDate;
	unsigned long tsMillis;
	int eventCode;
	int data;
} ConnectionEventInfo;

// This structure is what's stored in retained memory
typedef struct {
	unsigned long eventMagic; // CONNECTION_EVENT_MAGIC
	uint8_t numFailureReboots;
	uint8_t reserved3;
	uint8_t reserved2;
	uint8_t reserved1;
	size_t eventCount;
	ConnectionEventInfo events[CONNECTION_EVENT_MAX];
} ConnectionEventData;

// This is where the retained memory is allocated. Currently 522 bytes.
// There are checks in connectionEventSetup() to initialize it on first
// use and if the format changes dramatically.
retained ConnectionEventData connectionEventData;

// This is used to slow down publishing of data to once every 1010 milliseconds to avoid
// exceeding the publish rate limit.
unsigned long connectionEventLastSent = 0;

void connectionEventSetup(); // forward declaration
void connectionEventLoop(); // forward declaration
void connectionEventAdd(int eventCode, int data = 0); // forward declaration


// This code is to handle keeping track of connection state and rebooting if necessary

bool cloudConnectDebug(); // forward declaration
void smartReboot(int reason, bool forceResetModem); // forward declaration
void watchdogCallback(); // forward declaration
int testerFnCallback(String arg); // forward declaration
void testerFnLoopHandler(char *mutableData); // forward declaration

bool isCellularReady = false;
bool isCloudConnected = false;
unsigned long listeningStart = 0;
unsigned long cloudCheckStart = 0;
char *testerFnLoopData = NULL;

// Optional: Initialize the Application Watchdog. This runs as a separate thread so if you
// block the main loop thread for more than APP_WATCHDOG_TIMEOUT milliseconds, the callback
// will be called. This currently logs an event in retained memory then resets.
// When the Electron reboots and comes back online, these events will be published.
// https://docs.particle.io/reference/firmware/electron/#application-watchdog
ApplicationWatchdog wd(APP_WATCHDOG_TIMEOUT, watchdogCallback);




//
//
//
void setup() {
	//
	Serial.begin(9600);

	// We store connection events in retained memory
	connectionEventSetup();


	// This function is used only for tested, so you can trigger the various things
	// like reset, modemReset, and appWatchdog for testing purposes
	Particle.function("testerFn", testerFnCallback);

	// If you are using a 3rd party SIM card, you will likely have to set this
	// https://docs.particle.io/reference/firmware/electron/#particle-keepalive-
	// Particle.keepAlive(180);

	// This is used only for sleep with pin tests using testerFn
	pinMode(SLEEP_TEST_PIN, INPUT_PULLUP);

	// We use semi-automatic mode so we can disconnect if we want to, but basically we
	// use it like automatic, as we always connect initially.
	Particle.connect();
}

void loop() {

	// Check cellular status - used for event logging mostly
	bool temp = Cellular.ready();
	if (temp != isCellularReady) {
		// Cellular state changed
		isCellularReady = temp;
		connectionEventAdd(CONNECTION_EVENT_CELLULAR_READY, isCellularReady);
		Serial.printlnf("cellular %s", isCellularReady ? "up" : "down");
	}

	// Check cloud connection status
	temp = Particle.connected();
	if (temp != isCloudConnected) {
		// Cloud connection state changed
		isCloudConnected = temp;
		connectionEventAdd(CONNECTION_EVENT_CLOUD_CONNECTED, isCloudConnected);
		Serial.printlnf("cloud connection %s", isCloudConnected ? "up" : "down");

		if (isCloudConnected) {
			// After successfully connecting to the cloud, clear the failure reboot counter
			connectionEventData.numFailureReboots = 0;
		}
		else {
			// Cloud just disconnected, start measuring how long we've been disconnected
			cloudCheckStart = millis();
		}
	}

	if (!isCloudConnected) {
		// Not connected to the cloud - check to see if we've spent long enough in this state to reboot
		if (CLOUD_WAIT_FOR_REBOOT != 0 && millis() - cloudCheckStart >= CLOUD_WAIT_FOR_REBOOT) {
			// The time to wait to connect to the cloud has expired, reboot

			bool forceResetModem = false;

			if (isCellularReady) {
				// Generate events about the state of the connection before rebooting. We also
				// do some ping tests, and if we can't successfully ping, we force reset the mode
				// in hopes to clear this problem more quickly.
				forceResetModem = cloudConnectDebug();
			}

			// Reboot
			smartReboot(CONNECTION_EVENT_REBOOT_NO_CLOUD, forceResetModem);
		}
	}


	if (Cellular.listening()) {
		// Entered listening mode (blinking blue). Could be from holding down the MODE button, or
		// by repeated connection failures, see: https://github.com/spark/firmware/issues/687
		if (listeningStart == 0) {
			listeningStart = millis();
			connectionEventAdd(CONNECTION_EVENT_LISTENING_ENTERED);
			Serial.println("entered listening mode");
		}
		else {
			if (LISTEN_WAIT_FOR_REBOOT != 0 && millis() - listeningStart >= LISTEN_WAIT_FOR_REBOOT) {
				// Reboot
				smartReboot(CONNECTION_EVENT_REBOOT_LISTENING, false);
			}
		}
	}

	if (testerFnLoopData != NULL) {
		char *mutableData = testerFnLoopData;
		testerFnLoopData = NULL;

		testerFnLoopHandler(mutableData);

		free(mutableData);
	}

	// Handle sending out saved events
	connectionEventLoop();
}

// reason is the reason code, one of the CONNECTION_EVENT_* constants
// forceResetMode will reset the modem even immediately instead of waiting for multiple failures
void smartReboot(int reason, bool forceResetModem) {
	connectionEventAdd(reason, ++connectionEventData.numFailureReboots);
	Serial.printlnf("smartReboot reason=%d numFailureReboots=%d forceResetModem=%d", reason, connectionEventData.numFailureReboots, forceResetModem);

	if (connectionEventData.numFailureReboots >= NUM_REBOOTS_BEFORE_RESETTING_MODEM || forceResetModem) {
		// We try not to do this because it requires the cellular handshake process to repeat,
		// which takes time and data.
		// This makes the reset more like unplugging the battery and power and doing a cold restart.
		connectionEventAdd(CONNECTION_EVENT_MODEM_RESET);
		connectionEventData.numFailureReboots = 0;

		Serial.println("resetting modem");

		Particle.disconnect();

		// 16:MT silent reset (with detach from network and saving of NVM parameters), with reset of the SIM card
		Cellular.command(30000, "AT+CFUN=16\r\n");

		Cellular.off();

		delay(1000);
	}

	System.reset();
}

// This is called when timing out connecting to the cloud. It adds some debugging events to
// help log the current state for debugging purposes.
// It returns true to force a modem reset immediately, false to use the normal logic for whether to reset the modem.
bool cloudConnectDebug() {
	int res = Cellular.command(PING_TIMEOUT, "AT+UPING=\"8.8.8.8\"\r\n");
	connectionEventAdd(CONNECTION_EVENT_PING_DNS, res);

	res = Cellular.command(PING_TIMEOUT, "AT+UPING=\"api.particle.io\"\r\n");
	connectionEventAdd(CONNECTION_EVENT_PING_API, res);

	// If pinging api.particle.io does not succeed, then reboot the modem right away
	return (res != RESP_OK);
}

// Called from the application watchdog when the main loop is blocked
void watchdogCallback() {
	// This isn't quite safe; connectionEventAdd should only be called from the main loop thread,
	// but since by definition the main loop thread is stuck when the app watchdog fires, this is
	// probably not that unsafe. (The application watchdog runs in a separate thread.)
	connectionEventAdd(CONNECTION_EVENT_APP_WATCHDOG);
	System.reset();
}

// This is the function registered with Particle.function(). Just copy the data and return so
// the successful response can be returned to the caller. Since we do things like reset, or
// enter an infinite loop, or sleep, doing this right from the callback causes the caller to
// time out because the response will never be received.
int testerFnCallback(String argStr) {
	// Process this in loop so the function won't time out
	testerFnLoopData = strdup(argStr.c_str());

	return 0;
}

// This does the actual work from the Particle.function(). It's called from looo().
void testerFnLoopHandler(char *mutableData) {
	// Parse argument into space-separated fields
	const char *argv[MAX_TESTERFN_ARGS];
	size_t argc = 0;

	char *cp = strtok(mutableData, " ");
	while(cp && argc < MAX_TESTERFN_ARGS) {
		argv[argc++] = cp;
		cp = strtok(NULL, " ");
	}
	if (argc == 0) {
		// No parameter, nothing to do
		return;
	}

	// Delay a bit here to make sure the function result is returned, otherwise if we
	// immediately go to sleep the function may return a timeout error.
	delay(500);

	// Process options here
	if (strcmp(argv[0], "reset") == 0) {
		smartReboot(CONNECTION_EVENT_TESTERFN_REBOOT, false);
	}
	else
	if (strcmp(argv[0], "modemReset") == 0) {
		smartReboot(CONNECTION_EVENT_TESTERFN_REBOOT, true);
	}
	else
	if (strcmp(argv[0], "appWatchdog") == 0) {
		connectionEventAdd(CONNECTION_EVENT_TESTERFN_APP_WATCHDOG, APP_WATCHDOG_TIMEOUT);

		while(true) {
			// Infinite loop!
		}
	}
	else
	if (strcmp(argv[0], "sleep") == 0 && argc >= 2) {
		// example usage from the Particle CLI:
		// particle call electron2 testerFn "sleep networkStandby 15"

		// optional duration in seconds, if not specified the default is 30
		int duration = 30;
		if (argc >= 3) {
			duration = atoi(argv[2]);
			if (duration == 0) {
				duration = 30;
			}
		}

		connectionEventAdd(CONNECTION_EVENT_SLEEP);

		if (strcmp(argv[1], "deep") == 0) {
			// SLEEP_MODE_DEEP requires cellular handshaking again (blinking green) and also
			// restarts running setup() again, so you'll get an event 0 (CONNECTION_EVENT_SETUP_STARTED)
			System.sleep(SLEEP_MODE_DEEP, duration);
		}
		else
		if (strcmp(argv[1], "stop") == 0) {
			// stop mode sleep stops the processor but execution will continue in loop() without going through setup again
			// This mode will shut down the cellular modem to save power so upon wake it requires cellular handshaking
			// again (blinking green)
			System.sleep(SLEEP_TEST_PIN, FALLING, duration);
		}
		else
		if (strcmp(argv[1], "networkStandby") == 0) {
			// stop mode sleep stops the processor but execution will continue in loop() without going through setup again
			// This mode keeps the cellular modem alive, so you should go right back into blinking cyan to handshake
			// to the cloud only
			System.sleep(SLEEP_TEST_PIN, FALLING, duration, SLEEP_NETWORK_STANDBY);
		}
	}
}

//
// Connection Event Code
//
// The idea is that we store small (16 byte) records of data in retained memory so they are preserved across rebooting
// the Electron. Then, when we get online, we publish these records so they'll be found in your event log.
//
// The event data might look like this:
//
// 1470912225,52,0,0;1470912226,1642,1,1;1470912228,3630,2,1;
//
// Each record, corresponding to a ConnectionEventInfo object consists of 4 comma-separated fields:
// date and time (Unix date format, seconds past January 1, 1970; the value returned by Time.now())
// millis() value
// eventCode - one of the CONNECTION_EVENT_* constants defined above
// data - depends on the event code
//
// Multiple records are packed into a single event up to the maximum event size (256 bytes); they are
// separated by semicolons.
//

// This should be called during setup()
void connectionEventSetup() {
	if (connectionEventData.eventMagic != CONNECTION_EVENT_MAGIC ||
		connectionEventData.eventCount > CONNECTION_EVENT_MAX) {
		//
		Serial.println("initializing connection event retained memory");
		connectionEventData.eventMagic = CONNECTION_EVENT_MAGIC;
		connectionEventData.eventCount = 0;
		connectionEventData.numFailureReboots = 0;
	}
	connectionEventAdd(CONNECTION_EVENT_SETUP_STARTED);
}

// This should be called from loop()
// If there are queued events and there is a cloud connection they're published, oldest first.
void connectionEventLoop() {
	if (connectionEventData.eventCount == 0) {
		// No events to send
		return;
	}

	if (!Particle.connected()) {
		// Not cloud connected, can't publish
		return;
	}

	if (millis() - connectionEventLastSent < PUBLISH_RATE_LIMIT) {
		// Need to wait before sending again to avoid exceeding publish limits
		return;
	}

	// Send events
	char buf[256]; // 255 data bytes, plus null-terminator
	size_t numHandled;
	size_t offset = 0;

	// Pack as many events as we have, up to what will fit in a 255 byte publish
	for(numHandled = 0; numHandled < connectionEventData.eventCount; numHandled++) {
		char entryBuf[64];

		ConnectionEventInfo *ev = &connectionEventData.events[numHandled];
		size_t len = snprintf(entryBuf, sizeof(entryBuf), "%lu,%lu,%d,%d;", ev->tsDate, ev->tsMillis, ev->eventCode, ev->data);
		if ((offset + len) >= sizeof(buf)) {
			// Not enough buffer space to send in this publish; try again later
			break;
		}
		strcpy(&buf[offset], entryBuf);
		offset += len;
	}
	connectionEventData.eventCount -= numHandled;
	if (connectionEventData.eventCount > 0) {
		Serial.printlnf("couldn't send all events, saving %d for later", connectionEventData.eventCount);
		memmove(&connectionEventData.events[0], &connectionEventData.events[numHandled], connectionEventData.eventCount * sizeof(ConnectionEventData));
	}
	else {
		Serial.printlnf("sent %d events", numHandled);
	}

	Particle.publish(CONNECTION_EVENT_NAME, buf, PRIVATE);
	connectionEventLastSent = millis();
}

// Add a new event. This should only be called from the main loop thread.
// Do not call from other threads like the system thread, software timer, or interrupt service routine.
void connectionEventAdd(int eventCode, int data /* = 0 */) {
	if (connectionEventData.eventCount >= CONNECTION_EVENT_MAX) {
		// Throw out oldest event
		Serial.println("discarding old event");
		connectionEventData.eventCount--;
		memmove(&connectionEventData.events[0], &connectionEventData.events[1], connectionEventData.eventCount * sizeof(ConnectionEventData));
	}

	// Add new event
	ConnectionEventInfo *ev = &connectionEventData.events[connectionEventData.eventCount++];
	ev->tsDate = Time.now();
	ev->tsMillis = millis();
	ev->eventCode = eventCode;
	ev->data = data;

	Serial.printlnf("connectionEvent event=%d data=%d", eventCode, data);
}



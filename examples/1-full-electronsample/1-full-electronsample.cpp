// Electron Sample Application for fault tolerance and problem debugging techniques
// Requires system firmware 0.6.1 or later!
//
// Original code location:
// https://github.com/rickkas7/electronsample

#include "Particle.h"

#include "AppWatchdogWrapper.h"
#include "BatteryCheck.h"
#include "ConnectionCheck.h"
#include "ConnectionEvents.h"
#include "SessionCheck.h"
#include "Tester.h"

// If you are using a 3rd party SIM card, put your APN here. See also
// the call to Particle.keepAlive in setup()
// STARTUP(cellular_credentials_set("YOUR_APN_GOES_HERE", "", "", NULL));

SerialLogHandler logHandler;

// We use retained memory keep track of connection events; these are saved and later uploaded
// to the cloud even after rebooting
void startup() {
	System.enableFeature(FEATURE_RETAINED_MEMORY);
	System.enableFeature(FEATURE_RESET_INFO);
}
STARTUP(startup);

// System threaded mode is not required here, but it's a good idea with 0.6.0 and later.
// https://docs.particle.io/reference/firmware/electron/#system-thread
SYSTEM_THREAD(ENABLED);

// SEMI_AUTOMATIC mode or system thread enabled is required here, otherwise we can't
// detect a failure to connect
// https://docs.particle.io/reference/firmware/electron/#semi-automatic-mode
SYSTEM_MODE(SEMI_AUTOMATIC);


// Manage connection-related events with this object. Publish with the event name "connEventStats" and store up to 32 events
// in retained memory. This provides better visibility into what your Electron is using but doesn't use too much data.
ConnectionEvents connectionEvents("connEventStats");

// Check session by sending and receiving an event every hour. This can help troubleshoot problems where
// your Electron is online but not communicating
SessionCheck sessionCheck(3600);

// Monitors the state of the connection, and sends this data using the ConnectionEvents.
// Handy for visibility.
ConnectionCheck connectionCheck;

// Tester adds a function that makes it possible exercise some of the feature remotely using functions.
// testerFn is the function and and the second parameter that's a pin to test pin sleep modes.
Tester tester("testerFn", D2);

// BatteryCheck is used to put the device to sleep immediately when the battery is low.
// 15.0 is the minimum SoC, if it's lower than that and not externally powered, it will
// sleep for the number of seconds in the second parameter, in this case, 3600 seconds = 1 hour.
BatteryCheck batteryCheck(15.0, 3600);

// This is a wrapper around the ApplicationWatchdog. It just makes using it easier. It writes
// a ConnectionEvents event to retained memory then does System.reset().
AppWatchdogWrapper watchdog(60000);

//
//
//
void setup() {
	//
	Serial.begin(9600);

	// If you're battery powered, it's a good idea to enable this. If a cellular or cloud connection cannot
	// be made, a full modem reset is first done. If that doesn't resolve the problem, on the second and
	// subsequent failures, the Electron will sleep for this many seconds. The intention is to set it to
	// maybe 10 - 20 minutes so if there is a problem like SIM paused or a network or cloud failure, the
	// Electron won't continuously try and fail to connect, depleting the battery.
	// connectionCheck.withFailureSleepSec(15 * 60);

	// We store connection events in retained memory. Do this early because things like batteryCheck will generate events.
	connectionEvents.setup();

	// Check if there's sufficient battery power. If not, go to sleep immediately, before powering up the modem.
	batteryCheck.setup();

	// Set up the other modules
	sessionCheck.setup();
	connectionCheck.setup();
	tester.setup();

	// We use semi-automatic mode so we can disconnect if we want to, but basically we
	// use it like automatic, as we always connect initially.
	Particle.connect();
}

void loop() {
	batteryCheck.loop();
	sessionCheck.loop();
	connectionCheck.loop();
	connectionEvents.loop();
	tester.loop();
}



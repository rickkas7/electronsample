
#include "SessionCheck.h"
#include "ConnectionCheck.h"

retained SessionRetainedData SessionCheck::sessionRetainedData;


SessionCheck::SessionCheck(time_t checkPeriodSecs, const char *eventSuffix) : checkPeriodSecs(checkPeriodSecs) {
	eventName = System.deviceID() + "/" + eventSuffix;
}

SessionCheck::~SessionCheck() {

}

void SessionCheck::setup() {
	if (sessionRetainedData.magic != SESSION_MAGIC) {
		sessionRetainedData.magic = SESSION_MAGIC;
		sessionRetainedData.lastCheckSecs = 0;
	}


	Particle.subscribe(eventName, &SessionCheck::subscriptionHandler, this, MY_DEVICES);
}

void SessionCheck::loop() {
	stateHandler(*this);
}

void SessionCheck::subscriptionHandler(const char *eventName, const char *data) {
	gotResponse = true;
}

void SessionCheck::waitToSendState() {
	if (millis() - stateTime < CHECK_PERIOD_MS) {
		return;
	}
	stateTime = millis();

	// Only do this check every 30 seconds. It's not time-critical and saves some unnecessary CPU cycles.

	if (!Time.isValid()) {
		// We use the real-time clock, so if the time is not set, we can't do this check.
		return;
	}

	if (!Particle.connected()) {
		// Can only do this if connected to the cloud as well
		return;
	}

	time_t now = Time.now();

	if (now - sessionRetainedData.lastCheckSecs < checkPeriodSecs) {
		// Not time to check yet
		return;
	}

	// Time to check again
	sessionRetainedData.lastCheckSecs = now;
	numFailures = 0;

	sendEvent();
}

void SessionCheck::sendEvent() {

	gotResponse = false;
	stateHandler = &SessionCheck::waitForResponseState;
	stateTime = millis();

	Log.info("publishing session check event %s", eventName.c_str());

	// Post our event
	Particle.publish(eventName, "", PRIVATE);
}

void SessionCheck::waitForResponseState() {
	if (gotResponse) {
		// Success
		stateHandler = &SessionCheck::waitToSendState;
		stateTime = millis();
	}

	if (millis() - stateTime < RECEIVE_TIMEOUT_MS) {
		// Waiting still
		return;
	}

	ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_SESSION_EVENT_LOST);

	// Failed to receive event
	if (++numFailures < NUM_FAILURES_BEFORE_RESET_SESSION) {
		// Try sending again just in case
		sendEvent();
		return;
	}

	ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_SESSION_RESET);

	// Too many tries, reset the session
	Particle.publish("spark/device/session/end", "", PRIVATE);

	delay(2000);

	// Reset the whole device just in case. If ConnectionCheck is present, do a full modem reset,
	// otherwise just reset.
	if (ConnectionCheck::getInstance()) {
		ConnectionCheck::getInstance()->fullModemReset();
	}
	else {
		System.reset();
	}
}


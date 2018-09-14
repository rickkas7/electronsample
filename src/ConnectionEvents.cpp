
#include "ConnectionEvents.h"


// This is where the retained memory is allocated. Currently 522 bytes.
// There are checks in ConnectionsEvents::setup() to initialize it on first
// use and if the format changes dramatically.
retained ConnectionEventData ConnectionEvents::connectionEventData;

ConnectionEvents *ConnectionEvents::instance;


ConnectionEvents::ConnectionEvents(const char *connectionEventName) : connectionEventName(connectionEventName) {
	instance = this;
}

ConnectionEvents::~ConnectionEvents() {

}



// This should be called during setup()
void ConnectionEvents::setup() {
	if (connectionEventData.eventMagic != CONNECTION_EVENT_MAGIC ||
		connectionEventData.eventCount > CONNECTION_EVENTS_MAX_EVENTS) {
		//
		Log.info("initializing connection event retained memory");
		connectionEventData.eventMagic = CONNECTION_EVENT_MAGIC;
		connectionEventData.eventCount = 0;
	}
	add(CONNECTION_EVENT_SETUP_STARTED);

	int resetReason = System.resetReason();
	if (resetReason != RESET_REASON_NONE) {
		add(CONNECTION_EVENT_RESET_REASON, resetReason);
	}
}

// This should be called from loop()
// If there are queued events and there is a cloud connection they're published, oldest first.
void ConnectionEvents::loop() {

	if (connectionEventData.eventCount == 0) {
		// No events to send
		return;
	}

	if (!canPublish()) {
		// Not time to publish yet
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
		Log.info("couldn't send all events, saving %d for later", connectionEventData.eventCount);
		memmove(&connectionEventData.events[0], &connectionEventData.events[numHandled], connectionEventData.eventCount * sizeof(ConnectionEventData));
	}
	else {
		Log.info("sent %d events", numHandled);
	}

	Particle.publish(connectionEventName, buf, PRIVATE);
	completedPublish();
}

bool ConnectionEvents::canPublish() {
	if (!Particle.connected()) {
		// Not cloud connected, can't publish
		return false;
	}

	if (millis() - connectionEventLastSent < PUBLISH_MIN_PERIOD_MS) {
		// Need to wait before sending again to avoid exceeding publish limits
		return false;
	}
	return true;
}

void ConnectionEvents::completedPublish() {
	connectionEventLastSent = millis();
}


// Add a new event. This should only be called from the main loop thread.
// Do not call from other threads like the system thread, software timer, or interrupt service routine.
void ConnectionEvents::add(int eventCode, int data /* = 0 */) {
	if (connectionEventData.eventCount >= CONNECTION_EVENTS_MAX_EVENTS) {
		// Throw out oldest event
		Log.info("discarding old event");
		connectionEventData.eventCount--;
		memmove(&connectionEventData.events[0], &connectionEventData.events[1], connectionEventData.eventCount * sizeof(ConnectionEventData));
	}

	// Add new event
	ConnectionEventInfo *ev = &connectionEventData.events[connectionEventData.eventCount++];
	ev->tsDate = Time.now();
	ev->tsMillis = millis();
	ev->eventCode = eventCode;
	ev->data = data;

	Log.info("connectionEvent event=%d data=%d", eventCode, data);
}

// static
void ConnectionEvents::addEvent(int eventCode, int data) {
	if (instance) {
		instance->add(eventCode, data);
	}
}




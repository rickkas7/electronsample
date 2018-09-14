#ifndef __CONNECTIONEVENTS_H
#define __CONNECTIONEVENTS_H

#include "Particle.h"

// This code is used to track connection events, used mainly for debugging
// and making sure this code works properly.
const size_t CONNECTION_EVENTS_MAX_EVENTS = 32;

// Data for each event is stored in this structure
typedef struct { // 16 bytes per event
	unsigned long tsDate;
	unsigned long tsMillis;
	int eventCode;
	int data;
} ConnectionEventInfo;

// This structure is what's stored in retained memory (520 bytes)
typedef struct {
	uint32_t eventMagic; // CONNECTION_EVENT_MAGIC
	size_t eventCount;
	ConnectionEventInfo events[CONNECTION_EVENTS_MAX_EVENTS]; // 32 entries, 16 bytes each
} ConnectionEventData;


/**
 * @brief Connection Event Code
 *
 * The idea is that we store small (16 byte) records of data in retained memory so they are preserved across rebooting
 * the Electron. Then, when we get online, we publish these records so they'll be found in your event log.
 *
 * The event data might look like this:
 *
 * 1470912225,52,0,0;1470912226,1642,1,1;1470912228,3630,2,1;
 *
 * Each record, corresponding to a ConnectionEventInfo object consists of 4 comma-separated fields:
 * date and time (Unix date format, seconds past January 1, 1970; the value returned by Time.now())
 * millis() value
 * eventCode - one of the CONNECTION_EVENT_* constants defined above
 * data - depends on the event code
 *
 * Multiple records are packed into a single event up to the maximum event size (256 bytes); they are
 * separated by semicolons.
*/
class ConnectionEvents {
public:
	ConnectionEvents(const char *connectionEventName);
	virtual ~ConnectionEvents();

	void setup();
	void loop();

	bool canPublish();
	void completedPublish();

	void add(int eventCode, int data = 0);

	static void addEvent(int eventCode, int data = 0);

	inline static ConnectionEvents *getInstance() { return instance; };

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
		CONNECTION_EVENT_TESTER_RESET,			// 10
		CONNECTION_EVENT_TESTER_APP_WATCHDOG,	// 11
		CONNECTION_EVENT_TESTER_SLEEP,			// 12
		CONNECTION_EVENT_LOW_BATTERY_SLEEP,		// 13
		CONNECTION_EVENT_SESSION_EVENT_LOST, 	// 14
		CONNECTION_EVENT_SESSION_RESET, 		// 15
		CONNECTION_EVENT_TESTER_RESET_SESSION,	// 16
		CONNECTION_EVENT_TESTER_RESET_MODEM,	// 17
		CONNECTION_EVENT_RESET_REASON,			// 18
		CONNECTION_EVENT_TESTER_SAFE_MODE,		// 19
		CONNECTION_EVENT_TESTER_PING,			// 20
		CONNECTION_EVENT_STOP_SLEEP_WAKE,		// 21
		CONNECTION_EVENT_FAILURE_SLEEP			// 22
	};

	static const unsigned long CONNECTION_EVENT_MAGIC = 0x5c39d416;
	static const unsigned long PUBLISH_MIN_PERIOD_MS = 1010;

private:
	const char *connectionEventName;

	// This is used to slow down publishing of data to once every 1010 milliseconds to avoid
	// exceeding the publish rate limit.
	unsigned long connectionEventLastSent = 0;

	static ConnectionEventData connectionEventData;
	static ConnectionEvents *instance;
};


#endif /* __CONNECTIONEVENTS_H */

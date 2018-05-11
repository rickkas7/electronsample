#ifndef __SESSIONCHECK_H
#define __SESSIONCHECK_H

#include "ConnectionEvents.h"

// This structure is what's stored in retained memory
typedef struct {
	uint32_t magic;
	time_t lastCheckSecs;
} SessionRetainedData;


class SessionCheck {
public:
	SessionCheck(time_t checkPeriodSecs, const char *eventSuffix = "sessionCheck");
	virtual ~SessionCheck();

	void setup();
	void loop();

	void subscriptionHandler(const char *eventName, const char *data);

	static const uint32_t SESSION_MAGIC = 0x4a6849fe;
	static const unsigned long CHECK_PERIOD_MS = 30000; // How often to do more intensive checks
	static const unsigned long RECEIVE_TIMEOUT_MS = 45000; // 45 seconds
	static const int NUM_FAILURES_BEFORE_RESET_SESSION = 2;

private:
	void waitToSendState();
	void sendEvent();
	void waitForResponseState();

	time_t checkPeriodSecs;
	String eventName;
	unsigned long stateTime = 0; // millis() value
	bool gotResponse = false;
	int numFailures = 0;
	std::function<void(SessionCheck&)> stateHandler = &SessionCheck::waitToSendState;

	static SessionRetainedData sessionRetainedData;
};

#endif /* __SESSIONCHECK_H  */

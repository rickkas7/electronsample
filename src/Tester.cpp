

#include "Tester.h"

#include "ConnectionCheck.h"
#include "ConnectionEvents.h"

Tester::Tester(const char *functionName, int sleepTestPin) :
	functionName(functionName), sleepTestPin(sleepTestPin) {

}


Tester::~Tester() {

}

void Tester::setup() {
	Particle.function(functionName, &Tester::functionHandler, this);
	if (sleepTestPin >= 0) {
		pinMode(sleepTestPin, INPUT_PULLUP);
	}
}

void Tester::loop() {

	if (functionData) {
		processOptions(functionData);
		free(functionData);
		functionData = NULL;
	}

	if (pingInterval > 0) {
		if (millis() - lastPing >= (unsigned long) (pingInterval * 1000)) {
			lastPing = millis();

			ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_PING, ++pingCounter);
		}
	}
}


// This is the function registered with Particle.function(). Just copy the data and return so
// the successful response can be returned to the caller. Since we do things like reset, or
// enter an infinite loop, or sleep, doing this right from the callback causes the caller to
// time out because the response will never be received.
int Tester::functionHandler(String argStr) {
	// Process this in loop so the function won't time out
	functionData = strdup(argStr.c_str());

	return 0;
}

// This does the actual work from the Particle.function(). It's called from looo().
void Tester::processOptions(char *mutableData) {
	// Parse argument into space-separated fields
	const char *argv[MAX_ARGS];
	size_t argc = 0;

	char *cp = strtok(mutableData, " ");
	while(cp && argc < MAX_ARGS) {
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
		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_RESET);
		System.reset();
	}
	else
	if (strcmp(argv[0], "modemReset") == 0) {
		if (ConnectionCheck::getInstance()) {
			ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_RESET_MODEM);
			ConnectionCheck::getInstance()->fullModemReset();
		}
		else {
			ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_RESET);
			System.reset();
		}
	}
	else
	if (strcmp(argv[0], "resetSession") == 0) {
		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_RESET_SESSION);
		Particle.publish("spark/device/session/end", "", PRIVATE);
	}
	else
	if (strcmp(argv[0], "safeMode") == 0) {
		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_SAFE_MODE);
		System.enterSafeMode();
	}
	else
	if (strcmp(argv[0], "appWatchdog") == 0) {
		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_APP_WATCHDOG, 0);

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

		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_TESTER_SLEEP, duration);

		if (strcmp(argv[1], "deep") == 0) {
			// SLEEP_MODE_DEEP requires cellular handshaking again (blinking green) and also
			// restarts running setup() again, so you'll get an event 0 (CONNECTION_EVENT_SETUP_STARTED)
			System.sleep(SLEEP_MODE_DEEP, duration);
		}
		else
		if (strcmp(argv[1], "deepStandby") == 0) {
			// SLEEP_MODE_DEEP requires cellular handshaking again (blinking green) and also
			// restarts running setup() again, so you'll get an event 0 (CONNECTION_EVENT_SETUP_STARTED)
			System.sleep(SLEEP_MODE_DEEP, duration, SLEEP_NETWORK_STANDBY);
		}
		else
		if (strcmp(argv[1], "stop") == 0) {
			// stop mode sleep stops the processor but execution will continue in loop() without going through setup again
			// This mode will shut down the cellular modem to save power so upon wake it requires cellular handshaking
			// again (blinking green)
			if (sleepTestPin >= 0) {
				System.sleep(sleepTestPin, FALLING, duration);

				ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_STOP_SLEEP_WAKE, 0);
			}
		}
		else
		if (strcmp(argv[1], "stopStandby") == 0) {
			// stop mode sleep stops the processor but execution will continue in loop() without going through setup again
			// This mode keeps the cellular modem alive, so you should go right back into blinking cyan to handshake
			// to the cloud only
			if (sleepTestPin >= 0) {
				System.sleep(sleepTestPin, FALLING, duration, SLEEP_NETWORK_STANDBY);

				ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_STOP_SLEEP_WAKE, 0);
			}
		}
	}
	else
	if (strcmp(argv[0], "ping") == 0 && argc >= 2) {
		// example usage from the Particle CLI:
		// particle call electron2 testerFn "ping start 30"
		// particle call electron2 testerFn "ping stop"
		if (strcmp(argv[1], "start") == 0) {
			if (argc >= 3) {
				pingInterval = (unsigned long) atoi(argv[2]);
			}
			else {
				pingInterval = 30;
			}
		}
		else
		if (strcmp(argv[1], "stop") == 0) {
			pingInterval = 0;
		}

	}

}

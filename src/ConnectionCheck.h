#ifndef __CONNECTIONCHECK_H
#define __CONNECTIONCHECK_H

#include "ConnectionEvents.h"

// This structure is what's stored in retained memory
typedef struct {
	uint32_t magic;
	uint32_t numFailures;
} ConnectionCheckRetainedData;

class ConnectionCheck {
public:
	ConnectionCheck();
	virtual ~ConnectionCheck();

	void setup();
	void loop();
	void fullModemReset();
	bool cloudConnectDebug();

	inline ConnectionCheck &withListenWaitForReboot(unsigned long value) { listenWaitForReboot = value; return *this; };
	inline ConnectionCheck &withCloudWaitForReboot(unsigned long value) { cloudWaitForReboot = value; return *this; };
	inline ConnectionCheck &withPingTimeout(unsigned long value) { pingTimeout = value; return *this; };
	inline ConnectionCheck &withFailureSleepSec(unsigned long value) { failureSleepSec = value; return *this; };

	static inline ConnectionCheck *getInstance() { return instance; };

	static const uint32_t CONNECTION_CHECK_MAGIC = 0x2e4ec594;

private:
	unsigned long listenWaitForReboot = 30000; // milliseconds
	unsigned long cloudWaitForReboot = 180000; // milliseconds
	unsigned long pingTimeout = 10000; // milliseconds
	unsigned long failureSleepSec = 0; // seconds, 0 = none

	bool isCellularReady = false;
	bool isCloudConnected = false;
	unsigned long listeningStart = 0;
	unsigned long cloudCheckStart = 0;

	static ConnectionCheck *instance;
	static ConnectionCheckRetainedData connectionCheckRetainedData;
};

#endif /* __CONNECTIONCHECK_H */

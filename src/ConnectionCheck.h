#ifndef __CONNECTIONCHECK_H
#define __CONNECTIONCHECK_H

#include "ConnectionEvents.h"


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

	static inline ConnectionCheck *getInstance() { return instance; };

private:
	unsigned long listenWaitForReboot = 30000; // milliseconds
	unsigned long cloudWaitForReboot = 180000; // milliseconds
	unsigned long pingTimeout = 10000; // milliseconds

	bool isCellularReady = false;
	bool isCloudConnected = false;
	unsigned long listeningStart = 0;
	unsigned long cloudCheckStart = 0;

	static ConnectionCheck *instance;
};

#endif /* __CONNECTIONCHECK_H */

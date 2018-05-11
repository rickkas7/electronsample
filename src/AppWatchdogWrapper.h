#ifndef __APPWATCHDOGWRAPPER_H
#define __APPWATCHDOGWRAPPER_H

#include "Particle.h"

class AppWatchdogWrapper {
public:
	AppWatchdogWrapper(unsigned long timeoutMs = 60000);
	virtual ~AppWatchdogWrapper();

	static void watchdogCallback();

private:
	ApplicationWatchdog wd;

};

#endif // __APPWATCHDOGWRAPPER_H

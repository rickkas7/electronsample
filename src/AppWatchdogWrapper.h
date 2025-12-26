#ifndef __APPWATCHDOGWRAPPER_H
#define __APPWATCHDOGWRAPPER_H

#include "Particle.h"

// This check is here so it can be a library dependency for a library that's compiled for both
// cellular and Wi-Fi.
#if Wiring_Cellular

class AppWatchdogWrapper {
public:
	AppWatchdogWrapper(unsigned long timeoutMs = 60000);
	virtual ~AppWatchdogWrapper();

	static void watchdogCallback();

private:
	ApplicationWatchdog wd;

};

#endif // Wiring_Cellular

#endif // __APPWATCHDOGWRAPPER_H

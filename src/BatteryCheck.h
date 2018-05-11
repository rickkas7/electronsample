#ifndef __BATTERYCHECK_H
#define __BATTERYCHECK_H

#include "Particle.h"

class BatteryCheck {
public:
	BatteryCheck(float minimumSoC = 15.0, long sleepTimeSecs = 3600);
	virtual ~BatteryCheck();

	void setup();

	void loop();

	void checkAndSleepIfNecessary();

	static const unsigned long CHECK_PERIOD_MS = 60000;

private:
	float minimumSoC;
	long sleepTimeSecs;
	unsigned long lastCheckMs = 0;
};

#endif // __BATTERYCHECK_H

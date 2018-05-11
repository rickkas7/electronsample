#include "BatteryCheck.h"

#include "ConnectionEvents.h"

static FuelGauge fuel;
static PMIC pmic;

BatteryCheck::BatteryCheck(float minimumSoC, long sleepTimeSecs) : minimumSoC(minimumSoC), sleepTimeSecs(sleepTimeSecs) {


}
BatteryCheck::~BatteryCheck() {

}

void BatteryCheck::setup() {
	checkAndSleepIfNecessary();
}

void BatteryCheck::loop() {
	if (millis() - lastCheckMs >= CHECK_PERIOD_MS) {
		lastCheckMs = millis();
		checkAndSleepIfNecessary();
	}
}

void BatteryCheck::checkAndSleepIfNecessary() {
	float soc = fuel.getSoC();

	// There is an SoC, it's less than the minimum, and there's no external power (USB or VIN)
	if (soc != 0.0 && soc < minimumSoC && !pmic.isPowerGood()) {
		ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_LOW_BATTERY_SLEEP, static_cast<int>(soc));

		System.sleep(SLEEP_MODE_DEEP, sleepTimeSecs);
	}

}


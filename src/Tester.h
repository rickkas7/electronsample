#ifndef __TESTER_H
#define __TESTER_H

#include "Particle.h"

class Tester {
public:
	Tester(const char *functionName, int sleepTestPin = -1);
	virtual ~Tester();

	void setup();
	void loop();

	int functionHandler(String argStr);
	void processOptions(char *mutableData);

	static const size_t MAX_ARGS = 5;

private:
	const char *functionName;
	int sleepTestPin;
	char *functionData = NULL;
	unsigned long lastPing = 0;
	int pingInterval = 0;
	int pingCounter = 0;
};

#endif // __TESTER_H


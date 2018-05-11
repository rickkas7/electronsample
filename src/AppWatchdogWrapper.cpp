

#include "AppWatchdogWrapper.h"

#include "ConnectionEvents.h"

// Note: The 1800 parameter is because the default stack size is too small in 0.7.0
AppWatchdogWrapper::AppWatchdogWrapper(unsigned long timeoutMs) : wd(timeoutMs, watchdogCallback, 1800) {

}

AppWatchdogWrapper::~AppWatchdogWrapper() {

}


// static
void AppWatchdogWrapper::watchdogCallback() {
	// This isn't quite safe; connectionEvents.add should only be called from the main loop thread,
	// but since by definition the main loop thread is stuck when the app watchdog fires, this is
	// probably not that unsafe. (The application watchdog runs in a separate thread.)
	ConnectionEvents::addEvent(ConnectionEvents::CONNECTION_EVENT_APP_WATCHDOG);
	System.reset();
}

# Electron Sample

*Particle Electron sample code for fault tolerance and problem debugging*

Most of the time the Electron just works, happily connecting and disconnecting from the cellular network and all is good. Except once in a while, some devices have a hard time getting reconnected. Disconnecting the battery and power usually solves the problem, but that can be a pain for remote devices.

This experiment is some code that might help recover from this situation automatically, and also log some data to better figure out what's actually happening, so if it's a bug, it can just be fixed so special code won't be necessary in the future.

It's set up as a library (electronsample) that you can include in your project. It's modular so you can just create the parts you want to use in your app, you don't need to use them all. The modules are:

- AppWatchdogWrapper
- BatteryCheck
- ConnectionCheck
- ConnectionEvents
- SessionCheck
- Tester

The latest version of this project is here:
[https://github.com/rickkas7/electronsample](https://github.com/rickkas7/electronsample)

This is the new 2018 version. If you're looking for [the original version](https://github.com/rickkas7/electronsample/tree/v1.0) it's still available as release v1.0 on Github.

## Using the Connection Event Log

This code sets aside a small portion (520 bytes) of retained memory for the connection event log. This allows various events and debugging information to be saved across restarts and uploaded via Particle.publish once a cloud connection has finally been made.

It shows when cellular and cloud connectivity was established or lost, along with timestamps and many other things of interest.

In the Event Log in the Particle Console you might see something like:

```
1470912226,1642,1,1;1470912228,3630,2,1;
```

The fields are:

- The date and time (Unix time, seconds since January 1, 1970), UTC.

- The value of millis().

- The event type, in this case it's 1, `CONNECTION_EVENT_CELLULAR_READY`.

- The event data, 1, which means cellular just went up. It would be 0 if cellular went down.

Each record is separated by a semicolon.

### Adding connection log to your code

Include the header file:

```
#include "ConnectionEvents.h"
```

Create a global object. The parameter is the name of the event to send. If you change connEventStats to something else you'll need to add the option to event-display to use a different event name using the `EVENT_NAME` option.

```
ConnectionEvents connectionEvents("connEventStats");
```

Make sure you call these out of setup() and loop, respectively.

```
connectionEvents.setup();
```

```
connectionEvents.loop();
```

Many of the other modules use connectionEvents, but do so only if you've set it up. So if you don't use the connection event log they won't attempt to log the data.

## Event Monitoring Tool

As it's a pain to read those entries, there's a script in the event-decoder directory in [github](https://github.com/rickkas7/electronsample). 

### Getting started

- Install [nodejs](https://nodejs.org/) if you haven't already done so. I recommend the LTS version.
- Download this repository.
- Install the dependencies

```
cd particle-node-api-examples/list-devices
npm install
```

- Get an auth token. The easiest place is in the settings icon at [https://build.particle.io](https://build.particle.io). 

- Set your auth token:

Mac or Linux:

```
export AUTH_TOKEN=fe12630d2dbbd1ca6e8e28bd5a4b953dd3f1c53f
```

Windows:

```
set AUTH_TOKEN=fe12630d2dbbd1ca6e8e28bd5a4b953dd3f1c53f
```

- Run the program:

```
npm start
```

### Other ways of handling your auth token

In addition to setting it in an environment variable, you can set it in command line argument:

```
node listdevices.js --AUTH_TOKEN fe12630d2dbbd1ca6e8e28bd5a4b953dd3f1c53f
```

Or in a file config.json in the same directory as config.js:

```
{
	"AUTH_TOKEN":"fe12630d2dbbd1ca6e8e28bd5a4b953dd3f1c53f"
}
```

### Sample event-decoder output

The event decoder outputs one line per event, with the fields, left to right:

- device name
- Time (UTC)
- millis value
- event
- optional data

```
electron3,2018-05-10T20:41:17.000Z,464572,CELLULAR_READY disconnected
electron3,2018-05-10T20:41:17.000Z,464572,CLOUD_CONNECTED disconnected
electron3,2018-05-10T20:44:17.000Z,644574,REBOOT_NO_CLOUD
electron3,2018-05-10T20:44:17.000Z,644574,MODEM_RESET
electron3,2018-05-10T20:44:40.000Z,62,SETUP_STARTED
electron3,2018-05-10T20:44:40.000Z,63,RESET_REASON RESET_REASON_POWER_MANAGEMENT
electron3,2018-05-10T20:47:40.000Z,180000,REBOOT_NO_CLOUD
electron3,2018-05-10T20:47:40.000Z,180002,MODEM_RESET
electron3,2018-05-10T20:47:53.000Z,62,SETUP_STARTED
electron3,2018-05-10T20:47:53.000Z,63,RESET_REASON RESET_REASON_POWER_MANAGEMENT
electron3,2018-05-10T20:48:16.000Z,23226,CELLULAR_READY connected
electron3,2018-05-10T20:48:18.000Z,25127,CLOUD_CONNECTED connected
```

Incidentally, `RESET_REASON_POWER_MANAGEMENT` is used when sleep mode is entered, which is how the connection monitor resets the device when something seems wrong, so you'll see that in the logs.

### Using an alternate event name

The `EVENT_NAME` parameter in config.json, exported variable, or the `--EVENT_NAME` command line option can be used to set the event name. For example:

```
export EVENT_NAME="SomethingElse"
npm start
```

## Connection Check

The ConnectionCheck module does several things:

- Monitors the state of cellular (Cellular.ready).
- Monitors the state of the Particle cloud connection (Particle.connected).
- Does a full modem reset if it takes too long to connect.
- Breaks out of listening mode if you stay in it too long.
- Logs information about whether Google DNS (8.8.8.8) and the Particle API server (api.particle.io) can be pinged (if cellular is up but cloud is not).

### Adding connection log to your code

Include the header file:

```
#include "ConnectionCheck.h"
```

Create a global object. The parameter is the name of the event to send. If you change connEventStats to something else you'll need to add the option to event-display to use a different event name using the `EVENT_NAME` option.

```
ConnectionCheck connectionCheck;
```

Make sure you call these out of setup() and loop, respectively.

```
connectionCheck.setup();
```

```
connectionCheck.loop();
```

### In event display

Here are some events you might see if the cloud connection is lost (in this case by disconnected the antenna).

```
electron1,2018-05-10T20:44:34.000Z,62,SETUP_STARTED
electron1,2018-05-10T20:44:34.000Z,63,RESET_REASON RESET_REASON_PIN_RESET
electron1,2018-05-10T20:44:58.000Z,24296,CELLULAR_READY connected
electron1,2018-05-10T20:45:08.000Z,34387,CLOUD_CONNECTED connected
electron3,2018-05-10T20:41:17.000Z,464572,CELLULAR_READY disconnected
electron3,2018-05-10T20:41:17.000Z,464572,CLOUD_CONNECTED disconnected
electron3,2018-05-10T20:44:17.000Z,644574,REBOOT_NO_CLOUD
electron3,2018-05-10T20:44:17.000Z,644574,MODEM_RESET
electron3,2018-05-10T20:44:40.000Z,62,SETUP_STARTED
electron3,2018-05-10T20:44:40.000Z,63,RESET_REASON RESET_REASON_POWER_MANAGEMENT
```

If the device has cellular but not cloud connectivity:

```
electron3,2018-05-11T10:17:26.000Z,2096,CELLULAR_READY connected
electron3,2018-05-11T10:17:58.000Z,33376,CLOUD_CONNECTED connected
electron3,2018-05-11T10:18:30.000Z,69180,TESTER_PING 1
electron3,2018-05-11T10:19:00.000Z,99180,TESTER_PING 2
electron3,2018-05-11T10:20:24.000Z,182528,CLOUD_CONNECTED disconnected
electron3,2018-05-11T10:20:30.000Z,189180,TESTER_PING 5
electron3,2018-05-11T10:21:00.000Z,219180,TESTER_PING 6
electron3,2018-05-11T10:21:30.000Z,249180,TESTER_PING 7
electron3,2018-05-11T10:22:00.000Z,279180,TESTER_PING 8
electron3,2018-05-11T10:22:30.000Z,309180,TESTER_PING 9
electron3,2018-05-11T10:23:00.000Z,339180,TESTER_PING 10
electron3,2018-05-11T10:23:24.000Z,362539,PING_DNS success
electron3,2018-05-11T10:23:28.000Z,367223,PING_API success
electron3,2018-05-11T10:23:28.000Z,367223,REBOOT_NO_CLOUD
electron3,2018-05-11T10:23:28.000Z,367224,MODEM_RESET
electron3,2018-05-11T10:24:20.000Z,62,SETUP_STARTED
electron3,2018-05-11T10:24:20.000Z,63,RESET_REASON RESET_REASON_POWER_MANAGEMENT
electron3,2018-05-11T10:24:43.000Z,23376,CELLULAR_READY connected
electron3,2018-05-11T10:24:53.000Z,33711,CLOUD_CONNECTED connected
```

## Application Watchdog

This code uses the application watchdog so if you don't return from loop for more than a minute, it resets the Electron, adding a connection event log entry so you know you have a blocking bug in your code.

### Adding application watchdog to your code

Include the header file:

```
#include "AppWatchdogWrapper.h"
```

Declare a global variable to initialize the watchdog with the timeout in milliseconds. 60000 (60 seconds) is a good value.

```
AppWatchdogWrapper watchdog(60000);
```

This just sets up an ApplicationWatchdog with a function that stores a ConnectionEvent that the watchdog was triggered and then does a System.reset.

### Application watchdog in the event display

```
electron3,2018-05-11T13:11:16.000Z,540004,APP_WATCHDOG
electron3,2018-05-11T13:11:16.000Z,62,SETUP_STARTED
electron3,2018-05-11T13:11:16.000Z,63,RESET_REASON RESET_REASON_USER
electron3,2018-05-11T13:11:18.000Z,2536,CELLULAR_READY connected
electron3,2018-05-11T13:11:19.000Z,3158,CLOUD_CONNECTED connected

```

## Battery Check

It's important to prevent the Electron from running down to zero battery, as it can corrupt the flash memory. The Battery Check module does two things:

- At startup, it checks the battery and goes to sleep immediately if the state of charge (SoC) is too low. It does this before turning on the cellular modem.
- Once an hour, it checks to see if the battery is too low, and puts the Electron in deep sleep as well.

The times and SoC are configurable. The battery check is also only done when powered by battery, so if you have external power the Electron will run, even while it has a discharged battery.

### Adding Battery Check to your code

Include the header file:

```
#include "BatteryCheck.h"
```

Initialize the global object. The first parameter is the minimum state of charge (15.0 is 15% SoC). A value of 15.0 to 20.0 is a good choice.

The second parameter is the amount of time to sleep in seconds (3600 = 1 hour) before checking again.

```
BatteryCheck batteryCheck(15.0, 3600);
```

Make sure you call these out of setup() and loop, respectively.

```
batteryCheck.setup();
```

```
batteryCheck.loop();
```

### In the event display

It didn't take an hour because I manually used the WKP pin to wake up the Electron after plugging in USB power.

```
electron1,2018-05-11T13:11:49.000Z,4920000,LOW_BATTERY_SLEEP
electron1,2018-05-11T13:42:25.000Z,99,SETUP_STARTED
electron1,2018-05-11T13:42:25.000Z,99,RESET_REASON RESET_REASON_POWER_MANAGEMENT
electron1,2018-05-11T13:42:49.000Z,24432,CELLULAR_READY connected
electron1,2018-05-11T13:42:50.000Z,25018,CLOUD_CONNECTED connected
```


## Session Check

The session check subscribes to an event on the device, and periodically publishes an event to makes sure round-trip communication is possible.

If two events are lost, then the cloud session is reset by publishing the `spark/device/session/end` event from the device. This can clear up some problems that simply resetting does not clear up.

### Adding Session Check to your code

Include the header file:

```
#include "SessionCheck.h"
```

Initialize a global object for checking the session. 3600 is the period in seconds, so 3600 is once per hour. The session check involves sending and receiving one event, so make sure you take into account how much data this will use when using small values!

```
SessionCheck sessionCheck(3600); 
```

Make sure you call these out of setup() and loop, respectively.

```
sessionCheck.setup();
```

```
sessionCheck.loop();
```

### In the event display

In this case, the modem reset took too long and the app watchdog kicked in, but that's actually good. It could have gotten stuck in breathing green if that had not happened.

```
electron3,2018-05-11T13:38:02.000Z,900000,SESSION_EVENT_LOST
electron3,2018-05-11T13:38:02.000Z,900000,SESSION_RESET
electron3,2018-05-11T13:38:10.000Z,907954,MODEM_RESET
electron3,2018-05-11T13:39:02.000Z,960004,APP_WATCHDOG
electron3,2018-05-11T13:39:02.000Z,62,SETUP_STARTED
electron3,2018-05-11T13:39:02.000Z,63,RESET_REASON RESET_REASON_USER
electron3,2018-05-11T13:39:05.000Z,3686,CELLULAR_READY connected
electron3,2018-05-11T13:39:16.000Z,13782,CLOUD_CONNECTED connected
```

## The Tester

The Tester module registers a function that makes it possible to exercise a bunch of things. You may not want to use this in your application, but it's handy for testing.

The code registers a function that can exercise some of the features that are hard to test using the CLI. For example, if the electron is named "electron2" you might use commands like:

```
particle call electron2 testerFn reset
particle call electron2 testerFn modemReset
```

Resets using System.reset(), optionally resetting the modem as well.

```
particle call electron2 testerFn appWatchdog
```

Enters an infinite loop to test the application watchdog. Takes 60 seconds to activate.

```
particle call electron2 testerFn resetSession
```

Resets the current cloud session by publishing the `spark/device/session/end` event from the device.

```
particle call electron2 testerFn safeMode
```

Causes the device to enter safe mode (breathing magenta). You might do this if you're having trouble doing an OTA flash because the current device firmware is interfering with it.

```
particle call electron2 testerFn "sleep deep"
```

Enters `SLEEP_MODE_DEEP` for 30 seconds. This turns off the cellular modem, so it will reconnect (blinking green) after waking up. You can also specify the number of seconds in the 3rd parameter; if not specified the default is 30.

```
particle call electron2 testerFn "sleep deepStandby 15"
```

Enters a SLEEP_MODE_DEEP for 15 seconds with `SLEEP_NETWORK_STANDBY`. This leaves the cellular modem on, so upon waking it should go right into blinking cyan to reconnect to the Particle cloud.


```
particle call electron2 testerFn "sleep stop 60"
```

Enters a stop mode sleep for 60 seconds. This uses a sleep with check on a pin. This turns off the cellular modem, so it will reconnect (blinking green) after waking up.


```
particle call electron2 testerFn "sleep stopStandby 15"
```

Enters a stop mode sleep for 15 seconds with `SLEEP_NETWORK_STANDBY`. This leaves the cellular modem on, so upon waking it should go right into blinking cyan to reconnect to the Particle cloud.

```
particle call electron2 testerFn "ping start 30"
```

Starts ping mode. The device will send a ping event every 30 seconds. This is useful because many cloud connectivity issues are only detected during the keep-alive ping or publishing.

```
particle call electron2 testerFn "ping stop"
```

Stops the periodic ping mode.


### Adding Tester to your code

Include the header file

```
#include "Tester.h"
```

Initialize it. The first parameter is the name of the function to register. The second parameter is a pin to use for pin + time (stop sleep mode) tests. You can omit this parameter (or pass -1) but if you do so, then any pin + time sleep tests won't actually sleep.

```
Tester tester("testerFn", D2);
```

Make sure you call these out of setup() and loop, respectively.

```
tester.setup();
```

```
tester.loop();
```
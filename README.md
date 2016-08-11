# Electron Sample

*Particle Electron sample code for fault tolerance and problem debugging*

Most of the time the Electron just works, happily connecting and disconnecting from the cellular network and all is good. Except once in a while, some devices have a hard time getting reconnected. Disconnecting the battery and power usually solves the problem, but that can be a pain for remote devices.

This experiment is some code that might help recover from this situation automatically, and also log some data to better figure out what's actually happening, so if it's a bug, it can just be fixed so special code won't be necessary in the future.

The code is not designed to be a drop in replacement for anything, but a set of tools you can copy and paste into your code to help add fault tolerance to your app. You can also just flash [electronsample.cpp] (https://github.com/rickkas7/electronsample/blob/master/electronsample.cpp) to an Electron and run it for fun.

The latest version of this project is here:
[https://github.com/rickkas7/electronsample] (https://github.com/rickkas7/electronsample)

## Using the Connection Event Log

This code sets aside a small portion (522 bytes) of retained memoury for the connection event log. This allows various events and debugging information to be saved across restarts and uploaded via Particle.publish once a cloud connection has finally been made.

It shows when cellular and cloud connectivity was established or lost, along with timestamps and many other things of interest.

In the Event Log in the Particle Console you might see something like:

```
1470912226,1642,1,1;1470912228,3630,2,1;
```

The fields are:

1. The date and time (Unix time, seconds since January 1, 1970).

2. The value of millis().

3. The event type, in this case it's 1, `CONNECTION_EVENT_CELLULAR_READY`.

4. The event data, 1, which means celluar just went up. It would be 0 if cellular went down.

Each record is separated by a semicolon.



## Connection Debugging

If it takes too long to make a connection to the cloud the code will attempt a reset. The connection event log will indicate whether it was able to make a cellular connection or not. If there is cellular but not cloud, it will also do some additional tests. It currently will ping the Google DNS (8.8.8.8) and also the Particle API server (api.particle.io, which also tests DNS). These results are included in the connection event log.

It also has listening mode detection, and will reset the Electron if the Electron somehow ends up in listening mode, blinking dark blue, for more than 30 seconds.


## Smart Reboot

One of the things people have done is force a reset using `System.reset()`. This works great on the Photon, but on the Electron has one flaw: It doesn't reset the modem. This is normally advantageous because it saves data and makes reconnecting much faster, but is not helpful if you're trying to recover from an unknown connection problem.

One way to solve this is to enter `SLEEP_MODE_DEEP` for a short period of time, but in this case I used a technique of sending a `AT+CFUN=16` directly at the modem to reset both the modem and the SIM card. Maybe that will help.

Also, because it's more efficient to leave the modem up on a reset, this code tries to do that first, and if that fails, then resets the modem. It also does certain checks first, so if the modem is really acting strangely, it will immediately do a modem reset.


## Application Watchdog

This code uses the application watchdog so if you don't return from loop for more than a minute, it resets the Electron, adding a connection event log entry so you know you have a blocking bug in your code.


## Using the testerFn

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
particle call electron2 testerFn "sleep deep"
```

Enters `SLEEP_MODE_DEEP` for 30 seconds. This turns off the cellular modem, so it will reconnect (blinking green) after waking up. You can also specify the number of seconds in the 3rd parameter; if not specfied the default is 30.


```
particle call electron2 testerFn "sleep stop 60"
```

Enters a stop mode sleep for 60 seconds. This uses a sleep with check on a pin. This turns off the cellular modem, so it will reconnect (blinking green) after waking up.


```
particle call electron2 testerFn "sleep networkStandby 15"
```
Enters a stop mode sleep for 15 seconds with `SLEEP_NETWORK_STANDBY`. This leaves the cellular modem on, so upon waking it should go right into blinking cyan to reconnect to the Particle cloud.


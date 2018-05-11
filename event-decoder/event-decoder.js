// 1. Install dependencies
// 	npm install
// 
// 2. Set your auth token:
//  export AUTH_TOKEN=xxxxxxx
// You can generate one, or get it from the Settings tab at https://build.particle.io.
// 
// 3. Run the program:
//	npm start
//
// You can also save the auth token in a file config.json that will be read at startup if present.
const config = require('./config');

var Particle = require('particle-api-js');
var particle = new Particle();

var deviceIdToName = {};

particle.listDevices({ auth:config.get('AUTH_TOKEN') }).then(
	function(devices) {
		// console.log("devices", devices);
		for(var ii = 0; ii < devices.body.length; ii++) {
			// console.log(devices.body[ii].id + "," + devices.body[ii].name);
			deviceIdToName[devices.body[ii].id] = devices.body[ii].name;
		}
		console.log('device names loaded');
		
		particle.getEventStream({ deviceId:'mine', auth:config.get('AUTH_TOKEN'), name:config.get('EVENT_NAME') }).then(
			function(stream) {
				stream.on('event', function(event) {
					// console.log("event: ", event);
					
					var deviceName = deviceIdToName[event.coreid] || event.coreid;

					var events = event.data.split(';');
					
					for(var ii = 0; ii < events.length; ii++) {
						
						var fields = events[ii].split(',');
						
						if (fields.length == 4) {
							var dateMillis = parseInt(fields[0], 10) * 1000;							
							var dateStr = ((dateMillis != 0) ? new Date(dateMillis).toISOString() : "no date"); 
							var millisStr = fields[1];
							var eventCode = parseInt(fields[2], 10);

							var data = parseInt(fields[3], 10);
														
							switch(eventCode) {
							case 0:
								msg = 'SETUP_STARTED';
								break;
								
							case 1:
								msg = 'CELLULAR_READY ' + (data ? 'connected' : 'disconnected');
								break;
								
							case 2:
								msg = 'CLOUD_CONNECTED ' + (data ? 'connected' : 'disconnected');
								break;
								
							case 3:
								msg = 'LISTENING_ENTERED';
								break;
								
							case 4:
								msg = 'MODEM_RESET';
								break;
							
							case 5:
								msg = 'REBOOT_LISTENING';
								break;
								
							case 6:
								msg = 'REBOOT_NO_CLOUD';
								break;

							case 7:
								msg = 'PING_DNS ' + (data ? 'success' : 'failed');
								break;

							case 8:
								msg = 'PING_API ' + (data ? 'success' : 'failed');
								break;

							case 9:
								msg = 'APP_WATCHDOG';
								break;

							case 10:
								msg = 'TESTER_RESET';
								break;

							case 11:
								msg = 'TESTER_APP_WATCHDOG';
								break;

							case 12:
								msg = 'TESTER_SLEEP';
								break;

							case 13:
								msg = 'LOW_BATTERY_SLEEP';
								break;

							case 14:
								msg = 'SESSION_EVENT_LOST';
								break;

							case 15:
								msg = 'SESSION_RESET';
								break;

							case 16:
								msg = 'TESTER_RESET_SESSION';
								break;

							case 17:
								msg = 'TESTER_RESET_MODEM';
								break;

							case 18:
								msg = 'RESET_REASON ';
								switch(data) {
								case 0:
									msg += 'RESET_REASON_NONE';
									break;

								case 10:
									msg += 'RESET_REASON_UNKNOWN';
									break;

								case 20:
									msg += 'RESET_REASON_PIN_RESET';
									break;

								case 30:
									msg += 'RESET_REASON_POWER_MANAGEMENT';
									break;

								case 40:
									msg += 'RESET_REASON_POWER_DOWN';
									break;

								case 50:
									msg += 'RESET_REASON_POWER_BROWNOUT';
									break;

								case 60:
									msg += 'RESET_REASON_WATCHDOG';
									break;

								case 70:
									msg += 'RESET_REASON_UPDATE';
									break;

								case 80:
									msg += 'RESET_REASON_UPDATE_ERROR';
									break;

								case 90:
									msg += 'RESET_REASON_UPDATE_TIMEOUT';
									break;

								case 100:
									msg += 'RESET_REASON_FACTORY_RESET';
									break;

								case 110:
									msg += 'RESET_REASON_SAFE_MODE';
									break;

								case 120:
									msg += 'RESET_REASON_DFU_MODE';
									break;

								case 130:
									msg += 'RESET_REASON_PANIC';
									break;

								case 140:
									msg += 'RESET_REASON_USER';
									break;
								}
								break;
								
							case 19:
								msg = 'TESTER_SAFE_MODE';
								break;

							case 20:
								msg = 'TESTER_PING ' + data;
								break;
								
							case 21:
								msg = 'STOP_SLEEP_WAKE';
								break;
							}
							console.log(deviceName + ',' + dateStr + ',' + millisStr + ',' + msg);
						}
					}
				});
			},
			function(err) {
				console.log("Failed to getEventStream: ", err);
			}
		);
	},
	function(err) {
		console.log("error listing devices", err);
	}
);




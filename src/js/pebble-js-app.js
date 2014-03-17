
//Globals
var device_code;
var user_code;
var verification_url;

//Request for an access token
var req = new XMLHttpRequest();
var oauth = "https://accounts.google.com/o/oauth2/device/code";
req.open('POST', oauth, true);
req.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
req.setRequestHeader("client_id","404936121299-r2tb0n7t3bl7ia3s9sl4t1qivuok3umr.apps.googleusercontent.com");
req.setRequestHeader("scope","https://www.googleapis.com/auth/calendar.readonly");
req.onload = function(e) {
 	   if (req.readyState == 4 && req.status == 200) {
 	     if(req.status == 200) {
		console.log("GOT RESPONSE");
		console.log(req.responseText);
		data = JSON.parse(req.responseText);
		device_code = data.device_code;
		user_code = data.user_code;
		verification_url = data.verification_url;
		Pebble.sendAppMessage( { "verificationCode" : user_code }, successful_delivery, failure_delivery);
     	 	} else { console.log("ERROR ON TOKEN REQUEST OH NO"); }
   	   }
}


Pebble.addEventListener("showConfiguration",
	function() {
		console.log("Showing configuration window");
		Pebble.openURL("http://www.google.com/device");
	}
);

Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");}
);

Pebble.addEventListener("ready",
			function(e) {
			    console.log("JavaScript app ready and running!");
			    Pebble.sendAppMessage( { jsReady : 42, }, successful_delivery,failure_delivery);
			}
);


function successful_delivery(e) {
    console.log("Successfully delivered message with transactionId="
		+ e.data.transactionId);
}

function failure_delivery(e) {
    console.log("Unable to deliver message with transactionId="
		+ e.data.transactionId
		+ " Error is: " + e.error.message);
}


Pebble.addEventListener("appmessage",
  function(e) {
    console.log("Received message: " + e.payload);
    console.log("Text" + e.payload["0"]);
    if(e.payload.noAccessToken != undefined) {
	console.log("We need to get an access token!");
	req.send("client_id=404936121299-r2tb0n7t3bl7ia3s9sl4t1qivuok3umr.apps.googleusercontent.com&scope=https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fcalendar.readonly");
	console.log("Request sent!");
    }
    //    if(e.payload.dummy == undefined) {
    //	console.log("THERE IS NO DUMMY");
    //    }

  }
);


function sendTimezoneToWatch() {
  // Get the number of seconds to add to convert localtime to utc
  var offsetMinutes = new Date().getTimezoneOffset() * 60;
  // Send it to the watch
  Pebble.sendAppMessage({ timezoneOffset: offsetMinutes })
}

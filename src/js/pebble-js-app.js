
//Globals
var device_code;
var user_code;
var verification_url;

//Permanent data that will be stored in the pebble
var access_token;
var refresh_token;
var expires_in; //The time this token in valid for, in seconds(usually 6 hours)


//Data for requests
var client_id = "404936121299-r2tb0n7t3bl7ia3s9sl4t1qivuok3umr.apps.googleusercontent.com";
var client_secret = "JbjE1hvvcYUguvog4f2nFB_A";
var grant_type = "http://oauth.net/grant_type/device/1.0";
var scope = "https://www.googleapis.com/auth/calendar.readonly";

//Request for an access token
var req = new XMLHttpRequest();
var oauth = "https://accounts.google.com/o/oauth2/device/code";
req.open('POST', oauth, true);
req.setRequestHeader("Content-Type","application/x-www-form-urlencoded");

req.onload = function(e) {
 	   if (req.readyState == 4 && req.status == 200) {
 	     if(req.status == 200) {
		console.log(req.responseText);
		data = JSON.parse(req.responseText);
		device_code = data.device_code;
		user_code = data.user_code;
		verification_url = data.verification_url;
		Pebble.sendAppMessage( { "verificationCode" : user_code }, successful_delivery, failure_delivery);

		//Set the device code, now that we have it
		token_request_data = "client_id=" + client_id + "&client_secret=" + client_secret + "&code=" + device_code
		+ "&grant_type=" + grant_type;

		console.log("TOKEN REQUEST DATA:" + token_request_data);

     	 	} else { console.log("ERROR ON OAUTH REQUEST OH NO"); }
   	   }
}

var token_request = new XMLHttpRequest();
var token_request_url = "https://accounts.google.com/o/oauth2/token";
var token_request_data;

token_request.open('POST', token_request_url, true);
token_request.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
token_request.onload = function(e) {
    if (token_request.readyState == 4 && token_request.status == 200) {
	if(token_request.status == 200) {
	    console.log("GOT THE REAL DEAL RESPONSE");
	    console.log(token_request.responseText);
	    data = JSON.parse(token_request.responseText);
	    if (data.error == undefined) { //There is no error
		access_token = data.access_token;
		refresh_token = data.refresh_token;
		expires_in = data.expires_in;
		Pebble.sendAppMessage( { gotTokens : 1 }, successful_delivery,failure_delivery);
		Pebble.sendAppMessage( { accessToken : access_token }, successful_delivery,failure_delivery);
                Pebble.sendAppMessage( { refreshToken : refresh_token }, successful_delivery,failure_delivery);
	    } else {
		console.log("THERE WAS AN ERROR IN AUTHORIZATION. THE ERROR IS: " + data.error);
	    }

	} else { console.log("ERROR ON TOKEN REQUEST REQUEST OH NO"); }
    }
}


Pebble.addEventListener("showConfiguration",
	function() {
		console.log("Showing configuration window");
		Pebble.openURL("http://www.google.com/device");
	}
);

Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");
  // On compeletion of a config screen, attempt to obtain the request tokens
  token_request.send(token_request_data);
    }
);

Pebble.addEventListener("ready",
			function(e) {
			    console.log("JavaScript app ready and running!");
			    Pebble.sendAppMessage( { jsReady : 42 }, successful_delivery,failure_delivery);
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
    console.log(e.payload);
    if(e.payload.noAccessToken != undefined) {
	console.log("We need to get an access token!");
	req.send("client_id=" + client_id + "&scope=" + scope);
	console.log("Request sent!");
    }
    if(e.payload.accessToken != undefined) {
	console.log("GOT ACCESS TOKEN!!!");
	access_token = e.payload.accessToken;
	console.log(access_token);
	retrieveData();
    }
    if(e.payload.refreshToken != undefined) {
	refresh_token = e.payload.refreshToken;
    }
  }
);


function sendTimezoneToWatch() {
  // Get the number of seconds to add to convert localtime to utc
  var offsetMinutes = new Date().getTimezoneOffset() * 60;
  // Send it to the watch
  Pebble.sendAppMessage({ timezoneOffset: offsetMinutes })
}

function offset() {
    var d = new Date();
    var n = d.getTimezoneOffset();
    return n/60; //Divide by 60 minutes/hour
}

function retrieveData() {
    var getCalendars = new XMLHttpRequest();
    var url = "https://www.googleapis.com/calendar/v3/users/me/calendarList";
    url = url + "?key=" + client_id + "&access_token=" +  access_token;
    console.log("URL:" + url);
    getCalendars.open('GET',url,true);

    getCalendars.onload = function(e) {
	console.log("WE LOADED");
	console.log(getCalendars.responseText);
	console.log("STATUS:" + getCalendars.status);
	if (getCalendars.readyState == 4 && getCalendars.status == 200) {
	    if(getCalendars.status == 200) {
                data = JSON.parse(getCalendars.responseText);
            } else { console.log("ERROR ON CALENDAR REQUEST OH NO"); }
	}
    }

    getCalendars.send();
    console.log("Sent calendar message");

}
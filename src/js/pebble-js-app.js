
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
var grant_type_refresh = "refresh_token";
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
     	 	} else { console.log("ERROR ON OAUTH REQUEST OH NO"); }
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
  // This shouldn't work if we didn't actually close the screen, because we won't have the device_code
  var request_data = "client_id=" + client_id + "&client_secret=" + client_secret + "&code=" + device_code
	    + "&grant_type=" + grant_type;
  console.log("ATTEMPTING TO FETCH ACCESS TOKEN(NOT REFRESH TOKEN), BECAUSE WE'RE BONQUERRS");
  getAccessToken(request_data);
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
    if(e.payload.noAccessToken != undefined) {
	console.log("JS determined it needs an access token! GOING TO FETCH A NEW ACCESS TOKEN");
	req.send("client_id=" + client_id + "&scope=" + scope);
    }
    if(e.payload.accessToken != undefined) {
	console.log("JS received accessToken");
	access_token = e.payload.accessToken;
	retrieveCalendars();
    }
    if(e.payload.refreshToken != undefined) {
      refresh_token = e.payload.refreshToken;
    }
  }
);


function getAccessToken(data) {
    var token_request = new XMLHttpRequest();
    var token_request_url = "https://accounts.google.com/o/oauth2/token";
    var token_request_data;
    token_request.open('POST', token_request_url, true);
    token_request.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
    token_request.onload = function(e) {
	if (token_request.readyState == 4) {
	    console.log(token_request.responseText);
	    if(token_request.status == 200) {
		data = JSON.parse(token_request.responseText);
		if (data.error == undefined) { //There is no error   
		    access_token = data.access_token;
		    refresh_token = data.refresh_token;
		    expires_in = data.expires_in;
		    Pebble.sendAppMessage({ gotTokens : 1, accessToken : access_token, refreshToken : refresh_token }, successful_delivery,failure_delivery);
		    
		    //After we get the access tokens, we always want to try and grab the calendar data
		    retrieveCalendars();
		} else {
		    console.log("THERE WAS AN ERROR IN AUTHORIZATION. THE ERROR IS: " + data.error);
		}

	    } else { console.log("ERROR ON TOKEN REQUEST OH NO"); }
	}
    }
    token_request.send(data);
}


function retrieveCalendars() {
    var getCalendars = new XMLHttpRequest();
    var url = "https://www.googleapis.com/calendar/v3/users/me/calendarList";
    url = url + "?key=" + client_id + "&access_token=" +  access_token;
    getCalendars.open('GET',url,true);

    getCalendars.onload = function(e) {
	console.log(getCalendars.responseText);
	if (getCalendars.readyState == 4) {
	    if(getCalendars.status == 200) {
                data = JSON.parse(getCalendars.responseText);
		parseCalendars(data);
            } else if(getCalendars.status == 401) {
		console.log("Access token probably expired. Grabbing a new one...");
		//REFRESH TOKEN DATA
		token_request_data = "client_id=" + client_id + "&client_secret=" + client_secret + "&refresh_token=" + refresh_token + "&grant_type=" + grant_type_refresh;
		getAccessToken(token_request_data);
	    } else { 
		console.log("ERROR ON CALENDAR REQUEST OH NO"); 
	    }
	}
    }

    getCalendars.send();
    console.log("Sent request to fetch calendars");

}

function parseCalendars(data){
    console.log(data);
    var id_array = [] ; //An array of the id's of all the calendars we'll need
    for (i = 0; i < data.items.length; i++) {
	id_array.push(data.items[i].id);
    }
    var nowString = formatDate(new Date());
    var tomorrowString = formatDate(new Date(new Date().getTime() + 24 * 60 * 60 * 1000)); //Offset by 24-hours
    var offsetString = "UTC" + formatOffset(new Date());
    
    var json_data = {};
    json_data["items"] = [];

    for (i = 0; i < id_array.length; i++ ){
	json_data["items"][i] = {};
	json_data["items"][i]["id"] = id_array[i];
    }
    json_data["timeMin"] = nowString;
    json_data["timeMax"] = tomorrowString;
    json_data["timeZone"] = offsetString;

    data = JSON.stringify(json_data);
    console.log(data);
    freeBusyRequest(data);
}

function freeBusyRequest(data) {
    var req = new XMLHttpRequest();
    var url = "https://www.googleapis.com/calendar/v3/freeBusy";
    url = url + "?key=" + client_id + "&access_token=" +  access_token;
    req.open('POST', url, true);
    req.setRequestHeader("Content-Type","application/json");

    req.onload = function(e) {
	console.log(req.responseText);
	if (req.readyState == 4 && req.status == 200) {
	    if(req.status == 200) {
                data = JSON.parse(req.responseText);
	    } else { console.log("ERROR ON FREEBUSY REQUEST OH NO"); }
	}
    }
    req.send(data);
}


//Format the date in a way that is acceppted by Google Calendar's HTTP requests
function formatOffset(d) {
    var halfHour;
    var negative; //We should MAKE it negative
    var zero;

    var offset = d.getTimezoneOffset() / 60;

    if (offset < 0) {
        negative = false;
        offset = offset * -1;
    }
    else
        negative = true;

    if (offset % 1 === 0)
        halfHour = false;
    else {
        halfHour = true;
        offset = offset - 0.5; //Get rid of the offset for now...                                                                                             
    }
    if (offset< 10)
        zero = true;
    else
        zero = false;

    var stringOffset = "";
    if(negative)
        stringOffset = stringOffset + "-";
    if(zero)
        stringOffset = stringOffset + "0";
    stringOffset = stringOffset + offset;
    if(halfHour)
        stringOffset = stringOffset + ":30";
    else
        stringOffset = stringOffset + ":00";    
    return stringOffset;
}

function formatDate(d) {
    var day = d.getDate();
    var month = d.getMonth() + 1; //Date is 0-indexed, we want 1-indexed
    var year = d.getFullYear();
    var hours = d.getHours();
    var minutes = d.getMinutes();
    var offset = d.getTimezoneOffset() / 60;
    var halfHour;
    var negative; //We should MAKE it negative
    var zero;

    var stringOffset = formatOffset(d);

    var endString = year + "-" + month + "-" + day + "T" + hours + ":" + minutes + ":00" + stringOffset;
    return endString;
}
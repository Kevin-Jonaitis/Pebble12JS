Pebble.addEventListener("ready",
    function(e) {
	console.log("HEY THERE. I'M RUNNING");
	var req = new XMLHttpRequest();
        var oauth = "https://accounts.google.com/o/oauth2/device/code";
	req.open('POST', oauth, true);
	req.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
	req.setRequestHeader("client_id","404936121299-r2tb0n7t3bl7ia3s9sl4t1qivuok3umr.apps.googleusercontent.com");
	req.setRequestHeader("scope","https://www.googleapis.com/auth/calendar.readonly");

 	req.onload = function(e) {
	console.log("LOADED");
 	   if (req.readyState == 4 && req.status == 200) {
 	     if(req.status == 200) {
		console.log("GOT RESPONSE");
		console.log(req.responseText);
     	 	} else { console.log("ERROR OH NO"); }
    	   }
  	}
req.send("client_id=404936121299-r2tb0n7t3bl7ia3s9sl4t1qivuok3umr.apps.googleusercontent.com&scope=https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fcalendar.readonly");
console.log("sent request!");
    }
);

Pebble.addEventListener("showConfiguration",
	function() {
		console.log("EVENT IS HERE");
		Pebble.openURL("http://www.google.com/device");
		setTimeout(testFinished, 2000);
	}
);

//Pebble.addEventListener("webviewclosed", function(e) {
//  console.log("configuration closed");
//})


function testFinished() {
	console.log(document.URL);
	console.log(window.location.href);
	if(document.url == 'https://accounts.google.com/o/oauth2/device/approval') {
		console.log("WINNER WINNER!!!");
		var location = "pebblejs://close";
		var req = new XMLHttpRequest();
		req.open('POST', oauth, true);
		req.send(null);
	} else {
		setTimeout(testFinished, 2000);
	}
}


Pebble.addEventListener("appmessage",
  function(e) {
    console.log("Received message: " + e.payload);
  }
);

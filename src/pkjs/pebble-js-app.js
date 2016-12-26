// File: pebble-js-app.js
// Written by Matthew Clark of SetPebble
var setPebbleToken = 'WUMT';

Pebble.addEventListener('ready', function(e) {
});

Pebble.addEventListener('appmessage', function(e) {
  var key = e.payload.action;
  if (typeof(key) != 'undefined') {
    var settings = localStorage.getItem(setPebbleToken);
    if (typeof(settings) == 'string') {
      try {
        Pebble.sendAppMessage(JSON.parse(settings));
      } catch (err) {
      }
    }
    
    var request = new XMLHttpRequest();
    request.open('GET', 'http://x.SetPebble.com/api/' + setPebbleToken + '/' + Pebble.getAccountToken(), true);
    request.onload = function(e) {
      if (request.readyState == 4 && request.status == 200){
        try {
          Pebble.sendAppMessage(JSON.parse(request.responseText));
        } catch (err) {
        }
      }
    };
    request.send(null);
  }
});

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('http://x.SetPebble.com/' + setPebbleToken + '/' + Pebble.getAccountToken());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if ((typeof(e.response) == 'string') && (e.response.length > 0)) {
    try {
      Pebble.sendAppMessage(JSON.parse(e.response));
      localStorage.setItem(setPebbleToken, e.response);
    } catch(err) {
    }
  }
});
javascript:(function(){ var vdrserver='127.0.0.1:43280'; var xmlhttp=new XMLHttpRequest(); xmlhttp.open("GET", "http://" + vdrserver + "/play?url=" + encodeURIComponent(location.href), true); xmlhttp.send(null); })();
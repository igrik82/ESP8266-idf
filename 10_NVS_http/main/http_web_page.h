#pragma once

#include <string>

const std::string web_page = R"(
<!doctype html>
<html lang=en>
<head>
<meta charset=UTF-8 />
<title>Configuration</title>
<style>body{font-family:Arial,sans-serif;max-width:500px;margin:7px auto;padding:5px;background-color:#f5f5f5}.section{border:1px solid #ccc;padding:15px;margin-bottom:20px;border-radius:5px;background-color:#f9f9f9}.title{color:#57565d;margin-top:20px;padding-top:5px;padding-bottom:0;text-align:center}.inner_title{color:#57565d;margin-top:0;padding-top:5px;padding-bottom:5px;text-align:center}.config-item{display:flex;align-items:center;margin-bottom:15px;margin:10px 0}.config-item label{width:120px;font-size:14px;color:#908e89;margin-left:15px;text-align:left}.sensor_label{width:220px;margin:0 15px}input{width:200px;padding:5px;margin-left:0}button{background-color:#1d6ac9;color:white;width:215px;padding:6px;border:0;border-radius:4px;margin-top:10px;margin-left:135px;cursor:pointer}.status-message{position:fixed;top:100px;left:50%;transform:translateX(-50%);margin-top:0;padding:7px 25px;border-radius:5px;font-weight:bold;opacity:0;visibility:hidden;transition:all .3s ease;z-index:1000}.success{background-color:#4caf50;color:white}.error{background-color:#f44336;color:white}.show{opacity:1;visibility:visible;top:3px}.sensor-container{display:flex;font-size:14px;color:#908e89;gap:20px}#temp1{color:#57565d}#temp2{color:#57565d}</style>
</head>
<body>
<div id=statusMessage class=status-message></div>
<h2 class=title>Configuration HDD Station</h2>
<form class=section>
<div class=sensor-container>
<div class=sensor_label>
<label>Sensor 0:</label>
<label id=temp1>-- °C</label>
</div>
<div class=sensor_label>
<label>Sensor 1:</label>
<label id=temp2>-- °C</label>
</div>
</div>
</form>
<script>const tempElements={temp1:document.getElementById("temp1"),temp2:document.getElementById("temp2"),};let isUpdating=false;async function fetchTemperatures(){if(isUpdating)return;try{isUpdating=true;document.body.classList.add("loading");const response=await fetch("/api/temperatures");if(!response.ok)throw new Error("Network failure");const data=await response.json();tempElements.temp1.textContent=`${data.temp1.toFixed(1)}°C`;tempElements.temp2.textContent=`${data.temp2.toFixed(1)}°C`;}catch(error){console.error("Error:",error);tempElements.temp1.textContent="Not responding";tempElements.temp2.textContent="Not responding";}finally{isUpdating=false;document.body.classList.remove("loading");}}
setInterval(fetchTemperatures,3000);fetchTemperatures();</script>
<form class=section onsubmit="event.preventDefault();saveSettings(this,'/save-fan')\"">
<h2 class=inner_title>Fan</h2>
<div class=config-item>
<label>Min HDD temp:</label>
<input type=number name=min_temp placeholder=%d />
</div>
<div class=config-item>
<label>Max HDD temp:</label>
<input type=number name=max_temp placeholder=%d />
</div>
<div class=config-item>
<label>Frequency:</label>
<input type=number name=fan_freq placeholder=%d />
</div>
<button type=submit>Save</button>
</form>
<form class=section onsubmit="event.preventDefault();saveSettings(this,'/save-sensor')\"">
<h2 class=inner_title>Sensors temperature correction</h2>
<div class=config-item>
<label>Sensor 0:</label>
<input type=number name=sens_corr_0 placeholder=%f />
</div>
<div class=config-item>
<label>Sensor 1:</label>
<input type=number name=sens_corr_1 placeholder=%f />
</div>
<button type=submit>Save</button>
</form>
<form class=section onsubmit="event.preventDefault();saveSettings(this,'/save-wifi')\"">
<h2 class=inner_title>WI-FI</h2>
<div class=config-item>
<label>SSID name:</label>
<input type=text name=ssid placeholder=%s />
</div>
<div class=config-item>
<label>Password:</label>
<input type=password name=wifi_password placeholder=%s />
</div>
<button type=submit>Save</button>
</form>
<form class=section onsubmit="event.preventDefault();saveSettings(this,'/save-mqtt')\"">
<h2 class=inner_title>MQTT</h2>
<div class=config-item>
<label>Address:</label>
<input type=text name=mqtt_address placeholder=%s />
</div>
<div class=config-item>
<label>Port:</label>
<input type=number name=mqtt_port placeholder=%d />
</div>
<div class=config-item>
<label>Username:</label>
<input type=text name=mqtt_user placeholder=%s />
</div>
<div class=config-item>
<label>Password:</label>
<input type=password name=mqtt_password placeholder=%s />
</div>
<button type=submit>Save</button>
</form>
<script>async function saveSettings(formElement,endpoint){const statusElement=document.getElementById("statusMessage");try{const formData=new FormData(formElement);const response=await fetch(endpoint,{method:"POST",body:formData,});if(response.ok){showStatus("Settings saved successfully!","success");}else{showStatus("Error saving settings ...","error");}}catch(error){showStatus("Network error: "+error.message,"error");}}
function showStatus(message,type){const statusElement=document.getElementById("statusMessage");statusElement.textContent=message;statusElement.className=`status-message ${type}show`;setTimeout(()=>{statusElement.classList.remove("show");},3000);}</script>
</body>
</html>
)";

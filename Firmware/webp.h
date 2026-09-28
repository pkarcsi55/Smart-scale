#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="hu">

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width, initial-scale=1.0">

<title>WiFi erőmérő</title>

<style>

* {
  box-sizing: border-box;
}

body {
  font-family: Arial, sans-serif;
  background: #111;
  color: white;
  text-align: center;
  margin: 0;
  padding: 8px;
}

.topbar {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 7px;
  width: 100%;
  margin-bottom: 8px;
}

button,
select {
  font-size: 16px;
  padding: 9px 12px;
  border-radius: 7px;
  border: none;
  white-space: nowrap;
}

#mass {
  font-size: 42px;
  font-weight: bold;
  min-width: 180px;
  padding: 2px 12px;
  white-space: nowrap;
}

.unit {
  font-size: 22px;
  font-weight: normal;
}

.graphContainer {
  width: 100%;
}

canvas {
  background: white;
  width: 100%;
  height: 65vh;
  min-height: 300px;
  max-height: 600px;
  display: block;
}

.info {
  display: flex;
  justify-content: center;
  gap: 30px;
  margin-top: 7px;
  font-size: 14px;
  color: #aaa;
}

@media (max-width: 750px) {

  body {
    padding: 4px;
  }

  .topbar {
    gap: 3px;
  }

  button,
  select {
    font-size: 13px;
    padding: 8px 6px;
  }

  #mass {
    font-size: 30px;
    min-width: 125px;
    padding: 0 4px;
  }

  .unit {
    font-size: 17px;
  }

  canvas {
    height: 67vh;
  }
}

</style>

</head>

<body>


<div class="topbar">

  <button onclick="tare()">
    TÁRA
  </button>

  <button onclick="startDynamic()">
    START
  </button>

  <button onclick="stopDynamic()">
    STOP
  </button>

  <div id="mass">
    0.0 <span class="unit">g</span>
  </div>

  <select id="forceScale"
          onchange="changeScale()">

    <option value="0.5">0,5 N</option>
    <option value="1">1 N</option>
    <option value="5" selected>5 N</option>
    <option value="10">10 N</option>

  </select>

  <button onclick="clearGraph()">
    TÖRLÉS
  </button>

</div>


<div class="graphContainer">

  <canvas id="graph"
          width="1200"
          height="600">
  </canvas>

</div>


<div class="info">

  <div id="raw">
    RAW: 0
  </div>

  <div id="rate">
    Mintavétel: -- Hz
  </div>

  <div id="clients">
    Kliensek: 0
  </div>

</div>


<script>

// ==================================================
// CANVAS
// ==================================================

const canvas =
  document.getElementById("graph");

const ctx =
  canvas.getContext("2d");


// ==================================================
// BEÁLLÍTÁS
// ==================================================

const DISPLAY_SECONDS = 10;

let forceScale = 5;

let samples = [];

let running = false;

let firstTime = null;


// ==================================================
// WEBSOCKET
// ==================================================

let socket;

function connectWebSocket()
{
  socket =
    new WebSocket(
      "ws://" +
      window.location.hostname +
      ":81/"
    );


  socket.onopen =
    function()
    {
      console.log(
        "WebSocket connected"
      );
    };


  socket.onclose =
    function()
    {
      console.log(
        "WebSocket disconnected"
      );

      // újracsatlakozás
      setTimeout(
        connectWebSocket,
        1000
      );
    };


  socket.onmessage =
    function(event)
    {
      processMessage(
        event.data
      );
    };
}


// ==================================================
// ÜZENET FELDOLGOZÁSA
// ==================================================

function processMessage(msg)
{
  const parts =
    msg.split(";");


  if(parts.length < 5)
    return;


  if(parts[0] !== "D")
    return;


  // ------------------------------------------------
  // AKTUÁLIS ÉRTÉKEK
  // ------------------------------------------------

  const raw =
    parseInt(parts[1]);

  const mass =
    parseFloat(parts[2]);

  const rate =
    parseFloat(parts[3]);

  const clients =
    parseInt(parts[4]);


  document.getElementById("mass").innerHTML =
    mass.toFixed(1) +
    ' <span class="unit">g</span>';


  document.getElementById("raw").innerHTML =
    "RAW: " + raw;


  document.getElementById("rate").innerHTML =
    "Mintavétel: " +
    rate.toFixed(1) +
    " Hz";


  document.getElementById("clients").innerHTML =
    "Kliensek: " +
    clients;


  // ------------------------------------------------
  // DINAMIKUS MINTÁK
  // ------------------------------------------------

  if(!running)
    return;


  for(let i = 5;
      i < parts.length;
      i++)
  {
    const pair =
      parts[i].split(",");


    if(pair.length !== 2)
      continue;


    const t =
      parseInt(pair[0]);


    const f =
      parseFloat(pair[1]);


    if(firstTime === null)
      firstTime = t;


    samples.push({
      t:(t-firstTime)/1000000.0,
      f:f
    });
  }


  // ------------------------------------------------
  // CSAK AZ UTOLSÓ 10 MÁSODPERC
  // ------------------------------------------------

  if(samples.length > 0)
  {
    const newestTime =
      samples[
        samples.length-1
      ].t;


    const cutoff =
      newestTime -
      DISPLAY_SECONDS;


    while(
      samples.length > 0 &&
      samples[0].t < cutoff
    )
    {
      samples.shift();
    }
  }


  drawGraph();
}


// ==================================================
// TÁRA
// ==================================================

function tare()
{
  fetch("/tare");
}


// ==================================================
// START
// ==================================================

function startDynamic()
{
  samples = [];

  firstTime = null;

  running = true;

  drawGraph();
}


// ==================================================
// STOP
// ==================================================

function stopDynamic()
{
  running = false;
}


// ==================================================
// TÖRLÉS
// ==================================================

function clearGraph()
{
  samples = [];

  firstTime = null;

  drawGraph();
}


// ==================================================
// SKÁLA
// ==================================================

function changeScale()
{
  forceScale =
    parseFloat(
      document.getElementById(
        "forceScale"
      ).value
    );

  drawGraph();
}


// ==================================================
// GRAFIKON
// ==================================================

function drawGraph()
{
  ctx.clearRect(
    0,
    0,
    canvas.width,
    canvas.height
  );


  const left = 65;
  const right = 15;
  const top = 20;

  const bottom =
    canvas.height - 45;


  const graphWidth =
    canvas.width -
    left -
    right;


  const graphHeight =
    bottom -
    top;


  // =================================================
  // HÁLÓ
  // =================================================

  ctx.strokeStyle =
    "#dddddd";

  ctx.lineWidth = 1;


  for(let i = 0;
      i <= 5;
      i++)
  {
    const y =
      top +
      i *
      graphHeight / 5;


    ctx.beginPath();

    ctx.moveTo(
      left,
      y
    );

    ctx.lineTo(
      canvas.width-right,
      y
    );

    ctx.stroke();
  }


  for(let i = 0;
      i <= 10;
      i++)
  {
    const x =
      left +
      i *
      graphWidth / 10;


    ctx.beginPath();

    ctx.moveTo(
      x,
      top
    );

    ctx.lineTo(
      x,
      bottom
    );

    ctx.stroke();
  }


  // =================================================
  // Y TENGELY
  // =================================================

  ctx.fillStyle =
    "black";

  ctx.font =
    "20px Arial";


  for(let i = 0;
      i <= 5;
      i++)
  {
    const value =
      forceScale *
      (5-i) / 5;


    const y =
      top +
      i *
      graphHeight / 5;


    ctx.fillText(
      value.toFixed(1),
      8,
      y+6
    );
  }


  ctx.fillText(
    "N",
    45,
    17
  );


  if(samples.length < 2)
    return;


  // =================================================
  // IDŐABLAK
  // =================================================

  const newestTime =
    samples[
      samples.length-1
    ].t;


  let t0;
  let t1;


  if(newestTime <
     DISPLAY_SECONDS)
  {
    t0 = 0;
    t1 = DISPLAY_SECONDS;
  }
  else
  {
    t1 = newestTime;

    t0 =
      t1 -
      DISPLAY_SECONDS;
  }


  // =================================================
  // X TENGELY
  // =================================================

  for(let i = 0;
      i <= 10;
      i += 2)
  {
    const time =
      t0 + i;


    const x =
      left +
      i *
      graphWidth /
      DISPLAY_SECONDS;


    ctx.fillText(
      time.toFixed(0) + " s",
      x-15,
      canvas.height-12
    );
  }


  // =================================================
  // ERŐGÖRBE
  // =================================================

  ctx.strokeStyle =
    "blue";

  ctx.lineWidth = 3;

  ctx.beginPath();


  let drawing = false;


  for(let i = 0;
      i < samples.length;
      i++)
  {
    const s =
      samples[i];


    if(s.t < t0)
      continue;


    const x =
      left +
      (s.t-t0) /
      DISPLAY_SECONDS *
      graphWidth;


    let f = s.f;


    if(f < 0)
      f = 0;


    if(f > forceScale)
      f = forceScale;


    const y =
      bottom -
      (f/forceScale) *
      graphHeight;


    if(!drawing)
    {
      ctx.moveTo(
        x,
        y
      );

      drawing = true;
    }
    else
    {
      ctx.lineTo(
        x,
        y
      );
    }
  }


  ctx.stroke();
}


// ==================================================
// INDÍTÁS
// ==================================================

drawGraph();

connectWebSocket();

</script>

</body>

</html>

)rawliteral";
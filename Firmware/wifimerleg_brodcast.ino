/*
 * WiFi mérleg + dinamikus erőmérő V4
 * ESP32 + HX711 + WebSocket broadcast
 *
 * HX711:
 *   DOUT -> GPIO 22
 *   SCK  -> GPIO 19
 *
 * Kalibráció:
 *   1717.17 count/g
 *
 * WiFi AP:
 *   SSID: MERLEG
 *   URL : http://192.168.4.1
 *
 * HTTP server:      port 80
 * WebSocket server: port 81
 *
 * A HX711 minden mintáját eltároljuk.
 * A mintákat csomagokban, kb. 20 Hz-cel
 * broadcastoljuk az összes kliensnek.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "HX711.h"
#include "webp.h"

// ==================================================
// HARDVER
// ==================================================

#define HX_DOUT 22
#define HX_SCK  19

HX711 scale;

// ==================================================
// KALIBRÁCIÓ
// ==================================================

const float calibrationFactor = 1717.17f;
long tareOffset = 0;

// ==================================================
// WIFI
// ==================================================

const char* ssid = "MERLEG";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

// ==================================================
// AKTUÁLIS MÉRÉS
// ==================================================

long rawValue = 0;
float mass_g = 0.0f;
float force_N = 0.0f;

// ==================================================
// MINTAVÉTELI FREKVENCIA
// ==================================================

uint32_t rateStartUs = 0;
uint32_t rateSamples = 0;
float sampleRateHz = 0.0f;

// ==================================================
// WEBSOCKET KLIENSEK
// ==================================================

volatile int clientCount = 0;

// ==================================================
// BROADCAST PUFFER
// ==================================================

struct WsSample
{
  uint32_t t_us;
  float force;
};

// 20 minta bőven elég.
// 50 ms alatt várhatóan kb. 5 minta érkezik.

const int WS_BUFFER_SIZE = 20;

WsSample wsBuffer[WS_BUFFER_SIZE];
int wsSampleCount = 0;

// kb. 20 WebSocket csomag / másodperc

const uint32_t WS_SEND_INTERVAL_US = 50000UL;

uint32_t lastWsSendUs = 0;

// ==================================================
// HTTP
// ==================================================

void handleRoot()
{
  server.send_P(
    200,
    "text/html",
    INDEX_HTML
  );
}

// --------------------------------------------------
// TÁRA
// --------------------------------------------------

void handleTare()
{
  tareOffset = rawValue;

  server.send(
    200,
    "text/plain",
    "OK"
  );

  Serial.println("# TARE");
}

// ==================================================
// WEBSOCKET ESEMÉNYEK
// ==================================================

void webSocketEvent(
  uint8_t num,
  WStype_t type,
  uint8_t *payload,
  size_t length
)
{
  switch(type)
  {
    case WStype_CONNECTED:
    {
      clientCount++;

      IPAddress ip =
        webSocket.remoteIP(num);

      Serial.print("# WS client connected: ");
      Serial.print(num);
      Serial.print("  IP=");
      Serial.print(ip);
      Serial.print("  clients=");
      Serial.println(clientCount);

      break;
    }

    case WStype_DISCONNECTED:
    {
      if(clientCount > 0)
        clientCount--;

      Serial.print("# WS client disconnected: ");
      Serial.print(num);
      Serial.print("  clients=");
      Serial.println(clientCount);

      break;
    }

    default:
      break;
  }
}

// ==================================================
// WEBSOCKET BROADCAST
// ==================================================

void sendWebSocketData()
{
  if(wsSampleCount == 0)
    return;

  /*
   * Formátum:
   *
   * D;raw;mass;rate;clients;t1,f1;t2,f2;...
   *
   * Példa:
   *
   * D;345076;153.620;96.1;3;
   * 1234567,1.5023;
   * 1244980,1.5102;...
   *
   * Egyetlen üzenetben:
   *
   * D;345076;153.620;96.1;3;1234567,1.5023;...
   */

  String msg;

  msg.reserve(400);

  msg = "D;";

  msg += String(rawValue);
  msg += ";";

  msg += String(mass_g, 3);
  msg += ";";

  msg += String(sampleRateHz, 1);
  msg += ";";

  msg += String(clientCount);

  for(int i = 0; i < wsSampleCount; i++)
  {
    msg += ";";

    msg += String(
      wsBuffer[i].t_us
    );

    msg += ",";

    msg += String(
      wsBuffer[i].force,
      5
    );
  }

  // -----------------------------------------------
  // Ugyanaz az üzenet minden csatlakozott kliensnek
  // -----------------------------------------------

  webSocket.broadcastTXT(msg);

  wsSampleCount = 0;
}

// ==================================================
// SETUP
// ==================================================

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("# WiFi Force Sensor V4");
  Serial.println("# WebSocket broadcast");
  Serial.println("# DOUT = GPIO22");
  Serial.println("# SCK  = GPIO19");

  // =================================================
  // HX711
  // =================================================

  scale.begin(
    HX_DOUT,
    HX_SCK
  );

  Serial.println("# Waiting for HX711...");

  while(!scale.is_ready())
  {
    delay(1);
  }

  // =================================================
  // KEZDETI TÁRA
  // =================================================

  Serial.println("# Initial tare...");

  int64_t sum = 0;

  const int N = 30;

  for(int i = 0; i < N; i++)
  {
    while(!scale.is_ready())
    {
      delay(1);
    }

    sum += scale.read();
  }

  tareOffset =
    (long)(sum / N);

  Serial.print("# Initial tare = ");
  Serial.println(tareOffset);

  // =================================================
  // WIFI AP
  // =================================================

  WiFi.mode(WIFI_AP);

  WiFi.softAP(
    ssid,
    password
  );

  Serial.println();
  Serial.println("# WiFi AP started");

  Serial.print("# SSID: ");
  Serial.println(ssid);

  Serial.print("# IP: ");
  Serial.println(WiFi.softAPIP());

  // =================================================
  // HTTP SERVER
  // =================================================

  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/tare",
    handleTare
  );

  server.begin();

  Serial.println("# HTTP server started");

  // =================================================
  // WEBSOCKET SERVER
  // =================================================

  webSocket.begin();

  webSocket.onEvent(
    webSocketEvent
  );

  Serial.println("# WebSocket server started on port 81");

  // =================================================
  // SERIAL HEADER
  // =================================================

  Serial.println();
  Serial.println(
    "# time_ms;raw;mass_g;force_N"
  );

  lastWsSendUs = micros();
}

// ==================================================
// LOOP
// ==================================================

void loop()
{
  // =================================================
  // HÁLÓZATI KISZOLGÁLÁS
  // =================================================

  server.handleClient();

  webSocket.loop();

  // =================================================
  // HX711
  // =================================================

  if(scale.is_ready())
  {
    uint32_t t =
      micros();

    long raw =
      scale.read();

    rawValue =
      raw;

    // =================================================
    // MINTAVÉTELI FREKVENCIA
    // =================================================

    if(rateStartUs == 0)
    {
      rateStartUs = t;
      rateSamples = 0;
    }
    else
    {
      rateSamples++;

      uint32_t elapsed =
        t - rateStartUs;

      if(elapsed >= 1000000UL)
      {
        sampleRateHz =
          (float)rateSamples *
          1000000.0f /
          (float)elapsed;

        rateSamples = 0;
        rateStartUs = t;
      }
    }

    // =================================================
    // KALIBRÁCIÓ
    // =================================================

    long corrected =
      raw -
      tareOffset;

    mass_g =
      (float)corrected /
      calibrationFactor;

    force_N =
      (mass_g / 1000.0f) *
      9.80665f;

    // =================================================
    // WEBSOCKET PUFFER
    // =================================================

    if(wsSampleCount < WS_BUFFER_SIZE)
    {
      wsBuffer[wsSampleCount].t_us =
        t;

      wsBuffer[wsSampleCount].force =
        force_N;

      wsSampleCount++;
    }

    // =================================================
    // SERIAL
    // =================================================

    Serial.print(millis());
    Serial.print(";");

    Serial.print(rawValue);
    Serial.print(";");

    Serial.print(mass_g, 3);
    Serial.print(";");

    Serial.println(force_N, 5);
  }

  // =================================================
  // BROADCAST
  // =================================================

  uint32_t now =
    micros();

  if(
    (uint32_t)(now - lastWsSendUs)
    >= WS_SEND_INTERVAL_US
  )
  {
    sendWebSocketData();

    lastWsSendUs = now;
  }
}
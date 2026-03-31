#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ============================
// Motor Pins
// ============================
const int STEP1 = 2;   // spindle motor step
const int DIR1  = 3;   // spindle motor dir

const int STEP2 = 0;   // traverse motor step
const int DIR2  = 1;   // traverse motor dir

// ============================
// Hall Sensors
// ============================
const int LEFT_SENSOR  = 4;
const int RIGHT_SENSOR = 5;

// ============================
// Motion Parameters
// ============================
volatile int SPEED1_US = 8000;   // spindle interval
volatile int SPEED2_US = 10000;   // traverse interval

const unsigned int STEP_PULSE_US = 4;

// ============================
// WiFi / Preferences
// ============================
const char* AP_SSID = "CoilWinder";
const char* AP_PASS = "12345678";

WebServer server(80);
Preferences prefs;

// ============================
// Motion State
// ============================
bool traverseRight = true;
bool machineRunning = false;

unsigned long lastStep1Time = 0;
unsigned long lastStep2Time = 0;
unsigned long lastReverseTime = 0;

const unsigned long REVERSE_LOCKOUT_MS = 120;

// ============================
// Sensor Logic
// ============================
// If your KY-003 modules are opposite polarity,
// change LOW to HIGH in these functions.
bool leftTriggered() {
  return digitalRead(LEFT_SENSOR) == LOW;
}

bool rightTriggered() {
  return digitalRead(RIGHT_SENSOR) == LOW;
}

// ============================
// Reverse Traverse
// ============================
void reverseTraverse(bool goRight) {
  traverseRight = goRight;
  digitalWrite(DIR2, traverseRight ? HIGH : LOW);
  lastReverseTime = millis();
}

// ============================
// HTML
// ============================
String sensorBadge(bool active) {
  if (active) {
    return "<span style='color:#ffffff;background:#16a34a;padding:6px 12px;border-radius:999px;font-weight:bold;'>DETECTED</span>";
  }
  return "<span style='color:#111827;background:#e5e7eb;padding:6px 12px;border-radius:999px;font-weight:bold;'>IDLE</span>";
}

String htmlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="1">
  <title>Coil Winding Machine</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f5f7fb;
      margin: 0;
      padding: 20px;
      color: #111827;
    }
    .container {
      max-width: 760px;
      margin: auto;
    }
    h1 {
      margin-bottom: 8px;
      font-size: 28px;
    }
    .subtitle {
      color: #6b7280;
      margin-bottom: 18px;
    }
    .card {
      background: white;
      border-radius: 16px;
      padding: 18px;
      margin-bottom: 16px;
      box-shadow: 0 4px 16px rgba(0,0,0,0.08);
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    .item {
      background: #f9fafb;
      border-radius: 12px;
      padding: 12px;
    }
    .label {
      font-size: 13px;
      color: #6b7280;
      margin-bottom: 6px;
    }
    .value {
      font-size: 20px;
      font-weight: bold;
    }
    label {
      display: block;
      margin-top: 12px;
      margin-bottom: 6px;
      font-weight: bold;
    }
    input[type=number] {
      width: 100%;
      padding: 12px;
      border-radius: 10px;
      border: 1px solid #d1d5db;
      box-sizing: border-box;
      font-size: 16px;
    }
    .btn {
      display: inline-block;
      padding: 12px 18px;
      border: none;
      border-radius: 12px;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
      margin-top: 12px;
      margin-right: 8px;
    }
    .run  { background: #16a34a; color: white; }
    .stop { background: #dc2626; color: white; }
    .save { background: #2563eb; color: white; width: 100%; }
    .warn {
      margin-top: 12px;
      padding: 12px;
      border-radius: 12px;
      background: #fff7ed;
      color: #9a3412;
      font-size: 14px;
    }
    .footer {
      text-align: center;
      color: #6b7280;
      font-size: 13px;
      margin-top: 10px;
    }
    @media (max-width: 640px) {
      .grid {
        grid-template-columns: 1fr;
      }
      .btn {
        width: 100%;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Coil Winding Machine</h1>
    <div class="subtitle">Sensor-based traverse control with Wi-Fi setup panel</div>

    <div class="card">
      <div class="grid">
        <div class="item">
          <div class="label">Machine Status</div>
          <div class="value">%RUNSTATE%</div>
        </div>
        <div class="item">
          <div class="label">Traverse Direction</div>
          <div class="value">%DIRSTATE%</div>
        </div>
        <div class="item">
          <div class="label">Left Sensor</div>
          <div class="value">%LEFTSTATE%</div>
        </div>
        <div class="item">
          <div class="label">Right Sensor</div>
          <div class="value">%RIGHTSTATE%</div>
        </div>
      </div>

      <form action="/run" method="GET" style="display:inline;">
        <button class="btn run" type="submit">Run Machine</button>
      </form>

      <form action="/stop" method="GET" style="display:inline;">
        <button class="btn stop" type="submit">Stop Machine</button>
      </form>

      <div class="warn">
        This version reverses only from the hall sensors. If a sensor or magnet fails, the carriage will not auto-stop.
      </div>
    </div>

    <div class="card">
      <form action="/save" method="GET">
        <label for="speed1">Spindle Speed (microseconds)</label>
        <input type="number" id="speed1" name="speed1" min="100" value="%SPEED1%">

        <label for="speed2">Traverse Speed (microseconds)</label>
        <input type="number" id="speed2" name="speed2" min="100" value="%SPEED2%">

        <button class="btn save" type="submit">Save Settings</button>
      </form>
    </div>

    <div class="footer">
      Wi-Fi AP: CoilWinder
    </div>
  </div>
</body>
</html>
)rawliteral";

  html.replace("%RUNSTATE%", machineRunning ? "RUNNING" : "STOPPED");
  html.replace("%DIRSTATE%", traverseRight ? "RIGHT" : "LEFT");
  html.replace("%LEFTSTATE%", sensorBadge(leftTriggered()));
  html.replace("%RIGHTSTATE%", sensorBadge(rightTriggered()));
  html.replace("%SPEED1%", String(SPEED1_US));
  html.replace("%SPEED2%", String(SPEED2_US));

  return html;
}

// ============================
// Web Handlers
// ============================
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSave() {
  if (server.hasArg("speed1")) SPEED1_US = server.arg("speed1").toInt();
  if (server.hasArg("speed2")) SPEED2_US = server.arg("speed2").toInt();

  prefs.begin("coilcfg", false);
  prefs.putInt("speed1", SPEED1_US);
  prefs.putInt("speed2", SPEED2_US);
  prefs.end();

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleRun() {
  machineRunning = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStop() {
  machineRunning = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

// ============================
// Setup
// ============================
void setup() {
  pinMode(STEP1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(STEP2, OUTPUT);
  pinMode(DIR2, OUTPUT);

  pinMode(LEFT_SENSOR, INPUT_PULLUP);
  pinMode(RIGHT_SENSOR, INPUT_PULLUP);

  digitalWrite(DIR1, HIGH);   // spindle fixed direction
  digitalWrite(DIR2, HIGH);   // traverse starts to the right
  traverseRight = true;

  prefs.begin("coilcfg", true);
  SPEED1_US = prefs.getInt("speed1", 1500);
  SPEED2_US = prefs.getInt("speed2", 2000);
  prefs.end();

  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/run", handleRun);
  server.on("/stop", handleStop);
  server.begin();
}

// ============================
// Step Pulse Helper
// ============================
void pulseStepPin(int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(STEP_PULSE_US);
  digitalWrite(pin, LOW);
}

// ============================
// Loop
// ============================
void loop() {
  server.handleClient();

  if (!machineRunning) return;

  unsigned long nowMicros = micros();
  unsigned long nowMillis = millis();

  // Reverse only by sensors
  bool lockoutExpired = (nowMillis - lastReverseTime) > REVERSE_LOCKOUT_MS;

  if (lockoutExpired) {
    if (traverseRight && rightTriggered()) {
      reverseTraverse(false);
    } else if (!traverseRight && leftTriggered()) {
      reverseTraverse(true);
    }
  }

  // Spindle motor
  if ((long)(nowMicros - lastStep1Time) >= SPEED1_US) {
    lastStep1Time = nowMicros;
    pulseStepPin(STEP1);
  }

  // Traverse motor
  if ((long)(nowMicros - lastStep2Time) >= SPEED2_US) {
    lastStep2Time = nowMicros;
    pulseStepPin(STEP2);
  }
}
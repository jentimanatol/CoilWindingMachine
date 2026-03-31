#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ============================
// Motor Pins (same base config)
// ============================
const int STEP1 = 2;   // spindle motor step
const int DIR1  = 3;   // spindle motor dir

const int STEP2 = 0;   // traverse motor step
const int DIR2  = 1;   // traverse motor dir

// ============================
// New Hall Sensors
// ============================
const int LEFT_SENSOR  = 4;
const int RIGHT_SENSOR = 5;

// ============================
// Timing / Motion Parameters
// ============================
volatile int TRAVEL_STEPS = 2000;   // fallback max traverse steps
volatile int SPEED1_US    = 1500;   // spindle pulse interval
volatile int SPEED2_US    = 2000;   // traverse pulse interval

const unsigned int STEP_PULSE_US = 4;   // step pulse width

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
unsigned long lastStep1Time = 0;
unsigned long lastStep2Time = 0;
unsigned long lastReverseTime = 0;

long traverseCount = 0;                 // fallback position counter
const unsigned long REVERSE_LOCKOUT_MS = 120;  // debounce / re-trigger block

bool machineRunning = true;

// ============================
// Sensor Logic
// ============================
// KY-003 modules are often ACTIVE LOW when magnet is detected.
// If your module behaves opposite, change LOW to HIGH below.
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
  traverseCount = 0;
  lastReverseTime = millis();
}

// ============================
// HTML Page
// ============================
String htmlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Coil Winding Machine</title>
  <style>
    body { font-family: Arial; max-width: 700px; margin: 20px auto; padding: 10px; }
    h1 { margin-bottom: 10px; }
    .card { border: 1px solid #ccc; border-radius: 10px; padding: 16px; margin-bottom: 14px; }
    label { display: block; margin-top: 10px; font-weight: bold; }
    input[type=number] { width: 100%; padding: 8px; margin-top: 5px; box-sizing: border-box; }
    button {
      margin-top: 14px; padding: 10px 16px; border: none; border-radius: 8px;
      cursor: pointer; font-size: 16px;
    }
    .saveBtn { background: #2563eb; color: white; }
    .runBtn  { background: #16a34a; color: white; margin-right: 8px; }
    .stopBtn { background: #dc2626; color: white; }
    .status { font-size: 17px; margin: 8px 0; }
    .small  { color: #555; font-size: 14px; }
  </style>
</head>
<body>
  <h1>Coil Winding Machine</h1>

  <div class="card">
    <div class="status"><b>Status:</b> %RUNSTATE%</div>
    <div class="status"><b>Traverse Direction:</b> %DIRSTATE%</div>
    <div class="status"><b>Left Sensor:</b> %LEFTSTATE%</div>
    <div class="status"><b>Right Sensor:</b> %RIGHTSTATE%</div>
    <div class="small">Magnet should trigger only when carriage reaches the corresponding edge.</div>
    <form action="/run" method="GET" style="display:inline;">
      <button class="runBtn" type="submit">Run</button>
    </form>
    <form action="/stop" method="GET" style="display:inline;">
      <button class="stopBtn" type="submit">Stop</button>
    </form>
  </div>

  <div class="card">
    <form action="/save" method="GET">
      <label>Fallback Travel Steps</label>
      <input type="number" name="travel" value="%TRAVEL%">

      <label>Spindle Speed (microseconds)</label>
      <input type="number" name="speed1" value="%SPEED1%">

      <label>Traverse Speed (microseconds)</label>
      <input type="number" name="speed2" value="%SPEED2%">

      <button class="saveBtn" type="submit">Save Settings</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

  html.replace("%RUNSTATE%", machineRunning ? "RUNNING" : "STOPPED");
  html.replace("%DIRSTATE%", traverseRight ? "RIGHT" : "LEFT");
  html.replace("%LEFTSTATE%", leftTriggered() ? "TRIGGERED" : "IDLE");
  html.replace("%RIGHTSTATE%", rightTriggered() ? "TRIGGERED" : "IDLE");
  html.replace("%TRAVEL%", String(TRAVEL_STEPS));
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
  if (server.hasArg("travel")) TRAVEL_STEPS = server.arg("travel").toInt();
  if (server.hasArg("speed1")) SPEED1_US = server.arg("speed1").toInt();
  if (server.hasArg("speed2")) SPEED2_US = server.arg("speed2").toInt();

  prefs.begin("coilcfg", false);
  prefs.putInt("travel", TRAVEL_STEPS);
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

  digitalWrite(DIR1, HIGH);  // spindle fixed direction
  digitalWrite(DIR2, HIGH);  // start traverse right
  traverseRight = true;

  prefs.begin("coilcfg", true);
  TRAVEL_STEPS = prefs.getInt("travel", 2000);
  SPEED1_US    = prefs.getInt("speed1", 1500);
  SPEED2_US    = prefs.getInt("speed2", 2000);
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
// Main Loop
// ============================
void loop() {
  server.handleClient();

  if (!machineRunning) {
    return;
  }

  unsigned long nowMicros = micros();
  unsigned long nowMillis = millis();

  // ----------------------------
  // Sensor-based reversal
  // ----------------------------
  bool lockoutExpired = (nowMillis - lastReverseTime) > REVERSE_LOCKOUT_MS;

  if (lockoutExpired) {
    if (traverseRight && rightTriggered()) {
      reverseTraverse(false); // go left
    } 
    else if (!traverseRight && leftTriggered()) {
      reverseTraverse(true);  // go right
    }
  }

  // ----------------------------
  // Spindle motor
  // ----------------------------
  if ((long)(nowMicros - lastStep1Time) >= SPEED1_US) {
    lastStep1Time = nowMicros;
    pulseStepPin(STEP1);
  }

  // ----------------------------
  // Traverse motor
  // ----------------------------
  if ((long)(nowMicros - lastStep2Time) >= SPEED2_US) {
    lastStep2Time = nowMicros;
    pulseStepPin(STEP2);
    traverseCount++;

    // fallback reverse if a sensor was missed
    if (traverseCount >= TRAVEL_STEPS) {
      reverseTraverse(!traverseRight);
    }
  }
}
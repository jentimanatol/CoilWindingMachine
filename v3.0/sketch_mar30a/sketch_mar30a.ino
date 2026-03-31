#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ======================================================
// EEPROM-LIKE STORAGE USING PREFERENCES
// ======================================================
Preferences prefs;

// ======================================================
// CONFIG VALUES
// ======================================================
int SPEED1_US = 1500;
int SPEED2_US = 2000;

// ======================================================
// MOTOR PINS
// ======================================================
#define STEP1 2
#define DIR1  3

#define STEP2 0
#define DIR2  1

// ======================================================
// SENSOR PINS
// ======================================================
#define LEFT_SENSOR  6
#define RIGHT_SENSOR 7

// ======================================================
// STEP PULSE WIDTH
// ======================================================
#define STEP_PULSE_US 200

// ======================================================
// MOTOR INTERNAL VARIABLES
// ======================================================
unsigned long t1 = 0;
unsigned long t2 = 0;

bool traverseDir = true;     // true = right, false = left
bool machineRunning = false;

unsigned long lastReverseMs = 0;
const unsigned long REVERSE_LOCKOUT_MS = 120;

// ======================================================
// WIFI ACCESS POINT
// ======================================================
const char* ssid = "CoilWinder";
const char* password = "12345678";

WebServer server(80);

// ======================================================
// SENSOR FUNCTIONS
// ======================================================
// KY-003 is often ACTIVE LOW when magnet is detected.
// If your logic is inverted, change LOW to HIGH.
bool leftTriggered() {
  return digitalRead(LEFT_SENSOR) == LOW;
}

bool rightTriggered() {
  return digitalRead(RIGHT_SENSOR) == LOW;
}

// ======================================================
// HTML PAGE
// ======================================================
String badge(bool active) {
  if (active) {
    return "<span style='background:#16a34a;color:white;padding:6px 12px;border-radius:12px;font-weight:bold;'>DETECTED</span>";
  }
  return "<span style='background:#374151;color:white;padding:6px 12px;border-radius:12px;font-weight:bold;'>IDLE</span>";
}

String htmlPage() {
  String page = "<html><head><title>Coil Winder</title>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<meta http-equiv='refresh' content='1'>";
  page += "<style>";
  page += "body{font-family:Arial;background:#111;color:white;padding:20px;max-width:800px;margin:auto;}";
  page += ".logo{font-size:28px;font-weight:bold;text-align:center;color:#4da6ff;margin-bottom:15px;}";
  page += ".card{background:#1f2937;padding:18px;border-radius:16px;margin-bottom:16px;}";
  page += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;}";
  page += ".box{background:#111827;padding:14px;border-radius:12px;}";
  page += ".label{font-size:13px;color:#9ca3af;margin-bottom:6px;}";
  page += ".value{font-size:22px;font-weight:bold;}";
  page += "input{width:100%;font-size:18px;padding:10px;border-radius:10px;border:none;box-sizing:border-box;}";
  page += "button{font-size:18px;padding:12px 18px;border:none;border-radius:12px;cursor:pointer;margin-top:10px;}";
  page += ".run{background:#16a34a;color:white;margin-right:8px;}";
  page += ".stop{background:#dc2626;color:white;}";
  page += ".save{background:#2563eb;color:white;width:100%;margin-top:16px;}";
  page += ".warn{background:#3f1d1d;color:#fca5a5;padding:12px;border-radius:12px;margin-top:12px;font-size:14px;}";
  page += "@media(max-width:640px){.grid{grid-template-columns:1fr;}}";
  page += "</style></head><body>";

  page += "<div class='logo'>UMass Lowell — 2025</div>";
  page += "<h2>Coil Winder Control</h2>";

  page += "<div class='card'><div class='grid'>";

  page += "<div class='box'><div class='label'>Machine Status</div><div class='value'>";
  page += machineRunning ? "RUNNING" : "STOPPED";
  page += "</div></div>";

  page += "<div class='box'><div class='label'>Traverse Direction</div><div class='value'>";
  page += traverseDir ? "RIGHT" : "LEFT";
  page += "</div></div>";

  page += "<div class='box'><div class='label'>Left Sensor</div><div class='value'>";
  page += badge(leftTriggered());
  page += "</div></div>";

  page += "<div class='box'><div class='label'>Right Sensor</div><div class='value'>";
  page += badge(rightTriggered());
  page += "</div></div>";

  page += "</div>";

  page += "<form action='/run' method='GET' style='display:inline;'>";
  page += "<button class='run' type='submit'>Run Machine</button></form>";

  page += "<form action='/stop' method='GET' style='display:inline;'>";
  page += "<button class='stop' type='submit'>Stop Machine</button></form>";

  page += "<div class='warn'>This version reverses only from the two hall sensors. If a sensor or magnet fails, the carriage will not auto-stop.</div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<form action='/save' method='GET'>";
  page += "Speed Motor 1 (us):<br><input name='s1' value='" + String(SPEED1_US) + "'><br><br>";
  page += "Speed Motor 2 (us):<br><input name='s2' value='" + String(SPEED2_US) + "'><br><br>";
  page += "<button class='save' type='submit'>Save Settings</button>";
  page += "</form></div>";

  page += "</body></html>";
  return page;
}

// ======================================================
// WEB HANDLERS
// ======================================================
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSave() {
  if (server.hasArg("s1")) SPEED1_US = server.arg("s1").toInt();
  if (server.hasArg("s2")) SPEED2_US = server.arg("s2").toInt();

  prefs.putInt("speed1", SPEED1_US);
  prefs.putInt("speed2", SPEED2_US);

  server.send(200, "text/html", "<h2>Saved!</h2><a href='/'>Back</a>");
}

void handleRun() {
  machineRunning = true;
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

void handleStop() {
  machineRunning = false;
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  prefs.begin("coil", false);
  SPEED1_US = prefs.getInt("speed1", SPEED1_US);
  SPEED2_US = prefs.getInt("speed2", SPEED2_US);

  Serial.println("Loaded EEPROM values:");
  Serial.println(SPEED1_US);
  Serial.println(SPEED2_US);

  pinMode(STEP1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(STEP2, OUTPUT);
  pinMode(DIR2, OUTPUT);

  pinMode(LEFT_SENSOR, INPUT_PULLUP);
  pinMode(RIGHT_SENSOR, INPUT_PULLUP);

  digitalWrite(DIR1, HIGH);
  digitalWrite(DIR2, HIGH);
  traverseDir = true;

  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/run", handleRun);
  server.on("/stop", handleStop);
  server.begin();

  Serial.println("Web server started!");
}

// ======================================================
// MAIN LOOP
// ======================================================
void loop() {
  server.handleClient();

  if (!machineRunning) return;

  unsigned long now = micros();
  unsigned long nowMs = millis();

  // ----------------------------------------------------
  // SENSOR-BASED REVERSE
  // ----------------------------------------------------
  if (nowMs - lastReverseMs > REVERSE_LOCKOUT_MS) {
    if (traverseDir && rightTriggered()) {
      traverseDir = false;
      digitalWrite(DIR2, LOW);
      lastReverseMs = nowMs;
      Serial.println("Right sensor triggered -> moving LEFT");
    }
    else if (!traverseDir && leftTriggered()) {
      traverseDir = true;
      digitalWrite(DIR2, HIGH);
      lastReverseMs = nowMs;
      Serial.println("Left sensor triggered -> moving RIGHT");
    }
  }

  // ----------------------------------------------------
  // MOTOR 1 — BOBBIN ROTATION
  // ----------------------------------------------------
  if (now - t1 >= (unsigned long)SPEED1_US) {
    t1 = now;

    digitalWrite(STEP1, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP1, LOW);
    delayMicroseconds(STEP_PULSE_US);
  }

  // ----------------------------------------------------
  // MOTOR 2 — TRAVERSE
  // ----------------------------------------------------
  if (now - t2 >= (unsigned long)SPEED2_US) {
    t2 = now;

    digitalWrite(STEP2, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP2, LOW);
    delayMicroseconds(STEP_PULSE_US);
  }
}
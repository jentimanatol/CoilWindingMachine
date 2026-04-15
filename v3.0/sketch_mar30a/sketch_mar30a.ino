#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

Preferences prefs;

// ============================
// CONFIG
// ============================
int SPEED1_US = 1500;
int SPEED2_US = 2000;

// ============================
// MOTOR PINS
// ============================
#define STEP1 2
#define DIR1  3

#define STEP2 0
#define DIR2  1

// ============================
// SENSOR PINS (CONFIRMED)
// ============================
#define LEFT_SENSOR  6
#define RIGHT_SENSOR 7

// ============================
#define STEP_PULSE_US 200

unsigned long t1 = 0;
unsigned long t2 = 0;

bool traverseDir = true;   // true = RIGHT

// lock system
bool leftLock = false;
bool rightLock = false;

// ============================
// WIFI
// ============================
const char* ssid = "CoilWinder";
const char* password = "12345678";

WebServer server(80);

// ============================
// SENSOR LOGIC (CONFIRMED)
// ============================
bool leftTriggered() {
  return digitalRead(LEFT_SENSOR) == LOW;
}

bool rightTriggered() {
  return digitalRead(RIGHT_SENSOR) == LOW;
}








// ============================
// WEB PAGE
// ============================
String htmlPage() {
  String p = "<html><head><meta name='viewport' content='width=device-width'>";
  p += "<meta http-equiv='refresh' content='1'>";
  p += "<style>body{font-family:Arial;background:#111;color:white;padding:20px;}</style>";
  p += "</head><body>";

  p += "<h2>Coil Winder</h2>";

  p += "Direction: ";
  p += traverseDir ? "RIGHT" : "LEFT";
  p += "<br><br>";

  p += "LEFT sensor: ";
  p += leftTriggered() ? "DETECTED" : "IDLE";
  p += "<br>";

  p += "RIGHT sensor: ";
  p += rightTriggered() ? "DETECTED" : "IDLE";
  p += "<br><br>";

  p += "<form action='/save'>";
  p += "Speed1:<input name='s1' value='" + String(SPEED1_US) + "'><br>";
  p += "Speed2:<input name='s2' value='" + String(SPEED2_US) + "'><br>";
  p += "<button>Save</button></form>";

  p += "</body></html>";
  return p;
}

// ============================
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSave() {
  if (server.hasArg("s1")) SPEED1_US = server.arg("s1").toInt();
  if (server.hasArg("s2")) SPEED2_US = server.arg("s2").toInt();

  prefs.putInt("s1", SPEED1_US);
  prefs.putInt("s2", SPEED2_US);

  server.send(200, "text/html", "Saved <a href='/'>Back</a>");
}

// ============================
void setup() {
  Serial.begin(115200);

  prefs.begin("coil", false);
  SPEED1_US = prefs.getInt("s1", SPEED1_US);
  SPEED2_US = prefs.getInt("s2", SPEED2_US);

  pinMode(STEP1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(STEP2, OUTPUT);
  pinMode(DIR2, OUTPUT);

  pinMode(LEFT_SENSOR, INPUT_PULLUP);
  pinMode(RIGHT_SENSOR, INPUT_PULLUP);

  digitalWrite(DIR1, HIGH);
  digitalWrite(DIR2, HIGH);

  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();

  Serial.println("SYSTEM READY");
}

// ============================
void loop() {
  server.handleClient();

  unsigned long now = micros();

  bool left = leftTriggered();
  bool right = rightTriggered();

  // ============================
  // REVERSAL LOGIC (STABLE)
  // ============================

  // moving RIGHT → hit RIGHT sensor
  if (traverseDir && right && !rightLock) {
    traverseDir = false;
    digitalWrite(DIR2, LOW);

    rightLock = true;
    leftLock = false;

    Serial.println("RIGHT -> LEFT");
  }

  // moving LEFT → hit LEFT sensor
  if (!traverseDir && left && !leftLock) {
    traverseDir = true;
    digitalWrite(DIR2, HIGH);

    leftLock = true;
    rightLock = false;

    Serial.println("LEFT -> RIGHT");
  }

  // unlock when leaving sensor zone
  if (!right) rightLock = false;
  if (!left)  leftLock = false;

  // ============================
  // MOTOR 1 (unchanged)
  // ============================
  if (now - t1 >= SPEED1_US) {
    t1 = now;
    digitalWrite(STEP1, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP1, LOW);
    delayMicroseconds(STEP_PULSE_US);
  }

  // ============================
  // MOTOR 2 (unchanged)
  // ============================
  if (now - t2 >= SPEED2_US) {
    t2 = now;
    digitalWrite(STEP2, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP2, LOW);
    delayMicroseconds(STEP_PULSE_US);
  }
}   
 

 
  
   
    

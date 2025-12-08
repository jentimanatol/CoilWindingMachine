#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ======================================================
// EEPROM-LIKE STORAGE USING PREFERENCES
// ======================================================
Preferences prefs;

// ======================================================
// DEFAULT CONFIG VALUES (used if no EEPROM values saved)
// ======================================================
int TRAVEL_STEPS = 2000;
int SPEED1_US = 1500;
int SPEED2_US = 2000;

// ======================================================
// MOTOR PINS (for your XIAO ESP32-S3/C6 hardware)
// ======================================================
#define STEP1 2   // Motor 1 STEP  
#define DIR1  3   // Motor 1 DIR

#define STEP2 0   // Motor 2 STEP
#define DIR2  1   // Motor 2 DIR

#define STEP_PULSE_US 200

// ======================================================
// MOTOR INTERNAL VARIABLES
// ======================================================
unsigned long t1 = 0;
unsigned long t2 = 0;

unsigned int traverseCount = 0;
bool traverseDir = true;

// ======================================================
// WIFI ACCESS POINT
// ======================================================
const char* ssid = "CoilWinder";
const char* password = "12345678";

WebServer server(80);

// ======================================================
// HTML PAGE
// ======================================================
// String htmlPage() {
//   String page = "<html><head><title>Coil Winder</title>";
//   page += "<style>body{font-family:Arial;background:#111;color:white;padding:20px;}input{width:120px;font-size:18px;}</style>";
//   page += "</head><body>";
//   page += "<h2>Coil Winder Control</h2>";
//   page += "<form action='/save' method='GET'>";
//   page += "Travel Steps:<br><input name='ts' value='" + String(TRAVEL_STEPS) + "'><br><br>";
//   page += "Speed Motor 1 (us):<br><input name='s1' value='" + String(SPEED1_US) + "'><br><br>";
//   page += "Speed Motor 2 (us):<br><input name='s2' value='" + String(SPEED2_US) + "'><br><br>";
//   page += "<input type='submit' value='Save Settings' style='font-size:20px;'></form>";
//   page += "</body></html>";
//   return page;
// }





String htmlPage() {
  String page = "<html><head><title>Coil Winder</title>";
  page += "<style>";
  page += "body{font-family:Arial;background:#111;color:white;padding:20px;}";
  page += ".logo{font-size:28px;font-weight:bold;text-align:center;color:#4da6ff;margin-bottom:15px;}";
  page += "input{width:120px;font-size:18px;}";
  page += "</style></head><body>";

  // ---------- LOGO TEXT ----------
  page += "<div class='logo'>UMass Lowell — 2025</div>";

  page += "<h2>Coil Winder Control</h2>";
  page += "<form action='/save' method='GET'>";
  page += "Travel Steps:<br><input name='ts' value='" + String(TRAVEL_STEPS) + "'><br><br>";
  page += "Speed Motor 1 (us):<br><input name='s1' value='" + String(SPEED1_US) + "'><br><br>";
  page += "Speed Motor 2 (us):<br><input name='s2' value='" + String(SPEED2_US) + "'><br><br>";
  page += "<input type='submit' value='Save Settings' style='font-size:20px;'></form>";
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
  if (server.hasArg("ts")) TRAVEL_STEPS = server.arg("ts").toInt();
  if (server.hasArg("s1")) SPEED1_US = server.arg("s1").toInt();
  if (server.hasArg("s2")) SPEED2_US = server.arg("s2").toInt();

  // SAVE TO EEPROM (Preferences)
  prefs.putInt("travel", TRAVEL_STEPS);
  prefs.putInt("speed1", SPEED1_US);
  prefs.putInt("speed2", SPEED2_US);

  server.send(200, "text/html",
              "<h2>Saved!</h2><a href='/'>Back</a>");
}

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  // ====== RESTORE VALUES FROM EEPROM ======
  prefs.begin("coil", false);
  TRAVEL_STEPS = prefs.getInt("travel", TRAVEL_STEPS);
  SPEED1_US     = prefs.getInt("speed1", SPEED1_US);
  SPEED2_US     = prefs.getInt("speed2", SPEED2_US);

  Serial.println("Loaded EEPROM values:");
  Serial.println(TRAVEL_STEPS);
  Serial.println(SPEED1_US);
  Serial.println(SPEED2_US);

  // ====== MOTOR SETUP ======
  pinMode(STEP1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(STEP2, OUTPUT);
  pinMode(DIR2, OUTPUT);

  digitalWrite(DIR1, HIGH);
  digitalWrite(DIR2, HIGH);

  // ====== WIFI AP ======
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // ====== WEB ROUTES ======
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("Web server started!");
}

// ======================================================
// MAIN LOOP
// ======================================================
void loop() {
  server.handleClient();

  unsigned long now = micros();

  // ----------------------------------------------------
  // MOTOR 1 — BOBBIN ROTATION
  // ----------------------------------------------------
  if (now - t1 >= SPEED1_US) {
    t1 = now;

    digitalWrite(STEP1, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP1, LOW);
    delayMicroseconds(STEP_PULSE_US);
  }

  // ----------------------------------------------------
  // MOTOR 2 — TRAVERSE L↔R
  // ----------------------------------------------------
  if (now - t2 >= SPEED2_US) {
    t2 = now;

    digitalWrite(STEP2, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP2, LOW);
    delayMicroseconds(STEP_PULSE_US);

    traverseCount++;

    if (traverseCount >= TRAVEL_STEPS) {
      traverseDir = !traverseDir;
      digitalWrite(DIR2, traverseDir ? HIGH : LOW);
      traverseCount = 0;
    }
  }
}

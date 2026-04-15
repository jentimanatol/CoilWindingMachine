// ============================================================
// CONFIRM PIN 2 + 3 — STEP and DIR
// Motor should change direction every 3 seconds
// ============================================================

#define STEP1 2
#define DIR1  3

int SPEED1_US = 800;

void setup() {
  Serial.begin(115200);
  pinMode(STEP1, OUTPUT);
  pinMode(DIR1,  OUTPUT);
  Serial.println("Confirm pin 2+3 test...");
}

void loop() {
  digitalWrite(DIR1, HIGH);
  Serial.println("DIR HIGH — going RIGHT");
  unsigned long t = millis();
  while (millis() - t < 3000) {
    digitalWrite(STEP1, HIGH);
    delayMicroseconds(SPEED1_US);
    digitalWrite(STEP1, LOW);
    delayMicroseconds(SPEED1_US);
  }

  digitalWrite(DIR1, LOW);
  Serial.println("DIR LOW — going LEFT");
  t = millis();
  while (millis() - t < 3000) {
    digitalWrite(STEP1, HIGH);
    delayMicroseconds(SPEED1_US);
    digitalWrite(STEP1, LOW);
    delayMicroseconds(SPEED1_US);
  }
}



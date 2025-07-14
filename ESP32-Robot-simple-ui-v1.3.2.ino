#include <WiFi.h>
#include <WebServer.h>

// WiFi config
const char* ssid = "Your-wifi-SSID";
const char* password = "your_wifi_password";

// Webserver
WebServer server(80);

// Motor IN pins (ESP32)
const int motorPins[12] = {
  5, 4, 18, 19,    // L298N1 IN1..IN4 -> Motor 0,1
  21, 22, 23, 25,  // L298N2 IN5..IN8 -> Motor 2,3
  26, 27, 32, 33   // L298N3 IN9..IN12 -> Motor 4,5
};

// Define which motors are LEFT and RIGHT explicitly for turning
const int LEFT_MOTORS[3] = {1, 3, 5};   // Motor indices for LEFT side
const int RIGHT_MOTORS[3] = {0, 3, 4};  // Motor indices for RIGHT side

// Relay pins for headlight and tail light
const int HEADLIGHT_PIN = 15; // D15
const int TAILLIGHT_PIN = 14; // D14

// Direction states:
// 0 = STOP, 1 = FORWARD, 2 = BACKWARD, 3 = LEFT, 4 = RIGHT
volatile int directionState = 0;

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 12; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }

  // Relay pins
  pinMode(HEADLIGHT_PIN, OUTPUT);
  pinMode(TAILLIGHT_PIN, OUTPUT);
  digitalWrite(HEADLIGHT_PIN, LOW);
  digitalWrite(TAILLIGHT_PIN, HIGH); // Tail light ON by default

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/set_direction", handleSetDirection);
  server.on("/stop", handleStop);

  server.begin();
  Serial.println("Server started.");
}

void loop() {
  server.handleClient();

  switch (directionState) {
    case 0:
      stopAllMotors();
      digitalWrite(HEADLIGHT_PIN, LOW);
      digitalWrite(TAILLIGHT_PIN, HIGH);
      break;
    case 1:
      setAllMotorsForward();
      digitalWrite(HEADLIGHT_PIN, HIGH);
      digitalWrite(TAILLIGHT_PIN, LOW);
      break;
    case 2:
      setAllMotorsBackward();
      digitalWrite(HEADLIGHT_PIN, LOW);
      digitalWrite(TAILLIGHT_PIN, HIGH);
      break;
    case 3:
      setLeftTurn();
      digitalWrite(HEADLIGHT_PIN, HIGH);
      digitalWrite(TAILLIGHT_PIN, LOW);
      break;
    case 4:
      setRightTurn();
      digitalWrite(HEADLIGHT_PIN, HIGH);
      digitalWrite(TAILLIGHT_PIN, LOW);
      break;
  }
}

void stopAllMotors() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(motorPins[i * 2], LOW);
    digitalWrite(motorPins[i * 2 + 1], LOW);
  }
}

void setAllMotorsForward() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(motorPins[i * 2], HIGH);
    digitalWrite(motorPins[i * 2 + 1], LOW);
  }
}

void setAllMotorsBackward() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(motorPins[i * 2], LOW);
    digitalWrite(motorPins[i * 2 + 1], HIGH);
  }
}

void setLeftTurn() {
  for (int i = 0; i < 3; i++) {
    int motorIdx = LEFT_MOTORS[i];
    digitalWrite(motorPins[motorIdx * 2], LOW);
    digitalWrite(motorPins[motorIdx * 2 + 1], HIGH);
  }
  for (int i = 0; i < 3; i++) {
    int motorIdx = RIGHT_MOTORS[i];
    digitalWrite(motorPins[motorIdx * 2], HIGH);
    digitalWrite(motorPins[motorIdx * 2 + 1], LOW);
  }
}

void setRightTurn() {
  for (int i = 0; i < 3; i++) {
    int motorIdx = LEFT_MOTORS[i];
    digitalWrite(motorPins[motorIdx * 2], HIGH);
    digitalWrite(motorPins[motorIdx * 2 + 1], LOW);
  }
  for (int i = 0; i < 3; i++) {
    int motorIdx = RIGHT_MOTORS[i];
    digitalWrite(motorPins[motorIdx * 2], LOW);
    digitalWrite(motorPins[motorIdx * 2 + 1], HIGH);
  }
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Motor Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; background: #111; color: #eee; }
    h2 { color: #0f0; }
    .button {
      width: 150px; height: 60px; margin: 15px;
      font-size: 20px; border: none; border-radius: 10px;
      color: white; cursor: pointer; background: #007BFF;
    }
    .button:active { background: #0056b3; }
  </style>
</head>
<body>
  <h2>ESP32 Direction Control</h2>
  <button class="button" ontouchstart="send('FORWARD')" ontouchend="send('STOP')" onmousedown="send('FORWARD')" onmouseup="send('STOP')">FORWARD</button><br>
  <button class="button" ontouchstart="send('LEFT')" ontouchend="send('STOP')" onmousedown="send('LEFT')" onmouseup="send('STOP')">LEFT</button>
  <button class="button" ontouchstart="send('RIGHT')" ontouchend="send('STOP')" onmousedown="send('RIGHT')" onmouseup="send('STOP')">RIGHT</button><br>
  <button class="button" ontouchstart="send('BACKWARD')" ontouchend="send('STOP')" onmousedown="send('BACKWARD')" onmouseup="send('STOP')">BACKWARD</button>
<p>Made by Harsha (titanx992ti) </p>
<script>
function send(action) {
  var xhr = new XMLHttpRequest();
  if (action == 'STOP') {
    xhr.open("GET", "/stop", true);
  } else {
    xhr.open("GET", "/set_direction?action=" + action, true);
  }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleSetDirection() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    action.toUpperCase();
    if (action == "FORWARD") {
      directionState = 1;
    } else if (action == "BACKWARD") {
      directionState = 2;
    } else if (action == "LEFT") {
      directionState = 3;
    } else if (action == "RIGHT") {
      directionState = 4;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  directionState = 0;
  server.send(200, "text/plain", "Stopped");
}

#include <WiFi.h>
#include <WebServer.h>

// WiFi
const char* ssid = "Tlcm";
const char* password = "01234567";

// Asterisk AMI
const char* asterisk_ip = "10.148.245.142";   // change if Ubuntu IP changes
const int asterisk_port = 5038;
const char* ami_user = "esp32";
const char* ami_pass = "1234";

// Pins
const int buttonPin = 13;
const int buzzerPin = 15;
const int ledPin = 2;   // LED pin

WiFiClient client;
WebServer server(80);

bool lastState = HIGH;
unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 500;

void sendAMICommand(const String& cmd) {
  client.print(cmd);
  client.print("\r\n");
}

bool loginAMI() {
  sendAMICommand("Action: Login");
  sendAMICommand("Username: " + String(ami_user));
  sendAMICommand("Secret: " + String(ami_pass));
  sendAMICommand("");

  delay(500);

  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line.indexOf("Success") >= 0) {
      return true;
    }
  }
  return false;
}

void makeCall() {
  sendAMICommand("Action: Originate");
  sendAMICommand("Channel: Local/999@internal");
  sendAMICommand("Application: Wait");
  sendAMICommand("Data: 60");
  sendAMICommand("Async: true");
  sendAMICommand("");

  Serial.println("Call command sent to 2001 and 2002");
}

void ringBuzzer() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(300);
    digitalWrite(buzzerPin, LOW);
    delay(150);
  }
}

void ledOn() {
  digitalWrite(ledPin, HIGH);
  server.send(200, "text/plain", "LED ON 5s");

  delay(5000);   // LED ON for 5 seconds
  digitalWrite(ledPin, LOW);

  Serial.println("LED ON for 5 seconds");
}

void ledOff() {
  digitalWrite(ledPin, LOW);
  server.send(200, "text/plain", "LED OFF");
  Serial.println("LED OFF");
}

void triggerCall() {
  Serial.println("Button pressed");

  ringBuzzer();

  if (client.connect(asterisk_ip, asterisk_port)) {
    Serial.println("Connected to Asterisk");

    if (loginAMI()) {
      Serial.println("Login OK");
      delay(300);
      makeCall();
    } else {
      Serial.println("Login failed");
    }

    delay(500);
    client.stop();
  } else {
    Serial.println("Connection failed");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  server.on("/led/on", ledOn);
  server.on("/led/off", ledOff);
  server.begin();

  Serial.println("HTTP LED server started");
}

void loop() {
  server.handleClient();

  bool currentState = digitalRead(buttonPin);

  if (currentState == LOW) {
    Serial.println("Button is pressed");
    triggerCall();

    // wait until you release the button
    while (digitalRead(buttonPin) == LOW) {
      server.handleClient();
      delay(20);
    }

    delay(500); // anti double-click
  }

  delay(50);
}
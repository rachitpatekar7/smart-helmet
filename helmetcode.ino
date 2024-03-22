#include <SoftwareSerial.h>
#include <TinyGPS++.h> // Library for parsing NMEA data from GPS module
#include <Adafruit_FONA.h> // Library for SIM800L GSM module

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

SoftwareSerial bluetooth(8, 9); // RX, TX pins for Bluetooth (connect according to your setup)
const int irSensorPin = 7; // IR sensor pin
const int alcoholSensorPin = A0; // Alcohol sensor pin
const int vibrationPin = 10; // Vibration sensor pin connected to Arduino digital pin 2
const int tiltPin = 11; // Tilt sensor pin connected to Arduino digital pin 5
const int buzzer = 12;

TinyGPSPlus gps;

void setup() {
  pinMode(irSensorPin, INPUT);
  pinMode(alcoholSensorPin, INPUT);
  pinMode(vibrationPin, INPUT);
  pinMode(tiltPin, INPUT);
  pinMode(buzzer, OUTPUT);
  Serial.begin(9600);
  bluetooth.begin(38400);

  // Initialize GSM module
  fonaSS.begin(4800);
  if (!fona.begin(fonaSS)) {
    Serial.println(F("Couldn't find GSM module"));
    while (1);
  }
  Serial.println(F("GSM module initialized"));

  // Wait for GPS module to initialize
  delay(1000);
}

void loop() {
  int irValue = digitalRead(irSensorPin);
  int alcoholValue = analogRead(alcoholSensorPin);
  int vibrationValue = digitalRead(vibrationPin);
  int tiltValue = digitalRead(tiltPin);

  // Convert the tilt sensor reading to degrees
  Serial.println(tiltValue);

  if (alcoholValue > 550) {
    bluetooth.write('2'); // Sending '2' to indicate alcohol detected
  } else {
    if (irValue == LOW) {
      bluetooth.write('0'); // Sending '0' to indicate object detected by IR sensor
    } else {
      bluetooth.write('1'); // Sending '1' if no object detected
    }
  }

  // If both vibration and tilt are detected and helmet is worn, send '3' to slave
  if (vibrationValue == HIGH && tiltValue == HIGH && irValue == LOW) {
    digitalWrite(buzzer, HIGH);
    Serial.println("accident");
    delay(800); // Adjust beep duration as needed
    digitalWrite(buzzer, LOW);

    // Retrieve GPS coordinates
    if (gps.location.isValid()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();

      // Send emergency message with GPS coordinates
      sendEmergencyMessage(latitude, longitude);
    } else {
      Serial.println("Invalid GPS data");
    }
  }

  // Update GPS data
  while (Serial.available() > 0) {
    if (gps.encode(Serial.read())) {
      break;
    }
  }

  delay(100); // Delay for stability
}

void sendEmergencyMessage(float latitude, float longitude) {
  // Format message with GPS coordinates
  String message = "ACCIDENT HAPPENED! Location: https://maps.google.com/maps?q=";
  message += String(latitude, 6);
  message += ",";
  message += String(longitude, 6);

  // Send message
  Serial.println("Sending emergency message...");
  if (!fona.sendSMS("EmergencyNumber", message)) { // Replace "EmergencyNumber" with the phone number
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }
}


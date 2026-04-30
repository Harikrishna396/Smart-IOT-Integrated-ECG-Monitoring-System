#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Wi-Fi credentials
#define WIFI_SSID "S1"
#define WIFI_PASSWORD "12345678"

// Firebase credentials
#define FIREBASE_HOST "ecg-monitoring-4aa87-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "dPLl0B4yWNh20XBLRH4Y1LGrO2s3OjMQQJe11A2R"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

int ecgPin = A0;        // ECG sensor connected to A0
int ecgsignal = 0;
String lastStatus = "";

#define WINDOW_SIZE 10
int ecgBuffer[WINDOW_SIZE];
int indexBuffer = 0;

void setup() {
  Serial.begin(115200);

  // Connect Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  // Firebase setup
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Connected to Firebase!");

  // Initialize ECG buffer
  for (int i = 0; i < WINDOW_SIZE; i++) ecgBuffer[i] = 0;
}

void loop() {
  // Read ECG value
  ecgsignal = analogRead(ecgPin);
   if (ecgsignal < 10) { // likely bad reading
    ecgsignal = ecgBuffer[(indexBuffer + WINDOW_SIZE - 1) % WINDOW_SIZE]; // keep last good value
  }

  // Add to buffer for smoothing
  ecgBuffer[indexBuffer] = ecgsignal;
  indexBuffer = (indexBuffer + 1) % WINDOW_SIZE;

  // Calculate average ECG
  long sum = 0;
  for (int i = 0; i < WINDOW_SIZE; i++) sum += ecgBuffer[i];
  int avgECG = sum / WINDOW_SIZE;

  Serial.print("Average ECG: ");
  Serial.println(avgECG);

  // Determine status
  String status;
  if (avgECG > 600 || avgECG < 300) status = "Abnormal";
  else status = "Normal";

  // Upload status only if changed
  if (status != lastStatus) {
    Firebase.setString(fbdo, "/patients/karthik/status", status);
    lastStatus = status;
    Serial.print("Status updated: ");
    Serial.println(status);
  }

  // Always upload ECG value and locations
  Firebase.setInt(fbdo, "/patients/karthik/ecg_signal", ecgsignal);
  Firebase.setString(fbdo, "/patients/karthik/patient_location", "18.1309,83.4005");
  Firebase.setString(fbdo, "/patients/karthik/hospital_location", "18.1201,83.4005");

  delay(500);  // repeat every 0.5 sec
}
#include <Arduino.h>
#include <ESP32Servo.h>
#include <MQUnifiedsensor.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define trigPin1 23
#define echoPin1 22
#define servoPin 13
#define trigPin2 19
#define echoPin2 21

Servo servo;
// const char *ssid = "Waroeng MieSoeh";
// const char *password = "mienyemek";
const char *ssid = "KOSKU";
const char *password = "koskud09";
const float maxHeight = 30.0;
float duration_us, distance1, distance2;

#define board "ESP32"
#define Voltage_Resolution 3.3
#define pin 36
#define type "MQ-135"
#define ADC_Bit_Resolution 12
#define RatioMQ135CleanAir 3.6

MQUnifiedsensor MQ135(board, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  servo.attach(servoPin);
  servo.write(0);

  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.setA(102.2); MQ135.setB(-3.445);
  MQ135.init();

  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update();
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0 / 10);
  Serial.println("done!.");

  if (isinf(calcR0)) { Serial.println("Warning: Connection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while (1); }
  if (calcR0 == 0) { Serial.println("Warning: Connection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while (1); }

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(15);
  digitalWrite(trigPin1, LOW);

  duration_us = pulseIn(echoPin1, HIGH);
  distance1 = 0.017 * duration_us;

  if (distance1 < 10)
    servo.write(90);
  else
    servo.write(0);

  Serial.print("Jarak Objek : ");
  Serial.print(distance1);
  Serial.println(" cm");
  
  float distance2 = measureDistance();
  float percentage = ((maxHeight - distance2) / maxHeight) * 100.0;
  if (percentage < 0) {
    percentage = 0;
  }
  Serial.print("Persentase: ");
  Serial.print(percentage);
  Serial.println(" %");
  
  MQ135.update();
  float hasil = MQ135.readSensor();
  Serial.print("NH3: "); 
  Serial.print(hasil);
  Serial.println(" PPM");
  delay(2000);

  HTTPClient http;
  String serverURL = "http://127.0.0.1/send_data?gas_ppm=" + String(hasil) + "&trash_capacity=" + String(percentage) + "&trash_can_id=1";
  http.begin(serverURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

  delay(2000);
}

float measureDistance() {
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  float duration = pulseIn(echoPin2, HIGH);
  float distance = (float)duration * 0.0343 / 2.0;

  return distance;
}



#include <ESP32Servo.h>

Servo servoMotor;

int pos = 0;
int servoPin = 16;
const int trigPin = 17;
const int echoPin = 18;
const int redLED = 25;
const int yellowLED = 26;
const int greenLED = 27;
long duration;
int distance;

void setup() {
  Serial.begin(9600);
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	servoMotor.setPeriodHertz(50);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  servoMotor.attach(servoPin);
  servoMotor.write(0);

	servoMotor.attach(servoPin, 500, 2500);

}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.0344 / 2;
  Serial.print("Distance = ");
  Serial.println(distance);

  if (distance > 100) {
    servoMotor.write(95);
    trafficLightControl(4, 1, 0);
  } else {
    servoMotor.write(5);
    trafficLightControl(0, 1, 4);
  }

  delay(250);

}

void trafficLightControl(int redDuration, int yellowDuration, int greenDuration) {
  digitalWrite(redLED, HIGH);
  delay(redDuration * 250);
  digitalWrite(redLED, LOW);

  digitalWrite(yellowLED, HIGH);
  delay(yellowDuration * 250);
  digitalWrite(yellowLED, LOW);

  digitalWrite(greenLED, HIGH);
  delay(greenDuration * 250);
  digitalWrite(greenLED, LOW);
}

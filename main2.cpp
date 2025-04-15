
// this was used gfrom the example that was provided to us from the github for the gyroscope and accelerometer
#include "SparkFunLSM6DSO.h"
#include "Wire.h"
//#include "SPI.h"

LSM6DSO myIMU; //initializing the LSM6DSO


//these are the libraries for the bluetooth mostly provided for us fro  the documentaton of Lab 4
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLE2902.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


BLECharacteristic *pCharacteristic; 

// this is the baseline and threshold that we will use later to calculate the steps
// baseline will be used later for calibration purposes
// honestly the threshold can be cahnged for each person since each person has a different speed of walking
// we made it 0.5 for demostration purposes, feel free to change for more variety
float baseline = 0.0;
float threshold = 0.5;
bool stepDetected = false;
int stepCount = 0;

//code for clibration where it runs for 100 times to get it more or less calibrated
void calibrateSensor() {
  float sum = 0;
  int readings = 100;

  Serial.println("Calibrating...");
  for (int i = 0; i < readings; i++) {
    float ax = myIMU.readFloatAccelX();
    float ay = myIMU.readFloatAccelY();
    float magnitude = sqrt(ax * ax + ay * ay);
    sum += magnitude;
    delay(20);
  }

  baseline = sum / readings;
  Serial.print("Baseline acceleration: ");
  Serial.println(baseline);
}

//from the documentation of Lab 4 how to connect to bluetooth and how it will send info to the phone.
void setupBLE() {
  BLEDevice::init("SDSUCS");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("Waiting for steps...");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0); 
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE advertising started");
}

 
//initialize everything here in setup, start with the pins and check if the LSM^DSO is connected properly or not.
void setup() {
  Serial.begin(9600);
  delay(100);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(10);

  if (!myIMU.begin()) {
    Serial.println("Could not connect to LSM6DSO. Freezing.");
    while (1);
  }

  if (myIMU.initialize(BASIC_SETTINGS)) {
    Serial.println("Sensor initialized with basic settings.");
  }

  //calibrate the sensor
  calibrateSensor();
  setupBLE();

  char buf[32];
  snprintf(buf, sizeof(buf), "Steps: %d", stepCount);
  pCharacteristic->setValue(buf);
  pCharacteristic->notify();

}


//here in loop it will take the accelerometers values and if the magnitude is greater than the threshoold plus the baseline which is the calibration then it will mark a step
void loop() {
  float ax = myIMU.readFloatAccelX();
  float ay = myIMU.readFloatAccelY();
  float magnitude = sqrt(ax * ax + ay * ay);

  //this in a loop so it continuesly updates
  if (magnitude > (baseline + threshold) && !stepDetected) {
    stepDetected = true;
    stepCount++;

    Serial.print("Step Count: ");
    Serial.println(stepCount);

    char buf[32];
    snprintf(buf, sizeof(buf), "Steps: %d", stepCount);
    pCharacteristic->setValue(buf);
    pCharacteristic->notify();
  }

  if (magnitude < (baseline + threshold)) {
    stepDetected = false;
  }

  delay(20); // 20 ms sampling rate
}

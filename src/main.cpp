#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4 // Pin for DS18B20 data
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define TURBIDITY_SENSOR_PIN 34 // Pin for the turbidity sensor

BLEServer* pServer = NULL;
BLECharacteristic* pTemperatureCharacteristic = NULL;
BLECharacteristic* pTurbidityCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUIDs for the BLE service and characteristics, ensure they are unique
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TEMPERATURE_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define TURBIDITY_UUID         "beb5483e-36e1-4688-b7f5-eadf15001a69"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);

  // Initialize sensors
  sensors.begin();

  // Create and set the BLE device name
  BLEDevice::init("ESP32_BLE_Server");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create a BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a temperature characteristic
  pTemperatureCharacteristic = pService->createCharacteristic(
                                      TEMPERATURE_UUID,
                                      BLECharacteristic::PROPERTY_READ |
                                      BLECharacteristic::PROPERTY_NOTIFY
                                    );
  pTemperatureCharacteristic->addDescriptor(new BLE2902());

  // Create a turbidity characteristic
  pTurbidityCharacteristic = pService->createCharacteristic(
                                      TURBIDITY_UUID,
                                      BLECharacteristic::PROPERTY_READ |
                                      BLECharacteristic::PROPERTY_NOTIFY
                                    );
  pTurbidityCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();
  // Begin advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    sensors.requestTemperatures(); // Request temperature data
    float temperature = sensors.getTempCByIndex(0); // Read temperature
    int turbidityValue = analogRead(TURBIDITY_SENSOR_PIN); // Read the value from the turbidity sensor
    float voltage = turbidityValue * (3.3 / 4095.0); // Convert the analog value to voltage
    float ntu = -1000 * voltage + 3000; // Convert voltage to NTU (Nephelometric Turbidity Units)

    // Update the values of BLE characteristics
    pTemperatureCharacteristic->setValue(&temperature, sizeof(temperature));
    pTemperatureCharacteristic->notify();
    pTurbidityCharacteristic->setValue(&ntu, sizeof(ntu));
    pTurbidityCharacteristic->notify();

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.print("Turbidity: ");
    Serial.print(ntu);
    Serial.println(" NTU");
  }

  // Implement the logic to handle device connection changes
  // This part of the code was not shown in the initial fragment
}

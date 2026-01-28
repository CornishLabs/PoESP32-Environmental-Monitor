#include <Wire.h>
#include <SensirionI2cSht4x.h>
#include <Adafruit_BMP280.h>
#include <ETH.h>
#include "esp_system.h"

// ------------------- Ethernet Config -------------------
#define ETH_ADDR 1
#define ETH_POWER_PIN 5
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_IP101
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN

bool eth_connected = false;
WiFiClient client;

// ------------------- InfluxDB Settings -------------------
const char INFLUX_HOST[] = "192.168.23.12";
const int INFLUX_PORT = 8086;
const char INFLUX_DB[] = "arduino";
char buf[256];

// ------------------- Sensor Source -------------------
char SENSOR_SOURCE[32] = "UNKNOWN";

// ------------------- SHT40 Sensor -------------------
SensirionI2cSht4x sht40;
float cTemp = 0.0;
float pHumidity = 0.0;
bool sampleError = false;

// ------------------- BMP280 Sensor -------------------
Adafruit_BMP280 bmp;
float pressure = 0.0;

// ------------------- Sampling -------------------
#define SAMPLE_INTERVAL 30000

// ------------------- Helpers -------------------
int ctof(float x)
{
  return round(1.8 * x + 32);
}

// ------------------- Sensor Functions -------------------
bool takeSample()
{
  float tempC = 0, humidity = 0;

  // --- Read SHT40 ---
  int error = sht40.measureHighPrecision(tempC, humidity);
  if (error != 0)
  {
    Serial.print("SHT40 read error: ");
    Serial.println(error);
    sampleError = true;
    return false;
  }

  cTemp = tempC;
  pHumidity = humidity;

  // --- Read BMP280 ---
  if (!bmp.begin(0x76))
  {
    Serial.println("BMP280 not detected!");
    sampleError = true;
  }
  else
  {
    float rawPressure = bmp.readPressure();
    if (rawPressure == 0)
    {
      Serial.println("BMP280 read error");
      sampleError = true;
    }
    else
    {
      pressure = rawPressure / 100.0; // Pa → hPa
    }
  }

  return !sampleError;
}

void printSample()
{
  if (sampleError)
  {
    Serial.println("Sample invalid");
  }
  else
  {
    Serial.print("Temp: ");
    Serial.print(cTemp);
    Serial.print(" °C | ");
    Serial.print("Humidity: ");
    Serial.print(pHumidity);
    Serial.print(" % | ");
    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");
  }
}

// ------------------- InfluxDB Send -------------------
void sendToInflux(float tempC, float humidity, float press)
{
  if (!eth_connected)
  {
    Serial.println("No Ethernet connection, skipping Influx send");
    return;
  }

  snprintf(buf, sizeof(buf),
           "Lab_Sensors,SOURCE=%s Temperature=%.2f,Humidity=%.2f,Pressure=%.2f",
           SENSOR_SOURCE, tempC, humidity, press);

  Serial.print("Sending to InfluxDB: ");
  Serial.println(buf);

  if (!client.connect(INFLUX_HOST, INFLUX_PORT))
  {
    Serial.println("Connection to InfluxDB failed");
    return;
  }

  client.print("POST /write?db=");
  client.print(INFLUX_DB);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(INFLUX_HOST);
  client.println("User-Agent: ESP32-SHT40");
  client.println("Connection: close");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(strlen(buf));
  client.println();
  client.println(buf);

  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 2000)
  {
    while (client.available())
    {
      Serial.write(client.read());
      timeout = millis();
    }
  }
  Serial.println();
  client.stop();
}

// ------------------- Ethernet Event Handler -------------------
void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("Ethernet started");
    ETH.setHostname("esp32-sht40");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("Ethernet connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print("Ethernet got IP: ");
    Serial.println(ETH.localIP());
    eth_connected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("Ethernet disconnected");
    eth_connected = false;
    break;
  default:
    break;
  }
}

// ------------------- Setup -------------------
void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting Ethernet + SHT40 + BMP280 + InfluxDB...");

  // --- Determine MAC address before Ethernet ---
  uint8_t mac[6];
  if (esp_efuse_mac_get_default(mac) == ESP_OK)
  {
    Serial.print("MAC address: ");
    for (int i = 0; i < 6; i++)
    {
      if (i)
        Serial.print(":");
      if (mac[i] < 16)
        Serial.print("0");
      Serial.print(mac[i], HEX);
    }
    Serial.println();

    // Convert MAC to a string "DE:AD:BE:00:11:22"
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Map MAC to SENSOR_SOURCE
    if (strcmp(macStr, "B0:A7:32:56:A4:68") == 0)
    {
      strcpy(SENSOR_SOURCE, "EXP_TABLE_MON1");
    }
    else if (strcmp(macStr, "B0:A7:32:55:CF:F8") == 0)
    {
      strcpy(SENSOR_SOURCE, "LASER_TABLE_MON1");
    }
    else
    {
      strcpy(SENSOR_SOURCE, "UNKNOWN");
    }

    Serial.print("Sensor source set to: ");
    Serial.println(SENSOR_SOURCE);
  }
  else
  {
    Serial.println("Failed to get MAC address, using default source");
    strcpy(SENSOR_SOURCE, "UNKNOWN");
  }

  // --- Start Ethernet (DHCP, optional) ---
  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);

  // --- Init SHT40 ---
  Wire.begin(16, 17);
  sht40.begin(Wire, SHT40_I2C_ADDR_44);
  sht40.softReset();
  delay(100);

  uint32_t serialNumber;
  if (sht40.serialNumber(serialNumber) != 0)
  {
    Serial.println("SHT40 not detected!");
    sampleError = true;
  }
  else
  {
    Serial.print("SHT40 detected, serial: ");
    Serial.println(serialNumber);
  }

  // --- Init BMP280 ---
  if (!bmp.begin(0x76))
  {
    Serial.println("BMP280 not detected!");
    sampleError = true;
  }
  else
  {
    bmp.setSampling(Adafruit_BMP280::MODE_FORCED,
                    Adafruit_BMP280::SAMPLING_X1,
                    Adafruit_BMP280::SAMPLING_X1,
                    Adafruit_BMP280::FILTER_OFF);
    Serial.println("BMP280 initialized");
  }
}

// ------------------- Loop -------------------
unsigned long lastSample = 0;

void loop()
{
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_INTERVAL)
  {
    lastSample = now;

    if (eth_connected)
    {
      Serial.print("IP: ");
      Serial.println(ETH.localIP());
    }

    if (takeSample() && !sampleError)
    {
      printSample();
      sendToInflux(cTemp, pHumidity, pressure);
    }
  }
}

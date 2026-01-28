# The ESP32 Environmental Monitor
![PoESP32 Animated Image](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/images/PoESP32-Title.gif)
## Background
In our labs, it is important to have monitoring of environmental factors that can affect physics experiments. We choose to use a cheap PoE (power-over-ethernet) ESP32 board to communicate with an enviroment sensor. The readings are then sent over TCP message to an InfluxDB database which we monitor with Grafana.

## Requirements
1. M5Stack [PoESP32 device](https://shop.m5stack.com/products/esp32-ethernet-unit-with-poe)
2. M5Stack [ENV IV sensor unit](https://shop.m5stack.com/products/env-iv-unit-with-temperature-humidity-air-pressure-sensor-sht40-bmp280) (or any other I2C monitor)
3. A single [M5Stack ESP32 Downloader kit](https://shop.m5stack.com/products/esp32-downloader-kit) (only 1 needed for initial configuration)
4. A PoE network switch with an avaliable PoE port
5. A local lab network with an InfluxDB database set up to record incoming TCP messages for monitoring. This should be hosted on a PC with a fixed IP address. 

## Programming
_Once you've successfully programmed a single unit, skip step 1.  Repeating this process takes 5 minutes from start to finish._
1. [Set up your Arduino programming environment](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/ARDUINO-SETUP.md)
2. Disassemble the PoESP32 case
   - You will need a 1.5mm (M2) allen wrench to remove a single screw [pic](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/images/1-Allen.jpg)
   - Inserting a small flat head screwdriver into the slots flanking the Ethernet jack [pic](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/images/2-Slots.jpg), carefully separate the case halves; work it side by side to avoid damage [pic](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/images/3-Tabs.jpg)
> [!TIP]
> If you have fingernails, it can be quicker to slide a nail between the case halves, starting with the end opposite the Ethernet port and using another nail to pull the retaining tabs back
3. Plug in the ENV IV sensor unit [pic](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/images/5-Assembled.jpg)
4. With the USB-to-serial adapter unplugged, insert the pins in the correct orientation on the back of the PoESP32 mainboard [pic](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/images/4-Programmer.jpg)
> [!WARNING]
> Do not plug the PoESP32 device into Ethernet until after step 10 or you risk damaging your USB port! Additionally, Serial communication will fail when the ethernet cable is plugged in.
5. With light tension applied to ensure good connectivity to the programming through-hole vias on the PoESP32 (see step 4 pic), plug in the USB-to-serial adapter
   - The device is now in bootloader mode
6. First, we use Arduino to check some things about the ESP32 board.
   - Open the project `check_bus.ino`.
    - Select Tools->Board->esp32 and select "ESP32 Dev Module"
   - Select Tools->Port and select the USB-to-serial adapter
     - If you're unsure, unplug the USB-to-serial adapter, look at the port list, then plug it back in and select the new entry (repeating step 5)
   - Select Sketch->Upload to flash the device
   - After the flash is complete, in the Serial Monitor you should see some output code like
      ```
      I2C Scanner
      Found device at 0x44
      Found device at 0x76
      ```
      These are the bus addresses of the sensors that you will use to communicate with them. Record these number as you may need to modify the main code.
7. Now we will flash the main code to the board. In Arduino
   - Open the project `sensor_ethernet.ino`.
    - Select Tools->Board->esp32 and select "ESP32 Dev Module"
    - Select Tools->Port and select the USB-to-serial adapter
       - If you're unsure, unplug the USB-to-serial adapter, look at the port list, then plug it back in and select the new entry (repeating step 5)
   - Select Sketch->Upload to flash the device
   - When you see something similar to the following, the flash is complete.
     ```
     Writing at 0x000d0502... (100 %)
     Wrote 790896 bytes (509986 compressed) at 0x00010000 in 8.9 seconds (effective 714.8 kbit/s)...
     Hash of data verified.

     Leaving...
     Hard resetting via RTS pin...
    - In the Serial Monitor, you should see the following (exact values will change depending on your hardware):
      ```
      SHT40 detected, serial: 208058852
      BMP280 initialized
      Temp: 21.31 °C | Humidity: 43.03 % | Pressure: 987.83 hPa
      No Ethernet connection, skipping Influx send
      Starting Ethernet + SHT40 + BMP280 + InfluxDB...
      MAC address: B0:A7:32:55:DE:D8
      Sensor source set to: UNKNOWN
8. Now you need to modify the project `sensor_ethernet.ino` to work with your device and lab configuration.
      - change lines where the sensor is initialised (the ones that contain `!bmp.begin(0x76)`)  to reflect the final bus address that you recorded (this is probably already correct but should be checked).
      - change the InfluxDB settings starting `const char INFLUX_HOST[] = "192.168.23.12";` to match the configuration of your InfluxDB server. 
      - Add a block of code in the section `// Map MAC to SENSOR_SOURCE` to reflect the MAC address that you obtained in step 7. This maps your device's MAC address to a unique string that will be contained with the data sent to InfluxDB, allowing it to tell which sensor the readings came from. For example,
        ```
          } else if (strcmp(macStr, "B0:A7:32:55:CF:F8") == 0) {
        strcpy(SENSOR_SOURCE, "LASER_TABLE_MON1");
          ```
        associates the MAC address `B0:A7:32:55:CF:F8` with the label `LASER_TABLE_MON1` which will be sent alongside all data that the sensor collects.
9. Reflash the board with the modified code following the procedure in step 7.
10. Disconnect the USB-to-serial adapter and reassemble the case
11. Connect the PoESP32 to a PoE network port and mount as appropriate
    - The holes in the PoESP32 and ENV IV sensor cases work great with zip ties for rack install or screws if attaching to a backboard
      - See the /3Dmodels folder for print-able mounting plates or [Guidance and Limitations](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/README.md#guidance-and-limitations) for more detail
    - Do not mount the ENV IV directly on top of the PoESP32, as it generates enough heat to affect sensor readings
10. Configure your InfluxDB database as appropriate

## Guidance and Limitations
- For high humidity environments, this device will activate an internal sensor heater under certain conditions to ensure more accurate readings.
- Need a simple solution for mounting the PoESP32 and environmental monitor as a unitized assembly?  Within the /3Dmodels folder you will find:
  1. PoESP32-Environmental-1RU-Base.step : 3D print model to mount the PoESP32 assembly into a 1U rack space (w/optional wire cover and LED lightguides)
  2. PoESP32-Environmental-Mini.step : 3D print model for zip tie mounting (space constrained)
  3. PoESP32-Environmental-Mini-Magnet.step : 3D print model for magnet mounting (space constrained, compatible with 8mm x 2mm disc magnets)
  4. PoESP32-Environmental-Mid.step : 3D print model for zip tie mounting
  5. PoESP32-Environmental-Mid-Magnet.step : 3D print model for magnet mounting (compatible with 8mm x 2mm disc magnets)
  6. If you want to modify the models and make your own custom design:
     - [Onshape link - 1RU Base](https://cad.onshape.com/documents/126ed9d0ea20223ee2558e2e/w/5dc774b929e3cc882e2ccc02/e/70eb0dc678e6fe37b06c7b4d?renderMode=0&uiState=6674e589c5f25f7f012b9c1e)
     - [Onshape link - 1RU Lightguides for single-material printing](https://cad.onshape.com/documents/126ed9d0ea20223ee2558e2e/w/5dc774b929e3cc882e2ccc02/e/16542899019e9250b0249f1e?renderMode=0&uiState=6674e5a1c5f25f7f012b9c2a)
     - [Onshape link - 1RU Wire Cover](https://cad.onshape.com/documents/126ed9d0ea20223ee2558e2e/w/5dc774b929e3cc882e2ccc02/e/cc43e711478951692c63e341?renderMode=0&uiState=6674e5aec5f25f7f012b9c2f)
     - [Onshape link - Mini](https://cad.onshape.com/documents/126ed9d0ea20223ee2558e2e/w/f747afb8fc8c6e8e288e0fc9/e/70eb0dc678e6fe37b06c7b4d?renderMode=0&uiState=666d1111cd9bd3671768c9c6)
     - [Onshape link - Mini Magnetic](https://cad.onshape.com/documents/126ed9d0ea20223ee2558e2e/w/bfca9ecb4e85ab436c8e3736/e/70eb0dc678e6fe37b06c7b4d?renderMode=0&uiState=666d10f6cd9bd3671768c9a8)
     - [Onshape link - Mid](https://cad.onshape.com/documents/126ed9d0ea20223ee2558e2e/w/86908f6e4038f162632614a8/e/70eb0dc678e6fe37b06c7b4d?renderMode=0&uiState=6675aa9981594361815fa619)
     - [Onshape link - Mid Magnetic](https://cad.onshape.com/documents/126ed9d0ea20223ee2558e2e/w/887531df77e994a6e9e16eac/e/70eb0dc678e6fe37b06c7b4d?renderMode=0&uiState=666d10e8cd9bd3671768c99a)

## Technical Information
- Operating Specifications
  - Operating temperature: 0°F (-17.7°C) to 140°F (60°C)
  - Operating humidity: 5% to 90% (RH), non-condensing
- Sensor Accuracy
  - ±0.1 °C，±1.5 %RH
- Power Consumption
  - 6W maximum via 802.3af Power-over-Ethernet
- Ethernet
  - IP101G PHY
  - 10/100 Mbit twisted pair copper
  - IEEE 802.3af Power-over-Ethernet
- I/O Configuration
  - SHT40 temperature and humidity sensor
  - See [PORTINFO.md](https://github.com/CornishLabs/PoESP32-Environmental-Monitor/blob/main/PORTINFO.md)

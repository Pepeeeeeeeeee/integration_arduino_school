## A.U.R.A - ESP32

### Team members participating on development of the ESP32 code

- Peter Duga - Custom sensor libraries.
- Plamen Hristov - WiFi capabilities, offline data storage in RAM.

### Reviewers

- Peter Duga
- Plamen Hristov

### Installation

1. Clone the project from GitLab:
```bash
git clone git@gitlab.com:kdg-ti/integration-2.1/25-26/team-6-aura-ambient-unified-room-assessment/arduino.git
```
2. If necessary adjust WiFi credentials:
```C
static String WIFI_SSID = "REDACTED-NETWORK-NAME"; // network name

static String WIFI_PASS = ""; // individual password

static String API_URL = "http://10.134.178.156:8000/v1/ingest"; // data collector service

static String API_KEY = "REDACTED-API-KEY"; // api key to authenticate valid device in data collector service

static String DEVICE_UID = "ESP_TEST"; // Device name
```

   3. If your're using different pins adjust the following block in the code to your new pins (VCC/VDD is always power, the rest should be grounded):
```C
define DHT_PIN 5

define I2S_WS_PIN 25

define I2S_SCK_PIN 26

define I2S_SD_PIN 33

define BH1750_SCL_PIN 22

define BH1750_DAT_PIN 21
```
4.  Build and upload to esp.

### Hardware
- 1. Board -> ESP32 with WiFi module  
      A low-power dual-core microcontroller with built-in WiFi used to read sensors and send measurements to the API.
- 2. Light sensor -> BH1750  
      A digital lux sensor that measures ambient light intensity over I2C.
- 3. Microphone -> INMP441  
      A digital MEMS microphone that outputs I2S audio data for sound-level analysis.
- 4. Temperature & Humidity sensor -> DHT11  
      A basic digital temperature and humidity sensor communicating via a single-wire protocol.

### Libraries used
1. BH1750 -> Custom made light sensor library - Used to configure and read lux values from the BH1750 over the I2C bus.
	- 1.1  esp_err.h -> Provides standardized ESP-IDF error handling types.
	- 1.2  driver/i2c.h -> Provides low-level I2C driver functions for communicating with the BH1750 sensor.
	- 1.3  freertos/task.h -> Allows non-blocking timing and delays within FreeRTOS tasks.

2. DHT11 -> Custom made Temperature & Humidity library - Implements the single-wire protocol to read humidity and temperature from the DHT11.
	- 2.1  driver/gpio.h -> Enables direct control of the GPIO pin used for DHT11 communication.
	- 2.2  esp_err.h -> Provides return codes used for error checking during sensor reads.
	- 2.3  esp_timer.h -> Enables precise microsecond-level timing required by the DHT11 protocol.
	- 2.4  freertos/FreeRTOS.h -> Gives access to FreeRTOS base definitions.
	- 2.5  freertos/task.h -> Used for millisecond delays during the start signal.

3. INMP441_arduino -> Custom made microphone library (specific for Arduino framework) - Provides microphone initialization and RMS-based sound-level calculation using the Arduino I2S driver.
	- 3.1  I2S.h -> Provides high-level I2S audio input support in Arduino.
	- 3.2  math.h -> Used for computing RMS and logarithmic dB values.
	- 3.3  stdlib.h -> Used for allocating and freeing audio sample buffers.
	- 3.4  Arduino.h -> Base Arduino framework integration.

4. MeasurementBuffer -> Handles dynamic buffering of measurement objects before transmission.
	- 4.1  esp_heap_caps.h -> Provides access to specialized ESP32 memory allocations.
	- 4.2  Arduino.h -> Basic Arduino types and utilities used in the buffer.

5. APIClient -> Manages WiFi connectivity and sends sensor data to an external API endpoint.
	- 5.1  HTTPClient.h -> Used for making HTTP POST requests to the python data collector service.
	- 5.2  esp_system.h -> Provides access to system utilities like MAC address retrieval.
	- 5.3  time.h -> Used for timestamps and time-related operations.
	- 5.4  WiFi.h -> Handles WiFi connection setup and state management.
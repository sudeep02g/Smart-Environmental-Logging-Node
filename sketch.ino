#include <Arduino.h>
#include <Wire.h>          // Built-in library for I2C Communication
#include <SPI.h>           // Built-in library for SPI Communication
#include <SD.h>            // Built-in library for SD Cards
#include <DHT.h>           // Added library for DHT Sensor
#include <Adafruit_GFX.h>  // Added library for Screen graphics
#include <Adafruit_SSD1306.h> // Added library for OLED Driver


// 1. Hardware Pin Definitions (Matching our visual wiring!)
// Define the dimensions of our I2C OLED display

#define DHT_PIN         15
#define SD_CS_PIN       13
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create the OLED display driver object on the I2C bus
// -1 means the display doesn't share a physical reset pin with the ESP32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Create the DHT sensor object
// DHT22 is the specific silicon model we are using
DHT dht(DHT_PIN, DHT22);

// Global RAM variables to hold our live environment measurements
float liveTemperature = 0.0;
float liveHumidity    = 0.0;
bool  sensorError      = false; // Safety flag if a wire gets cut


// 2. Non-Blocking Timing Thresholds (In Milliseconds)
const unsigned long OLED_INTERVAL   = 500;   // Refresh screen every 500ms
const unsigned long SENSOR_INTERVAL = 2000;  // Sample sensor every 2 seconds (2000ms)
const unsigned long LOG_INTERVAL    = 10000; // Log to SD Card every 10 seconds (10000ms)

// 3. Time Tracker Variables (Stored in RAM to track execution history)
unsigned long lastOledTime   = 0;
unsigned long lastSensorTime = 0;
unsigned long lastLogTime    = 0;

void setup() {
    // 1. Initialize the UART Serial port for debugging at 115200 baud
    Serial.begin(115200);
    delay(500); // Small hardware stabilization delay on boot
    Serial.println("\n=== SYSTEM BOOT INITIALIZATION ===");

    // 2. Initialize the DHT22 Sensor protocol
    dht.begin();
    Serial.println("[OK] DHT22 Sensor Bus Initialized.");

    // 3. Initialize the I2C OLED Display at address 0x3C
    // SSD1306_SWITCHCAPVCC generates 3.3V internal display voltage from the bus
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("[ERROR] OLED Allocation Failed! Check I2C wiring.");
        while(1); // Hard freeze the CPU if critical display hardware is missing
    }
    Serial.println("[OK] I2C OLED Display Initialized.");
    
    // Clear display buffer and show a quick welcome message
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println("ENV NODE ONLINE");
    display.display(); // Blast pixels onto physical screen

    // 4. Initialize the SPI MicroSD Card Module
    Serial.println("Initializing SD Card SPI Bus...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[WARNING] SD Card Mount Failed! Check SPI wiring or insert card.");
        // We won't crash the loop here, because we want the live screen to keep working 
        // even if the SD card is temporarily removed!
    } else {
        Serial.println("[OK] SPI SD Card Module Initialized.");
        
        // Safety: If log file doesn't exist yet, create it and write CSV Headers
        File logFile = SD.open("/log.csv", FILE_WRITE);
        if (logFile) {
            // Check if file is completely empty (size == 0) to add headers
            if (logFile.size() == 0) {
                logFile.println("Timestamp_ms,Temperature_C,Humidity_Percent");
                Serial.println("[FILE] Created log.csv and appended headers.");
            }
            logFile.close(); // Always close file to flush bits and save power
        }
    }
    // === READ AND PRINT FILE FOR VERIFICATION ===
        File logFileRead = SD.open("/log.csv", FILE_READ);
        if (logFileRead) {
            Serial.println("\n--- READING LOG.CSV FROM SD CARD ---");
            while (logFileRead.available()) {
                Serial.write(logFileRead.read());
            }
            Serial.println("-------------------------------------\n");
            logFileRead.close();
        }
    Serial.println("=== SYSTEM RUNNING IN PARALLEL LOOP ===\n");
}


void loop() {
    // This runs CONTINOUSLY at maximum CPU clock speed
    unsigned long currentTime = millis(); // Capture current system uptime clock

    // === TASK 1: Live OLED Dashboard Refresh ===
    if (currentTime - lastOledTime >= OLED_INTERVAL) {
        lastOledTime = currentTime; 
        
        // Clear the previous frame out of the screen buffer
        display.clearDisplay();
        
        // Draw a clean frame title
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("--- ENV MONITOR ---");
        
        // Check our safety flag from Task 2
        if (sensorError) {
            display.setCursor(0, 24);
            display.setTextSize(2);
            display.println("SENSOR ERR");
        } else {
            // Render the live data onto the screen buffer
            display.setTextSize(1);
            
            display.setCursor(0, 20);
            display.print("Temp:     ");
            display.setTextSize(2); // Make the numbers big and easy to read
            display.print(liveTemperature, 1); // 1 decimal place
            display.setTextSize(1);
            display.println(" C");
            
            display.setCursor(0, 44);
            display.setTextSize(1);
            display.print("Humidity: ");
            display.setTextSize(2);
            display.print(liveHumidity, 1);
            display.setTextSize(1);
            display.println(" %");
        }
        
        // Push the compiled pixel frame over I2C to the physical screen
        display.display();
    }

    // === TASK 2: Sensor Sampling ===
    if (currentTime - lastSensorTime >= SENSOR_INTERVAL) {
        lastSensorTime = currentTime; 
        
        // Read values from physical sensor registers
        float h = dht.readHumidity();
        float t = dht.readTemperature();

        // Safety Check: If the sensor is unplugged, read values will be "NaN" (Not a Number)
        if (isnan(h) || isnan(t)) {
            Serial.println("[ERROR] Failed to read from DHT sensor!");
            sensorError = true;
        } else {
            // Update our global variables so other tasks can use the fresh data
            liveTemperature = t;
            liveHumidity = h;
            sensorError = false;
            
            Serial.print("[SENSOR] Temp: ");
            Serial.print(liveTemperature);
            Serial.print(" C | Humidity: ");
            Serial.print(liveHumidity);
            Serial.println(" %");
        }
    }

    // === TASK 3: Persistent Flash Storage Logging ===
    if (currentTime - lastLogTime >= LOG_INTERVAL) {
        lastLogTime = currentTime; 
        
        // Only log if the sensor is working perfectly
        if (!sensorError) {
            // Open the file on the SD card in append mode
            File dataFile = SD.open("/log.csv", FILE_WRITE);
            
            if (dataFile) {
                // Write data separated by commas (CSV format)
                dataFile.print(currentTime);
                dataFile.print(",");
                dataFile.print(liveTemperature, 1);
                dataFile.print(",");
                dataFile.println(liveHumidity, 1); // println adds a new line at the end
                
                // Close the file to safely save the bits to flash memory
                dataFile.close();
                Serial.println("[SD CARD] Data packet successfully saved to log.csv");
                // Read back the file immediately to prove it logged!
                File testRead = SD.open("/log.csv", FILE_READ);
                if (testRead) {
                    Serial.println("--- CURRENT FILE CONTENTS ---");
                    while (testRead.available()) {
                        Serial.write(testRead.read());
                    }
                    Serial.println("-----------------------------");
                    testRead.close();
                }
            } else {
                Serial.println("[ERROR] Could not open log.csv file!");
            }
        }
    }
}

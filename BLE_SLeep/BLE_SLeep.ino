#include <bluefruit.h>

// Include the systemOff function from wiring.h
extern void systemOff(uint32_t pin, uint8_t wake_logic);

// Beacon uses the Manufacturer Specific Data field in the advertising packet,
// which means you must provide a valid Manufacturer ID. Update
// the field below to an appropriate value. For a list of valid IDs see:
// https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers
// - 0x004C is Apple
// - 0x0822 is Adafruit
// - 0x0059 is Nordic
// For testing with this sketch, you can use nRF Beacon app
// - on Android you may need change the MANUFACTURER_ID to Nordic
// - on iOS you may need to change the MANUFACTURER_ID to Apple.
//   You will also need to "Add Other Beacon, then enter Major, Minor that you set in the sketch
#define MANUFACTURER_ID   0x0059

// GPIO pin for wake-up (GPIO5)
#define WAKEUP_PIN 8

// Advertising duration in milliseconds (10 seconds)
#define ADV_DURATION_MS 10000

// "nRF Connect" app can be used to detect beacon
uint8_t beaconUuid[16] = {
  0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
  0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0
};

// A valid Beacon packet consists of the following information:
// UUID, Major, Minor, RSSI @ 1M
BLEBeacon beacon(beaconUuid, 1, 2, -54);

// Timestamp for tracking advertising duration
uint32_t advStartTime = 0;

void setup() {
  Serial.begin(115200);

  // Uncomment to blocking wait for Serial connection
  while ( !Serial ) delay(10);

  Serial.println("\n=====================================================");
  Serial.println("Bluefruit52 Beacon with Sleep/Wake Functionality");
  Serial.println("=====================================================\n");

  Bluefruit.begin();

  // off Blue LED for lowest power consumption
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(0);    // Check bluefruit.h for supported values

  // Manufacturer ID is required for Manufacturer Specific Data
  beacon.setManufacturer(MANUFACTURER_ID);

  // Setup the advertising packet
  startAdv();

  Serial.printf("Broadcasting beacon with MANUFACTURER_ID = 0x%04X\n", MANUFACTURER_ID);
  Serial.println("\n--- Device Operation ---");
  Serial.println("✓ Advertising for 10 seconds");
  Serial.println("✓ Then entering System OFF mode (ultra-low power)");
  Serial.printf("✓ Wake up by setting GPIO%d to HIGH\n", WAKEUP_PIN);
  Serial.println("\nScanning for this beacon:");
  Serial.println("- Android: nRF Beacon app with MANUFACTURER_ID = 0x0059");
  Serial.println("- iOS: nRF Beacon app with MANUFACTURER_ID = 0x004C\n");
  Serial.println("=====================================================\n");

  // Record the start time
  advStartTime = millis();
}

void startAdv(void)
{  
  // Advertising packet
  // Set the beacon payload using the BLEBeacon class populated
  // earlier in this example
  Bluefruit.Advertising.setBeacon(beacon);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.setName("ISC nRF52832");
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * Apple Beacon specs
   * - Type: Non-connectable, scannable, undirected
   * - Fixed interval: 100 ms -> fast = slow = 100 ms
   */
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 160);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void goToSleep(void)
{
  // Stop advertising to save power
  Bluefruit.Advertising.stop();
  
  Serial.println("\n=====================================================");
  Serial.println(">>> Entering System OFF mode (ultra-low power sleep)");
  Serial.println("=====================================================");
  Serial.println("Device is now in deep sleep...");
  Serial.println("Current consumption: ~2-5 µA\n");
  Serial.printf("Waiting for GPIO%d to go HIGH to wake up...\n", WAKEUP_PIN);
  Serial.println("(Connect GPIO5 to 3.3V to wake the device)\n");
  
  // Small delay to ensure serial is flushed
  delay(100);
  
  // Enter System OFF mode
  // GPIO5 is configured as pull-down input
  // Device wakes when GPIO5 goes HIGH (wake_logic = 1)
  systemOff(WAKEUP_PIN, 1);
  
  // Code execution will NOT reach here until device is woken up
  // and goes through setup() again
}

void loop() {
  // Check if 10 seconds have passed
  uint32_t elapsedTime = millis() - advStartTime;
  
  if (elapsedTime >= ADV_DURATION_MS) {
    // Time to sleep
    goToSleep();
    // This will not return, device will reset on wake-up
  }
  
  // Print countdown every 2 seconds during advertising
  if ((elapsedTime % 2000) == 0 && elapsedTime > 0) {
    uint32_t remainingTime = (ADV_DURATION_MS - elapsedTime) / 1000;
    Serial.printf("Advertising... %ld seconds remaining\n", remainingTime);
  }
  
  // Keep the device responsive during advertising period
  delay(100);
}

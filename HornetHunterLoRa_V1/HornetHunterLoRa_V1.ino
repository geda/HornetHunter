#include <RadioLib.h>
#include <SPI.h>

// ESP32 Pins (your wiring)
#define LORA_NSS 4    // Blue
#define LORA_RESET 3  // White
#define LORA_DIO0 6   // Grey
#define LORA_SCK 5    // Orange
#define LORA_MISO 7   // Green
#define LORA_MOSI 10  // Yellow

SPIClass* customSPI = new SPIClass(FSPI);
SX1276 radio = new Module(
  LORA_NSS,
  LORA_DIO0,
  LORA_RESET,
  RADIOLIB_NC,
  *customSPI);

// bit rate is fixed here; used to compute max freqDev
const float BIT_RATE_KBPS = 4.8;  // [kbps]
// your “nominal” deviation you’d like to use if the constraints allow
const float NOMINAL_DEV_KHZ = 5.0;  // [kHz]

// all allowed FSK rxBw values, descending
const float RXBW_LIST_KHZ[] = {
  250.0, 200.0, 166.7, 125.0, 100.0, 83.3, 62.5, 50.0,
  41.7, 31.3, 25.0, 20.8, 15.6, 12.5, 10.4, 7.8,
  6.3, 5.2, 3.9, 3.1, 2.6
};
const int NUM_RXBW = sizeof(RXBW_LIST_KHZ) / sizeof(RXBW_LIST_KHZ[0]);

int bwIndex = 0;
unsigned long lastStepMs = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  customSPI->begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

  // hardware reset
  pinMode(LORA_RESET, OUTPUT);
  digitalWrite(LORA_RESET, LOW);
  delay(100);
  digitalWrite(LORA_RESET, HIGH);
  delay(100);

  // initialize FSK at your freq, bit rate, and a safe “starting” deviation
  int state = radio.beginFSK(
    868.0,         // frequency
    1.2,           // bitrate
    1.2,           // freq deviation
    5.0,           // RX bandwidth
    10,            // preamble length
    3,             // sync word length
    { 0xC1,0x94,0xC1}, // a 3-byte sync word
    RADIOLIB_FSK_SHAPING_NONE,
    true           // CRC on
  );
  if (st != RADIOLIB_ERR_NONE) {
    Serial.print("FSK init failed, code ");
    Serial.println(st);
    while (true)
      ;
  }
  Serial.println("FSK ready for CW RSSI");

  // disable packet engine
  //radio.setSyncWord(nullptr, 0);
  //radio.setPreambleLength(0);
  //radio.setCrcFiltering(false);

  // smoothing
  //radio.setRSSIConfig(8, 0);

  // start continuous receive so RSSI register updates
  st = radio.startReceive();
  if (st != RADIOLIB_ERR_NONE) {
    Serial.print("startReceive failed, code ");
    Serial.println(st);
    while (true)
      ;
  }

  // timestamp our first step
  lastStepMs = millis();
  bwIndex = 0;
}

void loop() {
  // every 30 s, step to the next smaller rxBw
  if (millis() - lastStepMs >= 5000) {
    lastStepMs = millis();
    bwIndex = (bwIndex + 1) % NUM_RXBW;
    float newBw = RXBW_LIST_KHZ[bwIndex];

    // apply new Rx bandwidth
    //radio.setRxBandwidth(newBw);   // sets receiver bandwidth
    //radio.setAFCBandwidth(newBw);  // keep AFC in sync

    // recompute freqDev so: freqDev ≤ 200 kHz, freqDev + br/2 ≤ 250 kHz
    float maxDevByBr = 250.0f - (BIT_RATE_KBPS / 2.0f);
    float chosenDev = NOMINAL_DEV_KHZ;
    if (chosenDev > maxDevByBr) { chosenDev = maxDevByBr; }
    if (chosenDev > 200.0f) { chosenDev = 200.0f; }
    if (chosenDev < 0.6f) { chosenDev = 0.6f; }
  //  radio.setFrequencyDeviation(chosenDev);
   // radio.startReceive();

    Serial.print("→ Switched to rxBw=");
    Serial.print(newBw, 1);
    Serial.print(" kHz, freqDev=");
    Serial.print(chosenDev, 1);
    Serial.println(" kHz");
  }

  // just print the live RSSI every loop
  float rssi = radio.getRSSI();
  if (rssi > -90) {
    Serial.print("CW RSSI: ");
    Serial.print(rssi, 1);
    Serial.println(" dBm");
  }

  delay(20);
}

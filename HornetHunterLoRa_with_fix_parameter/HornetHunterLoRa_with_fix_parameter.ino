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

  /*
  → Switched to rxBw=25.0 kHz, freqDev=5.0 kHz freq=430.815 MHz
CW RSSI: 0.0 dBm
*/

// bit rate is fixed here; used to compute max freqDev
const float BIT_RATE_KBPS = 4.8;  // [kbps]
// your “nominal” deviation you’d like to use if the constraints allow
const float NOMINAL_DEV_KHZ = 5.0;  // [kHz]

/*
const float RXBW_LIST_KHZ[] = {
  250.0, 200.0, 166.7, 125.0, 100.0, 83.3, 62.5, 50.0,
  41.7, 31.3, 25.0, 20.8, 15.6, 12.5, 10.4, 7.8,
  6.3, 5.2, 3.9, 3.1, 2.6
};
*/
const float NOMINAL_RXBW_KHZ = 2.6;

const float NOMINAL_FREQ_MHZ=430.825; // 430.825
const float NOMINAL_FREQ_MHZ_MAX=430.850; // 430.825

float freq = NOMINAL_FREQ_MHZ;
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
  NOMINAL_FREQ_MHZ,  // ← Change from 868.0 to 433.0 MHz
  1.2, // Bit rate
  NOMINAL_DEV_KHZ, // freqDev
  NOMINAL_RXBW_KHZ, //Receiver bandwidth in kHz
  10, //Transmission output power in dBm.
  1, // preambleLength Length of FSK preamble in bits.
  false);



  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("FSK init failed, code ");
    Serial.println(state);
    while (true)
      ;
  }
  Serial.println("FSK ready for CW RSSI");

  // disable packet engine
  //radio.setSyncWord(nullptr, 0);
  //radio.setPreambleLength(0);
  //radio.setCrcFiltering(false);

  // smoothing
  state = radio.setRSSIConfig(0, 0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("setRSSIConfig failed, code ");
    Serial.println(state);
    while (true)
      ;
  }

  // start continuous receive so RSSI register updates
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("startReceive failed, code ");
    Serial.println(state);
    while (true)
      ;
  }

  // timestamp our first step
  lastStepMs = millis();

}

void loop() {
  // every 30 s, step to the next smaller rxBw
  int state;

  //state = radio.startReceive();
  state = radio.receiveDirect();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("startReceive failed, code ");
    Serial.println(state);
  }


  // just print the live RSSI every loop
  float rssi = radio.getRSSI(false, true);

  if (rssi > -90) {
    Serial.print("CW RSSI: ");
    Serial.print(rssi, 1);
    Serial.println(" dBm");
  }

  
}
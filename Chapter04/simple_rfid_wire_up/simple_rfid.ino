// Include required libraries
#include <SPI.h>
#include <MFRC522.h>

// Pin definitions
const uint8_t SPI_PIN = 5;
const uint8_t RESET_PIN = 0;

// Define RFIDReader class
class RFIDReader {
public:
  RFIDReader(uint8_t spiPin, uint8_t resetPin) : reader(spiPin, resetPin) {}

  void initialize() {
    SPI.begin();
    reader.PCD_Init();
    for (uint8_t i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  }

  bool isNewCardPresent() {
    return reader.PICC_IsNewCardPresent();
  }

  bool readCardSerial() {
    return reader.PICC_ReadCardSerial();
  }

  void printReaderVersion() {
    reader.PCD_DumpVersionToSerial();
  }

  void printCardDetails() {
    if(isNewCardPresent() && readCardSerial()) {
      Serial.print(F("RFID Decimal: "));
      for (uint8_t i = 0; i < reader.uid.size; i++) {
        Serial.print(reader.uid.uidByte[i], DEC);
        Serial.print(i < reader.uid.size - 1 ? ", " : "\n");
      }
      reader.PICC_HaltA();
      reader.PCD_StopCrypto1();
    }
  }

private:
  MFRC522 reader;
  MFRC522::MIFARE_Key key;
};

// Create an RFIDReader object
RFIDReader myRFIDReader(SPI_PIN, RESET_PIN);

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println(F("System is starting"));

  // Initialize the RFID reader
  myRFIDReader.initialize();
  myRFIDReader.printReaderVersion();
}

void loop() {
  // Read RFID card details
  myRFIDReader.printCardDetails();
}
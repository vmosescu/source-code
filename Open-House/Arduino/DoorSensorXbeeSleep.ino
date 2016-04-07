#include <XBee.h>
#include <Printers.h>
#include <AltSoftSerial.h>
#include "binary.h"

#define STATE_PIN  13
#define SENSOR_INPUT  10
#define BATTERY_STATUS 0


XBeeWithCallbacks xbee;

AltSoftSerial SoftSerial;
#define DebugSerial Serial
#define XBeeSerial SoftSerial

// every n minute send status
const unsigned long SENT_PERIOD = 1000*60*2;
const uint8_t CLOSED = 0;
const uint8_t OPEN = 1;
const uint8_t UNDEFINED_STATUS = 2;

const uint8_t XBEE_CTS_PIN = 6;
const uint8_t XBEE_SLEEPRQ_PIN = 7;


const int DELAY = 5000;

uint8_t lastStatus = UNDEFINED_STATUS;
unsigned long lastSent = 0;
unsigned long firstClosed = 0;

void setup() {
  
  DebugSerial.begin(115200);
  DebugSerial.println(F("Starting..."));
  
  // Setup XBee serial communication
  XBeeSerial.begin(9600);
  xbee.begin(XBeeSerial);
  delay(1);

  // Setup callbacks
  xbee.onPacketError(printErrorCb, (uintptr_t)(Print*)&DebugSerial);
  xbee.onResponse(printErrorCb, (uintptr_t)(Print*)&DebugSerial);

 // state led
  pinMode(STATE_PIN,OUTPUT);
  // sensor pull-up input: LOW=close; HIGH=open
  pinMode(SENSOR_INPUT,INPUT_PULLUP);
  
}

void loop() {
  
  int newRead = digitalRead(SENSOR_INPUT); 
  uint8_t newStatus = getStatus(newRead);
  // if is undefined re-read
  if(newStatus==UNDEFINED_STATUS){
    DebugSerial.print(F("undefined status for input readed: "));
    DebugSerial.println(newRead);
    return;
  }
  // LED
  digitalWrite(STATE_PIN,newStatus);
  float batteryStatus = getBatteryStatus();
  unsigned long sentPeriod = millis()-lastSent;
  
  if(newStatus!=lastStatus || 
    sentPeriod>SENT_PERIOD){
        wakeUpAndSend(newStatus,batteryStatus);
  }
 
  lastStatus=newStatus;

}

uint8_t getStatus(int status){
  if(status==1){
    return OPEN;
  }
  if(status==0){
    return CLOSED;
  }
  return UNDEFINED_STATUS;
}


void wakeUpAndSend(uint8_t doorStatus, float batteryStatus){
  DebugSerial.println(F("Trying XBee to wake up"));
  // Wake up the XBee module, and wait until it is awake (up to 1000ms)
  pinMode(XBEE_SLEEPRQ_PIN, OUTPUT);
  if (!waitForPin(XBEE_CTS_PIN, LOW, 1000))
    DebugSerial.println(F("XBee failed to wake up"));
  uint8_t status = sendPacket(doorStatus,batteryStatus);
  if (status == NOT_JOINED_TO_NETWORK) {
    DebugSerial.println(F("Not joined, keeping XBee awake to join"));
    //doSleep(30000);
    delay(30000);
  }
  
  // Put the XBee back to sleep.
  //  Don't write HIGH, but let the internal pullup take care of that
  //  (this makes this code work for 5V Arduinos as well).
  pinMode(XBEE_SLEEPRQ_PIN, INPUT);

}


uint8_t sendPacket(uint8_t doorStatus, float batteryStatus) {
  DebugSerial.print(F("trimite: "));
  DebugSerial.print(doorStatus);
  DebugSerial.print("; baterie: ");
  DebugSerial.println(batteryStatus);
  

    // Prepare the Zigbee Transmit Request API packet
    ZBTxRequest txRequest;
    txRequest.setAddress64(0x0000000000000000);

    // Allocate 9 payload bytes: 1 type byte plus two floats of 4 bytes each
    AllocBuffer<6> packet;

    // Packet type, temperature, humidity
    packet.append<uint8_t>(1);
    packet.append<uint8_t>(doorStatus);
    packet.append<float>(batteryStatus);
    txRequest.setPayload(packet.head, packet.len());

    // And send it
  uint8_t status = xbee.sendAndWait(txRequest, 5000);
  if (status == 0) {
    DebugSerial.println(F("Succesfully sent packet"));
  } else {
    DebugSerial.print(F("Failed to send packet: 0x"));
    DebugSerial.println(status, HEX);
  }
    
    lastSent=millis();
    
    return status;
}

// Wait for the given pin to become the given value. Returns true when
// that happened, or false when timeout ms passed
bool waitForPin(uint8_t pin, uint8_t value, uint16_t timeout) {
  unsigned long start = millis();
  while(true) {
    if (digitalRead(pin) == value)
      return true;
    if (millis() - start > timeout)
      return false;
  }
}

float getBatteryStatus(){
  int batteryValue = analogRead(BATTERY_STATUS);
  float voltage = batteryValue * (3.30 / 1023.00) * 2;
  return voltage;
}

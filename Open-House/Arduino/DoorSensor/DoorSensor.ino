#include <XBee.h>
#include <Printers.h>
#include <AltSoftSerial.h>
#include "binary.h"

#define STATE_PIN  13
#define SENSOR_INPUT  10


XBeeWithCallbacks xbee;

AltSoftSerial SoftSerial;
#define DebugSerial Serial
#define XBeeSerial SoftSerial

// every n minute send status
const unsigned long SENT_PERIOD = 1000*15*1;
const unsigned long CLOSED_PERIOD = 1000*3;
const unsigned int CLOSED = 0;
const unsigned int OPEN = 1;
const unsigned int UNDEFINED_STATUS = -1;

const int DELAY = 5000;

int lastRead = -1;
int lastStatus = -1;
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
  int newStatus = getStatus(newRead);
  // if is undefined re-read
  if(newStatus==UNDEFINED_STATUS){
    DebugSerial.print(F("undefined status for input readed: "));
    DebugSerial.println(newRead);
    lastRead = newRead;
    return;
  }
  digitalWrite(STATE_PIN,newStatus);
  unsigned long sentPeriod = millis()-lastSent;
  
  if(newStatus!=lastStatus || 
    sentPeriod>SENT_PERIOD){
        sendPacket(newStatus);
  }
 
  lastRead = newRead;
  lastStatus=newStatus;

}

int getStatus(int status){
  if(status==1){
    return OPEN;
  }
  if(status==0){
    return CLOSED;
  }
  return UNDEFINED_STATUS;
}


void sendPacket(int status) {
  DebugSerial.print("trimite: ");
  DebugSerial.println(status);
  
/*
    // Prepare the Zigbee Transmit Request API packet
    ZBTxRequest txRequest;
    txRequest.setAddress64(0x0000000000000000);

    // Allocate 9 payload bytes: 1 type byte plus two floats of 4 bytes each
    AllocBuffer<9> packet;

    // Packet type, temperature, humidity
    packet.append<uint8_t>(1);
    packet.append<uint8_t>((uint8_t)status);
     txRequest.setPayload(packet.head, packet.len());

    // And send it
    xbee.send(txRequest);
 */   
    lastSent=millis();
}

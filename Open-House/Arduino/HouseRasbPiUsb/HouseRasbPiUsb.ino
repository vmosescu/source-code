#include <XBee.h>
#include <Printers.h>
#include <AltSoftSerial.h>
#include "binary.h"

char dataString[50] ={0};
const uint8_t CLOSED = 0;

XBeeWithCallbacks xbee;
AltSoftSerial SoftSerial;

#define USBSerial Serial
#define XBeeSerial SoftSerial


void setup() {
  // Setup usb serial output
  USBSerial.begin(9600);
  
  // Setup XBee serial communication
  XBeeSerial.begin(9600);
  xbee.begin(XBeeSerial);
  delay(1);

  // Setup callbacks
  //xbee.onPacketError(printErrorCb, (uintptr_t)(Print*)&DebugSerial);
  //xbee.onResponse(printErrorCb, (uintptr_t)(Print*)&DebugSerial);
  xbee.onZBRxResponse(processRxPacket);

}

void loop() {
  // Check the serial port to see if there is a new packet available
  xbee.loop();
}

void processRxPacket(ZBRxResponse& rx, uintptr_t) {
  Buffer b(rx.getData(), rx.getDataLength());
  uint8_t type = b.remove<uint8_t>();

  if (type == 1 && b.len() == 5) {
    uint8_t doorStatus = b.remove<uint8_t>();
    float batteryVoltage = b.remove<float>();
    sentToUsbSerial(doorStatus, batteryVoltage);
 }

}

void sentToUsbSerial(uint8_t doorStatusInt, float batteryVoltage){
  char* doorStatus = "open";
  if(doorStatusInt==CLOSED){
    doorStatus = "close";
  }
  // transform float in string
  char str_temp[6];
  dtostrf(batteryVoltage, 4, 2, str_temp);
  // {"door":"close","bts":"4.80"}
  sprintf(dataString,"{\"door\":\"%s\",\"bts\":\"%s\"}",doorStatus,str_temp);
  USBSerial.println(dataString);
}

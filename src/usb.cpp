#include "Beeton.h"

#include <FS.h>
#include <SD.h>
void Beeton::sendAllKnownThingsToUsb(){
  if(!lightThread){
    return;
  }
  sendUsb("BEGIN_THINGS");
  for(const auto& entry: thingIdToIp){
    uint16_t key = entry.first;
    const String& ip = entry.second;

    uint8_t thing = (key >> 8) & 0xFF;
    uint8_t id    = key & 0xFF;
    unsigned long lastSeen = lightThread->getLastEchoTime(ip);
    
    sendUsb("THING %02X:%d, lastSeen=%lu ms ago\n",
                      thing, id, millis() - lastSeen);
  }
  sendUsb("END_THINGS");
}

void Beeton::sendFileOverUsb(String filename){
  File f = SD.open("/beeton/" + filename);
  if (!f) {
      sendUsb("ERROR: File %s not found", filename.c_str());
      return;
  }

  sendUsb("BEGIN_FILE,%s", filename.c_str());
  while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
          sendUsb("%s", line.c_str());
      }
  }
  f.close();
  sendUsb("END_FILE,%s", filename.c_str());
}

void Beeton::sendUsb(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    Serial.print("[USB] ");
    Serial.println(buffer);  // for now just output directly
}

void Beeton::updateUsb() {
    static String input = "";

    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (input.length() > 0) {
                input.trim();  // Remove any accidental whitespace

                if (input.equalsIgnoreCase("GETTHINGS")) {
                    sendAllKnownThingsToUsb();
                }
                else if(input.startsWith("GETFILE,")){
                  String filename = input.substring(8);
                  sendFileOverUsb(filename);
                }
                else {
                    sendUsb("ECHO: %s", input.c_str());
                }

                input = "";
            }
        } else {
            input += c;
        }
    }
}

#include <Arduino.h>
#include <LightThread.h>
#include <Beeton.h>

// Define Thread network parameters
const String NETWORK_KEY = "00112233445566778899AABBCCDDEEFF"; // Replace with your key
const String NETWORK_NAME = "TestNetwork";
const int CHANNEL = 11;

#define JOIN_BUTTON_PIN 9
LightThread lightThread(JOIN_BUTTON_PIN, LightThread::JOINER,"", NETWORK_KEY,NETWORK_NAME, CHANNEL);
Beeton beeton(lightThread);

// Change to your device identity and action
const uint16_t MY_THING = 0x0001; // e.g., TRAIN
const uint16_t MY_ID = 1;

void handlePacket(const Packet& packet, const String& srcIp) {
    if (packet.thing == MY_THING && packet.id == MY_ID) {
        switch (packet.action) {
            case 0x02: // MOVE
                Serial.println("Received MOVE command");
                break;
            case 0x03: // HONK
                Serial.println("Received HONK command");
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    lightThread.begin();

    delay(1000); // Let the network start

    
}

void loop() {
    beeton.update(handlePacket); // listen for incoming actions

    if (lightThread.isState(LightThread::JOINER_PAIRED)){
        // === SEND STARTUP PACKET ===
        Packet packet;
        packet.reliable = 1;
        packet.thing = MY_THING;
        packet.id = MY_ID;
        packet.priority = 10;
        packet.sequence = millis();
        packet.action = 0x02; // MOVE
        static uint8_t payload[1] = {128}; // Speed value
        packet.dataLength = 1;
        packet.data = payload;

        beeton.sendPacket(packet);
        Serial.println("Sent MOVE packet");
    }

    

    delay(50);

}

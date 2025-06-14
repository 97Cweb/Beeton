
#include "Beeton.h"

Beeton beeton;
LightThread lightThread;

void setup() {
    Serial.begin(115200);
    lightThread.begin();
    beeton.begin(lightThread);

    beeton.onMessage([](uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
        Serial.printf("Received action %u for %u:%u with %d bytes\n", action, thing, id, payload.size());
        // You can now do high-level handling here
    });
}

void loop() {
    beeton.update();

    if (lightThread.isReady()) {
        // Example: send a SETSPEED action to MOTOR 1 every 5 seconds
        static unsigned long last = 0;
        if (millis() - last > 5000) {
            last = millis();
            Serial.printf("thing %02X\n",beeton.getThingId("train"));
            Serial.printf("action %02X\n",beeton.getActionId("train", "setspeed"));
            beeton.send(BEETON::RELIABLE, beeton.getThingId("train"), 1, beeton.getActionId("train", "setspeed"), 100);

        }
    }
}

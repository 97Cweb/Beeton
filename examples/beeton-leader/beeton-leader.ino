
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
}

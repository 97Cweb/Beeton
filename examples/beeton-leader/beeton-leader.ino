
#include "Beeton.h"

Beeton beeton;
LightThread lightThread;

void setup() {
    Serial.begin(115200);
    lightThread.begin();
    beeton.begin(lightThread);

    beeton.onMessage([](uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
        Serial.printf("update from %02X:%d received, action %02X occurred\n",thing, id, action);    });
}

void loop() {
    beeton.update();
}

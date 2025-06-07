
#include "Beeton.h"

// NOTE: You must define your own MYTHINGS and MYACTIONS in a file like MYBEETON.h
#include "../MYBEETON.h"

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
            beeton.send(BEETON::RELIABLE, MYTHINGS::MOTOR, 1, MYACTIONS::SETSPEED, 100);
        }
    }
}

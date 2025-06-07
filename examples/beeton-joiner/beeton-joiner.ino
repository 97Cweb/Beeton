#include <Beeton.h>

// NOTE: You must define your own MYTHINGS and MYACTIONS in a file like MYBEETON.h
#include "../MYBEETON.h"

LightThread lightThread;
Beeton beeton;

void setup() {
    Serial.begin(115200);
    delay(1000);

    lightThread.begin();
    beeton.begin(lightThread);

    // Declare available devices
    beeton.defineThings({
        { MYTHINGS::MOTOR, 1 },
        { MYTHINGS::MOTOR, 2 },
        { MYTHINGS::SENSOR, 1 }
    });

    // Handle only user-defined actions
    beeton.onMessage([](uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
        if (thing == MYTHINGS::MOTOR && action == MYACTIONS::SETSPEED && payload.size() == 1) {
            Serial.printf("[Motor %d] Set speed to %d\n", id, payload[0]);
        }
    });
}

void loop() {
    beeton.update();
    delay(10);
}

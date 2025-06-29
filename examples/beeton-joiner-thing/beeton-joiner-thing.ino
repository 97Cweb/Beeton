#include <Beeton.h>

LightThread lightThread;
Beeton beeton;

void setup() {
    Serial.begin(115200);
    delay(1000);

    lightThread.begin();
    beeton.begin(lightThread);

    // Handle only user-defined actions
    beeton.onMessage([](uint16_t thingId, uint8_t id, uint8_t actionId, const std::vector<uint8_t>& payload) {
        String thing = beeton.getThingName(thingId);
        String action = beeton.getActionName(thing, actionId);
        if (thing == "train"){
          if (action == "setspeed" && payload.size() == 1) {
            Serial.printf("[Motor %d] Set speed to %d\n", id, payload[0]);
            beeton.send(BEETON::RELIABLE,beeton.getThingId("train"),1,beeton.getActionId("train", "setspeed"),payload);
          } 
        } 
        else if (thing == "signal") {
            Serial.printf("🚦 Signal %d: Action %d\n", id, action);
        }
    });
}

void loop() {
    beeton.update();
    delay(10);
}

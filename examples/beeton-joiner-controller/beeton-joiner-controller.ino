#include <Beeton.h>

LightThread lightThread;
Beeton beeton;

void setup() {
    Serial.begin(115200);
    delay(1000);

    lightThread.begin();
    beeton.begin(lightThread);

    pinMode(GPIO_NUM_14,INPUT_PULLUP);
}

void loop() {
    beeton.update();
    if(lightThread.isReady()){
      
      bool buttonPress = !digitalRead(GPIO_NUM_14);
      Serial.printf("button %d\n",buttonPress);
      if (buttonPress){
        beeton.send(BEETON::RELIABLE, beeton.getThingId("train"), 1, beeton.getActionId("train", "setspeed"), 100);
      }
    }
    delay(10);
}

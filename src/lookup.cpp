#include "Beeton.h"
// Lookup functions for name ↔ ID mappings
String Beeton::getThingName(uint8_t thing) {
    return thingToName.count(thing) ? thingToName[thing] : "unknown";
}

String Beeton::getActionName(String thingName, uint8_t actionId) {
    return actionIdToName[thingName].count(actionId) ? actionIdToName[thingName][actionId] : "unknown";
}

uint8_t Beeton::getThingId(const String& name) {
    return nameToThing.count(name) ? nameToThing[name] : 0xFF;
}

uint8_t Beeton::getActionId(const String& thingName, const String& actionName) {
    if (actionNameToId.count(thingName) && actionNameToId[thingName].count(actionName)) {
        return actionNameToId[thingName][actionName];
    }
    return 0xFF;
}

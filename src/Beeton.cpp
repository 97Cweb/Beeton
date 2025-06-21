#include "Beeton.h"
#include <FS.h>
#include <SD.h>

// Initialize Beeton and register callbacks with LightThread
void Beeton::begin(LightThread& lt) {
    lightThread = &lt;
    
    // Load name→ID mappings from SD card
    loadMappings();
    
    // Register callback for all incoming UDP messages
    lightThread->registerUdpReceiveCallback([this](const String& srcIp, const bool reliable, const std::vector<uint8_t>& payload) {
		if (payload.size() < 4) {
			log_d("Beeton: Ignored short packet from %s (len=%d)", srcIp.c_str(), payload.size());
			return;
		}
		
		uint8_t thing, id, action;
		std::vector<uint8_t> content;

                // Parse the message and route it internally
		if (parsePacket(payload, thing, id, action, content)) {
			handleInternalMessage(srcIp, reliable, thing, id, action, content);
		} else {
			log_w("Beeton: Invalid packet from %s", srcIp.c_str());
		}
	});
	
    // Register callback for join events (only runs on joiner)
    lightThread->registerJoinCallback([this](const String& ip, const String& hashmac) {
		// Only announce if we’re the joiner
		if (lightThread->getRole() != Role::JOINER) return;
                
                // Package all local things into a WHO_AM_I announcement
		std::vector<uint8_t> payload;
		for (const auto& entry : localThings) {
			payload.push_back(entry.thing);
			payload.push_back(entry.id);
		}

		std::vector<uint8_t> packet = buildPacket(0xFF, 0xFF, 0xFF, payload);

		lightThread->sendUdp(ip, BEETON::RELIABLE, packet);
		Serial.println("[Joiner] Sent WHO_AM_I automatically");
    });
	
    

}

// Forward update call to LightThread instance
void Beeton::update() {
    if (lightThread) lightThread->update();
}

// Overload for sending a message without payload
bool Beeton::send(bool reliable, uint8_t thing, uint8_t id, uint8_t action){
	std::vector<uint8_t> payload;  // empty vector
    return send(reliable, thing, id, action, payload);
}

// Overload for sending a message with a single byte payload
bool Beeton::send(bool reliable, uint8_t thing, uint8_t id, uint8_t action, uint8_t payloadByte) {
    std::vector<uint8_t> payload = { payloadByte };
    return send(reliable, thing, id, action, payload);
}

// Send message to a known (thing, id) destination, if its IP is known
bool Beeton::send(bool reliable, uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    if (!lightThread) return false;
    
    if(lightThread->getRole() == Role::LEADER){
      
      uint16_t key = (thing << 8) | id;

      if (!thingIdToIp.count(key)) {
          log_w("Beeton: No IP for thing %u id %u", thing, id);
          return false;
      }

      std::vector<uint8_t> packet = buildPacket(thing, id, action, payload);
      return lightThread->sendUdp(thingIdToIp[key], reliable, packet);
    }
    else if (lightThread->getRole() == Role::JOINER){
      std::vector<uint8_t> packet = buildPacket(thing, id, action, payload);
      return lightThread->sendUdp(lightThread->getLeaderIp(), reliable, packet);
    }
}


// Register user-defined message handler callback
void Beeton::onMessage(std::function<void(uint8_t, uint8_t, uint8_t, const std::vector<uint8_t>&)> cb) {
    messageCallback = cb;
}

// Provide list of local things this device represents
void Beeton::defineThings(const std::vector<BeetonThing>& list) {
    localThings.assign(list.begin(), list.end());
}

// Load .csv mappings for things, actions, and local IDs
void Beeton::loadMappings(const char* thingsPath, const char* actionsPath, const char* definePath) {
    if (!SD.begin()) {
        Serial.println("[Beeton] SD card mount failed!");
        return;
    }

    ensureFileExists(thingsPath);
    ensureFileExists(actionsPath);
    ensureFileExists(definePath);

    loadThings(thingsPath);
    loadActions(actionsPath);
    loadDefines(definePath);
}

void Beeton::ensureFileExists(const char* path) {
    String filePath = String(path);
    int slashIndex = filePath.lastIndexOf('/');

    // Step 1: Create folder if it doesn't exist
    if (slashIndex > 0) {
        String folder = filePath.substring(0, slashIndex);
        if (!SD.exists(folder)) {
            if (SD.mkdir(folder)) {
                Serial.printf("[Beeton] Created folder: %s\n", folder.c_str());
            } else {
                Serial.printf("[Beeton] Failed to create folder: %s\n", folder.c_str());
            }
        }
    }

    // Step 2: Create file if it doesn't exist
    if (!SD.exists(path)) {
        File f = SD.open(path, FILE_WRITE);
        if (f) {
            Serial.printf("[Beeton] Created blank file: %s\n", path);
            f.close();
        } else {
            Serial.printf("[Beeton] Failed to create file: %s\n", path);
        }
    }
}


void Beeton::loadThings(const char* path) {
    File file = SD.open(path);
    if (!file) return;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        line.toLowerCase();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int comma = line.indexOf(',');
        if (comma > 0) {
            String name = line.substring(0, comma);
            uint8_t id = line.substring(comma + 1).toInt();
            nameToThing[name] = id;
            thingToName[id] = name;
        }
    }
    file.close();
}

void Beeton::loadActions(const char* path) {
    File file = SD.open(path);
    if (!file) return;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        line.toLowerCase();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int first = line.indexOf(',');
        int second = line.indexOf(',', first + 1);
        if (first > 0 && second > first) {
            String thing = line.substring(0, first);
            String action = line.substring(first + 1, second);
            uint8_t id = line.substring(second + 1).toInt();
            actionNameToId[thing][action] = id;
            actionIdToName[thing][id] = action;
            Serial.printf("Parsed action mapping: %s,%s -> %d\n", thing.c_str(), action.c_str(), id);
        }
    }
    file.close();
}

void Beeton::loadDefines(const char* path) {
    File file = SD.open(path);
    if (!file) return;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        line.toLowerCase();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int comma = line.indexOf(',');
        if (comma > 0) {
            String thing = line.substring(0, comma);
            uint8_t id = line.substring(comma + 1).toInt();
            Serial.println(line);
            if (nameToThing.count(thing)) {
                localThings.push_back({ nameToThing[thing], id });
            }
        }
    }
    file.close();
}



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

// Construct a packet from components
std::vector<uint8_t> Beeton::buildPacket(uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> out;
    out.push_back(thing);
    out.push_back(id);
    out.push_back(action);
    out.push_back(static_cast<uint8_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

// Attempt to parse a received packet
bool Beeton::parsePacket(const std::vector<uint8_t>& raw, uint8_t& thing, uint8_t& id, uint8_t& action, std::vector<uint8_t>& payload) {
    if (raw.size() < 4) return false;

    thing = raw[0];
    id = raw[1];
    action = raw[2];
    uint8_t len = raw[3];

    if (raw.size() != 4 + len) return false;
    payload.assign(raw.begin() + 4, raw.end());
    return true;
}


void Beeton::handleInternalMessage(const String& srcIp, bool reliable, uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    // Handle packets directed to 0xFF: WHO_I_AM messages from joiners
    if (thing == 0xFF && id == 0xFF && action == 0xFF) {
    
      if (lightThread && lightThread->getRole() == Role::LEADER) {
              // Map each (thing, id) to the joiner's IP
        for (size_t i = 0; i + 1 < payload.size(); i += 2) {
	        uint8_t t = payload[i];
	        uint8_t i_ = payload[i + 1];
	        uint16_t key = (t << 8) | i_;
	        thingIdToIp[key] = srcIp;
	        Serial.printf("[Leader] Mapped %02X:%d to %s\n", t, i_, srcIp.c_str());
        }
      }
      return;
    }
    
    //handle messages coming from controllers seeking things
    else if (lightThread && lightThread->getRole() == Role::LEADER){
      //look up thingid
      uint8_t t = thing;
      uint16_t key = (thing << 8) | id;
      String dest = thingIdToIp[key];
      
      
      Serial.printf("forwarding message to %02X:%d at %s\n",thing,id, dest.c_str());
      send(reliable, thing, id, action, payload);
    }


    // Call user callback only for normal messages
    if (messageCallback) {
        messageCallback(thing, id, action, payload);
    }
}


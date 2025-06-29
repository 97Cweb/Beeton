#include "Beeton.h"
// Initialize Beeton and register callbacks with LightThread
void Beeton::begin(LightThread& lt) {
    lightThread = &lt;
    
    // Load name→ID mappings from SD card
    loadMappings();
    
    if (lightThread && lightThread->getRole() == Role::LEADER) {
      Serial.begin(115200);
      usbConnected = true;
      logBeeton(BEETON_LOG_INFO,"Serial Started for Leader");
    }
    
    // Register callback for all incoming UDP messages
    lightThread->registerUdpReceiveCallback([this](const String& srcIp, const bool reliable, const std::vector<uint8_t>& payload) {
		if (payload.size() < 7) {
			logBeeton(BEETON_LOG_DEBUG, "Ignored short packet from %s (len=%d)", srcIp.c_str(), payload.size());
			return;
		}
		
		uint8_t version, id, action;
		uint16_t thing;
		std::vector<uint8_t> content;

                // Parse the message and route it internally
		if (parsePacket(payload, version, thing, id, action, content)) {
			handleInternalMessage(srcIp, reliable, version, thing, id, action, content);
		} else {
			logBeeton(BEETON_LOG_WARN,"Invalid packet from %s", srcIp.c_str());
		}
	});
	
    // Register callback for join events (only runs on joiner)
    lightThread->registerJoinCallback([this](const String& ip, const String& hashmac) {
		// Only announce if we’re the joiner
		if (lightThread->getRole() != Role::JOINER) return;
                
                // Package all local things into a WHO_AM_I announcement
		std::vector<uint8_t> payload;
		for (const auto& entry : localThings) {
			payload.push_back(entry.thing << 8);
			payload.push_back(entry.thing && 0xff);
			payload.push_back(entry.id);
		}

		std::vector<uint8_t> packet = buildPacket(0xFFFF, 0xFF, 0xFF, payload);

		lightThread->sendUdp(ip, BEETON::RELIABLE, packet);
		logBeeton(BEETON_LOG_INFO,"Joiner Sent WHO_AM_I automatically");
    });
	
    

}

// Forward update call to LightThread instance
void Beeton::update() {
    if (lightThread) lightThread->update();
    
    if (lightThread && lightThread->getRole() == Role::LEADER) {
      updateUsb();
    }
}

// Overload for sending a message without payload
bool Beeton::send(bool reliable, uint16_t thing, uint8_t id, uint8_t action){
	std::vector<uint8_t> payload;  // empty vector
    return send(reliable, thing, id, action, payload);
}

// Overload for sending a message with a single byte payload
bool Beeton::send(bool reliable, uint16_t thing, uint8_t id, uint8_t action, uint8_t payloadByte) {
    std::vector<uint8_t> payload = { payloadByte };
    return send(reliable, thing, id, action, payload);
}

// Send message to a known (thing, id) destination, if its IP is known
bool Beeton::send(bool reliable, uint16_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    if (!lightThread) return false;
    
    logBeeton(BEETON_LOG_INFO, "Send %s → thing %04X id %u action %02X payload [%s]",
          reliable ? "RELIABLE" : "UNRELIABLE",
          thing,
          id,
          action,
          formatPayload(payload).c_str());

    
    if(lightThread->getRole() == Role::LEADER){
      
      uint32_t key = (thing << 8) | id;

      if (!thingIdToIp.count(key)) {
          logBeeton(BEETON_LOG_WARN,"Beeton: No IP for thing %u id %u", thing, id);
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
void Beeton::onMessage(std::function<void(uint16_t, uint8_t, uint8_t, const std::vector<uint8_t>&)> cb) {
    messageCallback = cb;
}

// Provide list of local things this device represents
void Beeton::defineThings(const std::vector<BeetonThing>& list) {
    localThings.assign(list.begin(), list.end());
}

// Construct a packet from components
std::vector<uint8_t> Beeton::buildPacket(uint16_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    uint16_t version = 1;
    std::vector<uint8_t> out;
    uint16_t payloadLen = static_cast<uint16_t>(payload.size());
    
    
    out.push_back(version);
    out.push_back(thing >> 8);    //high byte
    out.push_back(thing & 0xff);  //low byte
    out.push_back(id);
    out.push_back(action);
    
    out.push_back(payloadLen >> 8);
    out.push_back(payloadLen && 0xff);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

// Attempt to parse a received packet
bool Beeton::parsePacket(const std::vector<uint8_t>& raw, uint8_t& version,uint16_t& thing, uint8_t& id, uint8_t& action, std::vector<uint8_t>& payload) {
    if (raw.size() < 7) return false;
    version = raw[0];
    thing = (raw[1] << 8) | raw[2];
    id = raw[3];
    action = raw[4];
    uint16_t len = (raw[5] << 8 ) | raw[6];

    if (raw.size() < 7 + len) return false;
    payload.assign(raw.begin() + 7, raw.end());
    return true;
}


void Beeton::handleInternalMessage(const String& srcIp, bool reliable, uint8_t version, uint16_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    // Handle packets directed to 0xFF: WHO_I_AM messages from joiners
    if (thing == 0xFFFF && id == 0xFF && action == 0xFF) {
    
      if (lightThread && lightThread->getRole() == Role::LEADER) {
              // Map each (thing, id) to the joiner's IP
        for (size_t i = 0; i + 1 < payload.size(); i += 3) {
	        uint16_t t = (payload[i] << 8) | payload[i+1];
	        
	        uint8_t i_ = payload[i + 2];
	        uint32_t key = (t << 8) | i_;
	        thingIdToIp[key] = srcIp;
	        logBeeton(BEETON_LOG_INFO,"[Leader] Mapped %04X:%d to %s\n", t, i_, srcIp.c_str());
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
      
      // If the sender is not the registered owner of the thing, forward the message
      if(!dest.equals(srcIp)){
        logBeeton(BEETON_LOG_INFO,"forwarding message to %04X:%d at %s\n",thing,id, dest.c_str());
        send(reliable, thing, id, action, payload);
        return;
      }
      
      // Otherwise, fall through and invoke the leader's local message callback
    }


    // Call user callback only for normal messages
    if (messageCallback) {
        messageCallback(thing, id, action, payload);
    }
}

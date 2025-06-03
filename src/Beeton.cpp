#include "Beeton.h"

void Beeton::begin(LightThread& lt) {
    lightThread = &lt;

    lightThread->registerUdpReceiveCallback([this](const String& srcIp, const std::vector<uint8_t>& payload) {
		if (payload.size() < 4) {
			log_d("Beeton: Ignored short packet from %s (len=%d)", srcIp.c_str(), payload.size());
			return;
		}
		
		uint8_t thing, id, action;
		std::vector<uint8_t> content;

		if (parsePacket(payload, thing, id, action, content)) {
			handleInternalMessage(srcIp, thing, id, action, content);
		} else {
			log_w("Beeton: Invalid packet from %s", srcIp.c_str());
		}
	});
	
	lightThread->registerJoinCallback([this](const String& ip, const String& hashmac) {
		// Only announce if we’re the joiner
		if (lightThread->getRole() != Role::JOINER) return;

		std::vector<uint8_t> payload;
		for (const auto& entry : localThings) {
			payload.push_back(entry.thing);
			payload.push_back(entry.id);
		}

		std::vector<uint8_t> packet = buildPacket(0xFF, 0xFF, 0xFF, payload);

		lightThread->sendUdp(ip, BEETON::RELIABLE, packet);
		Serial.println("[Joiner] Sent WHOAREYOU_REPLY automatically");
	});

}

void Beeton::update() {
    if (lightThread) lightThread->update();
}

bool Beeton::send(bool reliable, uint8_t thing, uint8_t id, uint8_t action){
	std::vector<uint8_t> payload;  // empty vector
    return send(reliable, thing, id, action, payload);
}

bool Beeton::send(bool reliable, uint8_t thing, uint8_t id, uint8_t action, uint8_t payloadByte) {
    std::vector<uint8_t> payload = { payloadByte };
    return send(reliable, thing, id, action, payload);
}

bool Beeton::send(bool reliable, uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    if (!lightThread) return false;
    uint16_t key = (thing << 8) | id;

    if (!thingIdToIp.count(key)) {
        log_w("Beeton: No IP for thing %u id %u", thing, id);
        return false;
    }

    std::vector<uint8_t> packet = buildPacket(thing, id, action, payload);
    return lightThread->sendUdp(thingIdToIp[key], reliable, packet);
}

void Beeton::onMessage(std::function<void(uint8_t, uint8_t, uint8_t, const std::vector<uint8_t>&)> cb) {
    messageCallback = cb;
}

void Beeton::defineThings(const std::initializer_list<BeetonThing>& list) {
    localThings.assign(list.begin(), list.end());
}

std::vector<uint8_t> Beeton::buildPacket(uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> out;
    out.push_back(thing);
    out.push_back(id);
    out.push_back(action);
    out.push_back(static_cast<uint8_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

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

void Beeton::handleInternalMessage(const String& srcIp, uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload) {
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


    // Call user callback only for normal messages
    if (messageCallback) {
        messageCallback(thing, id, action, payload);
    }
}


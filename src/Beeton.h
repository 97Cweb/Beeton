#ifndef BEETON_PROTOCOL_H
#define BEETON_PROTOCOL_H

#include <Arduino.h>
#include <LightThread.h>
#include <vector>
#include <map>
#include <functional>

namespace BEETON {
    constexpr bool RELIABLE = true;
    constexpr bool UNRELIABLE = false;
}

struct BeetonThing {
    uint8_t thing;
    uint8_t id;
};

class Beeton {
public:
    void begin(LightThread& lt);
    void update();

    // Simple send API
	bool send(bool reliable, uint8_t thing, uint8_t id, uint8_t action);
    bool send(bool reliable, uint8_t thing, uint8_t id, uint8_t action, uint8_t payloadByte);
    bool send(bool reliable, uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload);

    // Message receive handler
    void onMessage(std::function<void(uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload)> cb);
	void defineThings(const std::initializer_list<BeetonThing>& list);

private:
    LightThread* lightThread = nullptr;
	std::map<uint16_t, String> thingIdToIp;  // thing<<8 | id → IP
    std::vector<BeetonThing> localThings;

    std::function<void(uint8_t, uint8_t, uint8_t, const std::vector<uint8_t>&)> messageCallback = nullptr;


    std::vector<uint8_t> buildPacket(uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload);
    bool parsePacket(const std::vector<uint8_t>& raw, uint8_t& thing, uint8_t& id, uint8_t& action, std::vector<uint8_t>& payload);
	void handleInternalMessage(const String& srcIp, uint8_t thing, uint8_t id, uint8_t action, const std::vector<uint8_t>& payload);
};

#endif // BEETON_PROTOCOL_H

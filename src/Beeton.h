#ifndef BEETON_H
#define BEETON_H

#include <Arduino.h>
#include <LightThread.h>
#include <map>


struct Packet {
    uint8_t reliable;              // 1 byte
    uint16_t thing;                // 2 bytes (enum)
    uint16_t id;                   // 2 bytes
    uint8_t priority;             // 1 byte
    uint32_t sequence;           // 4 bytes
    uint8_t action;               // 1 byte
    uint8_t dataLength;           // 1 byte
    uint8_t* data;                // pointer to payload
};


class Beeton {
public:
    Beeton(LightThread& lightThread);
    
    // Packet handling
    bool sendPacket(const Packet& packet);

    // Utility methods
	using PacketHandler = void(*)(const Packet&, const String&);
	void update(PacketHandler handler = nullptr);



private:
    LightThread& lightThread; // Reference to LightThread for communication
	PacketHandler handler = nullptr;
    bool parsePacket(const uint8_t* rawData, size_t length, Packet& packet);
    bool constructPacket(const Packet& packet, uint8_t* rawData, size_t& length);
	
	struct DeviceKey {
		uint16_t thing;
		uint16_t id;
		bool operator<(const DeviceKey& other) const {
			return std::tie(thing, id) < std::tie(other.thing, other.id);
		}
	};

	std::map<DeviceKey, String> joinerTable;

	
};

#endif
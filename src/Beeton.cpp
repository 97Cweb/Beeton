#include "Beeton.h"

Beeton::Beeton(LightThread& lightThread) : lightThread(lightThread) {
	lightThread.setNormalPacketHandler([this](const uint8_t* data, size_t len, const String& ip) {
    Packet packet;
    if (this->parsePacket(data, len, packet)) {
        DeviceKey key{packet.thing, packet.id};
        if (this->joinerTable.count(key) == 0) {
            this->joinerTable[key] = ip;
            Serial.printf("[JOIN] Device (%u:%u) registered from %s\n", packet.thing, packet.id, ip.c_str());
        }

        if (this->handler) {
            this->handler(packet, ip);
        }

        delete[] packet.data;
    }
});

	
}

bool Beeton::sendPacket(const Packet& packet) {
    uint8_t rawData[256]; // Adjust size as needed
    size_t length = 0;

    if (!constructPacket(packet, rawData, length)) {
        Serial.println("Failed to construct packet.");
        return false;
    }

    return lightThread.sendData(rawData, length, static_cast<LightThread::AckType>(packet.reliable));
}


bool Beeton::constructPacket(const Packet& packet, uint8_t* rawData, size_t& length) {
    length = 0;

    rawData[length++] = packet.thing >> 8;
    rawData[length++] = packet.thing & 0xFF;
    rawData[length++] = packet.id >> 8;
    rawData[length++] = packet.id & 0xFF;
    rawData[length++] = packet.priority;

    rawData[length++] = (packet.sequence >> 24) & 0xFF;
    rawData[length++] = (packet.sequence >> 16) & 0xFF;
    rawData[length++] = (packet.sequence >> 8) & 0xFF;
    rawData[length++] = packet.sequence & 0xFF;

    rawData[length++] = packet.action;
    rawData[length++] = packet.dataLength;

    for (uint8_t i = 0; i < packet.dataLength; ++i) {
        rawData[length++] = packet.data[i];
    }

    return true;
}


bool Beeton::parsePacket(const uint8_t* rawData, size_t length, Packet& packet) {
    if (length < 12) { // Minimum size with no data
        Serial.println("Invalid packet length.");
        return false;
    }

    size_t index = 0;

    packet.thing = (rawData[index++] << 8) | rawData[index++];
    packet.id = (rawData[index++] << 8) | rawData[index++];
    packet.priority = rawData[index++];

    packet.sequence = (rawData[index++] << 24) | (rawData[index++] << 16) |
                      (rawData[index++] << 8) | rawData[index++];

    packet.action = rawData[index++];
    packet.dataLength = rawData[index++];

    if (length < index + packet.dataLength) {
        Serial.println("Incomplete data field.");
        return false;
    }

    packet.data = new uint8_t[packet.dataLength];
    for (uint8_t i = 0; i < packet.dataLength; ++i) {
        packet.data[i] = rawData[index++];
    }

    return true;
}

void Beeton::update(PacketHandler userHandler) {
    this->handler = userHandler;
    lightThread.update(); // Let LightThread drive pairing, heartbeats, etc
}




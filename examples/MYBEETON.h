#ifndef MYBEETON_H
#define MYBEETON_H

// Define your device categories here.
// Each category should have a unique 8-bit value.
namespace MYTHINGS {
    constexpr uint8_t MOTOR  = 0x01;
    constexpr uint8_t SENSOR = 0x02;
    constexpr uint8_t LED    = 0x03;
    // Add more categories as needed
}

// Define the actions your devices can perform.
// These values must also be unique.
namespace MYACTIONS {
    constexpr uint8_t SETSPEED = 0x10;   // e.g., for motors
    constexpr uint8_t TURNON   = 0x11;   // e.g., for LEDs or relays
    constexpr uint8_t TURNOFF  = 0x12;
    constexpr uint8_t SETVALUE = 0x13;   // e.g., generic numeric input
    // Add more actions as needed
}

#endif // MYBEETON_H

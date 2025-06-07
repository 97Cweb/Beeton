# Beeton

**Beeton** is a custom protocol layer designed to run over mesh networks like OpenThread or future backends (e.g., WiFi). It targets model railroading and other real-time control systems, offering reliable, low-latency messaging.

## Features

- Layered on top of LightThread (OpenThread) for ESP32-C6
- Modular design — future support for WiFi and other transport layers
- Real-time mesh messaging with acknowledgments
- Suitable for model train control and layout automation

## Dependencies

This library depends on:

- [LightThread](https://github.com/97Cweb/LightThread)

Make sure LightThread is installed in your Arduino libraries folder or symlinked there.

## Installation

```bash
git clone https://github.com/YourUsername/Beeton.git
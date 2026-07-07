# CONCEPT

## Goal
Build a PoC with the ESP32-C6 containing a little webserver discoverable over mDNS "mm.local". The SSID + password will be hardcoded.

## Business Cases
1. **PoC for ESP32-C6 networking capabilities**: Demonstrate the ease of setting up a discoverable web server on the new ESP32-C6 platform.
2. **Base template for discoverable IoT devices**: Provide a reusable foundation for future IoT projects that require easy local discovery and a web-based UI.

## Use Cases
1. **Automatic Wi-Fi connection**: The device connects to a pre-configured Wi-Fi network upon boot.
2. **Local network discovery via mDNS**: Users can find the device by navigating to "mm.local" in their browser, without knowing the IP address.
3. **Web interface access**: Users can access a simple web page hosted on the device to view status or perform basic actions.

## High-Level Architecture
The system consists of three main functional components:
- **Network Manager**: Responsible for initializing the Wi-Fi stack and maintaining the connection to the access point using hardcoded credentials.
- **Discovery Service**: Implements the mDNS protocol to register the "mm.local" hostname and respond to discovery requests on the local network.
- **Web Service**: Runs a lightweight HTTP server to handle incoming requests and serve the user interface or basic API responses.

## Major Choices and Alternatives

### 1. Development Environment
- **Alternative A: ESP-IDF (Native)**: The official development framework for ESP32 chips. Provides full control over hardware and latest features. **(Chosen)**
- **Alternative B: Arduino Core for ESP32**: Offers a higher-level abstraction and wide library support, but may lack some advanced ESP32-C6 specific features or optimizations.
- **Alternative C: MicroPython**: Enables rapid prototyping with Python, but has higher memory overhead and less fine-grained control over low-level networking.

### 2. Web Server Framework
- **Alternative A: ESP-IDF HTTP Server**: Built-in component of ESP-IDF, lightweight and well-integrated with the underlying OS (FreeRTOS). **(Chosen)**
- **Alternative B: Mongoose Networking Library**: Feature-rich and multi-platform, but adds external dependency and complexity.
- **Alternative C: Libwebsockets**: Powerful for WebSocket-heavy applications, but might be overkill for a simple PoC web server.

### 3. Configuration Method
- **Alternative A: Hardcoded in Source**: Simplest approach for a PoC as per the initial goal. Easy to implement and test. **(Chosen)**
- **Alternative B: NVS (Non-Volatile Storage)**: Allows storing credentials that can be updated without re-flashing, providing more flexibility.
- **Alternative C: Configuration file in SPIFFS/LittleFS**: Useful for more complex configurations, but adds overhead of a filesystem.

## Discarded Alternatives Summary
- **Arduino Core & MicroPython**: Discarded in favor of ESP-IDF to leverage native capabilities and maintain a professional-grade firmware structure.
- **Mongoose & Libwebsockets**: Discarded to minimize dependencies and utilize the standard, well-supported ESP-IDF components.
- **NVS & Filesystem Config**: Discarded for the PoC stage to keep the implementation focused on the core goal of connectivity and discovery with minimal boilerplate.

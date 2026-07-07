# DESIGN

## Introduction
This document details the technical design for the ESP32-C6 Web Server PoC. It translates the high-level goals from `CONCEPT.md` into specific implementation choices and architectural details.

## Technical Architecture
The system follows a modular architecture within the ESP-IDF environment, leveraging FreeRTOS for task management and event handling.

![Top Architecture](./TOP_ARCHITECTURE.puml)

### Component Details

#### 1. Network Manager
- **Responsibility**: Manages the Wi-Fi station interface.
- **Implementation**: Uses `esp_wifi` component.
- **Interfaces**:
  - `wifi_init()`: Initializes the TCP/IP stack and Wi-Fi driver.
  - `wifi_connect()`: Starts the connection process using hardcoded credentials.
  - `WIFI_EVENT`: Handles connection, disconnection, and IP acquisition events.

#### 2. Discovery Service
- **Responsibility**: Handles mDNS registration for "mm.local".
- **Implementation**: Uses `mdns` component from ESP-IDF.
- **Interfaces**:
  - `mdns_init()`: Initializes the mDNS service.
  - `mdns_register_service()`: Registers the HTTP service and the "mm.local" hostname.

#### 3. Web Service
- **Responsibility**: Serves HTTP requests.
- **Implementation**: Uses `esp_http_server` component.
- **Interfaces**:
  - `http_server_start()`: Configures and starts the HTTP server.
  - `root_get_handler()`: Callback function for handling GET requests to "/".

## Major Choices and Alternatives

### 1. Web Content Storage
- **Alternative A: Inlined Strings**: Hardcode the HTML/CSS/JS directly in the C source code as strings. Simple for PoC but harder to maintain. **(Chosen)**
- **Alternative B: SPIFFS (SPI Flash File System)**: Store files in a flash partition. Easier to manage larger sites, but adds filesystem overhead and build complexity.
- **Alternative C: Binary Embedding (CMake `target_add_binary_data`)**: Embed files into the binary during compilation. Cleaner than inlining strings, but still requires flash space and specific CMake configuration.

### 2. Wi-Fi Reconnection Strategy
- **Alternative A: Simple Retry Loop**: Try to reconnect in a tight loop. Easy to implement but can be inefficient or cause issues if the AP is down for long.
- **Alternative B: Exponential Backoff**: Increase wait time between retries. More robust for network stability.
- **Alternative C: Event-Driven Reconnect**: Trigger reconnection based on `WIFI_EVENT_STA_DISCONNECTED` events with a fixed delay. **(Chosen)**

### 3. mDNS Service Advertising
- **Alternative A: Default 'http' Service**: Use `_http._tcp` service type. Standard for web servers and widely recognized by browsers/discovery tools. **(Chosen)**
- **Alternative B: Custom Service Type**: Define a unique service type (e.g., `_mm._tcp`). Useful for specific app discovery but requires custom client logic.
- **Alternative C: Multiple Service Types**: Advertise both 'http' and custom types. Provides maximum flexibility but consumes more resources.

## Discarded Alternatives Summary
- **SPIFFS & Binary Embedding**: Discarded to keep the PoC simple and minimize dependencies on filesystem or complex build steps.
- **Simple Retry & Exponential Backoff**: Event-driven reconnection was chosen as it aligns best with the ESP-IDF event loop pattern, providing a clean and efficient mechanism.
- **Custom & Multiple Service Types**: Discarded to maintain standard compatibility for the web server discovery using standard tools.

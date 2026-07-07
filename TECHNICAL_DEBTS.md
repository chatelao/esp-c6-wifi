# TECHNICAL DEBTS

This file logs technical debts identified during the development of the ESP32-C6 Web Server PoC.

| Debt | Description | Potential Impact | Priority |
|---|---|---|---|
| Hardcoded Wi-Fi Credentials | SSID and password are hardcoded in the source code. | Security risk; lack of flexibility for different environments. | Medium |
| Lack of Automated Tests | No CI/CD tests or local unit/integration tests are currently implemented. | Risk of regressions; harder to verify new features. | High |
| Inlined HTML Strings | Web content is hardcoded as C strings instead of using a filesystem or binary embedding. | Difficult to maintain and scale the web UI. | Low |
| Missing Security (HTTP) | The web server uses plain HTTP without encryption (TLS). | Data transmitted is not secure on the local network. | Medium |
| Lack of Error Handling | Basic implementation might not robustly handle all network or server error states. | Device stability and user experience issues. | Low |

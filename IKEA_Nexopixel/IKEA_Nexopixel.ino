#include <Arduino.h>
#include <Matter.h>
#include <Preferences.h>
#include <FastLED.h>

// Hardware-Definitionen
#define NUM_LEDS              100
#define DATA_PIN               18   // GPIO18 (D10) auf dem XIAO ESP32-C6
#define BUTTON_PIN              9   // GPIO9 (BOOT-Button) für Decommissioning / Toggle
#define STATUS_LED_PIN         15   // GPIO15: Gelbe Onboard-LED beim XIAO ESP32-C6 (Active-Low)

// Timing-Konstanten
#define NVS_SAVE_DELAY_MS    3000   // Schützt NVS-Flash vor Wear-Out bei Farbverläufen
#define BLINK_FAST_MS         150   // Blink-Intervall (ms), wenn NeoPixel AN (ca. 3.3 Hz)
#define BLINK_SLOW_MS        1500   // Blink-Intervall (ms), wenn NeoPixel AUS (ca. 0.33 Hz)

CRGB leds[NUM_LEDS];

// MatterColorLight (Device Type 0x010D für RGB-Strips)
MatterColorLight ColorLight;
Preferences matterPref;

const char *onOffPrefKey = "OnOff";
const char *hsvColorPrefKey = "HSV";

// Synchronisations-Primitive für Thread-Sicherheit zwischen Matter-Task und Arduino-Loop
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool led_update_pending = false;
volatile bool nvs_save_pending = false;
volatile uint32_t last_state_change_ms = 0;

bool current_state = false;
espHsvColor_t current_hsv = {0, 254, 128}; // Matter-Normbereich 0..254

// Physisches Update der WS2812B LEDs (läuft im Arduino-Loop)
void renderLEDs(bool state, espHsvColor_t hsv) {
    if (state && hsv.v > 0) {
        // Skalierung vom Matter-Bereich (0..254) auf FastLED-Bereich (0..255)
        uint8_t h = map(hsv.h, 0, 254, 0, 255);
        uint8_t s = map(hsv.s, 0, 254, 0, 255);
        uint8_t v = map(hsv.v, 0, 254, 0, 255);
        
        fill_solid(leds, NUM_LEDS, CHSV(h, s, v));
    } else {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
    FastLED.show();
}

// Offizielle Callback-Signatur für MatterColorLight
bool setLightState(bool state, espHsvColor_t colorHSV) {
    portENTER_CRITICAL(&stateMux);
    current_state = state;
    current_hsv = colorHSV;
    led_update_pending = true;
    nvs_save_pending = true;
    last_state_change_ms = millis();
    portEXIT_CRITICAL(&stateMux);
    
    return true; // Quittiert dem IKEA DIRIGERA den Empfang (ESP_OK)
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Onboard-LED initialisieren (Active-Low: HIGH = initial ausgeschaltet)
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);

    // 1. FastLED mit Strombegrenzung (Brownout-Schutz für 100 LEDs)
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 1500);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();

    // 2. Letzten Zustand und letzte Farbe aus dem NVS-Flash laden
    matterPref.begin("MatterPrefs", false);
    bool lastOnOffState = matterPref.getBool(onOffPrefKey, false);
    uint32_t prefHsv = matterPref.getUInt(hsvColorPrefKey, (0UL << 16) | (254UL << 8) | 128UL);
    espHsvColor_t lastHsvColor = {
        uint8_t((prefHsv >> 16) & 0xFF), 
        uint8_t((prefHsv >> 8) & 0xFF), 
        uint8_t(prefHsv & 0xFF)
    };

    // Lokale Statusvariablen mit NVS-Werten synchronisieren (wichtig für Boot-Zustand)
    current_state = lastOnOffState;
    current_hsv = lastHsvColor;
    led_update_pending = true;

    // 3. MatterColorLight initialisieren und Callback binden
    ColorLight.begin(lastOnOffState, lastHsvColor);
    ColorLight.onChange(setLightState);

    // 4. Matter-Stack starten
    Matter.begin();

    if (!Matter.isDeviceCommissioned()) {
        Serial.println("=== Bereit für IKEA DIRIGERA Pairing ===");
        Serial.printf("Manueller Pairing-Code: %s\n", Matter.getManualPairingCode().c_str());
        Serial.printf("QR-Code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
    } else {
        // Synchronisiert die Hardware direkt nach dem Booten mit dem gespeicherten NVS-Zustand
        ColorLight.updateAccessory();
    }
}

uint32_t btn_timer = 0;
bool btn_active = false;
bool decommission_triggered = false;

uint32_t status_led_timer = 0;
bool status_led_phys_state = true; // HIGH = LED aus (Active-Low)

void loop() {
    uint32_t now = millis();

    // 1. Thread-sicherer Snapshot des aktuellen Zustands für Loop-Operationen
    portENTER_CRITICAL(&stateMux);
    bool active_state = (current_state && current_hsv.v > 0);
    bool do_led_update = led_update_pending;
    bool local_state = current_state;
    espHsvColor_t local_hsv = current_hsv;
    led_update_pending = false;
    portEXIT_CRITICAL(&stateMux);

    // 2. Entkoppelte Ausführung von FastLED
    if (do_led_update) {
        renderLEDs(local_state, local_hsv);
        Serial.printf("[IKEA Thread Update] Power: %s | HSV(0-254): (%u, %u, %u)\n",
                      local_state ? "ON" : "OFF", local_hsv.h, local_hsv.s, local_hsv.v);
    }

    // 3. Nicht-blockierender Heartbeat für die XIAO Onboard-LED (GPIO 15)
    uint32_t blink_interval = active_state ? BLINK_FAST_MS : BLINK_SLOW_MS;
    if (now - status_led_timer >= blink_interval) {
        status_led_timer = now;
        status_led_phys_state = !status_led_phys_state;
        digitalWrite(STATUS_LED_PIN, status_led_phys_state ? HIGH : LOW);
    }

    // 4. Verzögertes NVS-Speichern (Debouncing gegen Flash-Wear-Out)
    if (nvs_save_pending && (now - last_state_change_ms > NVS_SAVE_DELAY_MS)) {
        portENTER_CRITICAL(&stateMux);
        bool save_state = current_state;
        espHsvColor_t save_hsv = current_hsv;
        nvs_save_pending = false;
        portEXIT_CRITICAL(&stateMux);

        uint32_t packed_hsv = (uint32_t(save_hsv.h) << 16) | (uint32_t(save_hsv.s) << 8) | uint32_t(save_hsv.v);
        matterPref.putBool(onOffPrefKey, save_state);
        matterPref.putUInt(hsvColorPrefKey, packed_hsv);
        Serial.println("[NVS] Zustand verschleißfrei im Flash gesichert.");
    }

    // 5. Button-Logik: Kurzer Klick = Toggle, > 5 Sekunden Halten = Decommissioning
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!btn_active) {
            btn_timer = now;
            btn_active = true;
            decommission_triggered = false;
        } else if (!decommission_triggered && (now - btn_timer > 5000)) {
            decommission_triggered = true;
            Serial.println("Decommissioning! Lösche Matter-Fabric...");
            ColorLight = false;
            Matter.decommission();
            
        }
    } else if (btn_active) {
        if (!decommission_triggered && (now - btn_timer < 1000)) {
            ColorLight.toggle(); // Schaltet um und informiert den IKEA DIRIGERA per Thread
        }
        btn_active = false;
    }

    delay(15); // ~66 Hz Refresh-Rate
}

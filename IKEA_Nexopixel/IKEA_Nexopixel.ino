#include <Arduino.h>
#include <Matter.h>
#include <Preferences.h>
#include <FastLED.h>

// Hardware-Definitionen
#define NUM_LEDS    100
#define DATA_PIN    18  // GPIO18 auf dem ESP32-C6
#define BUTTON_PIN  9   // GPIO9 (BOOT-Button) für Decommissioning / Toggle

CRGB leds[NUM_LEDS];

// MatterColorLight (Device Type 0x010D ohne Mireds-Konflikt, exakt für RGB-Strips)
MatterColorLight ColorLight;
Preferences matterPref;

const char *onOffPrefKey = "OnOff";
const char *hsvColorPrefKey = "HSV";

// Thread-sichere Zustandsvariablen zur Entkopplung von Radio-Callback und FastLED-Timing
volatile bool led_update_pending = false;
bool current_state = false;
espHsvColor_t current_hsv = {0, 254, 128}; // Alle Werte im Matter-Normbereich 0..254!

// Physisches Update der WS2812B LEDs (läuft im Arduino-Loop, blockiert NICHT den Thread-Stack)
void renderLEDs() {
    if (current_state && current_hsv.v > 0) {
        // Exakte Skalierung vom Matter-Bereich (0..254) auf FastLED-Bereich (0..255)
        uint8_t h = map(current_hsv.h, 0, 254, 0, 255);
        uint8_t s = map(current_hsv.s, 0, 254, 0, 255);
        uint8_t v = map(current_hsv.v, 0, 254, 0, 255);
        
        fill_solid(leds, NUM_LEDS, CHSV(h, s, v));
    } else {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
    FastLED.show();
}

// Offizielle Callback-Signatur laut Espressif Docs für MatterColorLight:
// bool onChangeCallback(bool newState, espHsvColor_t newColor)
bool setLightState(bool state, espHsvColor_t colorHSV) {
    current_state = state;
    current_hsv = colorHSV;
    led_update_pending = true; // Signalisiert dem loop(), die LEDs zu aktualisieren

    // Zustand im NVS-Flash sichern für Stromausfall-Sicherheit
    matterPref.putBool(onOffPrefKey, state);
    matterPref.putUInt(hsvColorPrefKey, (uint32_t)(colorHSV.h << 16 | colorHSV.s << 8 | colorHSV.v));
    
    return true; // Quittiert dem IKEA DIRIGERA den Empfang (ESP_OK)
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // 1. FastLED mit Strombegrenzung (Brownout-Schutz für 100 LEDs)
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 1500);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();

    // 2. Letzten Zustand und letzte Farbe aus dem NVS-Flash laden
    matterPref.begin("MatterPrefs", false);
    bool lastOnOffState = matterPref.getBool(onOffPrefKey, false);
    uint32_t prefHsv = matterPref.getUInt(hsvColorPrefKey, (0 << 16 | 254 << 8 | 128));
    espHsvColor_t lastHsvColor = {
        uint8_t(prefHsv >> 16), 
        uint8_t(prefHsv >> 8), 
        uint8_t(prefHsv)
    };

    // 3. MatterColorLight initialisieren und Callback binden
    ColorLight.begin(lastOnOffState, lastHsvColor);
    ColorLight.onChange(setLightState);

    // 4. Matter-Stack starten (Tools -> Matter Network -> Thread muss in der IDE aktiv sein!)
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

void loop() {
    // 1. Entkoppelte Ausführung von FastLED, sobald die IKEA App neue Farbwerte gesendet hat
    if (led_update_pending) {
        led_update_pending = false;
        renderLEDs();
        Serial.printf("[IKEA Thread Update] Power: %s | HSV(0-254): (%u, %u, %u)\n",
                      current_state ? "ON" : "OFF", current_hsv.h, current_hsv.s, current_hsv.v);
    }

    // 2. Button-Logik: Kurzer Klick = An/Aus (meldet Zustand live an IKEA App zurück!),
    //    5 Sekunden Halten = Decommissioning (Werksreset)
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (!btn_active) {
            btn_timer = millis();
            btn_active = true;
        } else if (millis() - btn_timer > 5000) {
            Serial.println("Decommissioning! Lösche Matter-Fabric...");
            ColorLight = false;
            Matter.decommission();
        }
    } else if (btn_active) {
        if (millis() - btn_timer < 1000) {
            ColorLight.toggle(); // Schaltet um und informiert den IKEA DIRIGERA per Thread!
        }
        btn_active = false;
    }

    delay(15); // ~66 Hz maximale Refresh-Rate für butterweiche Farbwechsel
}

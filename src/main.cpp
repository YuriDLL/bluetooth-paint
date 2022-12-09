#include "NimBLEDevice.h"
#include "led-control.h"

enum
{
    ZONE_EYE,
    ZONE_HAIR,
    ZONE_AMULET,
    ZONE_BACKGROUND,
    ZONE_COUNT
};

static LEDZone *zones[ZONE_COUNT];

static const NimBLEUUID zones_uuids[ZONE_COUNT] = {
    NimBLEUUID("c68d68c0-4295-4f89-b48d-9a2a2979121b"), // ZONE_EYE
    NimBLEUUID("a3e220db-7b8e-4ccb-89b9-4ce4fc7d5f1a"), // ZONE_HAIR
    NimBLEUUID("44259e34-4b1d-4ce6-92e7-b5830bfbb16b"), // ZONE_AMULET
    NimBLEUUID("f9b5b172-7dab-4882-9dea-efcfa7c5c89f") // ZONE_BACKGROUND
};

static const NimBLEUUID effect_char = NimBLEUUID("2bfe8422-fce0-4c99-b503-48106286d7a2");
static const NimBLEUUID period_char = NimBLEUUID("cd51666b-364a-45e8-be5c-bae24b4e6a8c");
static const NimBLEUUID chance_per_sec_char = NimBLEUUID("010666bd-da27-4a45-8dd4-dcefef51a4ec");
static const NimBLEUUID color1_char = NimBLEUUID("7fbe55ad-2ab9-47e4-961e-de6d1afb81b2");
static const NimBLEUUID color2_char = NimBLEUUID("626e6dce-6a0a-49e6-a704-1a21d8362fcb");

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        LEDZone *zone = get_zone(pCharacteristic->getService()->getUUID());
        effect_settings_t settings = zone->get_effect_settings();

        Serial.print(": onWrite()");
        if (pCharacteristic->getUUID().equals(effect_char))
        {
            settings.type = pCharacteristic->getValue().data()[0];
        }
        else if (pCharacteristic->getUUID().equals(period_char))
        {
            settings.period = pCharacteristic->getValue().data()[0];
        }
        else if (pCharacteristic->getUUID().equals(color1_char))
        {
            uint8_t *color_array = (uint8_t *)pCharacteristic->getValue().data();
            settings.color1 = *color_array & 0xFFFFFF;
        }
        zone->set_effect(settings);
    };

    LEDZone *get_zone(NimBLEUUID service_uuid)
    {
        uint8_t zone_index = ZONE_EYE;
        for (int i=0; i < ZONE_COUNT; i++)
        {
            if (service_uuid.equals(zones_uuids[i]))
            {
                zone_index = i;
            }
        }

        return zones[zone_index];
    }
};

static CharacteristicCallbacks chrCallbacks;

void setup()
{
    Serial.begin(115200);
    Serial.print("Start\n");

    zones[ZONE_EYE] = new LEDZone();
    zones[ZONE_EYE]->led_numbers = {44, 51};
    zones[ZONE_EYE]->set_effect((effect_settings_t){.type = EFFECT_CONST, .period = 1.0f, .color1 = 0xff0000});

    zones[ZONE_HAIR] = new LEDZone();
    zones[ZONE_HAIR]->led_numbers = {25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 40, 41, 42, 43, 45, 46, 47, 48, 49, 50, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 78, 79, 80, 81};
    zones[ZONE_HAIR]->set_effect((effect_settings_t){.type = EFFECT_CONST, .color1 = 0x9776e3});

    zones[ZONE_AMULET] = new LEDZone();
    zones[ZONE_AMULET]->led_numbers = {74, 75, 76, 77};
    zones[ZONE_AMULET]->set_effect((effect_settings_t){.type = EFFECT_CONST, .period = 3.0f, .color1 = 0x00ff00});

    zones[ZONE_BACKGROUND] = new LEDZone();
    zones[ZONE_BACKGROUND]->led_numbers = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 38, 39, 65, 66, 67, 68, 69, 70, 71, 72, 73,
                                           82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106};
    zones[ZONE_BACKGROUND]->set_effect((effect_settings_t){.type = EFFECT_GRADIENT, .period = 2.0f, .chance_per_sec = 10, .color1 = 0x1d8a04});

    NimBLEDevice::init("Paint");

    NimBLEServer *pServer = NimBLEDevice::createServer();
    for (uint8_t i = 0; i < 4; i++)
    {
        NimBLEService *pService = pServer->createService(zones_uuids[i]);
        NimBLECharacteristic *pCharacteristic;
        pCharacteristic = pService->createCharacteristic(effect_char, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 1);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pCharacteristic = pService->createCharacteristic(period_char, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 2);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pCharacteristic = pService->createCharacteristic(chance_per_sec_char, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 1);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pCharacteristic = pService->createCharacteristic(color1_char, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 3);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pCharacteristic = pService->createCharacteristic(color2_char, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 3);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pService->start();
    }

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->start();
}

// This is the Arduino main loop function.
void loop()
{
    for (int i = 0; i < ZONE_COUNT; i++)
    {
        if (zones[i] != NULL)
        {
            zones[i]->loop();
        }
    }
    delay(100);
}
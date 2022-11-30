#include "NimBLEDevice.h"
#include <SPI.h>
#include "led-control.h"

enum
{
    ZONE_EYE,
    ZONE_HAIR,
    ZONE_AMULET,
    ZONE_BACKGROUND,
    ZONE_COUNT
};

static LEDZone* zones[ZONE_COUNT];

static const NimBLEUUID eye_service = NimBLEUUID("c68d68c0-4295-4f89-b48d-9a2a2979121b");
static const NimBLEUUID hair_service = NimBLEUUID("a3e220db-7b8e-4ccb-89b9-4ce4fc7d5f1a");
static const NimBLEUUID amulet_service = NimBLEUUID("44259e34-4b1d-4ce6-92e7-b5830bfbb16b");
static const NimBLEUUID background_service = NimBLEUUID("f9b5b172-7dab-4882-9dea-efcfa7c5c89f");

static const NimBLEUUID effect_char = NimBLEUUID("2bfe8422-fce0-4c99-b503-48106286d7a2");
static const NimBLEUUID period_char = NimBLEUUID("cd51666b-364a-45e8-be5c-bae24b4e6a8c");
static const NimBLEUUID color1_char = NimBLEUUID("7fbe55ad-2ab9-47e4-961e-de6d1afb81b2");
static const NimBLEUUID color2_char = NimBLEUUID("626e6dce-6a0a-49e6-a704-1a21d8362fcb");

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic)
    {
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onRead()");
        Serial.println(pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        LEDZone* zone = get_zone(pCharacteristic->getService()->getUUID());
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
            uint8_t *color_array = (uint8_t*) pCharacteristic->getValue().data();
            settings.color1 = *color_array & 0xFFFFFF;
        }
        zone->set_effect(settings);

    };

    LEDZone* get_zone(NimBLEUUID service_uuid)
    {
        uint8_t zone_index = ZONE_EYE;
        if (service_uuid.equals(eye_service))
        {
            zone_index = ZONE_EYE;
        }
        else if (service_uuid.equals(hair_service))
        {
            zone_index = ZONE_HAIR;
        }
        else if (service_uuid.equals(amulet_service))
        {
            zone_index = ZONE_AMULET;
        }
        else if (service_uuid.equals(background_service))
        {
            zone_index = ZONE_BACKGROUND;
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
    zones[ZONE_EYE]->led_numbers = {0 , 2, 4};
    zones[ZONE_EYE]->set_effect((effect_settings_t){.type=EFFECT_CONST, .color1=0x002f00});

    zones[ZONE_HAIR] = new LEDZone();
    zones[ZONE_HAIR]->led_numbers = {1, 3, 5};
    zones[ZONE_HAIR]->set_effect((effect_settings_t){.type=EFFECT_BLINK, .period = 1.0f, .color1=0x2f0000});

    zones[ZONE_AMULET] = new LEDZone();
    zones[ZONE_AMULET]->led_numbers = {6, 7};
    zones[ZONE_AMULET]->set_effect((effect_settings_t){.type=EFFECT_GRADIENT, .period = 0.03f, .color1=0x2f0000, .color2=0x2f2f2f});

    NimBLEDevice::init("Paint");

    NimBLEServer *pServer = NimBLEDevice::createServer();
    NimBLEService *pService[4];
    pService[0] = pServer->createService(eye_service);
    pService[1] = pServer->createService(hair_service);
    pService[2] = pServer->createService(amulet_service);
    pService[3] = pServer->createService(background_service);

    for (uint8_t i = 0; i < 4; i++)
    {
        NimBLECharacteristic *pCharacteristic;
        pCharacteristic = pService[i]->createCharacteristic(effect_char, NIMBLE_BLE_ATT_CLT_READ | NIMBLE_BLE_ATT_CLT_WRITE, 1);
        pCharacteristic->setValue(0);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pCharacteristic = pService[i]->createCharacteristic(period_char, NIMBLE_BLE_ATT_CLT_READ | NIMBLE_BLE_ATT_CLT_WRITE, 1);
        pCharacteristic->setValue(0);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pCharacteristic = pService[i]->createCharacteristic(color1_char, NIMBLE_BLE_ATT_CLT_READ | NIMBLE_BLE_ATT_CLT_WRITE, 3);
        pCharacteristic->setValue(0);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pCharacteristic = pService[i]->createCharacteristic(color2_char, NIMBLE_BLE_ATT_CLT_READ | NIMBLE_BLE_ATT_CLT_WRITE, 3);
        pCharacteristic->setValue(0);
        pCharacteristic->setCallbacks(&chrCallbacks);

        pService[i]->start();
    }

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(eye_service);
    pAdvertising->start();
}

// This is the Arduino main loop function.
void loop()
{
    for (int i=0; i< ZONE_COUNT; i++)
    {
        zones[i]->loop();
    }
    delay(100);
}
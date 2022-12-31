#include "NimBLEDevice.h"
#include "led-control.h"
#include <Preferences.h>

Preferences prefs;

enum
{
    ZONE_EYE,
    ZONE_HAIR,
    ZONE_AMULET,
    ZONE_BACKGROUND,
    ZONE_COUNT
};

enum
{
    CHAR_EFFECT,
    CHAR_PERIOD,
    CHAR_CHANCE,
    CHAR_COLOR1,
    CHAR_COLOR2,
    CHAR_COUNT
};

static LEDZone *zones[ZONE_COUNT];

static const NimBLEUUID zones_uuids[ZONE_COUNT] = {
    NimBLEUUID("c68d68c0-4295-4f89-b48d-9a2a2979121b"), // ZONE_EYE
    NimBLEUUID("a3e220db-7b8e-4ccb-89b9-4ce4fc7d5f1a"), // ZONE_HAIR
    NimBLEUUID("44259e34-4b1d-4ce6-92e7-b5830bfbb16b"), // ZONE_AMULET
    NimBLEUUID("f9b5b172-7dab-4882-9dea-efcfa7c5c89f")  // ZONE_BACKGROUND
};

static const NimBLEUUID char_uuids[CHAR_COUNT] = {
    NimBLEUUID("2bfe8422-fce0-4c99-b503-48106286d7a2"), // CHAR_EFFECT
    NimBLEUUID("cd51666b-364a-45e8-be5c-bae24b4e6a8c"), // CHAR_PERIOD
    NimBLEUUID("010666bd-da27-4a45-8dd4-dcefef51a4ec"), // CHAR_CHANCE
    NimBLEUUID("7fbe55ad-2ab9-47e4-961e-de6d1afb81b2"), // CHAR_COLOR1
    NimBLEUUID("626e6dce-6a0a-49e6-a704-1a21d8362fcb"), // CHAR_COLOR2
};

static const int char_len[CHAR_COUNT] = {
    1, // CHAR_EFFECT
    2, // CHAR_PERIOD
    1, // CHAR_CHANCE
    3, // CHAR_COLOR1
    3, // CHAR_COLOR2
};

static const char *settings_name[ZONE_COUNT] =
    {
        "eye",
        "hair",
        "amulet",
        "back"};

static void load_settings(effect_settings_t settings[ZONE_COUNT]);
static void save_setting(effect_settings_t setting, uint8_t zone_indx);
static void *get_char_p(effect_settings_t *setting, uint8_t char_indx);
static void print_setting(effect_settings_t setting, uint8_t zone_id);

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        uint8_t zone_indx = get_zone_indx(pCharacteristic->getService()->getUUID());
        LEDZone *zone = zones[zone_indx];
        effect_settings_t settings = zone->get_effect_settings();

        if (pCharacteristic->getUUID().equals(char_uuids[CHAR_PERIOD]))
        {
            const uint8_t *bytes;
            bytes = pCharacteristic->getValue().data();
            settings.period = bytes[0];
            settings.period += bytes[1] / 256;
        }
        else
        {
            for (int i = 0; i < CHAR_COUNT; i++)
            {
                if (pCharacteristic->getUUID().equals(char_uuids[i]))
                {
                    memcpy(get_char_p(&settings, i), pCharacteristic->getValue().data(), char_len[i]);
                }
            }
        }

        print_setting(settings, zone_indx);
        save_setting(settings, zone_indx);
        zone->set_effect(settings);
    }

    uint8_t get_zone_indx(NimBLEUUID service_uuid)
    {
        uint8_t zone_index = ZONE_EYE;
        for (int i = 0; i < ZONE_COUNT; i++)
        {
            if (service_uuid.equals(zones_uuids[i]))
            {
                zone_index = i;
            }
        }

        return zone_index;
    }
};

static CharacteristicCallbacks chrCallbacks;

void setup()
{
    Serial.begin(115200);
    Serial.print("Start\n");

    prefs.begin("settings");

    effect_settings_t settings[ZONE_COUNT];
    load_settings(settings);

    static const std::list<int> led_numbers[ZONE_COUNT] =
        {
            {44, 51},
            {25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 40, 41, 42, 43, 45, 46, 47, 48, 49, 50, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 78, 79, 80, 81},
            {74, 75, 76, 77},
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 38, 39, 65, 66, 67, 68, 69, 70, 71, 72, 73,
             82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106}};

    for (int i = 0; i < ZONE_COUNT; i++)
    {
        zones[i] = new LEDZone();
        zones[i]->led_numbers = led_numbers[i];
        zones[i]->set_effect(settings[i]);
    }

    NimBLEDevice::init("Paint");

    NimBLEServer *pServer = NimBLEDevice::createServer();
    for (uint8_t i = 0; i < ZONE_COUNT; i++)
    {
        uint8_t buf[3] = {0};
        NimBLEService *pService = pServer->createService(zones_uuids[i]);
        for (int char_i = 0; char_i < CHAR_COUNT; char_i++)
        {
            NimBLECharacteristic *pCharacteristic;
            pCharacteristic = pService->createCharacteristic(char_uuids[char_i], NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, char_len[char_i]);
            if (pCharacteristic->getUUID().equals(char_uuids[CHAR_PERIOD]))
            {
                uint8_t fix_point_period[2] = {(uint8_t)settings[i].period, (uint8_t)(settings[i].period * 255)};
                pCharacteristic->setValue(fix_point_period, 2);
            }
            else
            {
                pCharacteristic->setValue((uint8_t *)get_char_p(&settings[i], char_i), char_len[char_i]);
                pCharacteristic->setCallbacks(&chrCallbacks);
            }
        }

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

static void load_settings(effect_settings_t settings[ZONE_COUNT])
{
    static const effect_settings_t default_settings[ZONE_COUNT] =
        {
            [ZONE_EYE] = (effect_settings_t){.type = EFFECT_CONST, .period = 1.0f, .color1 = 0xff0000},
            [ZONE_HAIR] = (effect_settings_t){.type = EFFECT_CONST, .color1 = 0x9776e3},
            [ZONE_AMULET] = (effect_settings_t){.type = EFFECT_CONST, .period = 3.0f, .color1 = 0x00ff00},
            [ZONE_BACKGROUND] = (effect_settings_t){.type = EFFECT_GRADIENT, .period = 2.0f, .chance_per_sec = 10, .color1 = 0x1d8a04}};

    effect_settings_t buf;
    for (int i = 0; i < ZONE_COUNT; i++)
    {
        if (prefs.getBytes(settings_name[i], &settings[i], sizeof(effect_settings_t)) == 0)
        {
            settings[i] = default_settings[i];
        }
    }
}

static void save_setting(effect_settings_t setting, uint8_t zone_indx)
{
    prefs.putBytes(settings_name[zone_indx], &setting, sizeof(effect_settings_t));
}

static void *get_char_p(effect_settings_t *setting, uint8_t char_indx)
{
    if (char_indx == CHAR_EFFECT)
    {
        return &setting->type;
    }
    else if (char_indx == CHAR_PERIOD)
    {
        return &setting->period;
    }
    else if (char_indx == CHAR_CHANCE)
    {
        return &setting->chance_per_sec;
    }
    else if (char_indx == CHAR_COLOR1)
    {
        return &setting->color1;
    }
    else if (char_indx == CHAR_COLOR2)
    {
        return &setting->color2;
    }
    return NULL;
}

static void print_setting(effect_settings_t setting, uint8_t zone_id)
{
    Serial.printf("Setting of zone=%s\n", settings_name[zone_id]);
    Serial.printf("type=%d\n", setting.type);
    Serial.printf("period=%f\n", setting.period);
    Serial.printf("chance_per_sec=%d\n", setting.chance_per_sec);
    Serial.printf("color1=(r%d\t;g%d\t;b%d)\n", (setting.color1 >> 16) & 0xff, (setting.color1 >> 8) & 0xff, setting.color1 & 0xff);
    Serial.printf("color2=(r%d\t;g%d\t;b%d)\n", (setting.color2 >> 16) & 0xff, (setting.color2 >> 8) & 0xff, setting.color2 & 0xff);
}
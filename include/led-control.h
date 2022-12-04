#include <stdint.h>
#include <list>

#define NUM_LED 106
#define TIC_IN_SEC 10

enum
{
    EFFECT_OFF = 0,
    EFFECT_CONST = 1,
    EFFECT_BLINK = 2,
    EFFECT_GRADIENT = 3,
    EFFECT_RANDOM = 4,
    EFFECT_COUNT
};

typedef struct effect_settings_t
{
    uint8_t type;
    float period;
    uint8_t chance_per_sec;
    uint32_t color1;
    uint32_t color2;
}effect_settings_t;

class Effect
{
public:
    virtual void fill_array() = 0;
};

class LEDZone
{
public:
    LEDZone();
    std::list<int> led_numbers;
    void loop();
    void set_effect(effect_settings_t settings);
    effect_settings_t get_effect_settings();
private:
    effect_settings_t settings;
    Effect *effect = NULL;
};

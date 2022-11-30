#include "led-control.h"
#include "FastLED.h"

#define DATA_PIN 12

typedef struct float_rgb_t
{
    float red;
    float green;
    float blue;
} float_rgb_t;

static CRGB leds[NUM_LED];
static bool is_init = false;

class ConstEffect : public Effect
{
public:
    ConstEffect(std::list<int> new_leds, uint32_t color_code_new)
    {
        leds_nums = new_leds;
        color_code = color_code_new;
    }

    void fill_array()
    {
        for (int led_num : leds_nums)
        {
            leds[led_num] = CRGB(color_code);
        }
    }

private:
    std::list<int> leds_nums;
    uint32_t color_code;
};

class GradientEffect : public Effect
{
public:
    GradientEffect(std::list<int> new_leds, effect_settings_t settings)
    {
        color1 = CRGB(settings.color1);
        color2 = CRGB(settings.color2);

        leds_nums = new_leds;
        current_tic = 1;
        is_direction_1to2 = true;
        tic_in_period = settings.period * TIC_IN_SEC;
        red_scale = ((float)(CRGB(color2).red - CRGB(color1).red)) / (float)tic_in_period;
        green_scale = ((float)(CRGB(color2).green - CRGB(color1).green)) / (float)tic_in_period;
        blue_scale = ((float)(CRGB(color2).blue - CRGB(color1).blue)) / (float)tic_in_period;
        current_color = (float_rgb_t){
            .red = (float)CRGB(color1).red,
            .green = (float)CRGB(color1).green,
            .blue = (float)CRGB(color1).blue,
        };
    }

    void fill_array()
    {
        if (is_direction_1to2)
        {
            current_color.red += red_scale;
            current_color.green += green_scale;
            current_color.blue += blue_scale;
        }
        else
        {
            current_color.red -= red_scale;
            current_color.green -= green_scale;
            current_color.blue -= blue_scale;
        }
        for (int led_index : leds_nums)
        {
            leds[led_index] = CRGB(current_color.red, current_color.green, current_color.blue);
        }

        if (current_tic == 0 || current_tic >= tic_in_period)
        {
            is_direction_1to2 = !is_direction_1to2;
        }
        if (is_direction_1to2)
        {
            current_tic++;
        }
        else
        {
            current_tic--;
        }
    }

private:
    std::list<int> leds_nums;
    CRGB color1;
    CRGB color2;
    uint16_t tic_in_period;
    bool is_direction_1to2;
    float_rgb_t current_color;

    uint16_t current_tic;
    float red_scale, green_scale, blue_scale;
};



LEDZone::LEDZone()
{
    if (!is_init)
    {
        FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LED);
        is_init = true;
    }
}

void LEDZone::loop()
{
    if (effect != NULL)
    {
        effect->fill_array();
    }
    FastLED.show();
}

void LEDZone::set_effect(effect_settings_t settings)
{
    this->settings = settings;
    if (effect != NULL)
    {
        delete effect;
    }

    if (settings.type == EFFECT_OFF)
    {
        effect = new ConstEffect(led_numbers, 0);
    }
    else if (settings.type == EFFECT_CONST)
    {
        effect = new ConstEffect(led_numbers, settings.color1);
    }
    else if (settings.type == EFFECT_BLINK)
    {
        settings.color2 = 0;
        effect = new GradientEffect(led_numbers, settings);
    }
    else if (settings.type == EFFECT_GRADIENT)
    {
        effect = new GradientEffect(led_numbers, settings);
    }
}

effect_settings_t LEDZone::get_effect_settings()
{
    if (effect != NULL)
    {
        return settings;
    }
    else
    {
        return (effect_settings_t){
            .type = EFFECT_OFF,
        };
    }
}
#include <stdio.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_COLOR_RED    0x01
#define LED_COLOR_GREEN  0x02
#define LED_COLOR_YELLOW 0x03

#define DISPLAY_BUFFER_LEN (64 * 32 / 8)

struct led_t {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t oe;
    uint8_t r1;
    uint8_t r2;
    uint8_t g1;
    uint8_t g2;
    uint8_t stb;
    uint8_t clk;
    uint8_t display_buffer[DISPLAY_BUFFER_LEN];
    uint8_t color;
    esp_timer_handle_t led_timer;
};

static struct led_t led = {
    .a   = 12,
    .b   = 5,
    .c   = 2,
    .d   = 15,
    .oe  = 14,
    .r1  = 23,
    .r2  = 22,
    .g1  = 27,
    .g2  = 13,
    .stb = 17,
    .clk = 18,
    .display_buffer = {0},
    .color = LED_COLOR_YELLOW
};

static void led_gpio_init() {
    gpio_config_t conf;
    conf.intr_type = GPIO_PIN_INTR_DISABLE;
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pin_bit_mask = (1ULL << led.a)            
            | (1ULL << led.b)
            | (1ULL << led.c)
            | (1ULL << led.d)
            | (1ULL << led.oe)
            | (1ULL << led.r1)
            | (1ULL << led.r2)
            | (1ULL << led.g1)
            | (1ULL << led.g2)
            | (1ULL << led.stb)
            | (1ULL << led.clk);
    conf.pull_down_en = 0;
    conf.pull_up_en = 1;
    gpio_config(&conf);
}

static void led_scan() {
    static uint8_t row = 0;

    uint8_t *head = led.display_buffer + row * 8;

    uint8_t *ptr = head;

    for (uint8_t byte = 0; byte < 8; byte++) {
        uint8_t pixels_u = *ptr;
        uint8_t pixels_d = *(ptr + 128);
        ptr++;
        for (uint8_t bit = 0; bit < 8; bit++) {
            gpio_set_level(led.clk, 0);
            if(led.color & LED_COLOR_RED) {
                gpio_set_level(led.r1, pixels_u & (0x80 >> bit));
                gpio_set_level(led.r2, pixels_d & (0x80 >> bit));
            }
            if(led.color & LED_COLOR_GREEN) {
                gpio_set_level(led.g1, pixels_u & (0x80 >> bit));
                gpio_set_level(led.g2, pixels_d & (0x80 >> bit));
            }
            gpio_set_level(led.clk, 1);
        }
    }

    gpio_set_level(led.oe, 1);

    gpio_set_level(led.a, (row & 0x01));
    gpio_set_level(led.b, (row & 0x02));
    gpio_set_level(led.c, (row & 0x04));
    gpio_set_level(led.d, (row & 0x08));

    gpio_set_level(led.stb, 0);
    gpio_set_level(led.stb, 1);
    gpio_set_level(led.stb, 0);

    gpio_set_level(led.oe, 0);

    row = (row + 1) & 0x0F;
}

static void led_timer_callback(void* arg) {
    led_scan();
}

static void led_timer_init() {
    const esp_timer_create_args_t display_timer_args = {
        .callback = &led_timer_callback,
        .name = "led_timer",
    };
    esp_timer_create(&display_timer_args, &(led.led_timer));
}


void app_main(void) {
    led_gpio_init();
    led_timer_init();
    for(int i = 0; i < 16; i++)
        led.display_buffer[i] = 0b11111111;
    esp_timer_start_periodic(led.led_timer, 1000);
}

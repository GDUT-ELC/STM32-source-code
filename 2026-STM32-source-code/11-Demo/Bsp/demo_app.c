#include "demo_app.h"
#include "demo_config.h"
#include "OLED.h"
#include "key.h"
#include "led.h"
#include "buzzer.h"
#include "motor.h"
#include "adc_driver.h"
#include "adc.h"
#include "MPU6050.h"
#include "attitude.h"
#include "hcsr04.h"
#include "uart_driver.h"
#include "esp8266.h"
#include "app_flash.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enum { PAGE_LED, PAGE_BUZZER, PAGE_OLED, PAGE_KEY, PAGE_MOTOR, PAGE_ADC,
       PAGE_MPU, PAGE_HCSR, PAGE_UART, PAGE_ESP, PAGE_FLASH, PAGE_COUNT };

/* Keil Watch: g_demo, g_motor_feedback, HCSR04, g_esp_state, g_flash_info. */
struct {
    uint8_t page, last_key, motor_ready, uart_ready, mpu_ready, mpu_id;
    uint8_t motor_mode, led_run, buzzer_run, oled_mode, alarm, mpu_view;
    uint16_t adc_raw, adc_mv;
    HAL_StatusTypeDef adc_status;
    int16_t ax, ay, az, gx, gy, gz;
    uint32_t key_count[4], uart_rx[2], uart_tx[2], uart_errors[2];
    GD25Q_Status flash_result;
    uint8_t flash_pass, flash_runs;
} g_demo;
attitude_t g_attitude;

static char screen[4][17];
static uint32_t sensor_tick, render_tick, led_tick, hcsr_tick, mpu_retry_tick;
static uint32_t boot_tick;
static uint32_t mpu_tick;
static uint8_t led_color, led_brightness;
static uint8_t packet[UART_DRIVER_BUFFER_SIZE];
static char last_uart_text[17] = "Send from PC";

static void restart_mpu(void)
{
    attitude_reset(&g_attitude);
    g_demo.mpu_ready = (MPU6050_Init() == HAL_OK);
    if (g_demo.mpu_ready) (void)MPU6050_GetID(&g_demo.mpu_id);
    mpu_tick = HAL_GetTick();
}

/* All OLED rows stay within 16 characters; redraw only changed rows. */
static void row(uint8_t line, const char *format, ...)
{
    char formatted[64], padded[17];
    size_t length;
    va_list args;
    va_start(args, format);
    (void)vsnprintf(formatted, sizeof(formatted), format, args);
    va_end(args);
    length = strlen(formatted);
    if (length > 16U) length = 16U;
    memset(padded, ' ', 16U);
    memcpy(padded, formatted, length);
    padded[16] = '\0';
    if (memcmp(screen[line - 1U], padded, 17U) != 0) {
        oled_showstring(line, 1, padded);
        memcpy(screen[line - 1U], padded, 17U);
    }
}

static void stop_outputs(void)
{
    motor_stop_all();
    g_demo.motor_mode = 0;
    g_demo.led_run = g_demo.buzzer_run = g_demo.alarm = 0;
    buzzer_off();
    WS_SetGRBColor(0, 0, 0);
}

static void change_page(uint8_t page)
{
    stop_outputs();
    g_demo.page = page;
    g_demo.led_run = (page == PAGE_LED);
    g_demo.buzzer_run = (page == PAGE_BUZZER);
    oled_clear();
    memset(screen, 0, sizeof(screen));
}

static void flash_test(void)
{
		uint8_t written[] = {1, 2, 3, 4, 5, 6};
		uint8_t readback[sizeof(written)] = {0};
    uint8_t i;
    /* Alternating data also exercises the erase path on a repeated test. */
    if (g_demo.flash_runs & 1U)
        for (i = 0; i < sizeof(written); ++i) written[i] ^= 0xFFU;
    g_demo.flash_pass = 0;
    g_demo.flash_result = app_flash_init();
    if (g_demo.flash_result == GD25Q_OK)
        g_demo.flash_result = app_flash_save(DEMO_FLASH_TEST_ADDR, written, sizeof(written));
    if (g_demo.flash_result == GD25Q_OK)
        g_demo.flash_result = app_flash_load(DEMO_FLASH_TEST_ADDR, readback, sizeof(readback));
    if (g_demo.flash_result == GD25Q_OK) {
        if (memcmp(written, readback, sizeof(written)) == 0) g_demo.flash_pass = 1;
        else g_demo.flash_result = GD25Q_ERR_VERIFY;
    }
    ++g_demo.flash_runs;
}

static void action(void)
{
    switch (g_demo.page) {
    case PAGE_LED:
        g_demo.led_run ^= 1U;
        if (!g_demo.led_run) WS_SetGRBColor(0, 0, 0);
        break;
    case PAGE_BUZZER: g_demo.buzzer_run ^= 1U; break;
    case PAGE_OLED:
        g_demo.oled_mode ^= 1U;
        oled_clear();
        memset(screen, 0, sizeof(screen));
        break;
    case PAGE_KEY: memset(g_demo.key_count, 0, sizeof(g_demo.key_count)); break;
    case PAGE_MOTOR:
        if (!g_demo.motor_ready) break;
        g_demo.motor_mode = (g_demo.motor_mode + 1U) % 4U;
        if (g_demo.motor_mode == 1U || g_demo.motor_mode == 2U) {
            float sign = g_demo.motor_mode == 1U ? 1.0f : -1.0f;
            motor_set_target_cps(MOTOR_A, sign * DEMO_MOTOR_A_CPS);
            motor_set_target_cps(MOTOR_B, sign * DEMO_MOTOR_B_CPS);
        } else if (g_demo.motor_mode == 3U) {
            motor_set_pwm(MOTOR_A, 300);
            motor_set_pwm(MOTOR_B, -300);
        } else motor_stop_all();
        break;
    case PAGE_MPU:
        if (g_demo.mpu_ready) g_demo.mpu_view = (g_demo.mpu_view + 1U) % 3U;
        else restart_mpu();
        break;
    case PAGE_HCSR: g_demo.alarm ^= 1U; break;
    case PAGE_UART: {
        static uint8_t message[] = "11-Demo UART test\r\n";
        if (uart_driver_send(message, sizeof(message) - 1U) == HAL_OK)
            g_demo.uart_tx[0] += sizeof(message) - 1U;
        if (HAL_UART_Transmit(&huart2, message, sizeof(message) - 1U, 100U) == HAL_OK)
            g_demo.uart_tx[1] += sizeof(message) - 1U;
        break;
    }
    case PAGE_ESP: (void)esp8266_init(); break;
    case PAGE_FLASH: flash_test(); break;
    default: break;
    }
}

static void uart_task(void)
{
    UART_HandleTypeDef *ports[2] = {&huart4, &huart2};
    uint16_t length;
    uint32_t i, j;
    for (i = 0; i < 2U; ++i) {
        UART_DriverRxStatus status = uart_driver_read_port(ports[i], packet, sizeof(packet), &length);
        if (status == UART_DRIVER_RX_READY) {
            g_demo.uart_rx[i] += length;
            if (HAL_UART_Transmit(ports[i], packet, length, 100U) == HAL_OK)
                g_demo.uart_tx[i] += length;
            else ++g_demo.uart_errors[i];
            for (j = 0; j < 16U && j < length; ++j)
                last_uart_text[j] = (packet[j] >= 32U && packet[j] <= 126U) ? (char)packet[j] : '.';
            last_uart_text[j] = '\0';
        } else if (status == UART_DRIVER_RX_OVERFLOW || status == UART_DRIVER_RX_ERROR) {
            static uint8_t message[] = "UART RX overflow/error\r\n";
            ++g_demo.uart_errors[i];
            (void)HAL_UART_Transmit(ports[i], message, sizeof(message) - 1U, 100U);
        }
    }
}

static void sample_sensors(void)
{
    g_demo.adc_status = HAL_ADC_Start(&hadc1);
    if (g_demo.adc_status == HAL_OK) {
        g_demo.adc_status = HAL_ADC_PollForConversion(&hadc1, 10U);
        if (g_demo.adc_status == HAL_OK) {
            g_demo.adc_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
            g_demo.adc_mv = (uint16_t)(adc_convert_to_voltage(g_demo.adc_raw) * 1000.0f);
        }
        (void)HAL_ADC_Stop(&hadc1);
    }
}

static void sample_mpu(void)
{
    if (!g_demo.mpu_ready) return;
    g_demo.mpu_ready = (MPU6050_GetData(&g_demo.ax, &g_demo.ay, &g_demo.az,
                                       &g_demo.gx, &g_demo.gy, &g_demo.gz) == HAL_OK);
    if (g_demo.mpu_ready)
        attitude_update(&g_attitude, g_demo.ax, g_demo.ay, g_demo.az,
                        g_demo.gx, g_demo.gy, g_demo.gz, HAL_GetTick());
    else attitude_reset(&g_attitude);
}

static void angle_row(uint8_t line, const char *name, float degrees)
{
    int32_t tenth = (int32_t)(degrees * 10.0f + (degrees < 0.0f ? -0.5f : 0.5f));
    char sign = tenth < 0 ? '-' : '+';
    if (tenth < 0) tenth = -tenth;
    row(line, "%s:%c%ld.%lddeg", name, sign, (long)(tenth / 10L), (long)(tenth % 10L));
}

static void render(void)
{
    motor_feedback_t a, b;
    switch (g_demo.page) {
    case PAGE_LED:
        row(1, "01 WS2812 RGB");
        row(2, "Color: %s", led_color == 0U ? "RED" : led_color == 1U ? "GREEN" : "BLUE");
        row(3, "Brightness:%3u", led_brightness);
        if (g_ws_status != HAL_OK) row(4, "TIM10 ERROR:%u", (unsigned)g_ws_status);
        else row(4, "K3 %s", g_demo.led_run ? "pause" : "run");
        break;
    case PAGE_BUZZER:
        row(1, "02 PWM BUZZER"); row(2, "PE14 TIM1 CH4");
        row(3, "2.73kHz 1s beep"); row(4, "K3 %s", g_demo.buzzer_run ? "mute" : "run");
        break;
    case PAGE_OLED:
        if (g_demo.oled_mode) {
            /* UTF-8 escapes keep the test independent of the compiler codepage. */
            oled_showutf8string(1, 1, "\xE4\xB8\xAD\xE6\x96\x87\xE6\xB5\x8B\xE8\xAF\x95");
            row(2, "STM32F407VET6"); row(3, "OLED UTF8/GB2312"); row(4, "K3 number test");
        } else {
            row(1, "03 OLED TEST"); row(2, "45   -88"); row(3, "HEX:ABCD");
            oled_shownum(2, 1, 45, 2);
            oled_showsignednum(2, 6, -88, 2);
            oled_showhexnum(3, 5, 0xABCD, 4);
            oled_showbinnum(4, 1, 0x5678, 16);
        }
        break;
    case PAGE_KEY:
        row(1, "04 KEY Last:%u", g_demo.last_key);
        row(2, "K1:%lu K2:%lu", (unsigned long)g_demo.key_count[0], (unsigned long)g_demo.key_count[1]);
        row(3, "K3:%lu K4:%lu", (unsigned long)g_demo.key_count[2], (unsigned long)g_demo.key_count[3]);
        row(4, "K3 clear counts");
        break;
    case PAGE_MOTOR:
        motor_get_feedback(MOTOR_A, &a); motor_get_feedback(MOTOR_B, &b);
        row(1, "05 MOTOR %s", !g_demo.motor_ready ? "ERROR" : g_demo.motor_mode == 0U ? "STOP" :
            g_demo.motor_mode == 3U ? "PWM" : "PID");
        row(2, "A%+6ld B%+6ld", (long)a.speed_cps, (long)b.speed_cps);
        row(3, "PWM%+4d %+4d", a.pwm, b.pwm);
        row(4, "K3 step K4 stop");
        break;
    case PAGE_ADC:
        row(1, "06 ADC PB0/IN8"); row(2, "ADC:%4u /4095", g_demo.adc_raw);
        row(3, "Volt:%4umV", g_demo.adc_mv);
        row(4, g_demo.adc_status == HAL_OK ? "Change LDR light" : "ADC ERROR");
        break;
    case PAGE_MPU:
        if (g_demo.mpu_ready) {
            int16_t values[3];
            uint8_t i;
            if (g_demo.mpu_view == 2U) {
                if (!g_attitude.ready) {
                    row(1, "07 ANG CAL K3>A");
                    row(2, "Keep MPU still");
                    row(3, "Samples:%u/%u", g_attitude.calibration_samples, ATTITUDE_CAL_SAMPLES);
                    row(4, "Wait for angles");
                } else {
                    row(1, "07 ANG K3>A K4>Z");
                    angle_row(2, "Roll", g_attitude.roll_deg);
                    angle_row(3, "Pitch", g_attitude.pitch_deg);
                    angle_row(4, "YawRel", g_attitude.yaw_deg);
                }
                break;
            }
            if (g_demo.mpu_view == 0U) {
                row(1, "07 ACCEL g K3>G");
                values[0] = g_demo.ax; values[1] = g_demo.ay; values[2] = g_demo.az;
            } else {
                row(1, "07 GYRO K3>ANG");
                values[0] = g_demo.gx; values[1] = g_demo.gy; values[2] = g_demo.gz;
            }
            for (i = 0; i < 3U; ++i) {
                int32_t magnitude = values[i];
                char sign = magnitude < 0 ? '-' : '+';
                if (magnitude < 0) magnitude = -magnitude;
                /* MPU6050_Init sets +/-16g (2048 LSB/g), +/-2000deg/s
                 * (16.4 LSB/(deg/s)). Use rounded fixed point, no float printf. */
                if (g_demo.mpu_view == 0U) {
                    int32_t milli_g = (magnitude * 1000L + 1024L) / 2048L;
                    row(i + 2U, "Acc%c:%c%ld.%03ldg", 'X' + i, sign,
                        (long)(milli_g / 1000L), (long)(milli_g % 1000L));
                } else {
                    int32_t tenth_dps = (magnitude * 100L + 82L) / 164L;
                    row(i + 2U, "Gyro%c:%c%ld.%ldd/s", 'X' + i, sign,
                        (long)(tenth_dps / 10L), (long)(tenth_dps % 10L));
                }
            }
        } else {
            row(1, "07 MPU6050 ERR");
            row(2, "ID:%02X want:68", g_demo.mpu_id);
            row(3, "Check PA4 / PA5"); row(4, "K3 retry");
        }
        break;
    case PAGE_HCSR: {
        uint32_t tenth = (uint32_t)(hcsr04_getdistance(&HCSR04) * 10.0f);
        row(1, "08 HC-SR04");
        if (HCSR04.valid) row(2, "Dist:%lu.%lucm", (unsigned long)(tenth / 10U), (unsigned long)(tenth % 10U));
        else row(2, "NO ECHO / RANGE");
        row(3, "Echo:%luus", (unsigned long)HCSR04.high_time);
        row(4, "K3 alarm:%s", g_demo.alarm ? "ON" : "OFF");
        break;
    }
    case PAGE_UART:
        row(1, "09 UART 115200");
        row(2, "4 R%lu T%lu E%lu", (unsigned long)g_demo.uart_rx[0], (unsigned long)g_demo.uart_tx[0], (unsigned long)g_demo.uart_errors[0]);
        row(3, "2 R%lu T%lu E%lu", (unsigned long)g_demo.uart_rx[1], (unsigned long)g_demo.uart_tx[1], (unsigned long)g_demo.uart_errors[1]);
        row(4, "%s", g_demo.uart_ready ? last_uart_text : "UART INIT ERROR");
        break;
    case PAGE_ESP:
        row(1, "10 ESP8266"); row(2, "%s", esp8266_status_string());
        row(3, "UART5 RX:%lu", (unsigned long)g_esp_rx_bytes);
        row(4, "K3 retry/test");
        break;
    case PAGE_FLASH:
        row(1, "12 GD25Q80"); row(2, "ID:%06lX", (unsigned long)g_flash_info.id);
        row(3, "%s %s", g_demo.flash_pass ? "PASS" : "FAIL", GD25Q_StatusString(g_demo.flash_result));
        row(4, "K3 R/W @001000");
        break;
    default: break;
    }
}

void Demo_Init(void)
{
    oled_init();
    row(1, "11-DEMO SELFTEST"); row(2, "Init peripherals");
    row(3, "K1 next K2 prev"); row(4, "K3 test K4 stop");
    g_demo.motor_ready = (motor_init() == HAL_OK);
    /* HC-SR04 still works if a motor initialization stage failed. */
    if ((htim5.Instance->CR1 & TIM_CR1_CEN) == 0U) (void)HAL_TIM_Base_Start(&htim5);
    g_demo.uart_ready = (uart_driver_init() == HAL_OK);
    restart_mpu();
    g_demo.mpu_view = 2U;
    flash_test();
    boot_tick = HAL_GetTick();
    change_page(PAGE_LED);
}

void Demo_Task(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t key = key_getnum();
    if (key != 0U) {
        g_demo.last_key = key;
        ++g_demo.key_count[key - 1U];
        if (key == 1U) change_page((g_demo.page + 1U) % PAGE_COUNT);
        else if (key == 2U) change_page((g_demo.page + PAGE_COUNT - 1U) % PAGE_COUNT);
        else if (key == 3U) action();
        else {
            stop_outputs();
            if (g_demo.page == PAGE_MPU && g_demo.mpu_view == 2U)
                attitude_zero_yaw(&g_attitude);
        }
    }
    uart_task();
    if (g_esp_state == ESP_IDLE && (uint32_t)(now - boot_tick) >= 2000U) (void)esp8266_init();
    esp8266_process();
    hcsr04_process();
    if ((uint32_t)(now - hcsr_tick) >= 60U) { hcsr_tick = now; hcsr04_start(); }
    if ((uint32_t)(now - sensor_tick) >= 100U) { sensor_tick = now; sample_sensors(); }
    if ((uint32_t)(HAL_GetTick() - mpu_tick) >= 20U) {
        sample_mpu();
        /* Schedule after acquisition: software I2C can itself take >20ms. */
        mpu_tick = HAL_GetTick();
    }
    if (!g_demo.mpu_ready && (uint32_t)(now - mpu_retry_tick) >= 1000U) {
        mpu_retry_tick = now;
        restart_mpu();
    }
    if (g_demo.page == PAGE_LED && g_demo.led_run && (uint32_t)(now - led_tick) >= 20U) {
        led_tick = now;
        if (led_brightness >= 252U) { led_brightness = 0; led_color = (led_color + 1U) % 3U; }
        else led_brightness += 4U;
        WS_SetBrightnessColor(led_color == 0U ? 255U : 0U, led_color == 1U ? 255U : 0U,
                              led_color == 2U ? 255U : 0U, led_brightness);
    }
    if ((g_demo.page == PAGE_BUZZER && g_demo.buzzer_run && (now % 2000U) < 1000U) ||
        (g_demo.alarm && HCSR04.valid && HCSR04.distance < 10.0f)) buzzer_on();
    else buzzer_off();
    if ((uint32_t)(now - render_tick) >= 100U) { render_tick = now; render(); }
}

/* All HAL callbacks have one owner; dispatch explicitly by peripheral. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) motor_control_tick();
}
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == ECHO_Pin) hcsr04_echo_irq();
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *port)
{
    uart_driver_rx_callback(port);
    esp8266_rx_callback(port);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *port)
{
    uart_driver_error_callback(port);
    esp8266_error_callback(port);
}

#include "attitude.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
static uint32_t now_ms;

static void calibrate(attitude_t *s, int16_t ax, int16_t ay, int16_t az,
                      int16_t gx, int16_t gy, int16_t gz)
{
    unsigned i;
    attitude_reset(s);
    for (i = 0; i < ATTITUDE_CAL_SAMPLES; ++i) {
        now_ms += 20U;
        attitude_update(s, ax, ay, az, gx, gy, gz, now_ms);
    }
    assert(s->ready);
}

static void test_calibration_and_tilt(void)
{
    attitude_t s;
    unsigned i;
    calibrate(&s, 0, 0, 2048, 16, -20, 33);
    assert(fabsf(s.roll_deg) < 0.01f && fabsf(s.pitch_deg) < 0.01f);
    assert(s.gyro_bias[0] == 16 && s.gyro_bias[1] == -20 && s.gyro_bias[2] == 33);
    for (i = 0; i < 1000U; ++i) {
        now_ms += 20U;
        attitude_update(&s, 0, 0, 2048, 16, -20, 33, now_ms);
    }
    assert(fabsf(s.yaw_deg) < 0.01f);
    calibrate(&s, 0, 1448, 1448, 0, 0, 0);
    assert(fabsf(s.roll_deg - 45.0f) < 0.05f && fabsf(s.pitch_deg) < 0.05f);
    calibrate(&s, -1448, 0, 1448, 0, 0, 0);
    assert(fabsf(s.pitch_deg - 45.0f) < 0.05f && fabsf(s.roll_deg) < 0.05f);
    calibrate(&s, 0, 0, -2048, 0, 0, 0);
    assert(fabsf(fabsf(s.roll_deg) - 180.0f) < 0.05f);
    attitude_reset(&s);
    for (i = 0; i < 100U; ++i) {
        now_ms += 20U;
        attitude_update(&s, 0, 0, 2048, 164, 0, 0, now_ms);
    }
    assert(!s.ready && s.calibration_samples == 0U);
    for (i = 0; i < 100U; ++i) {
        now_ms += 20U;
        attitude_update(&s, 0, 0, 0, 0, 0, 0, now_ms);
    }
    assert(!s.ready && s.calibration_samples == 0U);
    attitude_update(&s, 0, 0, 2048, 0, 0, 0, now_ms);
    for (i = 0; i < 100U; ++i)
        attitude_update(&s, 0, 0, 2048, 0, 0, 0, now_ms);
    assert(!s.ready && s.calibration_samples == 1U);
    puts("PASS: static tilt, gyro bias removal, motion/invalid gravity calibration rejection");
}

static void test_rotations_and_gaps(void)
{
    attitude_t s;
    unsigned i;
    float saved_yaw;
    calibrate(&s, 0, 0, 2048, 0, 0, 0);
    /* 90deg/s for one second, with 10/30ms intervals rather than fixed dt. */
    for (i = 0; i < 50U; ++i) {
        now_ms += (i & 1U) ? 30U : 10U;
        attitude_update(&s, 0, 0, 2048, 0, 0, 1476, now_ms);
    }
    assert(fabsf(s.yaw_deg - 90.0f) < 0.1f);
    attitude_zero_yaw(&s);
    assert(fabsf(s.yaw_deg) < 0.01f);
    for (i = 0; i < 50U; ++i) {
        now_ms += 20U;
        attitude_update(&s, 0, 0, 4096, 0, 0, 1476, now_ms);
    }
    assert(fabsf(s.yaw_deg - 90.0f) < 0.1f);
    saved_yaw = s.yaw_deg;
    now_ms += 2000U;
    attitude_update(&s, 0, 0, 2048, 0, 0, 1476, now_ms);
    assert(fabsf(s.yaw_deg - saved_yaw) < 0.01f);
    for (i = 0; i < 100U; ++i) {
        now_ms += 20U;
        attitude_update(&s, 0, 0, 2048, 0, 0, 1476, now_ms);
    }
    assert(s.yaw_deg >= -180.0f && s.yaw_deg <= 180.0f);
    assert(fabsf(fabsf(s.yaw_deg) - 90.0f) < 0.2f);
    /* Calibration and integration across uint32 millisecond wrap. */
    now_ms = UINT32_MAX - 900U;
    calibrate(&s, 0, 0, 2048, 0, 0, 0);
    for (i = 0; i < 50U; ++i) {
        now_ms += 20U;
        attitude_update(&s, 0, 0, 2048, 0, 0, 1476, now_ms);
    }
    assert(fabsf(s.yaw_deg - 90.0f) < 0.1f);
    puts("PASS: yaw 90deg, zeroing, variable dt, acceleration rejection, gaps and tick wrap");
}

static void test_tilt_correction_and_combined_rotation(void)
{
    attitude_t s;
    unsigned i;
    double q[4] = {1, 0, 0, 0}, t = 0;
    const double omega = sqrt(30.0 * 30.0 + 20.0 * 20.0 + 40.0 * 40.0);
    double dot;
    calibrate(&s, 0, 0, 2048, 0, 0, 0);
    /* Stationary 45deg tilt converges even if the movement was not sampled. */
    for (i = 0; i < 400U; ++i) {
        now_ms += 20U;
        attitude_update(&s, 0, 1448, 1448, 0, 0, 0, now_ms);
    }
    assert(fabsf(s.roll_deg - 45.0f) < 0.05f);
    calibrate(&s, 0, 0, 2048, 0, 0, 0);
    /* Independent analytic quaternion for simultaneous constant body rates:
     * X=30, Y=20, Z=40deg/s. Generate the corresponding gravity observations. */
    for (i = 0; i < 250U; ++i) {
        uint32_t dt_ms = i % 3U == 0U ? 10U : i % 3U == 1U ? 20U : 35U;
        double half_angle, v[3];
        t += (double)dt_ms / 1000.0;
        half_angle = omega * t * PI / 360.0;
        q[0] = cos(half_angle);
        q[1] = sin(half_angle) * 30.0 / omega;
        q[2] = sin(half_angle) * 20.0 / omega;
        q[3] = sin(half_angle) * 40.0 / omega;
        v[0] = 2.0 * (q[1] * q[3] - q[0] * q[2]);
        v[1] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
        v[2] = q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3];
        now_ms += dt_ms;
        attitude_update(&s, (int16_t)lround(v[0] * 2048.0),
                         (int16_t)lround(v[1] * 2048.0), (int16_t)lround(v[2] * 2048.0),
                         492, 328, 656, now_ms);
    }
    dot = fabs(q[0] * s.q[0] + q[1] * s.q[1] + q[2] * s.q[2] + q[3] * s.q[3]);
    assert(dot > 0.9998 && dot < 1.0001);
    assert(isfinite(s.roll_deg) && isfinite(s.pitch_deg) && isfinite(s.yaw_deg));
    puts("PASS: gravity tilt correction and simultaneous XYZ rotation against analytic quaternion");
}

int main(void)
{
    test_calibration_and_tilt();
    test_rotations_and_gaps();
    test_tilt_correction_and_combined_rotation();
    puts("All attitude tests passed.");
    return 0;
}

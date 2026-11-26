#include "attitude.h"
#include <math.h>
#include <string.h>

#define DEG_TO_RAD 0.01745329252f
#define RAD_TO_DEG 57.295779513f
#define ACC_LSB_PER_G 2048.0f
#define GYRO_LSB_PER_DPS 16.4f
#define GRAVITY_GAIN 2.0f

static float wrap_degrees(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static void publish(attitude_t *s)
{
    const float *q = s->q;
    float sin_pitch = 2.0f * (q[0] * q[2] - q[3] * q[1]);
    if (sin_pitch > 1.0f) sin_pitch = 1.0f;
    if (sin_pitch < -1.0f) sin_pitch = -1.0f;
    s->roll_deg = atan2f(2.0f * (q[0] * q[1] + q[2] * q[3]),
                        1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2])) * RAD_TO_DEG;
    s->pitch_deg = asinf(sin_pitch) * RAD_TO_DEG;
    s->yaw_deg = wrap_degrees(atan2f(2.0f * (q[0] * q[3] + q[1] * q[2]),
                                  1.0f - 2.0f * (q[2] * q[2] + q[3] * q[3]))
                                  * RAD_TO_DEG - s->yaw_zero_deg);
}

/* Seed tilt from gravity. Preserve the last heading when a sampling gap makes
 * gyro integration unreliable; no rotation is invented during the gap. */
static void seed_tilt(attitude_t *s, float ax, float ay, float az, float yaw_deg)
{
    float roll = atan2f(ay, az);
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));
    float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
    float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    float cy = cosf(yaw_deg * DEG_TO_RAD * 0.5f), sy = sinf(yaw_deg * DEG_TO_RAD * 0.5f);
    s->q[0] = cr * cp * cy + sr * sp * sy;
    s->q[1] = sr * cp * cy - cr * sp * sy;
    s->q[2] = cr * sp * cy + sr * cp * sy;
    s->q[3] = cr * cp * sy - sr * sp * cy;
}

void attitude_reset(attitude_t *s)
{
    memset(s, 0, sizeof(*s));
    s->q[0] = 1.0f;
}

static void clear_calibration(attitude_t *s)
{
    s->calibration_samples = 0;
    memset(s->gyro_sum, 0, sizeof(s->gyro_sum));
    memset(s->accel_sum, 0, sizeof(s->accel_sum));
}

void attitude_update(attitude_t *s, int16_t ax, int16_t ay, int16_t az,
                     int16_t gx, int16_t gy, int16_t gz, uint32_t now_ms)
{
    float a[3] = {(float)ax, (float)ay, (float)az};
    float g[3] = {(float)gx, (float)gy, (float)gz};
    float norm_a = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    uint8_t gravity_valid = (norm_a >= 0.85f * ACC_LSB_PER_G && norm_a <= 1.15f * ACC_LSB_PER_G);
    uint32_t elapsed = now_ms - s->last_ms;
    uint32_t i;
    if (!s->ready) {
        uint8_t still = gravity_valid;
        for (i = 0; i < 3U; ++i) {
            if (fabsf(g[i]) > 5.0f * GYRO_LSB_PER_DPS) still = 0;
            if (s->calibration_samples != 0U) {
                float n = (float)s->calibration_samples;
                if (fabsf(g[i] - (float)s->gyro_sum[i] / n) > 8.2f ||
                    fabsf(a[i] - (float)s->accel_sum[i] / n) > 80.0f) still = 0;
            }
        }
        if (!still || (s->calibration_samples != 0U && elapsed > 250U)) {
            clear_calibration(s);
            s->last_ms = now_ms;
            return;
        }
        /* Repeated reads at the same timestamp must not count as new samples. */
        if (s->calibration_samples != 0U && elapsed < 10U) return;
        if (s->calibration_samples == 0U) s->calibration_start_ms = now_ms;
        for (i = 0; i < 3U; ++i) {
            s->gyro_sum[i] += (int32_t)g[i];
            s->accel_sum[i] += (int32_t)a[i];
        }
        ++s->calibration_samples;
        s->last_ms = now_ms;
        if (s->calibration_samples >= ATTITUDE_CAL_SAMPLES &&
            (uint32_t)(now_ms - s->calibration_start_ms) >= 1000U) {
            for (i = 0; i < 3U; ++i)
                s->gyro_bias[i] = (float)s->gyro_sum[i] / (float)s->calibration_samples;
            seed_tilt(s, (float)s->accel_sum[0], (float)s->accel_sum[1], (float)s->accel_sum[2], 0.0f);
            s->ready = 1;
            publish(s);
        }
        return;
    }
    if (elapsed == 0U) return;
    s->last_ms = now_ms;
    if (elapsed > 250U) {
        if (gravity_valid) seed_tilt(s, a[0], a[1], a[2], s->yaw_deg + s->yaw_zero_deg);
        publish(s);
        return;
    }
    for (i = 0; i < 3U; ++i)
        g[i] = (g[i] - s->gyro_bias[i]) * (DEG_TO_RAD / GYRO_LSB_PER_DPS);
    /* Quaternion complementary filter: gravity corrects tilt, never heading.
     * Reject acceleration far from 1g, where gravity cannot be isolated. */
    if (gravity_valid) {
        float *q = s->q;
        float v[3] = {2.0f * (q[1] * q[3] - q[0] * q[2]),
                      2.0f * (q[0] * q[1] + q[2] * q[3]),
                      q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3]};
        for (i = 0; i < 3U; ++i) a[i] /= norm_a;
        g[0] += GRAVITY_GAIN * (a[1] * v[2] - a[2] * v[1]);
        g[1] += GRAVITY_GAIN * (a[2] * v[0] - a[0] * v[2]);
        g[2] += GRAVITY_GAIN * (a[0] * v[1] - a[1] * v[0]);
    }
    {
        float dt_half = (float)elapsed * 0.0005f;
        float w = s->q[0], x = s->q[1], y = s->q[2], z = s->q[3];
        float norm_q;
        s->q[0] += (-x * g[0] - y * g[1] - z * g[2]) * dt_half;
        s->q[1] += ( w * g[0] + y * g[2] - z * g[1]) * dt_half;
        s->q[2] += ( w * g[1] - x * g[2] + z * g[0]) * dt_half;
        s->q[3] += ( w * g[2] + x * g[1] - y * g[0]) * dt_half;
        norm_q = sqrtf(s->q[0] * s->q[0] + s->q[1] * s->q[1] +
                       s->q[2] * s->q[2] + s->q[3] * s->q[3]);
        for (i = 0; i < 4U; ++i) s->q[i] /= norm_q;
    }
    publish(s);
}

void attitude_zero_yaw(attitude_t *s)
{
    if (!s->ready) return;
    s->yaw_zero_deg = wrap_degrees(s->yaw_zero_deg + s->yaw_deg);
    publish(s);
}

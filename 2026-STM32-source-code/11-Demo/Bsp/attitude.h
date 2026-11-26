#ifndef ATTITUDE_H
#define ATTITUDE_H
#include <stdint.h>

#define ATTITUDE_CAL_SAMPLES 64U

/* MPU6050 configured for +/-16g and +/-2000deg/s. Angles use the sensor axes:
 * roll around X, pitch around Y; yaw is relative, without magnetic heading. */
typedef struct {
    float roll_deg, pitch_deg, yaw_deg;
    float q[4], gyro_bias[3], yaw_zero_deg;
    int32_t gyro_sum[3], accel_sum[3];
    uint32_t last_ms, calibration_start_ms;
    uint16_t calibration_samples;
    uint8_t ready;
} attitude_t;

void attitude_reset(attitude_t *state);
void attitude_update(attitude_t *state, int16_t ax, int16_t ay, int16_t az,
                     int16_t gx, int16_t gy, int16_t gz, uint32_t now_ms);
void attitude_zero_yaw(attitude_t *state);
#endif

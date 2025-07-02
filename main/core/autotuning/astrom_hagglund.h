#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t astrom_hagglund_start(float setpoint);

esp_err_t astrom_hagglund_get_pid(float *kp, float *ki, float *kd);

#ifdef __cplusplus
}
#endif 
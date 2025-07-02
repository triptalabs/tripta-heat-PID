#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ziegler_nichols_start(float setpoint);

esp_err_t ziegler_nichols_get_pid(float *kp, float *ki, float *kd);

#ifdef __cplusplus
}
#endif 
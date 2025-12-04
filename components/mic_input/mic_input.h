#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void mic_input_init(void);
size_t mic_input_read(int32_t *buffer, size_t samples);

#ifdef __cplusplus
}
#endif
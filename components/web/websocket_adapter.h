#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void websocket_start(void);
void websocket_stop(void);

void websocket_send_binary(const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#include "websocket_adapter.h"
#include "websocket_server.h"

void websocket_start(void)
{
    ws_server_start();
}

void websocket_stop(void)
{
    ws_server_stop();
}

void websocket_send_binary(const void *data, size_t len)
{
    ws_server_send_bin_all((char *)data, len);
}

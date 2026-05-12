#ifndef NOTIFY_HANDLER_H
#define NOTIFY_HANDLER_H
#include "mongoose.h"
void notify_list_handler(struct mg_connection *c, struct mg_http_message *hm);
void notify_read_handler(struct mg_connection *c, struct mg_http_message *hm);
#endif

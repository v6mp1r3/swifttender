#ifndef REPORT_HANDLER_H
#define REPORT_HANDLER_H
#include "mongoose.h"
void report_quarterly_handler(struct mg_connection *c, struct mg_http_message *hm);
void upload_handler          (struct mg_connection *c, struct mg_http_message *hm);
#endif

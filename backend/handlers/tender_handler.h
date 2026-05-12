#ifndef TENDER_HANDLER_H
#define TENDER_HANDLER_H
#include "../mongoose.h"
void tender_list_handler  (struct mg_connection *c, struct mg_http_message *hm);
void tender_create_handler(struct mg_connection *c, struct mg_http_message *hm);
void tender_get_handler   (struct mg_connection *c, struct mg_http_message *hm);
void tender_update_handler(struct mg_connection *c, struct mg_http_message *hm);
void tender_delete_handler(struct mg_connection *c, struct mg_http_message *hm);
#endif

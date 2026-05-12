#ifndef OFFER_HANDLER_H
#define OFFER_HANDLER_H
#include "../mongoose.h"
void offer_list_handler  (struct mg_connection *c, struct mg_http_message *hm);
void offer_create_handler(struct mg_connection *c, struct mg_http_message *hm);
void offer_winner_handler(struct mg_connection *c, struct mg_http_message *hm);
#endif

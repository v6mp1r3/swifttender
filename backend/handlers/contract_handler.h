#ifndef CONTRACT_HANDLER_H
#define CONTRACT_HANDLER_H
#include "../mongoose.h"
void contract_get_handler (struct mg_connection *c, struct mg_http_message *hm);
void contract_sign_handler(struct mg_connection *c, struct mg_http_message *hm);
void contract_docs_handler(struct mg_connection *c, struct mg_http_message *hm);
#endif

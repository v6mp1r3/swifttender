#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H
#include "../mongoose.h"
void auth_register_handler(struct mg_connection *c, struct mg_http_message *hm);
void auth_login_handler   (struct mg_connection *c, struct mg_http_message *hm);
void auth_me_handler      (struct mg_connection *c, struct mg_http_message *hm);
void auth_logout_handler  (struct mg_connection *c, struct mg_http_message *hm);
#endif

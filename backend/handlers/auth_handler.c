/*
 * auth_handler.c -- stub responses until feat(api): auth endpoints commit
 * Each function returns 501 Not Implemented with a descriptive message.
 */
#include "auth_handler.h"
#include "../router.h"

void auth_register_handler(struct mg_connection *c, struct mg_http_message *hm) {
    (void)hm;
    router_send_json(c, 501, "{\"error\":\"register not yet implemented\"}");
}
void auth_login_handler(struct mg_connection *c, struct mg_http_message *hm) {
    (void)hm;
    router_send_json(c, 501, "{\"error\":\"login not yet implemented\"}");
}
void auth_me_handler(struct mg_connection *c, struct mg_http_message *hm) {
    (void)hm;
    router_send_json(c, 501, "{\"error\":\"me not yet implemented\"}");
}
void auth_logout_handler(struct mg_connection *c, struct mg_http_message *hm) {
    (void)hm;
    router_send_json(c, 501, "{\"error\":\"logout not yet implemented\"}");
}

/* contract_handler.c -- stub until feat(api): contract endpoints */
#include "contract_handler.h"
#include "../router.h"

void contract_get_handler (struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void contract_sign_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void contract_docs_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }

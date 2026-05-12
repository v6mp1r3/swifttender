/* offer_handler.c -- stub until feat(api): offer endpoints */
#include "offer_handler.h"
#include "../router.h"

void offer_list_handler  (struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void offer_create_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void offer_winner_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }

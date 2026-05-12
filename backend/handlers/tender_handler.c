/* tender_handler.c -- stub until feat(api): tender endpoints */
#include "tender_handler.h"
#include "../router.h"

void tender_list_handler  (struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void tender_create_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void tender_get_handler   (struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void tender_update_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void tender_delete_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }

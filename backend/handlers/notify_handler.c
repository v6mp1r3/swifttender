/* notify_handler.c -- stub until feat(api): notification endpoints */
#include "notify_handler.h"
#include "../router.h"

void notify_list_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void notify_read_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }

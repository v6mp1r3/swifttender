/* report_handler.c -- stub until feat(api): report + upload endpoints */
#include "report_handler.h"
#include "../router.h"

void report_quarterly_handler(struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }
void upload_handler          (struct mg_connection *c, struct mg_http_message *hm) { (void)hm; router_send_json(c, 501, "{\"error\":\"not implemented\"}"); }

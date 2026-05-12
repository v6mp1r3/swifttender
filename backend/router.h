#ifndef ROUTER_H
#define ROUTER_H
struct mg_connection; struct mg_http_message;
void router_dispatch(struct mg_connection *c, struct mg_http_message *hm);
#endif

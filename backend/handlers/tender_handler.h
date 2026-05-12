#ifndef TENDER_HANDLER_H
#define TENDER_HANDLER_H

#include "mongoose.h"
#include "dsa/linked_list.h"

/* Global tender catalogue -- doubly linked list, init at startup */
extern LinkedList *g_tenders;

/* Called once from main.c to load thresholds + populate the list */
void tender_list_init(const char *data_dir);

/* Route handlers */
void tender_list_handler  (struct mg_connection *c, struct mg_http_message *hm);
void tender_create_handler(struct mg_connection *c, struct mg_http_message *hm);
void tender_get_handler   (struct mg_connection *c, struct mg_http_message *hm);
void tender_update_handler(struct mg_connection *c, struct mg_http_message *hm);
void tender_delete_handler(struct mg_connection *c, struct mg_http_message *hm);

#endif /* TENDER_HANDLER_H */

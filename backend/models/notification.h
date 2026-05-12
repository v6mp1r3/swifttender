#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <stdint.h>
#include <time.h>

typedef enum {
    NOTIF_WINNER        = 0,
    NOTIF_LOSER         = 1,
    NOTIF_NEW_TENDER    = 2,
    NOTIF_CONTRACT_READY= 3,
    NOTIF_DOC_REQUIRED  = 4,
    NOTIF_SIGNED        = 5
} NotifType;

typedef struct {
    uint32_t   id;
    uint32_t   user_id;       /* FK → User (recipient) */
    NotifType  type;
    uint32_t   tender_id;     /* FK → Tender (context) */
    char       message[512];
    int        is_read;
    time_t     created_at;
} Notification;

#endif /* NOTIFICATION_H */


#ifndef CONTRACT_H
#define CONTRACT_H

#include <stdint.h>
#include <time.h>

typedef enum {
    CONTRACT_DRAFT       = 0,
    CONTRACT_SIGNED_AUTH = 1,  /* signed by contracting authority */
    CONTRACT_SIGNED_BOTH = 2,  /* signed by both parties */
    CONTRACT_COMPLETED   = 3   /* delivery confirmed, docs uploaded */
} ContractStatus;

typedef struct {
    uint32_t       id;
    uint32_t       tender_id;    /* FK → Tender */
    uint32_t       offer_id;     /* FK → Offer  */
    char           file_path[256];
    ContractStatus status;
    time_t         created_at;
    time_t         signed_at;    /* 0 if not yet signed */
    int            active;
} Contract;

#endif /* CONTRACT_H */

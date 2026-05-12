CC      = gcc
CFLAGS  = -I. -std=c11 -Wall -Wextra -Wpedantic -g
LDFLAGS = -lm

TARGET  = swifttender

# All source files
SRCS =  main.c \
        mongoose.c \
        router.c \
        handlers/auth_handler.c \
        handlers/tender_handler.c \
        handlers/offer_handler.c \
        handlers/contract_handler.c \
        handlers/notify_handler.c \
        handlers/report_handler.c \
        dsa/hash_table.c \
        dsa/linked_list.c \
        dsa/heap.c \
        dsa/queue.c \
        dsa/binary_search.c \
        dsa/tree.c \
        models/user.c \
        models/tender.c \
        models/offer.c \
        storage/file_io.c \
        storage/upload.c \
        utils/json.c \
        utils/auth.c \
        utils/threshold.c \
        utils/ranking.c \
        utils/notify.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean run valgrind

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

valgrind: all
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)

# Note: notify.c and ranking.c added in feat(api) commits

#ifndef BAR_DRIVER_H
#define BAR_DRIVER_H

#include "bar.h"

#define BAR_STAFF 3
#define BAR_CUSTOMERS 10
#define DRINKS_PER_CUSTOMER 10
#define ORDER_BUFFER_SIZE 16

struct bar_order {
        int valid;
        int customer_id;
        int requested[DRINK_SIZE];
        int contents[DRINK_SIZE];
        struct semaphore *finished;
};

extern int runbar(int, char **);

void bar_open(void);
void bar_close(void);
void bar_customer_order(struct bar_order *order);
void bar_customer_done(void);
struct bar_order *bar_get_order(void);
void bar_fill_order(struct bar_order *order);

#endif

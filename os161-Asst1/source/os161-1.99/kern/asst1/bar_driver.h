#ifndef BAR_DRIVER_H
#define BAR_DRIVER_H

#include "bar.h"

#define BAR_STAFF 3              /* number of bartender threads */
#define BAR_CUSTOMERS 10         /* number of customer threads */
#define DRINKS_PER_CUSTOMER 10   /* each customer orders this many drinks */
#define ORDER_BUFFER_SIZE 16     /* bounded queue size for pending orders */

struct bar_order {
        int valid;                     /* 1 = real order, 0 = closing signal */
        int customer_id;               /* identifies who placed the order */
        int requested[DRINK_SIZE];     /* ingredients the customer asked for */
        int contents[DRINK_SIZE];      /* ingredients the bartender served */
        struct semaphore *finished;    /* customer waits here for this order */
};

extern int runbar(int, char **);

void bar_open(void);
void bar_close(void);
void bar_customer_order(struct bar_order *order);
void bar_customer_done(void);
struct bar_order *bar_get_order(void);
void bar_fill_order(struct bar_order *order);

#endif

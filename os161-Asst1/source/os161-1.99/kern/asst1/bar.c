/* Bar synchronization solution. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include "bar_driver.h"

static struct bar_order *order_buffer[ORDER_BUFFER_SIZE]; /* shared order queue */
static int order_in;                                      /* next enqueue slot */
static int order_out;                                     /* next dequeue slot */
static int customers_left;                                /* active customers */

static struct semaphore *empty_orders; /* free slots in order queue */
static struct semaphore *full_orders;  /* queued orders waiting for staff */
static struct lock *order_lock;        /* protects queue indices/count */
static struct lock *bottle_locks[NBOTTLES + 1]; /* one lock per bottle id */

static void
enqueue_order(struct bar_order *order)
{
        P(empty_orders);        /* wait until the order queue has space */
        lock_acquire(order_lock); /* protect queue write/index update */
        order_buffer[order_in] = order; /* place order in FIFO queue */
        order_in = (order_in + 1) % ORDER_BUFFER_SIZE; /* circular enqueue */
        lock_release(order_lock);
        V(full_orders);         /* wake a bartender waiting for an order */
}

static struct bar_order *
dequeue_order(void)
{
        struct bar_order *order;

        P(full_orders);         /* wait until at least one order exists */
        lock_acquire(order_lock); /* protect queue read/index update */
        order = order_buffer[order_out]; /* take oldest order */
        order_out = (order_out + 1) % ORDER_BUFFER_SIZE; /* circular dequeue */
        lock_release(order_lock);
        V(empty_orders);        /* signal one free queue slot */

        return order;
}

void
bar_open(void)
{
        int i;

        order_in = 0;                  /* reset queue write pointer */
        order_out = 0;                 /* reset queue read pointer */
        customers_left = BAR_CUSTOMERS; /* all customers start inside */

        empty_orders = sem_create("bar empty orders", ORDER_BUFFER_SIZE);
        KASSERT(empty_orders != NULL);

        full_orders = sem_create("bar full orders", 0); /* no orders yet */
        KASSERT(full_orders != NULL);

        order_lock = lock_create("bar order lock"); /* guards queue state */
        KASSERT(order_lock != NULL);

        for (i = 1; i <= NBOTTLES; i++) {
                bottle_locks[i] = lock_create("bar bottle lock"); /* per bottle */
                KASSERT(bottle_locks[i] != NULL);
        }
}

void
bar_close(void)
{
        int i;

        for (i = 1; i <= NBOTTLES; i++) {
                lock_destroy(bottle_locks[i]); /* release bottle resources */
        }

        lock_destroy(order_lock);
        sem_destroy(full_orders);
        sem_destroy(empty_orders);

        kprintf("The bar is closed, bye!!!\n");
}

void
bar_customer_order(struct bar_order *order)
{
        enqueue_order(order);   /* customer produces an order */
        P(order->finished);     /* wait until bartender fills this order */
}

void
bar_customer_done(void)
{
        int i;
        int last_customer;
        static struct bar_order closing_orders[BAR_STAFF];

        lock_acquire(order_lock);
        customers_left--;               /* this customer has left the bar */
        last_customer = customers_left == 0; /* last one must close queue */
        lock_release(order_lock);

        if (last_customer) {
                for (i = 0; i < BAR_STAFF; i++) {
                        closing_orders[i].valid = 0; /* poison pill order */
                        enqueue_order(&closing_orders[i]); /* stop one staff */
                }
        }
}

struct bar_order *
bar_get_order(void)
{
        return dequeue_order(); /* bartender consumes next queued order */
}

static void
sort_bottles(int bottles[], int nbottles)
{
        int i, j, tmp;

        for (i = 0; i < nbottles; i++) {
                for (j = i + 1; j < nbottles; j++) {
                        if (bottles[j] < bottles[i]) {
                                tmp = bottles[i];       /* simple ascending sort */
                                bottles[i] = bottles[j]; /* gives lock order */
                                bottles[j] = tmp;
                        }
                }
        }
}

void
bar_fill_order(struct bar_order *order)
{
        int bottles[DRINK_SIZE];
        int nbottles;
        int i;

        nbottles = 0;
        for (i = 0; i < DRINK_SIZE; i++) {
                if (order->requested[i] != NO_INGREDIENT) {
                        KASSERT(order->requested[i] >= 1);
                        KASSERT(order->requested[i] <= NBOTTLES);
                        bottles[nbottles++] = order->requested[i]; /* needed bottle */
                }
        }

        sort_bottles(bottles, nbottles); /* prevents circular wait deadlock */

        for (i = 0; i < nbottles; i++) {
                if (i == 0 || bottles[i] != bottles[i - 1]) {
                        lock_acquire(bottle_locks[bottles[i]]); /* reserve bottle */
                }
        }

        for (i = 0; i < DRINK_SIZE; i++) {
                order->contents[i] = order->requested[i]; /* fill the glass */
        }

        for (i = nbottles - 1; i >= 0; i--) {
                if (i == 0 || bottles[i] != bottles[i - 1]) {
                        lock_release(bottle_locks[bottles[i]]); /* free bottle */
                }
        }

        V(order->finished); /* wake the customer waiting for this drink */
}

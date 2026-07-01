#ifndef BAR_H
#define BAR_H

#define NO_INGREDIENT 0 /* empty ingredient slot in a drink */
#define BEER 1          /* bottle ids start at 1, matching output stats */
#define WINE 2
#define GIN 3
#define VODKA 4
#define RUM 5
#define TEQUILA 6
#define WHISKY 7
#define BRANDY 8
#define JUICE 9
#define TONIC 10

#define NBOTTLES 10    /* number of shared bottle locks/stat counters */
#define DRINK_SIZE 3   /* maximum ingredients per drink/order */

#endif

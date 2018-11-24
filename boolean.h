#ifndef BOOLEAN_H
#define BOOLEAN_H

/*
 * Make a Boolean type. C99 defines a _Bool data type which we could
 * use, but we are sticking to C89.
 */

typedef enum { False = 0, True = 1 } Boolean;

#endif

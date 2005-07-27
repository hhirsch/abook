#ifndef _XMALLOC_H
#define _XMALLOC_H

#include <stdlib.h> /* for size_t */

void		xmalloc_set_error_handler(void (*)(int));
void *		xmalloc(size_t);
void *		xmalloc0(size_t);
void *		xmalloc_inc(size_t, size_t);
void *		xrealloc(void *, size_t);
void		xfree(void *);

#endif

#ifndef _MISC_H
#define _MISC_H

char		*revstr(char *str);
char		*strupper(char *str);
char		*strlower(char *str);
char		*strtrim(char *);

char		*mkstr (const char *format, ... );
char		*strconcat (const char *str, ...);

int		safe_strcmp(const char *s1, const char * s2);

char		*my_getcwd();

char		*getaline(FILE *f);

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <stdarg.h>

#ifndef HAVE_SNPRINTF
int snprintf (char *str, size_t count, const char *fmt, ...);
#endif
#ifndef HAVE_VSNPRINTF
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
#endif

#endif

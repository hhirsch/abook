#ifndef _ABOOK_H
#define _ABOOK_H

void            refresh_screen();
void            statusline_msg(char *msg);
char		*ask_filename(char *prompt, int flags);
void            clear_statusline();
void		statusline_addstr(char *str);
char		*statusline_getnstr(char *str, int n, int use_filesel);
void		headerline(char *str);
void		refresh_statusline();
void		display_editor_help();
void		*abook_malloc(size_t size);
void		*abook_realloc(void *ptr, size_t size);
FILE		*abook_fopen (const char *path, const char *mode);
void		quit_abook();
#ifdef _AIX
int		strcasecmp (const char *, const char *);
int		strncasecmp (const char *, const char *, size_t);
#endif

#define MAIN_HELPLINE        "q:quit  ?:help  a:add  r:remove"

#define Y_STATUSLINE   	(LINES - 2)

#define MIN_LINES	20	
#define MIN_COLS	70	

#define DEFAULT_UMASK	066
#define DATAFILE	".abook.addressbook"

/*
 * some "abookwide" useful macros
 */

#define hide_cursor()	curs_set(0)
#define show_cursor()	curs_set(1)

#define safe_atoi(X)    (X == NULL) ? 0 : atoi(X)
#define my_free(X)	do {free(X); X=NULL;} while(0)
#define safe_str(X)	X == NULL ? "" : X
#define safe_strdup(X)	(X == NULL) ? NULL : strdup(X)

#ifndef min
#       define min(x,y) (((x)<(y)) ? (x):(y))
#endif

#ifndef max
#       define max(x,y) (((x)>(y)) ? (x):(y))
#endif

#define ISSPACE(c)	isspace((unsigned char)c)

#ifndef DEBUG
#	define NDEBUG	1
#else
#	undef NDEBUG
#endif

#endif

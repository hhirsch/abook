
/*
 * $Id$
 *  
 * by JH <jheinonen@bigfoot.com>
 *
 * Copyright (C) Jaakko Heinonen
 */


#define USE_FILESEL     1
/*#undef USE_FILESEL*/

#define ABOOK_SRC	1
/*#undef ABOOK_SRC*/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef ABOOK_SRC
#	include "abook_curses.h"
#	include "options.h"
#else
#	include <ncurses.h>
#endif
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include "estr.h"

#if defined(HAVE_LOCALE_H) || defined(HAVE_SETLOCALE)
#	include <locale.h>
#else
/*
 * this is a quick and dirty hack and it is
 * not likely to work well
 */
#	undef iscntrl
#	undef isprint
#	define iscntrl(X)       (X < 32)
#	define isprint(X)	(!iscntrl(X))
#endif

/*
 * hmmm, these should be universal and nice :)
 */

#define		XCTRL(x) ((x) & 31)

#define		ENTER_KEY	KEY_ENTER
#define		BACKSPACE_KEY	KEY_BACKSPACE
#define		CANCEL_KEY	XCTRL('g')
#define		TAB_KEY		'\t'

#ifdef ABOOK_SRC
#	include "abook.h"
#	define MALLOC(X)	abook_malloc(X)
#	define REALLOC(X, XX)	abook_realloc(X, XX)
#else
#	define MALLOC(X)	malloc(X)
#	define REALLOC(X, XX)	realloc(X, XX)
#endif

/* introduce filesel */
#ifdef USE_FILESEL
char            *filesel();
#endif

#define INITIAL_BUFSIZE		100

char *
wenter_string(WINDOW *win, const int maxlen, const int flags)
{
	int i = 0;
	int y, x;
	int ch;
	char *str = NULL;
	int size = maxlen > 0 ? maxlen + 1 : INITIAL_BUFSIZE; 

	getyx(win, y, x);
	str = MALLOC(size);

	for( ;; ) { /* main loop */
		if(flags & ESTR_DONT_WRAP && x+i>COLS-2) {
			mvwaddnstr(win, y, x, str+(1+x+i-COLS), COLS-x-1);
			wmove(win, y, COLS-1);
			wclrtoeol(win);
		} else
			wmove(win, y, x + i);

		wrefresh(win);

		switch( (ch = getch()) ) {
			case ENTER_KEY:
			case 10:
			case 13:
				goto out;
			case BACKSPACE_KEY:
			case 127:
				if(i > 0)
					mvwdelch(win, y, x + --i);
				continue;	
			case CANCEL_KEY:
				free(str);
				return NULL;
#ifdef USE_FILESEL
			case TAB_KEY:
				if( ! (flags & ESTR_USE_FILESEL) )
					continue;
				i = -1;
				free(str);
				str = filesel();
				goto out;
#endif
		}
		if( !isprint(ch) || iscntrl(ch)
			|| (maxlen > 0 && i + 1 > maxlen) )
			continue;
		str[i++] = ch;
		waddch(win,ch);
		if( i + 1 > size )
			str = REALLOC( str, size *= 2 );
		if( maxlen > 0 && i > maxlen)
			break;
	}
out:
	if( i >= 0 && str != NULL )
		str[i] = 0;
	
	return str;
}


#ifdef USE_FILESEL

/*
 * filesel.c
 * by JH <jheinonen@bigfoot.com>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


#define FILESEL_STATUSLINE      2

#define FILESEL_LIST_TOP        4
#define FILESEL_LIST_BOTTOM     (LINES-2)

#define FILESEL_LIST_LINES      (FILESEL_LIST_BOTTOM-FILESEL_LIST_TOP)
#define FILESEL_LIST_COLS       COLS

#define FILESEL_LAST_LIST_ITEM  ( filesel_first_list_item + FILESEL_LIST_LINES - 1 )


#define FLSL_TYPE_DIR		0
#define FLSL_TYPE_OTHER		1

struct filesel_list_item {
	char filename[256];
	char type;
};

#ifndef ABOOK_SRC

#include <errno.h>

static char *
my_getcwd()
{
	char *dir = NULL;
	int size = 100;

	dir = MALLOC(size);

	while( getcwd(dir, size) == NULL && errno == ERANGE )
		dir = REALLOC(dir, size *= 2);

	return dir;
}

#else
#	include "misc.h"
#endif


#define FILESEL_LAST_ITEM       (filesel_items -1)

int filesel_curitem = -1;
int filesel_first_list_item = -1;

int filesel_items=0;
struct filesel_list_item *lst = NULL ;

int filesel_list_capacity = 0;

WINDOW *filesel_list;

static void
filesel_close_list()
{
	free(lst);
	lst = NULL;
	filesel_items = 0;
	filesel_first_list_item = filesel_curitem = -1;
	filesel_list_capacity = 0;
}

static void
filesel_init_list()
{
	filesel_list = newwin(FILESEL_LIST_LINES, FILESEL_LIST_COLS,
			FILESEL_LIST_TOP, 0);
}

static void
init_filesel()
{
	if( isendwin() ) {
		initscr();
	}
	
	cbreak(); noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	filesel_init_list();
	clear();
}

static void
close_filesel()
{
	filesel_close_list();
	delwin(filesel_list);
	clear();
	refresh();
	endwin();
}

static void
filesel_check_list()
{
	if( filesel_curitem < 0 && filesel_first_list_item < 0 && filesel_items > 0 )
		filesel_curitem = filesel_first_list_item = 0;
}


static int
filesel_add_filesel_list_item(struct filesel_list_item item)
{
	if( ++filesel_items > filesel_list_capacity) {
		filesel_list_capacity =
		filesel_list_capacity < 1 ? 30:filesel_list_capacity << 1;
		lst = REALLOC(lst, filesel_list_capacity *
			sizeof(struct filesel_list_item) );
	}

	lst[FILESEL_LAST_ITEM] = item;

	filesel_check_list();
	
	return 0;
}


#define FILESEL_DIRS_FIRST
#define FILESEL_ALPHABETIC_ORDER

#ifdef FILESEL_ALPHABETIC_ORDER
#	ifndef FILESEL_DIRS_FIRST
#		define FILESEL_DIRS_FIRST
#	endif
#endif

#ifdef FILESEL_ALPHABETIC_ORDER
static int
filenamecmp(const void *a, const void *b)
{
	return strcmp( ((struct filesel_list_item *)a) -> filename,
			((struct filesel_list_item *)b) -> filename );
}
#endif

static void
filesel_sort_list()
{
#ifdef FILESEL_DIRS_FIRST
        int i, j, fdp=0;
        struct filesel_list_item tmp;

        while(lst[fdp].type == FLSL_TYPE_DIR)
        	if( ++fdp >= FILESEL_LAST_ITEM )
                	return;

	for(i = fdp + 1; i < filesel_items; i++) {
		if(lst[i].type == FLSL_TYPE_DIR ) {
			tmp = lst[fdp];
			lst[fdp++] = lst[i];
			for( j = i; j > fdp ; j--)
				lst[j] = lst[j - 1];
			lst[fdp] = tmp;
		}
	}
#endif
#ifdef FILESEL_ALPHABETIC_ORDER
#ifdef ABOOK_SRC
	if( options_get_int("filesel_sort") ) {
#endif
        if(fdp > 1)
		qsort((void *)lst, fdp, sizeof(struct filesel_list_item),
				filenamecmp );

	qsort((void *)lst + fdp * sizeof(struct filesel_list_item),
			FILESEL_LAST_ITEM - fdp + 1,
			sizeof(struct filesel_list_item),
			filenamecmp );
 
#ifdef ABOOK_SRC
	}
#endif
#endif
}        

static void
filesel_refresh_statusline()
{
	char *p = NULL;
	
	move(FILESEL_STATUSLINE,0);
	clrtoeol();

	mvaddnstr(FILESEL_STATUSLINE, 5,
			( p = my_getcwd() ), COLS-5 );

	free(p);
}

static void
filesel_highlight_line(WINDOW *win, int line)
{
	int i;

	wattron(win, A_STANDOUT);

	wmove(win, line, 0);
	for(i = 0; i < FILESEL_LIST_COLS; i++)
		waddch(win, ' ');
}

static void
filesel_print_list_line(int i, int line)
{
	if( lst[i].type == FLSL_TYPE_DIR )
		wattron(filesel_list, A_BOLD);
	
	mvwaddnstr(filesel_list, line, 5, lst[i].filename, COLS - 5);
}

static void
filesel_refresh_list()
{
	int i, line;
	
	werase(filesel_list);

	if( filesel_first_list_item < 0 || filesel_items < 1 ) {
		refresh();
		wrefresh(filesel_list);
		return;
	}

        for( line = 0, i = filesel_first_list_item ;
		i <= FILESEL_LAST_LIST_ITEM && i < filesel_items; line++, i++ ) {

		if(i == filesel_curitem) {
			filesel_highlight_line(filesel_list, line);
			wattron(filesel_list, A_STANDOUT);
		} else
			wattrset(filesel_list, 0);
		
		filesel_print_list_line(i, line);
        }

	refresh();
        wrefresh(filesel_list);
}	

static void
filesel_scroll_up()
{
	if( filesel_curitem < 1 )
		return;
	filesel_curitem--;

	if( filesel_first_list_item > 0 && filesel_curitem < filesel_first_list_item )
		filesel_first_list_item--;

	filesel_refresh_list();
}

static void
filesel_scroll_down()
{
	if( filesel_curitem > filesel_items - 2 )
		return;
	filesel_curitem++;

	if( filesel_curitem > FILESEL_LAST_LIST_ITEM )
		filesel_first_list_item++;

	filesel_refresh_list();
}


static void
filesel_page_up()
{
	if( filesel_curitem < 1 )
		return;
	
	if( filesel_curitem == filesel_first_list_item ) {
		filesel_curitem = (filesel_curitem -= FILESEL_LIST_LINES) < 0 ? 0 : filesel_curitem;
		filesel_first_list_item = filesel_curitem;
	} else
		filesel_curitem = filesel_first_list_item;
	
	filesel_refresh_list();
}

static void
filesel_page_down()
{
	if( filesel_curitem > filesel_items - 2 )
		return;

	if( filesel_curitem == FILESEL_LAST_LIST_ITEM ) {
		filesel_curitem = (filesel_curitem += FILESEL_LIST_LINES) > FILESEL_LAST_ITEM ?
			FILESEL_LAST_ITEM : filesel_curitem;
		filesel_first_list_item = filesel_curitem - FILESEL_LIST_LINES + 1;
		if( filesel_first_list_item < 0 )
			filesel_first_list_item = 0;
	} else
		filesel_curitem = FILESEL_LAST_LIST_ITEM < FILESEL_LAST_ITEM ?
			FILESEL_LAST_LIST_ITEM : FILESEL_LAST_ITEM;

	filesel_refresh_list();
}


static void
filesel_goto_home()
{
	if(filesel_items > 0) {
		filesel_first_list_item = 0;
		filesel_curitem = 0;
	}
	filesel_refresh_list();
}

static void
filesel_goto_end()
{
	if(filesel_items > 0) {
		filesel_curitem = FILESEL_LAST_ITEM;
		filesel_first_list_item = filesel_curitem - FILESEL_LIST_LINES + 1;
		if( filesel_first_list_item < 0 )
			filesel_first_list_item = 0;
	}
	filesel_refresh_list();
}


static int
filesel_read_dir()
{
	DIR *dir;
	struct dirent *entry;
	struct stat st;
	struct filesel_list_item item;

	dir = opendir(".");
	for(;;) {
		entry = readdir(dir);
		
		if(entry == NULL)
			break;

		stat(entry->d_name, &st);
		strcpy(item.filename, entry->d_name);
		item.type = S_ISDIR(st.st_mode) ?
			FLSL_TYPE_DIR : FLSL_TYPE_OTHER;
		filesel_add_filesel_list_item(item);
	}

	closedir(dir);
	filesel_sort_list();
	filesel_check_list();
	filesel_refresh_statusline();
	filesel_refresh_list();
	
	return 0;
}

static int
filesel_enter()
{
	char *dir, *newdir;

	if(lst[filesel_curitem].type == FLSL_TYPE_DIR) {
		dir = my_getcwd();
		newdir = MALLOC(strlen(dir)+strlen(lst[filesel_curitem].filename) +2 );
		strcpy(newdir, dir);
		strcat(newdir, "/");
		strcat(newdir,lst[filesel_curitem].filename);
		free(dir);
		if( (chdir(newdir)) != -1) {
			filesel_close_list();
			filesel_read_dir();
		}
		free(newdir);
		return 0;
	} else
		return 1;
}

static void
filesel_goto_item(int ch)
{
	int i;

	for(i=filesel_curitem+1; i<filesel_items; i++)
		if( lst[i].filename[0] == ch ) {
			filesel_curitem = i;
			break;
		}

	if( filesel_curitem > FILESEL_LAST_LIST_ITEM)
		filesel_first_list_item=filesel_curitem;

	filesel_refresh_list();
}


static int
filesel_loop()
{
	int ch;
	
	for(;;) {
		switch( (ch = getch()) ) {
			case '\t':
			case 'q': return 1;
			case 'R': filesel_close_list(); filesel_read_dir();
				  break;
			case 'k':
			case KEY_UP: filesel_scroll_up();	break;
			case 'j':
			case KEY_DOWN: filesel_scroll_down();	break;
			case 'K':
			case KEY_PPAGE: filesel_page_up();	break;
			case 'J':
			case KEY_NPAGE: filesel_page_down();	break;

			case KEY_HOME: filesel_goto_home();	break;
			case KEY_END: filesel_goto_end();	break;

			case '\n':
			case '\r': if( filesel_enter() ) {
					   return 0;
				}
				  break;
			default: filesel_goto_item(ch);
		}
	}
}

char *
filesel()
{
	char *dir, *tmp, *ptr = NULL;
	
        init_filesel();
	filesel_read_dir();

	if( !filesel_loop() ) {
		dir = my_getcwd();
		tmp = MALLOC(strlen(dir) + strlen(lst[filesel_curitem].filename) + 2);
		strcpy(tmp,dir);
		strcat(tmp, "/");
		strcat(tmp, lst[filesel_curitem].filename);
		ptr = strdup(tmp);
		free(tmp);
		free(dir);
	}
	
        close_filesel();

	return ptr;
}

#endif /* USE_FILESEL */


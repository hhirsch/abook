
/*
 * $Id$
 *
 * by JH <jheinonen@bigfoot.com>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdio.h>
#include <string.h>
#include "abook.h"
#include "ui.h"
#include "database.h"
#include "edit.h"
#include "list.h"
#include "misc.h"
#include "options.h"

int curitem = -1;
int first_list_item = -1;
char *selected = NULL;

extern int items;
extern list_item *database;
extern struct abook_field abook_fields[];

WINDOW *list = NULL;

void
init_list()
{
	list = newwin(LIST_LINES, LIST_COLS, LIST_TOP, 0);
	scrollok(list, TRUE);
}

void
close_list()
{
	delwin(list);
	list = NULL;
}

void
refresh_list()
{
	int i, line;
	
	werase(list);

	ui_print_number_of_items();
	
	if( items < 1 ) {
		refresh();
		wrefresh(list);
		return;
	}
	
	if(curitem < 0)
		curitem = 0;

	if(first_list_item < 0)
		first_list_item = 0;

	if(curitem < first_list_item)
		first_list_item = curitem;
	else if(curitem > LAST_LIST_ITEM)
		first_list_item = max(curitem - LIST_LINES + 1, 0);

        for( line = 0, i = first_list_item ; i <= LAST_LIST_ITEM && i < items;
			line++, i++ ) {

		if(i == curitem)
			highlight_line(list, line);
		
		print_list_line(i, line);

		wstandend(list);
        }

        wrefresh(list);
}

void
print_list_line(int i, int line)
{
	char tmp[MAX_EMAILSTR_LEN];
	int extra_column = options_get_int("extra_column");
	int real_emaillen = (extra_column > 2 && extra_column < ITEM_FIELDS) ?
		EMAILLEN : EMAILPOS - COLS;

	scrollok(list, FALSE);

	if( selected[i] )
		mvwaddch(list, line, 0, '*' );
	
	mvwaddnstr(list, line, NAMEPOS, database[i][NAME], NAMELEN);
	if( options_get_int( "show_all_emails"  ) )
		mvwaddnstr(list, line, EMAILPOS, database[i][EMAIL],
				real_emaillen);
	else {
		get_first_email(tmp, i);
		mvwaddnstr(list, line, EMAILPOS, tmp, real_emaillen);
	}

	if(extra_column > 2 && extra_column < ITEM_FIELDS) {
		if( !database[i][extra_column] ) {
			int extra_alternative =
				options_get_int("extra_alternative");
			
			if(extra_alternative > 2 &&
					extra_alternative < ITEM_FIELDS)
				extra_column = extra_alternative;
		}
		mvwaddnstr(list, line, EXTRAPOS,
				safe_str(database[i][extra_column]),
				EXTRALEN);
	}

	scrollok(list, TRUE);
}
	

void
list_headerline()
{
	int extra_column = options_get_int("extra_column");

	attrset(A_BOLD);
	mvaddstr(2, NAMEPOS, abook_fields[NAME].name);
	mvaddstr(2, EMAILPOS, abook_fields[EMAIL].name);
	if( extra_column > 2 && extra_column < ITEM_FIELDS )
		mvaddnstr(2, EXTRAPOS, abook_fields[extra_column].name,
				COLS-EXTRAPOS);
	attrset(A_NORMAL);
}

void
scroll_up()
{
	if( curitem < 1 )
		return;

	curitem--;

	refresh_list();
}

void
scroll_down()
{
	if( curitem > items - 2 )
		return;

	curitem++;

	refresh_list();
}


void
page_up()
{
	if( curitem < 1 )
		return;
	
	curitem = curitem == first_list_item ?
		((curitem -= LIST_LINES) < 0 ? 0 : curitem) : first_list_item;
	
	refresh_list();
}

void
page_down()
{
	if( curitem > items - 2 )
		return;

	curitem = curitem == LAST_LIST_ITEM ?
		((curitem += LIST_LINES) > LAST_ITEM ? LAST_ITEM : curitem) :
		min(LAST_LIST_ITEM, LAST_ITEM);

	refresh_list();
}


void
select_none()
{
        memset( selected, 0, items );
}

void
select_all()
{
        memset( selected, 1, items );
}

void
move_curitem(int direction)
{
        list_item tmp;

        if( curitem < 0 || curitem > LAST_ITEM )
                return;

        itemcpy(tmp, database[curitem]);

        switch(direction) {
                case MOVE_ITEM_UP:
                        if( curitem < 1 )
                                return;
                        itemcpy(database[curitem], database[curitem - 1]);
                        itemcpy(database[curitem-1], tmp);
                        scroll_up();
                        break;

                case MOVE_ITEM_DOWN:
                        if( curitem >= LAST_ITEM )
                                return;
                        itemcpy(database[curitem], database[curitem + 1]);
                        itemcpy(database[curitem+1], tmp);
                        scroll_down();
                        break;
        }
}

void
goto_home()
{
	if(items > 0)
		curitem = 0;
	
	refresh_list();
}

void
goto_end()
{
	if(items > 0)
		curitem = LAST_ITEM;

	refresh_list();
}


void
highlight_line(WINDOW *win, int line)
{
	wstandout(win);
	
#ifdef mvwchgat
	mvwchgat(win, line, 0, -1,  A_STANDOUT, 0, NULL);
#else
	/*
	 * buggy function: FIXME
	 */
	scrollok(win, FALSE);
	{
		int i;
		wmove(win, line, 0);
		for(i = 0; i < COLS; i++)
			waddch(win, ' ');
	/*wattrset(win, 0);*/
	}
	scrollok(win, TRUE);
#endif
}

int
selected_items()
{
	int i, n = 0;

	for(i = 0; i < items; i++)
		if(selected[i])
			n++;

	return n;
}

void
invert_selection()
{
	int i;

	if( items < 1 )
		return;

	for(i = 0; i < items; i++)
		selected[i] = !selected[i];
}

inline int
list_current_item()
{
	return curitem;
}

inline int
list_is_empty()
{
	return items < 1;
}

	

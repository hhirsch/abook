
/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdio.h>
#include <string.h>
#include "abook.h"
#include <assert.h>
#include "ui.h"
#include "database.h"
#include "edit.h"
#include "gettext.h"
#include "list.h"
#include "misc.h"
#include "options.h"
#include "xmalloc.h"


int curitem = -1;
int first_list_item = -1;
char *selected = NULL;

int extra_column = -1;
int extra_alternative = -1;

extern abook_field_list *fields_list;

static WINDOW *list = NULL;


int
init_extra_field(enum str_opts option)
{
	int ret = -1;
	char *option_str;

	option_str = opt_get_str(option);

	if(option_str && *option_str) {
		find_field_number(option_str, &ret);

		if(!strcmp(option_str, "name") || !strcmp(option_str, "email"))
			ret = -1;
	}

	return ret;
}

void
init_list()
{
	list = newwin(LIST_LINES, LIST_COLS, LIST_TOP, 0);
	scrollok(list, TRUE);

	/*
	 * init extra_column and extra alternative
	 */

	extra_column = init_extra_field(STR_EXTRA_COLUMN);
	extra_alternative = init_extra_field(STR_EXTRA_ALTERNATIVE);
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

	if(list_is_empty()) {
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

        for(line = 0, i = first_list_item;
			i <= LAST_LIST_ITEM && i < db_n_items();
			line++, i++) {

		print_list_line(i, line, i == curitem);
        }

	if(opt_get_bool(BOOL_SHOW_CURSOR)) {
		wmove(list, curitem - first_list_item, 0);
		/* need to call refresh() to update the cursor positions */
		refresh();
	}
        wrefresh(list);
}

void
print_list_line(int i, int line, int highlight)
{
	int extra = extra_column;
	char tmp[MAX_EMAILSTR_LEN], *emails;
	int real_emaillen = (extra_column > 0 || extra_alternative > 0) ?
		EMAILLEN : COLS - EMAILPOS;

	scrollok(list, FALSE);
	if(highlight)
		highlight_line(list, line);

	if(selected[i])
		mvwaddch(list, line, 0, '*' );

	mvwaddnstr(list, line, NAMEPOS, db_name_get(i),
		bytes2width(db_name_get(i), NAMELEN));

	if(opt_get_bool(BOOL_SHOW_ALL_EMAILS)) {
		emails = db_email_get(i);
		mvwaddnstr(list, line, EMAILPOS, emails,
				bytes2width(emails, real_emaillen));
		free(emails);
	} else {
		get_first_email(tmp, i);
		mvwaddnstr(list, line, EMAILPOS, tmp,
				bytes2width(tmp, real_emaillen));
	}

	if(extra < 0 || !db_fget_byid(i, extra))
		extra = extra_alternative;
	if(extra >= 0)
		mvwaddnstr(list, line, EXTRAPOS,
			safe_str(db_fget_byid(i, extra)),
			bytes2width(safe_str(db_fget_byid(i, extra)),
			EXTRALEN));

	scrollok(list, TRUE);
	if(highlight)
		wstandend(list);
}

void
list_headerline()
{
	char *str = NULL;

#if defined(A_BOLD) && defined(A_NORMAL)
	attrset(A_BOLD);
#endif

	mvaddstr(2, NAMEPOS, find_field("name", NULL)->name);
	mvaddstr(2, EMAILPOS, find_field("email", NULL)->name);
	if(extra_column > 0) {
		get_field_keyname(extra_column, NULL, &str);
		mvaddnstr(2, EXTRAPOS, str, COLS - EXTRAPOS);
	}

#if defined(A_BOLD) && defined(A_NORMAL)
	attrset(A_NORMAL);
#endif
}

void
scroll_up()
{
	if(curitem < 1)
		return;

	curitem--;

	refresh_list();
}

void
scroll_down()
{
	if(curitem > db_n_items() - 2)
		return;

	curitem++;

	refresh_list();
}


void
page_up()
{
	if(curitem < 1)
		return;

	curitem = curitem == first_list_item ?
		((curitem -= LIST_LINES) < 0 ? 0 : curitem) : first_list_item;

	refresh_list();
}

void
page_down()
{
	if(curitem > db_n_items() - 2)
		return;

	if(curitem == LAST_LIST_ITEM) {
		if((curitem += LIST_LINES) > last_item())
			curitem = last_item();
	} else {
		curitem = min(LAST_LIST_ITEM, last_item());
	}

	refresh_list();
}

void
select_none()
{
        memset(selected, 0, db_n_items());
}

void
select_all()
{
        memset(selected, 1, db_n_items());
}

void
list_set_selection(int item, int value)
{
	assert(is_valid_item(item));

	selected[item] = !!value;
}

void
list_invert_curitem_selection()
{
	assert(is_valid_item(curitem));

	selected[curitem] = !selected[curitem];
}

void
move_curitem(int direction)
{
        list_item tmp;

        if(curitem < 0 || curitem > last_item())
                return;

	tmp = item_create();
	item_copy(tmp, db_item_get(curitem));

	switch(direction) {
		case MOVE_ITEM_UP:
			if( curitem < 1 )
				goto out_move;
			item_copy(db_item_get(curitem),
					db_item_get(curitem - 1));
			item_copy(db_item_get(curitem-1), tmp);
			scroll_up();
			break;

		case MOVE_ITEM_DOWN:
			if(curitem >= last_item())
				goto out_move;
			item_copy(db_item_get(curitem),
					db_item_get(curitem + 1));
			item_copy(db_item_get(curitem + 1), tmp);
			scroll_down();
			break;
	}

out_move:
	item_free(&tmp);
}

void
goto_home()
{
	if(db_n_items() > 0)
		curitem = 0;

	refresh_list();
}

void
goto_end()
{
	if(db_n_items() > 0)
		curitem = last_item();

	refresh_list();
}

void
highlight_line(WINDOW *win, int line)
{
	wstandout(win);

	/*
	 * this is a tricky one
	 */
#if 0
/*#ifdef mvwchgat*/
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

	for(i = 0; i < db_n_items(); i++)
		if(selected[i])
			n++;

	return n;
}

void
invert_selection()
{
	int i;

	if(list_is_empty())
		return;

	for(i = 0; i < db_n_items(); i++)
		selected[i] = !selected[i];
}

int
list_is_empty()
{
	return db_n_items() < 1;
}

int
list_get_curitem()
{
	return curitem;
}

void
list_set_curitem(int i)
{
	curitem = i;
}

int
duplicate_item()
{
	list_item item;
	
	if(curitem < 0)
		return 1;

	item = item_create();
	item_duplicate(item, db_item_get(curitem));
	if(add_item2database(item)) {
		item_free(&item);
		return 1;
	}
	item_free(&item);

	curitem = last_item();
	refresh_list();

	return 0;
}


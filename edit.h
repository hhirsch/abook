#ifndef _EDIT_H
#define _EDIT_H

void		edit_item(int item);
void		get_first_email(char *str, int item);
void		add_item();

#define EDITW_COLS	(COLS - 6)
#define EDITW_LINES	(LINES - 5)
#define EDITW_TOP	2
#define EDITW_X		3

#define EDITOR_HELPLINE	"?:help c:contact a:address p:phone o:other"

#define TABLINE		2

#define MAX_TAB_LINES	7

#define TAB_COLON_POS	28
#define TAB_START_Y	5
#define TAB_START_X	(EDITW_COLS > 70 ? 8:4)

enum {
	TAB_CONTACT,
	TAB_ADDRESS,
	TAB_PHONE,
	TAB_OTHER
};

#define MAX_TAB		TAB_OTHER
	
#define TABS		(MAX_TAB+1)

#endif

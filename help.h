#ifndef _HELP_H
#define _HELP_H

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

static char *mainhelp[] = {

"	?		help\n",
"	q		quit\n",
"	^L		refresh screen\n",
"\n",
"	arrows / j,k	scroll list\n",
"	enter		view/edit item\n",
"	a		add item\n",
"	r / del		remove selected items\n",
"\n",
"	space		select item\n",
"	+		select all\n",
"	-		select none\n",
"	*		invert selection\n",
"\n",
"	w		write database to disk\n",
"	l		read database from disk\n",
"	C		clear whole database\n",
"	i		import database\n",
"	e		export database\n",
"	p		print database\n",
"	o		open database\n",
"\n",
"	s		sort database\n",
"	S		\"surname sort\"\n",
"\n",
"	/		find\n",
"	\\		find next\n",
"\n",
"	A		move current item up\n",
"	Z		move current item down\n",
"\n",
"	m		send mail with mutt\n",
"	u		view URL with www browser\n",
NULL

};

static char *editorhelp[] = {

"\n",
"	a,c,p,o / left,right	change tab\n",
"\n",
"	1 - 5			edit fields\n",
"\n",
"	r			roll e-mail addresses\n",
"\n",
"	u			undo\n",
"\n",
"	m			send mail with mutt\n",
"	v			view url with WWW browser\n",
"\n",
NULL

};

#endif

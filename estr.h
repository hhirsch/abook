
/*
 * $Id$
 *
 * by JH <jheinonen@bigfoot.com>
 *
 * Copyright (C) Jaakko Heinonen
 *
 */

#ifndef _ESTR_H
#define _ESTR_H


/*
 *
 *
 */

char	*wenter_string(WINDOW *win, const int maxlen, const int flags);

#define		ESTR_USE_FILESEL	1
#define		ESTR_DONT_WRAP		2

#endif


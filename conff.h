
/*
 *
 *  $Id$
 *
 *  Copyright (C) Jaakko Heinonen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _CONFF_H
#define _CONFF_H

#include <stdlib.h>


struct conff_node
{
	char *key, *value;

	struct conff_node *next;
};


int		conff_add_key(struct conff_node **ptr, char *key,
		char *value, int flags);
char		*conff_get_value(struct conff_node *node, char *key);
#ifdef DEBUG
void		print_values(struct conff_node *root);
#endif
void		conff_free_nodes(struct conff_node *node);
void		conff_remove_key(struct conff_node **node, char *key);
int		conff_save_file(struct conff_node *node, char *filename);
int		conff_load_file(struct conff_node **node,
		char *filename, int flags);
char		*strtrim(char *);


#define DONT_REPLACE_KEY	0
#define REPLACE_KEY		1

#endif

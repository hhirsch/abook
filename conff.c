
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

/*
 * conff.c version 0.3.0
 */

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "misc.h"
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include "conff.h"

#ifndef DEBUG
#	define NDEBUG	1
#else
#	undef NDEBUG
#endif

#include <assert.h>

#ifdef _AIX
int strcasecmp (const char *, const char *);
int strncasecmp (const char *, const char *, size_t);
#endif

#define COMMENT_CHAR	'#'

static void
conff_free_node(struct conff_node *node)
{
	free(node -> value);
	free(node -> key);
	free(node);
}

void
conff_add_key(struct conff_node **ptr, char *key, char *value, int flags)
{
	struct conff_node *new_item, *next = NULL;

	assert(key != NULL && value != NULL);

	for(; *ptr; ptr = &( (*ptr) -> next) ) 
		if(!strcasecmp(key, (*ptr) -> key ) ) {
			if (flags & REPLACE_KEY) {
				next = (*ptr) -> next;
				conff_free_node(*ptr);
				break;
			} else
				return;
		}
	
	 /*
	  * out of memory - error is ingnored
	  * NOTE: with REPLACE_KEY flag the node will be deleted in OOM
	  * situation
	  */
	if( (new_item = malloc(sizeof(struct conff_node))) == NULL )
		return;
	
	new_item -> key = strdup(key);
	new_item -> value = strdup(value);
	new_item -> next = next;

	*ptr = new_item;
}

char *
conff_get_value(struct conff_node *node, char *key)
{

	assert(key != NULL);
	
	for(; node ; node = node -> next) {
		if(!strcasecmp(node -> key, key))
			return node -> value;
	}

	return NULL; /* not found */
}

void
conff_free_nodes(struct conff_node *node)
{
	if(node != NULL) {
		conff_free_nodes( node -> next );
		conff_free_node( node );
	}
}

#ifdef DEBUG
void
print_values(struct conff_node *node)
{
        for(;node; node = node -> next)
		fprintf(stderr, "%s - %s\n", node -> key, node -> value);
}
#endif

void
conff_remove_key(struct conff_node **node, char *key)
{
	assert(key != NULL);
	
	for(; *node; node = &((*node) -> next) ) {
		if(!strcasecmp(key, (*node) -> key ) ) {
			struct conff_node *tmp = *node;
			*node = (*node) -> next;
			conff_free_node(tmp);
			return;
		}
	}
}

int
conff_save_file(struct conff_node *node, char *filename)
{
	FILE *out;

	assert(filename != NULL);

	if (!(out = fopen(filename, "w")))
		return -1;

	for(; node; node = node -> next)
		fprintf(out, "%s=%s\n", node -> key, node -> value);

	fputc('\n', out);
	fclose(out);

	return 0;
}

int
conff_load_file(struct conff_node **node, char *filename, int flags)
{
	FILE *in;
	char *line = NULL, *tmp;
	int i = 0;

	if (!(in = fopen(filename, "r")))
		return -1;

	for(;;) {
		i++;

		line = getaline(in);
		if( feof(in) )
			break;
		if(!line)
			continue;

		strtrim(line);

		if(*line == '\n' || *line == '\0' || *line == COMMENT_CHAR) {
			free(line);
			continue;
		}

		if ( (tmp = strchr(line, '=') )) {
			*tmp++ = 0;
			conff_add_key(node, strtrim(line), strtrim(tmp), flags);
		} else {
/*                      fprintf(stderr, "parse error2,line #%d\n",i);*/
			fclose(in);
			free(line);
			return i;
		}
		free(line);
	}

	free(line);
	fclose(in);

	return 0;
}


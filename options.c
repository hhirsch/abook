
/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "abook_curses.h"
#include "abook.h"
#include "options.h"
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

struct conff_node *abook_config;

static int	rcfile_exist();
static void	default_options();

extern char *rcfile;

static char *
abook_opt_conff_get_val(char *key)
{
	int tried;
	char *value = NULL;


	for(tried = 0; tried < 2; ) {
		if( ( value = conff_get_value(abook_config, key) )
				== 0 ) {
			tried ++;
			default_options(); /* try with defaults */
		} else
			return value;
	}
	return NULL;
}

int
options_get_int(char *key)
{
	char *value;
	int ret;
	
	if( ( value = abook_opt_conff_get_val(key) )
			== NULL)
		return 1;

	if( !strcasecmp(value, "true") )
		ret = 1;
	else
	if( !strcasecmp(value, "false") )
		ret = 0;
	else
		ret = safe_atoi(value);
	
	return ret;
}
	
char *
options_get_str(char *key)
{
	return abook_opt_conff_get_val(key); 
}
		
void
init_options()
{
	abook_config = NULL;

	if( rcfile_exist() )
		load_options();
	else
		default_options();
}


#if 1
extern int alternative_rcfile;
#endif

void
close_config()
{
#if 1
	if(!alternative_rcfile)
		save_options();
#endif

	conff_free_nodes(abook_config);
}

static int
rcfile_exist()
{
	return ( (0 == access(SYSWIDE_RCFILE, F_OK)) ||
			(0 == access(rcfile, F_OK)) );
}

void
load_options()
{
	int ret;
	
        if( (ret = conff_load_file(&abook_config, rcfile,
				REPLACE_KEY)) > 0) {
		fprintf(stderr, "%s: parse error at line %d\n", rcfile, ret);
		exit(1);
	}

	if( (ret = conff_load_file(&abook_config, SYSWIDE_RCFILE,
					DONT_REPLACE_KEY )) > 0) {
		fprintf(stderr, "%s: parse error at line %d\n",
				SYSWIDE_RCFILE, ret);
		exit(1);
	}
}

#if 1
void
save_options()
{
	if( rcfile_exist() ) /* don't overwrite existing config */
		return;

	conff_save_file(abook_config, rcfile);
}
#endif

static void
options_add_key(char *key, char *value)
{
	const int flags = DONT_REPLACE_KEY;

	conff_add_key(&abook_config, key, value, flags);
}

static void
default_options()
{
	options_add_key("autosave", "true");

	options_add_key("show_all_emails", "true");
	options_add_key("emailpos", "25");
	options_add_key("extra_column", "7");
	options_add_key("extra_alternative", "-1");
	options_add_key("extrapos", "65");

	options_add_key("mutt_command", "mutt");
	options_add_key("mutt_return_all_emails", "true");

	options_add_key("print_command", "lpr");

	options_add_key("filesel_sort", "false");

	options_add_key("www_command", "lynx");

	options_add_key("address_style", "eu");

	options_add_key("use_ascii_only", "false");
}

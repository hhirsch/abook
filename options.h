#ifndef _OPTIONS_H
#define _OPTIONS_H

#define RCFILE		"abookrc.0"
#define SYSWIDE_RCFILE	"/etc/abookrc"

#include "conff.h"

int	options_get_int(char *key);
char	*options_get_str(char *key);
void	init_options();
void	close_config();
void	load_options();
void	save_options();

#endif

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef ENABLE_NLS
#include <string.h>
#include "gettext.h"

char *
sgettext(const char *msgid)
{
	char *msgval = gettext(msgid);

	if(msgval == msgid)
		msgval = strrchr(msgid, '|') + 1;

	return msgval;
}
#endif

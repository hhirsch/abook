#ifndef _GETTEXT_H
#define _GETTEXT_H

# if ENABLE_NLS
#  include <libintl.h>
# else
#  define gettext(Msgid) ((const char *) (Msgid))

#  define textdomain(Domainname) ((const char *) (Domainname))
#  define bindtextdomain(Domainname, Dirname) ((const char *) (Dirname))
# endif

# define _(String) gettext (String)
# define gettext_noop(String) String
# define N_(String) gettext_noop (String)

#endif

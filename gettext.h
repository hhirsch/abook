#ifndef _GETTEXT_H
#define _GETTEXT_H

# if ENABLE_NLS
#  include <libintl.h>
char *sgettext(const char *msgid); /* Strip context prefix */
# else
#  define gettext(Msgid) ((const char *) (Msgid))
#  define sgettext(Msgid) ((const char *) (Msgid))

#  define textdomain(Domainname) ((const char *) (Domainname))
#  define bindtextdomain(Domainname, Dirname) ((const char *) (Dirname))
# endif /* ENABLE_NLS */

# define _(String) gettext (String)
# define gettext_noop(String) String
# define N_(String) gettext_noop (String)
# define S_(String) sgettext (String)

#endif /* _GETTEXT_H */

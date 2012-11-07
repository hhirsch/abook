
bin_PROGRAMS = abook

abook_SOURCES = abook.c abook_rl.c database.c edit.c \
		filter.c getname.c getopt.c getopt1.c gettext.c \
		ldif.c list.c mbswidth.c misc.c options.c \
		ui.c vcard.c views.c xmalloc.c \
		\
		abook.h abook_curses.h abook_rl.h database.h edit.h \
		filter.h getname.h getopt.h gettext.h \
		help.h list.h ldif.h mbswidth.h misc.h options.h \
		ui.h vcard.h views.h xmalloc.h

EXTRA_DIST = config.rpath  ANNOUNCE BUGS FAQ abook.1 abookrc.5 sample.abookrc \
		abook.spec contrib doc/HOWTO.translating_abook RELEASE_NOTES

abook_LDADD = @LIBINTL@


install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man1 $(DESTDIR)$(mandir)/man5
	$(INSTALL_DATA) $(srcdir)/abook.1 $(DESTDIR)$(mandir)/man1/
	$(INSTALL_DATA) $(srcdir)/abookrc.5 $(DESTDIR)$(mandir)/man5/

uninstall-local:
	-rm -f $(DESTDIR)$(mandir)/man1/abook.1
	-rm -f $(DESTDIR)$(mandir)/man5/abookrc.5


SUBDIRS = intl po

ACLOCAL_AMFLAGS = -I m4

if USE_INCLUDED_INTL_H
AM_CPPFLAGS = -Iintl
endif

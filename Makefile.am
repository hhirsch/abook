
bin_PROGRAMS = abook

abook_SOURCES = abook.c database.c filter.c list.c misc.c conff.c \
		options.c edit.c ldif.c estr.c ui.c getname.c \
		getopt.c getopt1.c \
 		abook.h database.h filter.h list.h misc.h help.h conff.h \
		options.h edit.h ldif.h estr.h abook_curses.h ui.h getname.h \
		getopt.h

EXTRA_DIST = ANNOUNCE BUGS FAQ abook.1 abookrc.5 sample.abookrc

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man1 $(DESTDIR)$(mandir)/man5
	$(INSTALL_DATA) $(srcdir)/abook.1 $(DESTDIR)$(mandir)/man1/
	$(INSTALL_DATA) $(srcdir)/abookrc.5 $(DESTDIR)$(mandir)/man5/

uninstall-local:
	-rm -f $(DESTDIR)$(mandir)/man1/abook.1
	-rm -f $(DESTDIR)$(mandir)/man5/abookrc.5

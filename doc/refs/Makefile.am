include $(top_srcdir)/Makefile.am.global

SUBDIRS=. library dwi
dist_noinst_SCRIPTS=doxyfile.local
CLEANFILES=doxyfile.local
dist_noinst_DATA=doxyfile.in

do_subst=sed -e 's,[@]TOP_PATH[@],$(TOP_PATH),g'

doxyfile.local: doxyfile.in
	$(do_subst) < doxyfile.in > doxyfile.local


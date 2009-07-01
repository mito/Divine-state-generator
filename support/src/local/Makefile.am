include $(top_srcdir)/Makefile.am.global

noinst_SCRIPTS=$(top_srcdir)/bin/publish_distribution.sh \
	       $(top_srcdir)/bin/delete_distribution.sh \
	       $(top_srcdir)/bin/refresh_doc.sh \
	       $(top_srcdir)/bin/link_module_divine.sh \
	       $(top_srcdir)/bin/print_releases_list.sh
CLEANFILES=$(noinst_SCRIPTS)

$(top_srcdir)/bin/publish_distribution.sh: publish_distribution.sh
	cp publish_distribution.sh $(top_srcdir)/bin/
	chmod 755 $(top_srcdir)/bin/publish_distribution.sh
	
$(top_srcdir)/bin/delete_distribution.sh: delete_distribution.sh
	cp delete_distribution.sh $(top_srcdir)/bin/
	
$(top_srcdir)/bin/refresh_doc.sh: refresh_doc.sh
	cp refresh_doc.sh $(top_srcdir)/bin/

$(top_srcdir)/bin/link_module_divine.sh: link_module_divine.sh
	cp link_module_divine.sh $(top_srcdir)/bin/

$(top_srcdir)/bin/print_releases_list.sh: print_releases_list.sh
	cp print_releases_list.sh $(top_srcdir)/bin/



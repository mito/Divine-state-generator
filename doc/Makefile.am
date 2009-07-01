include $(top_srcdir)/Makefile.am.global

SUBDIRS=refs tutorials

dist_pkgdoc_DATA=documentation.txt styles.css
dist_noinst_DATA=index.html #installation using hook, because we do not want
                            #to 'compile' it during installation
CLEANFILES=index.html styles.css

index.html: always_compile
	$(TOP_PATH)/support/src/makeutils/make_documentation_index.sh $(TOP_PATH)/doc $(TOP_PATH)/support/etc/doxygen/main.css

.PHONY:	always_compile
always_compile:

install-data-hook:
	cp index.html $(pkgdocdir)/


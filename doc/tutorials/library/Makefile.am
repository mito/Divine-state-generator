DOXYGEN_MORE_CONFIG_FILES=../doxyfile.local ../../refs/library/library_reference.tag

include $(top_srcdir)/doc/Makefile.am.doxygen

../doxyfile.local:
	cd .. && $(MAKE) doxyfile.local

../../refs/library/library_reference.tag:
	cd ../../refs/library && \
	sh -c 'if [ ! -e library_reference.tag ]; then $(MAKE); fi'


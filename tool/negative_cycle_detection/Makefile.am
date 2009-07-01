CXX=$(MPICXX)
include $(top_srcdir)/Makefile.am.global

noinst_HEADERS=counterexample.hh process_messages.hh remove_marks.hh shared_global.hh update_distance.hh walk_to_root.hh statistics.hh counterexample.hh

bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)negative_cycle_detection
__top_srcdir__bin___BINPREFIX_negative_cycle_detection_SOURCES = negative_cycle_detection.cc process_messages.cc remove_marks.cc shared_global.cc update_distance.cc walk_to_root.cc statistics.cc counterexample.cc



LDADD = $(DIVINE_LIB) $(PROMELA_LIB)


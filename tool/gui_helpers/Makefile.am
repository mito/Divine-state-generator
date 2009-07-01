include $(top_srcdir)/Makefile.am.global

#noinst_HEADERS=counterexample.hh process_messages.hh remove_marks.hh shared_global.hh update_distance.hh walk_to_root.hh statistics.hh counterexample.hh

bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)code_positions \
	       $(top_srcdir)/bin/$(BINPREFIX)reformat_ce_states
__top_srcdir__bin___BINPREFIX_code_positions_SOURCES = code_positions.cc
__top_srcdir__bin___BINPREFIX_reformat_ce_states_SOURCES = reformat_ce_states.cc

LDADD = $(SEVINE_LIB)


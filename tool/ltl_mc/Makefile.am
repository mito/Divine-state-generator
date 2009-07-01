CXX=$(MPICXX)
include $(top_srcdir)/Makefile.am.global
bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)ltl_mc

nodist_noinst_HEADERS=ac_detect_alg.hh back_level_edges_alg.hh deps_cycles_alg.hh map_alg.hh neg_cycle_alg.hh owcty_alg.hh owcty_reversed_alg.hh prop_driven_ndfs_alg.hh token_based_ndfs_alg.hh

nodist___top_srcdir__bin___BINPREFIX_ltl_mc_SOURCES = ltl_mc_main.cc ltl_mc.cc ac_detect_alg.cc distr_ostream.cc owcty_alg.cc owcty_reversed_alg.cc neg_cycle_alg.cc system_creator.cc ltl_mc_options.cc 
LDADD = $(DIVINE_LIB) $(PROMELA_LIB)


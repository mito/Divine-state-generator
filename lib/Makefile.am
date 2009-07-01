include $(top_srcdir)/Makefile.am.global

CXX=$(MPICXX)
# Automakes's variable for creating libraries:
lib_LIBRARIES = libsevine.a libbymoc_vm.a
if HAVEMPI
lib_LIBRARIES += libdivine.a
endif
# Custom variable to become the prefix of paths:
tsrc=$(top_srcdir)/src

if HAVEMCRL2
mcrl2_sources = $(tsrc)/system/mcrl2_system.cc \
		$(tsrc)/system/mcrl2_explicit_system.cc \
		$(tsrc)/system/mcrl2_system_trans.cc
endif

# Paths to sources for sequential part of the design
sequential_part_sources = $(tsrc)/common/reporter.cc \
                          $(tsrc)/common/inttostr.cc \
                          $(tsrc)/common/error.cc \
                          $(tsrc)/common/sysinfo.cc \
                          $(tsrc)/common/hash_function.cc \
			  $(tsrc)/common/sysopen.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_expr_grammar.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_symbol_table.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_grammar.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_symbol.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_lexer.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_parser.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_trans_grammar.cc \
                          $(tsrc)/system/dve/syntax_analysis/dve_proc_grammar.cc \
			  $(tsrc)/system/bymoc/bymoc_expression.cc \
			  $(tsrc)/system/bymoc/bymoc_transition.cc \
			  $(tsrc)/system/bymoc/bymoc_process.cc \
			  $(tsrc)/system/bymoc/bymoc_system_trans.cc \
			  $(tsrc)/system/bymoc/bymoc_system.cc \
			  $(tsrc)/system/bymoc/bymoc_explicit_system.cc \
			  $(tsrc)/system/bymoc/bymoc_process_decomposition.cc \
                          $(tsrc)/system/transition.cc \
                          $(tsrc)/system/dve/dve_explicit_system.cc \
                          $(tsrc)/system/dveC/dveC_explicit_system.cc \
                          $(tsrc)/system/dve/dve_process.cc \
                          $(tsrc)/system/dve/dve_system_trans.cc \
                          $(tsrc)/system/dve/dve_transition.cc \
                          $(tsrc)/system/dve/dve_system.cc \
                          $(tsrc)/system/dve/dve_expression.cc \
                          $(tsrc)/system/dve/dve_source_position.cc \
			  $(tsrc)/system/dve/dve_process_decomposition.cc \
			  $(tsrc)/system/dve/dve_prob_process.cc \
			  $(tsrc)/system/dve/dve_prob_transition.cc \
			  $(tsrc)/system/dve/dve_prob_system.cc \
			  $(tsrc)/system/dve/dve_prob_explicit_system.cc \
			  $(tsrc)/system/prob_system.cc \
			  $(tsrc)/system/prob_transition.cc \
                          $(tsrc)/system/state.cc \
                          $(tsrc)/system/path.cc \
                          $(tsrc)/system/system.cc \
                          $(tsrc)/system/system_trans.cc \
                          $(tsrc)/system/process.cc \
                          $(tsrc)/storage/compressor.cc \
                          $(tsrc)/storage/explicit_storage.cc \
			  $(tsrc)/por/spor.cc \
                          $(tsrc)/por/por.cc \
			  $(mcrl2_sources)

# Paths to sources for the distributed part of the design (using MPI)
distributed_part_sources = $(tsrc)/common/distr_reporter.cc \
			   $(tsrc)/distributed/message.cc \
                           $(tsrc)/distributed/network.cc \
                           $(tsrc)/distributed/distributed.cc \
                           $(tsrc)/common/logger.cc
libsevine_a_SOURCES = $(sequential_part_sources)

if HAVEMPI
libdivine_a_SOURCES = $(sequential_part_sources) $(distributed_part_sources)
endif

libbymoc_vm_a_SOURCES = $(tsrc)/system/bymoc/vm/bytecode.c \
                        $(tsrc)/system/bymoc/vm/hashtab.c \
			$(tsrc)/system/bymoc/vm/instr.c \
			$(tsrc)/system/bymoc/vm/instr_tools.c \
			$(tsrc)/system/bymoc/vm/instr_wrap.c \
			$(tsrc)/system/bymoc/vm/rt_err.c \
			$(tsrc)/system/bymoc/vm/state_promela.c \
			$(tsrc)/system/bymoc/vm/instr_step.c \
			$(tsrc)/system/bymoc/vm/nipsvm.c \
			$(tsrc)/system/bymoc/vm/state_parts.c



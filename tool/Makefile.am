if BUILDSUPPORT
  WHERE_TO_GO=ltl_mc
endif  

if HAVEMPI
  MPIGO=token_based_ndfs \
	distr_map \
	distr_reachability \
	property_driven_ndfs \
	deps_cycle_detection \
	back_level_edge \
	owcty_reversed \
	owcty \
	negative_cycle_detection \
	$(WHERE_TO_GO)
endif

SUBDIRS=simulator \
	ltl2ba \
	lmu2ba \
	reachability \
	model_manipulation \
	state_spaces \
	utils \
	gui_helpers \
	ndfs \
	$(MPIGO)

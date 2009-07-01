#include "statistics.hh"
#include "shared_global.hh"

using namespace divine;

updateable_info_t<statistics_info_t> statistics_info;
updateable_info_t<statistics_info_t> counterexample_info;
ulong_long_int_t statist_reached_states = 0;
ulong_long_int_t statist_reached_trans = 0;
ulong_long_int_t statist_reached_cross_trans = 0;
ulong_long_int_t statist_number_of_transitions = 0;
ulong_long_int_t statist_number_of_returning_states_to_queue = 0;
ulong_long_int_t statist_number_of_walks_to_root= 0;
ulong_long_int_t statist_number_of_updates = 0;
ulong_int_t statist_maximal_queue_size = 0;

static vminfo_t vm;

float get_consumed_memory()
{ return float(double(vm.getvmsize())/1024); }

void statistics_info_t::update()
{
 if (distributed.is_manager())
 { 
  consumed_memory = 0;
  maximal_consumed_memory = 0;
  reached_states = 0;
  number_of_transitions = 0;
  number_of_returning_states_to_queue = 0;
  number_of_walks_to_root = 0;
  number_of_updates = 0;
  maximal_queue_size = 0;
  maximal_amortization = 0;
 }
 float here_consumed_memory = get_consumed_memory();
 consumed_memory         += here_consumed_memory;
 if (here_consumed_memory>maximal_consumed_memory)
   maximal_consumed_memory=here_consumed_memory;
 reached_states          += statist_reached_states;
 number_of_transitions   += statist_number_of_transitions;
 number_of_returning_states_to_queue +=
    statist_number_of_returning_states_to_queue;
 number_of_walks_to_root += statist_number_of_walks_to_root;
 number_of_updates += statist_number_of_updates;
 if (maximal_amortization<walk_to_root_amortization_bound)
   maximal_amortization=walk_to_root_amortization_bound;
 if (maximal_queue_size<statist_maximal_queue_size)
   maximal_queue_size=statist_maximal_queue_size;
}

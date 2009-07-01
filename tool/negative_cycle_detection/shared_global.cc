#include "shared_global.hh"
#include "statistics.hh"
#include <queue>

//std::ofstream debfile;

using namespace divine;

bool should_print_statistics=false;
ulong_int_t hash_table_size=65534;
bool search_for_counterexample = false;
bool should_produce_report=false;
bool should_produce_trail=false;
bool should_produce_state_list=false;
bool should_produce_log=false;
bool file_name_base_specified = false;
std::string given_file_name_base;
std::string file_name_base = "unknown";

state_ref_t initial_state_ref;
int initiator_id;

bool is_idle = false;
bool message_finish_sent = false;
ulong_int_t walk_to_root_stamp=0;
bool negative_cycle_found = false;
sshort_int_t negative_cycle_founder_id;
bool negative_cycle_reconstruction = false;
state_ref_t negative_cycle_state_of_cycle;
array_t<state_t> negative_cycle;
ulong_int_t negative_cycle_number_of_received_states=0;
array_t<state_t> path_to_negative_cycle;
ulong_int_t path_to_negative_cycle_number_of_received_states=0;
array_t<state_t> * p_used_state_container=0;
sshort_int_t path_to_negative_cycle_founder_id=0;

bool is_negative_cycle_walk_initiator = false;
ulong_long_int_t walk_to_root_amortization_bound=503;

divine::size_int_t succs_calls=0;

distributed_t distributed;
explicit_storage_t Storage;
explicit_system_t * p_System = 0;
succ_container_t * p_succs;
std::queue<state_ref_t> state_queue;
appendix_t appendix;
message_t message;
distr_reporter_t reporter(&distributed);
logger_t logger;

void finish_program()
{
// debfile << "process " << distributed.network_id << " called finish" << endl;
 message_finish_sent=true;
 distributed.set_idle();
 is_idle=true;
 for (int i=0; i<distributed.cluster_size; ++i)
   if (i!=MANAGER_ID)
     distributed.network.send_urgent_message(
        (char*)(&negative_cycle_found),sizeof(bool),i,MESSAGE_FINISH);
}

state_ref_t pop_from_state_queue(appendix_t & appendix)
{
 state_ref_t result;
 result = state_queue.front(); state_queue.pop();
 //retrieving an appendix of state
 if (!Storage.get_app_by_ref(result,appendix))
   gerr << distributed.network_id << "Invalid reference to storage." << thr();
       
 appendix.in_queue_or_first_fisit--;
 Storage.set_app_by_ref(result,appendix);
 return result;
}

void push_to_state_queue(const state_ref_t ref)
{
 appendix_t appendix;
 if (!Storage.get_app_by_ref(ref,appendix))
   gerr << distributed.network_id << "Invalid reference to storage." << thr();
 push_to_state_queue(ref,appendix);
}

void push_to_state_queue(const state_ref_t ref, appendix_t & appendix)
{
  if ((appendix.in_queue_or_first_fisit%2==0))
  {
   appendix.in_queue_or_first_fisit++;
   if (!Storage.set_app_by_ref(ref,appendix))
     gerr << distributed.network_id << "Invalid reference to storage." << thr();
   state_queue.push(ref);
   if (state_queue.size()>statist_maximal_queue_size)
     statist_maximal_queue_size=state_queue.size();
  }
}



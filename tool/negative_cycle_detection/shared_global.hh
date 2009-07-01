#ifndef NEG_CYC_SHARED_GLOBAL_HH
#define NEG_CYC_SHARED_GLOBAL_HH

#include <divine.h>

#include <fstream>
#include <string>

//extern std::ofstream debfile;

//when NIL_STAMP is stored in appendix_t::origins_process, then no stamp
//is stored in the state
const divine::sshort_int_t NIL_STAMP            = -30000;
//when NEGATIVE_CYCLE_STAMP is stored in appendix_t::origins_process, then
//the state belongs to the negative cycle - marked during a reconstruction
//of negative cycle
const divine::sshort_int_t NEGATIVE_CYCLE_STAMP = -30001;

const divine::sshort_int_t PATH_TO_NEGATIVE_CYCLE_STAMP = -30002;

const divine::sshort_int_t NO_PARENT = -30000;

//structure used in explcit_storage_t to store appendix to a state
struct appendix_t {
  divine::slong_int_t distance;
  divine::sshort_int_t origins_process;
  divine::state_ref_t origin;
  divine::ulong_int_t stamp;
  divine::sshort_int_t parents_process;
  divine::state_ref_t parent;
  divine::sshort_int_t in_queue_or_first_fisit; // %2 = 1 iff state in queue; /2 = 1 iff state was visited for the first time (to detect reachable (cross) transitions properly
};


//options from command line:
extern bool should_print_statistics;
extern divine::ulong_int_t hash_table_size;
extern bool search_for_counterexample;
extern bool should_produce_trail;
extern bool should_produce_state_list;
extern bool should_produce_report;
extern bool should_produce_log;
extern bool should_produce_report;
extern bool file_name_base_specified;
extern string given_file_name_base;

//file name of an input without an extension (used for names of produced files):
extern std::string file_name_base;

extern divine::state_ref_t initial_state_ref;
extern int initiator_id;

extern bool message_finish_sent;
extern bool is_idle;
//variable storing a next possible stamp for walk_to_root()
extern divine::ulong_int_t walk_to_root_stamp;
//variable telling whether negative cycle has been found or not
extern bool negative_cycle_found;
//variable storing an identifier of computer, that found a negative cycle
extern divine::sshort_int_t negative_cycle_founder_id;
extern bool negative_cycle_reconstruction;
extern bool is_negative_cycle_walk_initiator;
//reference on state stored on computer `negative_cycle_founder_id' belonging
//to the negative cycle
extern divine::state_ref_t negative_cycle_state_of_cycle;
extern divine::array_t<divine::state_t> negative_cycle;
extern divine::ulong_int_t negative_cycle_number_of_received_states;
extern divine::array_t<divine::state_t> path_to_negative_cycle;
extern divine::ulong_int_t path_to_negative_cycle_number_of_received_states;
extern divine::array_t<divine::state_t> * p_used_state_container;
extern divine::sshort_int_t path_to_negative_cycle_founder_id;

extern divine::ulong_long_int_t walk_to_root_amortization_bound;

extern divine::size_int_t succs_calls;

extern divine::distributed_t distributed;
extern divine::explicit_storage_t Storage;
extern divine::explicit_system_t * p_System;
extern divine::succ_container_t * p_succs;
extern std::queue<divine::state_ref_t> state_queue;
extern appendix_t appendix;
extern divine::message_t message;
extern divine::distr_reporter_t reporter;
extern divine::logger_t logger;

const int MANAGER_ID = divine::NETWORK_ID_MANAGER;
const int MESSAGE_UPDATE_DISTANCE                     = divine::DIVINE_TAG_USER+1;
const int MESSAGE_VERTEX_TO_QUEUE                     = divine::DIVINE_TAG_USER+2;
const int MESSAGE_WALK_TO_ROOT                        = divine::DIVINE_TAG_USER+3;
const int MESSAGE_REMOVE_MARKS                        = divine::DIVINE_TAG_USER+4;
const int MESSAGE_NEGATIVE_CYCLE_FOUND_TO_MANAGER     = divine::DIVINE_TAG_USER+5;
const int MESSAGE_NEGATIVE_CYCLE_INITIATE_RECONSTRUCTION=divine::DIVINE_TAG_USER+6;
const int MESSAGE_NEGATIVE_CYCLE_WALK                 = divine::DIVINE_TAG_USER+7;
const int MESSAGE_NEGATIVE_CYCLE_STATE_OF_CYCLE       = divine::DIVINE_TAG_USER+8;
const int MESSAGE_NEGATIVE_CYCLE_END                  = divine::DIVINE_TAG_USER+9;
const int MESSAGE_COUNTEREXAMPLE_SCAN_VERTEX          = divine::DIVINE_TAG_USER+10;
const int MESSAGE_COUNTEREXAMPLE_SCAN_FINISH          = divine::DIVINE_TAG_USER+11;
const int MESSAGE_FINISH                              = divine::DIVINE_TAG_USER+50;

const char * const TRAIL_EXTENSION = ".trail";
const char * const STATE_LIST_EXTENSION = ".ce_states";
const char * const REPORT_EXTENSION = ".report";

void finish_program();
void push_to_state_queue(const divine::state_ref_t ref);
void push_to_state_queue(const divine::state_ref_t ref, appendix_t & appendix);
divine::state_ref_t pop_from_state_queue(appendix_t & appendix);

#endif

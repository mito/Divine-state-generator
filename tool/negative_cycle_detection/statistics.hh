#ifndef NEG_CYC_STATISTICS_HH
#define NEG_CYC_STATISTICS_HH

#include <divine.h>

struct statistics_info_t
 {
  float consumed_memory;
  float maximal_consumed_memory;
  divine::ulong_long_int_t reached_states;
  divine::ulong_long_int_t number_of_transitions;
  divine::ulong_long_int_t number_of_returning_states_to_queue;
  divine::ulong_long_int_t number_of_walks_to_root;
  divine::ulong_long_int_t number_of_updates;
  divine::ulong_int_t maximal_queue_size;
  divine::ulong_long_int_t maximal_amortization;
  void update();
 };

float get_consumed_memory();

extern divine::updateable_info_t<statistics_info_t> statistics_info;
extern divine::updateable_info_t<statistics_info_t> counterexample_info;

extern divine::ulong_long_int_t statist_reached_states;
extern divine::ulong_long_int_t statist_reached_trans;
extern divine::ulong_long_int_t statist_reached_cross_trans;
extern divine::ulong_long_int_t statist_number_of_transitions;
extern divine::ulong_long_int_t statist_number_of_returning_states_to_queue;
extern divine::ulong_long_int_t statist_number_of_walks_to_root;
extern divine::ulong_long_int_t statist_number_of_updates;
extern divine::ulong_int_t statist_maximal_queue_size;

#endif


#ifndef NEG_CYC_COUNTEREXAMPLE_HH
#define NEG_CYC_COUNTEREXAMPLE_HH

#include <divine.h>

void counterexample_negative_cycle_walk(divine::sshort_int_t ats_process,
                                        divine::state_ref_t at_ref,
                                        divine::ulong_int_t order);

void counterexample_scan(const divine::state_t v, const divine::state_ref_t v_ref);

void counterexample_push_to_queue_or_resend_task(divine::state_t u);

void counterexample_push_to_queue(const divine::state_t u,
                                  const divine::sshort_int_t owner_of_v,
                                  const divine::state_ref_t v_ref);

#endif

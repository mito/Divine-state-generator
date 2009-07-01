#ifndef NEG_CYC_UPDATE_DISTANCE
#define NEG_CYC_UPDATE_DISTANCE

#include <divine.h>

void update_distance_or_resend_task
          (divine::state_t u, const divine::sshort_int_t owner_of_v,
           const divine::state_ref_t v_ref, const divine::slong_int_t new_distance, const bool first_edge_relaxation);

void update_distance
          (divine::state_t u, const divine::sshort_int_t owner_of_v,
           const divine::state_ref_t v_ref, const divine::slong_int_t new_distance);

#endif

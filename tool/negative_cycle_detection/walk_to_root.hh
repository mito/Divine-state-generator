#ifndef NEG_CYC_WALK_TO_ROOT
#define NEG_CYC_WALK_TO_ROOT

#include <divine.h>

bool walk_to_root_amortization();

void walk_to_root(const divine::sshort_int_t origins_process,
                  const divine::state_ref_t origin,
                  const divine::ulong_int_t stamp,
                  divine::sshort_int_t ats_process, divine::state_ref_t at,
                  divine::ulong_int_t walk_length //additional parameter - because
                                          //of amortization
                  );

#endif

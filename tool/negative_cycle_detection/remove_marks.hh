#ifndef NEG_CYC_REMOVE_MARKS_HH
#define NEG_CYC_REMOVE_MARKS_HH

#include <divine.h>

void remove_marks_or_resend_task
                 (const divine::sshort_int_t origins_process,
                  const divine::state_ref_t origin,
                  const divine::ulong_int_t stamp,
                  const divine::sshort_int_t ats_process,
                  const divine::state_ref_t at_ref);

void remove_marks(const divine::sshort_int_t origins_process,
                  const divine::state_ref_t origin,
                  const divine::ulong_int_t stamp,
                  divine::sshort_int_t ats_process, divine::state_ref_t at_ref);

#endif

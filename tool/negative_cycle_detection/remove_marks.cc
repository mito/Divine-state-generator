#include "remove_marks.hh"
#include "shared_global.hh"

using namespace divine;

static void resend_task
                 (const sshort_int_t origins_process, const state_ref_t origin,
                  const ulong_int_t stamp,
                  const sshort_int_t ats_process,const state_ref_t at_ref)
{
 message.rewind();
 message.append_sshort_int(origins_process);
 message.append_state_ref(origin);
 message.append_ulong_int(stamp);
 //ats_process = destination of a message
 message.append_state_ref(at_ref);
 distributed.network.send_message(message,ats_process,MESSAGE_REMOVE_MARKS);
}

void remove_marks_or_resend_task
                 (const sshort_int_t origins_process, const state_ref_t origin,
                  const ulong_int_t stamp,
                  const sshort_int_t ats_process, const state_ref_t at_ref)
{
 if (ats_process!=distributed.network_id)
  resend_task(origins_process, origin, stamp, ats_process, at_ref);
 else
  remove_marks(origins_process, origin, stamp, ats_process, at_ref);
}

void remove_marks(const sshort_int_t origins_process, const state_ref_t origin,
                  const ulong_int_t stamp,
                  sshort_int_t ats_process, state_ref_t at_ref)
{
// debfile << "Remove marks called" << endl;
 bool done=false;
 while (!done)
  {
   if (ats_process==distributed.network_id)
    {
     Storage.get_app_by_ref(at_ref, appendix);
     if (appendix.origins_process!=NIL_STAMP)
      {
       appendix.origins_process = NIL_STAMP;
       Storage.set_app_by_ref(at_ref, appendix);
//       debfile << "Remove marks: at: "  << at_ref << " parent: " << appendix.parent << endl;
       ats_process = appendix.parents_process;
       at_ref      = appendix.parent;
      }
     else
       done=true;
    }
   else
     { resend_task(origins_process, origin, stamp, ats_process, at_ref);
       done=true; }
//   debfile << "Remove marks - done: " << done << endl;
  }
}




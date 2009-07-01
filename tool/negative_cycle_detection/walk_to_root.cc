#include "walk_to_root.hh"
#include "shared_global.hh"
#include "remove_marks.hh"
#include "process_messages.hh"
#include "statistics.hh"
#include <cstring>

using namespace divine;

static ulong_int_t walk_to_root_amortization_counter=0;


bool walk_to_root_amortization()
{
 walk_to_root_amortization_counter++;
// cout << distributed.network_id << ": " << walk_to_root_amortization_counter << endl;
 if (walk_to_root_amortization_counter>=walk_to_root_amortization_bound)
  {
   walk_to_root_amortization_counter=0;
   return true;
  }
 else return false;
}

static bool stamp_is_smaller(const sshort_int_t origins_process1,
                             const state_ref_t origin1,
                             const ulong_int_t stamp1,
                             const sshort_int_t origins_process2,
                             const state_ref_t origin2,
                             const ulong_int_t stamp2)
{
 if (origins_process1 < origins_process2 ||
     (origins_process1 == origins_process2 && (origin1 < origin2 ||
      (origin1 == origin2 && stamp1 < stamp2) )))
   return true;
 else
   return false;
}

//the content of then-branch branch of "if owner(at)=alpha" in WTR from a paper:
//this funtion can be called only when at_ref is valid on this machine, which
//is ensured by step_to_root_or_resend_task()
static bool step_to_root
                 (const sshort_int_t origins_process, const state_ref_t origin,
                  const ulong_int_t stamp,
                  sshort_int_t & ats_process, state_ref_t & at_ref,
                  ulong_int_t & walk_length //additional parameter - because
                                            //of amortization
                  )
{
// debfile << "step_to_root" << endl;
// debfile << ((distributed.network_id==initiator_id && at_ref==initial_state_ref)? "in source":"not in source") << endl;
 Storage.get_app_by_ref(at_ref, appendix);
// debfile << appendix.parents_process << ": at: " << at_ref << "parent: " << appendix.parent << endl;
 //if walk(at)=[origin,stamp]
 if (appendix.origins_process==origins_process && appendix.origin==origin &&
     appendix.stamp==stamp)
  {
   negative_cycle_state_of_cycle = at_ref;
   if (!distributed.is_manager())
     distributed.network.send_message(0,0,MANAGER_ID,
                                      MESSAGE_NEGATIVE_CYCLE_FOUND_TO_MANAGER);
   else manager_manages_found_negative_cycle();

   return true;
  }
 //if (at=source) || (walk(at) > [origin,stamp]) then...
 else if ((distributed.network_id==initiator_id && at_ref==initial_state_ref) ||
          stamp_is_smaller(origins_process, origin, stamp,
                           appendix.origins_process, appendix.origin,
                           appendix.stamp))
  {
//   debfile << "assigning last_walk_to_root_length=" << walk_length << endl;
   if (walk_length>walk_to_root_amortization_bound)
     walk_to_root_amortization_bound = walk_length;
   else
    {
     ulong_long_int_t statespace_estimation =
       statist_reached_states*distributed.cluster_size;
     if (statespace_estimation > walk_to_root_amortization_bound)
       walk_to_root_amortization_bound = statespace_estimation;
    }
//   debfile << "calling remove marks" << walk_length << endl;
   remove_marks_or_resend_task(origins_process, origin, stamp,
                               origins_process, origin);
//   debfile << "removing of marks finished" << walk_length << endl;
   return true;
  }
 //if (walk(at)=[nil,nil]) || (walk(at) < [origin,stamp]) then...
 else if (appendix.origins_process==NIL_STAMP ||
          stamp_is_smaller(appendix.origins_process, appendix.origin,
                           appendix.stamp, origins_process, origin, stamp))
  {
   walk_length++;
   appendix.origins_process = origins_process;
   appendix.origin          = origin;
   appendix.stamp           = stamp;
   Storage.set_app_by_ref(at_ref, appendix);
   ats_process = appendix.parents_process;
   at_ref      = appendix.parent;
   return false;
  }
 else
  {
   gerr << "Unexpected possibility in step_to_root()" << thr();
   return true; //unreachable return
  }
}



//the content of while-cycle from WTR in a paper
static bool step_to_root_or_resend_task
                 (const sshort_int_t origins_process, const state_ref_t origin,
                  const ulong_int_t stamp,
                  sshort_int_t & ats_process, state_ref_t & at_ref,
                  ulong_int_t & walk_length //additional parameter - because
                                            //of amortization
                  )
{
 if (ats_process!=distributed.network_id)
  {//else-branch of "if owner(at)=alpha" from a paper:
   message.rewind();
   message.append_sshort_int(origins_process);
   message.append_state_ref(origin);
   message.append_ulong_int(stamp);
   //ats_process = destination of a message
   message.append_state_ref(at_ref);
   message.append_ulong_int(walk_length);
   distributed.network.send_message(message,ats_process,MESSAGE_WALK_TO_ROOT);
                                    
   return true; //resending a task to another computer - this computer
                //completed its work for now
  }
 //corresponds to then-branch branch of "if owner(at)=alpha" from a paper:
 else return step_to_root(origins_process, origin, stamp, ats_process, at_ref,
                          walk_length);
}

//WTR from a paper
void walk_to_root(const sshort_int_t origins_process, const state_ref_t origin,
                  const ulong_int_t stamp,
                  sshort_int_t ats_process, state_ref_t at_ref,
                  ulong_int_t walk_length //additional parameter - because
                                          //of amortization
                  )
{
// debfile << "\nwalk to root at: " << at_ref << endl;
 bool done = false;
 while (!done)
  {
   done = step_to_root_or_resend_task(origins_process, origin, stamp,
                                      ats_process, at_ref,
                                      walk_length);
//   debfile << "done: " << done << endl;
  }
}


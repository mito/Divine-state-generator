#include "counterexample.hh"
#include "shared_global.hh"

using namespace divine;

static state_ref_t first_state_on_a_cycle;


static bool counterexample_negative_cycle_step(sshort_int_t & ats_process,
                                               state_ref_t & at_ref,
                                               const ulong_int_t order)
{
//   cout << distributed.network_id << ": " << at_ref << " != " << initial_state_ref << endl;
// debfile << "counterexample_negative_cycle_step at: " << at_ref << endl;
 state_t at = Storage.reconstruct(at_ref);
 message.rewind();
 message.append_state(at);
 message.append_ulong_int(order);
 distributed.network.send_message(message,MANAGER_ID,
                                  MESSAGE_NEGATIVE_CYCLE_STATE_OF_CYCLE);
 delete_state(at);
 if (!Storage.get_app_by_ref(at_ref,appendix))
   gerr <<"Unexpected error: invalid reference in negative cycle step"<<thr();
 if (negative_cycle_reconstruction)
  {//stamping of a negative cycle during its reconstruction:
//   debfile << "giving CYCLE stamp to " << at_ref << endl;
   appendix.origins_process = NEGATIVE_CYCLE_STAMP;
   Storage.set_app_by_ref(at_ref,appendix);
  }
  
 //if reaches the intial state of negative cycle => the entire cycle is
 //reconstructed
 if ((negative_cycle_reconstruction &&//test of end of cycle reconstruction
      is_negative_cycle_walk_initiator && at_ref==negative_cycle_state_of_cycle)
     ||
     (!negative_cycle_reconstruction &&//test of end of path reconstruction
      distributed.network_id == initiator_id && at_ref==initial_state_ref))
  {
   for (int i=0; i!=distributed.cluster_size; ++i)
     if (i!=distributed.network_id)
       distributed.network.send_message(0,0,i,MESSAGE_NEGATIVE_CYCLE_END);
   distributed.set_idle();
   return true;
  }
 else //otherwise go to the parent
  {
   at_ref = appendix.parent;
   ats_process = appendix.parents_process;
  }
    
 return false;
}

void counterexample_negative_cycle_walk(sshort_int_t ats_process,
                                        state_ref_t at_ref,
                                        ulong_int_t order)
{
// debfile << "counterexample_negative_cycle_walk at: " << at_ref << endl;
 bool done=false;
 while (!done)
  {
   if (ats_process!=distributed.network_id)
    {
     message.rewind();
     message.append_state_ref(at_ref);
     message.append_ulong_int(order);
     distributed.network.send_message(message,ats_process,
                                      MESSAGE_NEGATIVE_CYCLE_WALK);
     done = true;
    }
   else
     done = counterexample_negative_cycle_step(ats_process, at_ref, order);
   order++;
  }
}

void counterexample_push_to_queue_or_resend_task(state_t u,
                                                 const state_ref_t v_ref)
{
 int owner_of_u = distributed.partition_function(u);
 if (owner_of_u!=distributed.network_id)
  {
   message.rewind();
   message.append_state(u);
   message.append_state_ref(v_ref);
   distributed.network.send_message(message,owner_of_u,
                                    MESSAGE_COUNTEREXAMPLE_SCAN_VERTEX);
  }
 else counterexample_push_to_queue(u,distributed.network_id,v_ref);
}


void counterexample_scan(const state_t v, const state_ref_t v_ref)
{
 if (!Storage.get_app_by_ref(v_ref,appendix))
   gerr << "Unexpected error: invalid reference" << thr();
 
// debfile << "counterexample_scan: " << v_ref << " stamp: " << appendix.origins_process << "!=" << NEGATIVE_CYCLE_STAMP << endl;
 
 int succs_result = p_System->get_succs(v,*p_succs);
 
 if (!succs_normal(succs_result))
   if (succs_error(succs_result))
     gerr << "Found an error state during state space exploration" << thr();
 
 for (size_int_t i=0; i<p_succs->size(); ++i)
  {
   counterexample_push_to_queue_or_resend_task((*p_succs)[i], v_ref);
   delete_state((*p_succs)[i]);
  }
}


void counterexample_push_to_queue(const state_t u,
                                  const sshort_int_t owner_of_v,
                                  const state_ref_t v_ref)
{
 state_ref_t u_ref;
 //if state is already visited during a model checking, enque it
 //to the queue of states to be explored
 if (Storage.is_stored(u, u_ref))
  {
   if (!Storage.get_app_by_ref(u_ref,appendix))
     gerr << "Unexpected error: invalid reference" << thr();
   if (appendix.origins_process!=PATH_TO_NEGATIVE_CYCLE_STAMP)
    {
     appendix.parents_process = owner_of_v;
     appendix.parent = v_ref;
     if (appendix.origins_process==NEGATIVE_CYCLE_STAMP)
      {//state of reconstructed negative cycle found!
       //sending a finish command to all computers
//       debfile << "NEGATIVE_CYCLE_STAMP: " << u_ref << endl;
       for (int i=0; i!=distributed.cluster_size; ++i)
         if (i!=distributed.network_id)
           distributed.network.send_urgent_message(0,0,i,
                                           MESSAGE_COUNTEREXAMPLE_SCAN_FINISH);
       distributed.set_idle();
       is_idle = true;
       path_to_negative_cycle_founder_id=distributed.network_id;
       message_finish_sent=true;
       negative_cycle_state_of_cycle=u_ref;
       Storage.set_app_by_ref(u_ref,appendix);
      }
     else
      {
       //we store, that the state has been already explored
       appendix.origins_process = PATH_TO_NEGATIVE_CYCLE_STAMP;
       push_to_state_queue(u_ref,appendix);
      }
//     Storage.set_app_by_ref(u_ref,appendix);
    }
   //otherwise do not go through this state - we have been there before
  }
  //otherwise do not go through this state - it is not needed for the
  //reconstruction of the path to the cycle of the counterexample
 
}

//void counterexample_reconstruct_cycle(const bool first_on_cycle,
//                                      const state_ref_t state_ref)
//{
// if (first_on_cycle)
//  {
//   owns_first_state_on_a_cycle = true;
//   first_state_on_a_cycle = state_ref;
//  }
// bool done=false;
// state_t state;
// int parents_owner;
// state_ref_t parent, processed_state = state_ref;
// while (!done)
//  {
//   if (owns_first_state_on_a_cycle && processed_state==first_state_on_a_cycle)
//    {
//     done = true; //reconstruction of a cycle is finished;
//     if (distributed.cluster_size>1 && distributed.network_id!=MANAGER_ID)
//      {
//       //reconstruction finished ==> let know to manager
//       distributed.network.send_urgent_message(0,0,MANAGER_ID,
//                                          MESSAGE_NEGATIVE_CYCLE_RECONSTRUCTED);
//     //if not needed to send a message to manager, execute subsequent calls
//     //directly:
//     else 
//       counterexample_reconstruct_path_to_cycle();
//    }
//   else
//    {
//     state = state_from_storage(processed_state);
//     distributed.network.send_message(state.ptr, state.size, MANAGER_ID,
//                                      MESSAGE_NEGATIVE_CYCLE_STATE_OF_CYCLE);
//     mark_state_as_negative_cycle_state(processed_state);
//     get_parent(parents_owner,parent);
//     if (parents_owner!=distributed.network_id)
//      {
//       done = true;
//       message.rewind();
//       message.write_state_ref(parent);
//       distributed.network.send_message(message,parents_owner,
//                                      MESSAGE_NEGATIVE_CYCLE_RECONSTRUCT_CYCLE);
//      }
//     else processed_state = parent;
//    }
//  }
//}
//
//void counterexample_reconstruct_path_to_cycle
//{
//}

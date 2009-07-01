#include <divine.h>
#include "shared_global.hh"
#include "process_messages.hh"
#include "walk_to_root.hh"
#include "remove_marks.hh"
#include "update_distance.hh"
#include "counterexample.hh"

using namespace divine;

static message_t received_message;

//this function only receives messages, translates them to variables
//values and executes corresponding functions processing these variables
void process_message(char *buf, int size, int src, int tag, bool urgent)
{
// debfile << "process message called - tag " << tag << endl;
 //be really idle (maybe redundant, but safe)
// if (is_idle) { debfile << "idle" << endl; return; }
 if (!message_finish_sent) //ignore all messages after receiving of finish
  {
   received_message.load_data((byte_t*)(buf),size);
   switch (tag)
    {
     case MESSAGE_UPDATE_DISTANCE:
       {
        //load u, v and new_distance from message:
  //      debfile << "hello" << endl;
        state_t u;
        sshort_int_t owner_of_v;
        state_ref_t v_ref;
        slong_int_t new_distance;
        received_message.read_state(u);
        received_message.read_sshort_int(owner_of_v);
        received_message.read_state_ref(v_ref);
        received_message.read_slong_int(new_distance);
        update_distance(u,owner_of_v,v_ref,new_distance);
        delete_state(u);
       }
       break;
     case MESSAGE_VERTEX_TO_QUEUE:
       //when the state is returned because update is not possible because of
       //stamp in the vertex
       {
        state_ref_t new_state_to_queue_ref;
        received_message.read_state_ref(new_state_to_queue_ref);
        push_to_state_queue(new_state_to_queue_ref);
       }
       break;
     case MESSAGE_WALK_TO_ROOT: //walk to root (like in the paper)
       {
        sshort_int_t origins_process;
        state_ref_t origin;
        ulong_int_t stamp;
        state_ref_t at_ref;
        ulong_int_t walk_length;
        received_message.read_sshort_int(origins_process);
        received_message.read_state_ref(origin);
        received_message.read_ulong_int(stamp);
        //presume, that ats_process == this process
        received_message.read_state_ref(at_ref);
        received_message.read_ulong_int(walk_length);
        walk_to_root(origins_process,origin,stamp,distributed.network_id,at_ref,
                     walk_length);
       }
       break;
     case MESSAGE_REMOVE_MARKS: //removing marks (like in the paper)
       {
        sshort_int_t origins_process;
        state_ref_t origin;
        ulong_int_t stamp;
        state_ref_t at_ref;
        received_message.read_sshort_int(origins_process);
        received_message.read_state_ref(origin);
        received_message.read_ulong_int(stamp);
        received_message.read_state_ref(at_ref);
        remove_marks(origins_process,origin,stamp,distributed.network_id,at_ref);
       }
       break;
     case MESSAGE_NEGATIVE_CYCLE_FOUND_TO_MANAGER:
       if (!distributed.is_manager())
         gerr << "Message for manager received by a servant." << thr();
       negative_cycle_founder_id = src;
       manager_manages_found_negative_cycle();
       break;
     case MESSAGE_FINISH:
       distributed.set_idle();
       is_idle = true;
       message_finish_sent=true;
       received_message.read_bool(negative_cycle_found);
       break;
     default: gerr << "Unknown type of the message" << thr(); break;
    }
  }
}

void process_message_about_counterexample_reconstruction
                    (char *buf, int size, int src, int tag, bool urgent)
{
 received_message.load_data((byte_t*)(buf),size);
 switch (tag)
  {
   case MESSAGE_NEGATIVE_CYCLE_INITIATE_RECONSTRUCTION:
     is_negative_cycle_walk_initiator = true;
     //Negative cycle reconstruction needs to begin the walk in the parent
     //of a state of the negative cycle.
     //else branch: Reconstruction of path to tyhe negative cycle begins
     //             at the current state of the cycle assigned during
     //             the reachability of the negative cycle
     if (negative_cycle_reconstruction)
      {
       if (!Storage.get_app_by_ref(negative_cycle_state_of_cycle,appendix))
        gerr <<"Unexpected error: invalid reference to the begin of ce."<<thr();
       counterexample_negative_cycle_walk(appendix.parents_process,
                                          appendix.parent,0);
      }
     else
       counterexample_negative_cycle_walk(distributed.network_id,
                                          negative_cycle_state_of_cycle,0);
     break;
   case MESSAGE_NEGATIVE_CYCLE_WALK:
     {
      state_ref_t at_ref;
      ulong_int_t order;

      received_message.read_state_ref(at_ref);
      received_message.read_ulong_int(order);
      counterexample_negative_cycle_walk(distributed.network_id,
                                        at_ref,order);
     }
     break;
   case MESSAGE_NEGATIVE_CYCLE_STATE_OF_CYCLE:
     {
      if (!distributed.is_manager())
        gerr << "Unexpected error: message for manager received by non-manager"
             << thr();
      state_t state;
      ulong_int_t order;
      received_message.read_state(state);
      received_message.read_ulong_int(order);
      if (order>=p_used_state_container->size())
        p_used_state_container->extend_to(order+1);
      (*p_used_state_container)[order] = state;
      if (negative_cycle_reconstruction)
        negative_cycle_number_of_received_states++;
      else
        path_to_negative_cycle_number_of_received_states++;
     }
     break;
   case MESSAGE_NEGATIVE_CYCLE_END:
     distributed.set_idle();
     break;
   default: gerr << "Unknown type of the message" << thr(); break;
  }
}

void process_message_about_counterexamples_path_to_cycle_search
                    (char *buf, int size, int src, int tag, bool urgent)
{
 if (!message_finish_sent) //ignoting all messages after finish
  {
   received_message.load_data((byte_t*)(buf),size);
   switch (tag)
    {
     case MESSAGE_COUNTEREXAMPLE_SCAN_VERTEX:
      {//enque vertex to scan to the queue, if it was found before
       state_t state;
       state_ref_t parent_ref;
       received_message.read_state(state);
       received_message.read_state_ref(parent_ref);
       //we presume, that parent is from the sending process
       counterexample_push_to_queue(state,src,parent_ref);
       delete_state(state);
      }
     break;
     case MESSAGE_COUNTEREXAMPLE_SCAN_FINISH:
       distributed.set_idle();
       is_idle = true;
       path_to_negative_cycle_founder_id=src;
       message_finish_sent=true;
       break;
     default: gerr << "Unknown type of the message" << thr(); break;
    }
  }
}

void manager_manages_found_negative_cycle()
{
 negative_cycle_found=true;
/// if (search_for_counterexample)
///  {
///   //manager resends MESSAGE_NEGATIVE_CYCLE_FOUND to the rest of computers
///   if (distributed.cluster_size>1)
///     for (int i=0; i<distributed.cluster_size; ++i)
///       if (i!=MANAGER_ID)
///         distributed.network.send_urgent_message(0,0,i,
///                                  MESSAGE_NEGATIVE_CYCLE_FOUND_TO_SERVANT);
///  }
/// else finish_program();
 finish_program();
}


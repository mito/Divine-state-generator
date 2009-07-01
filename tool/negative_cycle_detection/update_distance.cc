#include "update_distance.hh"
#include "walk_to_root.hh"
#include "shared_global.hh"
#include "statistics.hh"

using namespace divine;

static ulong_int_t stamp;

void update_distance_or_resend_task
          (state_t u, const sshort_int_t owner_of_v, const state_ref_t v_ref,
           const slong_int_t new_distance, bool first_edge_relaxation)
{
 if (first_edge_relaxation)
   statist_reached_trans++;
 int owner_of_u = distributed.partition_function(u);
 if (owner_of_u!=distributed.network_id)
  {
   //sending a command to call update_distance() on an owner of `u'
   //see process_messages.cc for corresponding receiving counterpart
//   debfile << "resending an update" << endl;
   if (first_edge_relaxation)
     statist_reached_cross_trans++;
   message.rewind();
   message.append_state(u);
   message.append_sshort_int(owner_of_v);
   message.append_state_ref(v_ref);
   message.append_slong_int(new_distance);
   distributed.network.send_message(message,owner_of_u,MESSAGE_UPDATE_DISTANCE);
  }
 else update_distance(u,owner_of_v,v_ref,new_distance);
}

static void send_enque_command(const sshort_int_t owner_of_v,
                               const state_ref_t v_ref)
{
 message.rewind();
 message.append_state_ref(v_ref);
 distributed.network.send_message(message,owner_of_v,
                                  MESSAGE_VERTEX_TO_QUEUE);
}

void update_distance
          (state_t u, const sshort_int_t owner_of_v, const state_ref_t v_ref,
           const slong_int_t new_distance)
{
 //testing a storage of a state `u' in Storage:
// debfile << "update distance called" << endl;
 state_ref_t u_ref;
 appendix_t appendix;
 if (Storage.is_stored(u,u_ref))
   Storage.get_app_by_ref(u_ref, appendix);
 else
  {//`u' in not stored => store it
    Storage.insert(u,u_ref);
//   debfile << "inserted a state " << u_ref << endl;
   statist_reached_states++;
   appendix.distance = MAX_SLONG_INT;
   appendix.origins_process = NIL_STAMP;
   appendix.parents_process = NO_PARENT;
   appendix.in_queue_or_first_fisit=2;
   //needed to store now, because of posible back-enqueing of `v'
   Storage.set_app_by_ref(u_ref, appendix);
  }
// debfile << "\nupdate at " << u_ref << endl;
 
 //updating of distance (according to UPDATE function from the paper):
 if (appendix.distance > new_distance)
  {
   if (appendix.origins_process != NIL_STAMP)
    {
     statist_number_of_returning_states_to_queue++;
     if (owner_of_v==distributed.network_id)
       push_to_state_queue(v_ref);
     else
       send_enque_command(owner_of_v, v_ref);
//     debfile << "Enqueing back" << endl;
    }
   else
    {
     statist_number_of_updates++;
     appendix.distance        = new_distance;
     appendix.parents_process = owner_of_v;
//     debfile << "setting parent: " << v_ref << endl;
     appendix.parent          = v_ref;
     Storage.set_app_by_ref(u_ref, appendix);
     if (walk_to_root_amortization())
      {
       appendix.origins_process = distributed.network_id;
       appendix.origin          = u_ref;
       appendix.stamp           = stamp;
       Storage.set_app_by_ref(u_ref, appendix);
       statist_number_of_walks_to_root++;
       walk_to_root(distributed.network_id, u_ref, stamp,
                    owner_of_v, v_ref, 0);
       ++stamp;
      }
     push_to_state_queue(u_ref);
    }
  }
}



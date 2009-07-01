
#ifndef _DIVINE_REACH_OBS_HH_
#define _DIVINE_REACH_OBS_HH_

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "sevine.h"

using namespace std;

class reachability_observer_t {
public:
  reachability_observer_t(divine::explicit_system_t* s);
  reachability_observer_t(divine::explicit_system_t* s, int t, int vm);

  void set_goals(vector<divine::expression_t*> v);
  void set_predicates(vector<divine::expression_t*> p);
  void set_output(ostream* o) { out = o; }
  void set_verbose() {verbose = true;}
  
  bool notify_state(divine::state_t q, string state_id, bool deadlocked, int distance); //returns true if some interesting state was detected
  void notify_transition(const divine::enabled_trans_t & t);
  void notify_succ(string new_id, string old_id, string step_string);

  void register_path(string state_id, divine::path_t* path);
  
  bool found() {return _found; }
  int goal_found() {return _goal_found; }
  
  void print(bool paths);
  int get_states() {return states; }
  int get_transitions() {return transitions; }

  bool limit_reached() {return _limit_reached; }
  void update();

protected:  
  divine::explicit_system_t* System;
  vector<divine::expression_t*> goals;
  vector<divine::expression_t*> predicates;
  ostream* out;
  bool verbose;
  
  vector<vector<bool> > proc_state_visited;
  vector<vector<bool> > proc_trans_visited;

  vector<int> goal_distance;
  vector<string> goal_state_id;
  map<string, int> pred_seen;
  map<string, divine::path_t*> path_to_state;

  int deadlock_distance;
  string deadlock_state_id;
  int error_distance;
  string error_state_id;
  int states_total, states_covered, trans_total, trans_covered;
  bool _found;
  int _goal_found;
  int states, transitions;

  // veci na search_limit:
  vminfo_t vm_info;
  timeinfo_t time_info;
  bool _limit_reached;
  int time_limit, vm_limit;

  int trail_cnt;
  
  void initialize();
  void print_reach_info( bool paths, string state_id, int length);

};


#endif

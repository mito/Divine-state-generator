
#include <queue>
#include <stack>
#include <sstream>
#include "search.hh"

// {{{ helper stuff

string refstring(state_ref_t ref) {
  stringstream temp;
  temp << ref.hres << ":" << ref.id;
  return temp.str();
}

string myint2string(int num) {
  stringstream temp;
  temp << num;
  return temp.str();
}

// }}}

// {{{ full search


full_search_t::full_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o) : search_t(s,p,o) {
  state_ref_t pom_ref;
  if (parameters->paths) visited.set_appendix(pom_ref);
  visited.set_ht_size(parameters->htsize);
  visited.init();
}

full_search_t::full_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o, string out) : search_t(s,p,o) {
  state_ref_t pom_ref;
  if (parameters->paths) visited.set_appendix(pom_ref);
  visited.set_ht_size(parameters->htsize);
  visited.init();
  output.open(out.c_str());
}


path_t* full_search_t::reconstruct_path(state_ref_t& act_ref) {
  state_t act = visited.reconstruct(act_ref);
  path_t* path = new path_t(system);
  
  path->push_back(act);
  while (act != init) {
    delete_state(act);
    state_ref_t new_ref;
    visited.get_app_by_ref(act_ref, new_ref);
    act = visited.reconstruct(new_ref);
    act_ref = new_ref;
    path->push_front(act);    
  }
  delete_state(act);
  return path;
}


void full_search_t::do_search() {
  queue<state_ref_t> Wait;
  state_ref_t act_ref;
  int cur_level=1, next_level=0, level=0;
  init = system->get_initial_state();
  
  visited.insert(init, act_ref);
  Wait.push(act_ref);

  while (! Wait.empty() &&
	 !(parameters->first_goal && observer->goal_found() > 0) &&
	 ! observer->limit_reached()) {
    act_ref = Wait.front(); Wait.pop();
    state_t q = visited.reconstruct(act_ref);
    enabled_trans_container_t act_trans(*system);   
    int result = system->get_enabled_trans(q, act_trans);

    if (observer->notify_state(q, refstring(act_ref), result, level)
	&& parameters->paths) {
      observer->register_path(refstring(act_ref), reconstruct_path(act_ref));
    }

    for (enabled_trans_container_t::iterator i = act_trans.begin(); i!= act_trans.end(); ++i) {
      state_t r;
      state_ref_t next_ref;
      system->get_enabled_trans_succ(q, *i, r);
      
      if (! visited.is_stored(r,next_ref)) {
	visited.insert(r, next_ref);
	if (parameters->paths)
	  visited.set_app_by_ref(next_ref, act_ref);
	//	  observer->notify_succ(refstring(next_ref), refstring(act_ref),get_enabled_trans_string(system, *i, false));
	Wait.push(next_ref);
	next_level++;
      }
      
      observer->notify_transition(*i);
      
      delete_state(r);
    }
    delete_state(q);
    cur_level--;
    if (cur_level == 0) {
      level++;
      if (output.is_open()) { output << level << "\t"<<next_level<<endl; }
      cur_level = next_level; next_level = 0;
    }
  }
  if (output.is_open()) output.close();
}

// }}}

// {{{ ril search

//ril_search commented//ril_search_t::ril_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o) : search_t(s,p,o) {
//ril_search commented//  visited.set_ht_size(parameters->htsize);
//ril_search commented//  visited.init();
//ril_search commented//}
//ril_search commented//
//ril_search commented//void ril_search_t::write_free() {
//ril_search commented//  ofstream output("free.tmp");
//ril_search commented//  for (int i=0; i<system->get_process_count(); i++) {
//ril_search commented//    for (int j=0; j<system->get_process(i)->get_state_count(); j++)
//ril_search commented//      output << free[i][j]<< " ";
//ril_search commented//    output <<endl;
//ril_search commented//  }
//ril_search commented//  output.close();
//ril_search commented//}
//ril_search commented//
//ril_search commented//bool ril_search_t::initialize() {
//ril_search commented//  int tmp;
//ril_search commented//  
//ril_search commented//  if (parameters->special_file == "") {
//ril_search commented//    cerr << "No special file given, marking all  locations as free and writing them to file 'free.tmp'"<<endl;
//ril_search commented//    free.resize(system->get_process_count());
//ril_search commented//    for (int i=0; i<system->get_process_count(); i++) {
//ril_search commented//      process_t* proc = system->get_process(i);
//ril_search commented//      free[i].resize(proc->get_state_count());
//ril_search commented//      for (int j=0; j<proc->get_state_count(); j++) 
//ril_search commented//	free[i][j] = true;      
//ril_search commented//    }
//ril_search commented//    write_free();
//ril_search commented//    return true;
//ril_search commented//  }
//ril_search commented//
//ril_search commented//  if (parameters->special_file == "communication") {
//ril_search commented//    cerr << "Computing free locations wrt to communication and write them to file 'free.tmp'"<<endl;
//ril_search commented//    free.resize(system->get_process_count());
//ril_search commented//    for (int i=0; i<system->get_process_count(); i++) {
//ril_search commented//      process_t* proc = system->get_process(i);
//ril_search commented//      free[i].resize(proc->get_state_count());
//ril_search commented//      // mark according to communication
//ril_search commented//      for (int j=0; j<proc->get_state_count(); j++) {
//ril_search commented//	bool no_sync = true;
//ril_search commented//	for (int t=0; t<proc->get_trans_count(); t++) {
//ril_search commented//	  transition_t* tr = proc->get_transition(t);
//ril_search commented//	  if (tr->get_state1_lid() == j) 
//ril_search commented//	    no_sync = no_sync && tr->is_without_sync();
//ril_search commented//	}
//ril_search commented//	free[i][j] = no_sync;
//ril_search commented//      }
//ril_search commented//    }
//ril_search commented//    write_free();
//ril_search commented//    return true;
//ril_search commented//  }
//ril_search commented//
//ril_search commented//  ifstream input(parameters->special_file.c_str());
//ril_search commented//  free.resize(system->get_process_count());
//ril_search commented//  for (int i=0; i<system->get_process_count(); i++) {
//ril_search commented//    process_t* proc = system->get_process(i);
//ril_search commented//    free[i].resize(proc->get_state_count());
//ril_search commented//    for (int j=0; j<proc->get_state_count(); j++) {
//ril_search commented//      input >> tmp;
//ril_search commented//      free[i][j] = (tmp == 1);
//ril_search commented//    }
//ril_search commented//  }
//ril_search commented//  
//ril_search commented//  return true;
//ril_search commented//}

// int ril_search_t::select_enabled_transitions(state_t q, enabled_trans_container_t & result_trans) {
//   enabled_trans_container_t act_trans(*system);
//   int result = system->get_enabled_trans(q, act_trans);

//   int i = 0;
//   while (i < system->get_process_count() && result_trans.size() == 0) {
//     if (free[i][system->get_state_of_process(q,i)]) {
//       for (enabled_trans_container_t::iterator j = act_trans.begin(); j!= act_trans.end(); j++) {
// 	if (j->trans->get_process_gid() == i)
// 	  result_trans.push_back(*j);          
//       }
//     }
//     //    if (result_trans.size() !=0)       cerr << "free: "<<i<<endl;
//     i++;
//   }
  
//   if (result_trans.size()==0) {
//     //    cerr << "all"<<endl;
//     result = system->get_enabled_trans(q, result_trans);    
//   }
  
//   return result;
// }

//ril_search commented//int ril_search_t::select_enabled_transitions(state_t q, enabled_trans_container_t & result_trans) {
//ril_search commented//  enabled_trans_container_t act_trans(*system);
//ril_search commented//  int result = system->get_enabled_trans(q, act_trans);
//ril_search commented//
//ril_search commented//  int i = 0;
//ril_search commented//  while (i < system->get_process_count() && result_trans.size() == 0) {
//ril_search commented//    if (free[i][system->get_state_of_process(q,i)]) {
//ril_search commented//      bool ok = true, at_least_one = false;
//ril_search commented//      for (enabled_trans_container_t::iterator j = act_trans.begin(); j!= act_trans.end(); j++) {
//ril_search commented//        transition_t * const trans = system->get_sending_or_normal_trans(*j);
//ril_search commented//	if (trans->get_process_gid() == i) {
//ril_search commented//	  at_least_one = true;
//ril_search commented//	  if (! trans->is_without_sync()) {
//ril_search commented//	    transition_t * const sync_tr = system->get_receiving_trans(*j);
//ril_search commented//	    if (! free[sync_tr->get_process_gid()][sync_tr->get_state1_lid()])
//ril_search commented//	      ok = false;
//ril_search commented//	  }
//ril_search commented//	}
//ril_search commented//      }
//ril_search commented//      if (ok && at_least_one) 
//ril_search commented//	for (enabled_trans_container_t::iterator j = act_trans.begin(); j!= act_trans.end(); j++) 
//ril_search commented//  	  if (system->get_sending_or_normal_trans(*j)->get_process_gid() == i)
//ril_search commented//	    result_trans.push_back(*j);                
//ril_search commented//    }
//ril_search commented//    //    if (result_trans.size() !=0) cerr << "free: "<<i<<endl;
//ril_search commented//    i++;
//ril_search commented//  }
//ril_search commented//  
//ril_search commented//  if (result_trans.size()==0) {
//ril_search commented//    //    cerr << "all"<<endl;
//ril_search commented//    result = system->get_enabled_trans(q, result_trans);    
//ril_search commented//  }
//ril_search commented//  
//ril_search commented//  return result;
//ril_search commented//}
//ril_search commented//
//ril_search commented//
//ril_search commented//void ril_search_t::do_search() {
//ril_search commented//  queue<state_ref_t> Wait;
//ril_search commented//  state_ref_t act_ref;
//ril_search commented//  int cur_level=1, next_level=0, level=0;
//ril_search commented//  state_t init = system->get_initial_state();
//ril_search commented//
//ril_search commented//  if (! initialize()) return;
//ril_search commented//  
//ril_search commented//  visited.insert(init, act_ref);
//ril_search commented//  Wait.push(act_ref);
//ril_search commented//
//ril_search commented//  while (! Wait.empty() &&
//ril_search commented//	 !(parameters->first_goal && observer->goal_found() > 0) &&
//ril_search commented//	 ! observer->limit_reached()) {
//ril_search commented//    
//ril_search commented//    act_ref = Wait.front(); Wait.pop();
//ril_search commented//    state_t q = visited.reconstruct(act_ref);
//ril_search commented//    enabled_trans_container_t selected_trans(*system);
//ril_search commented//    
//ril_search commented//    //    int result = system->get_enabled_trans(q, act_trans);
//ril_search commented//    int result = select_enabled_transitions(q, selected_trans);
//ril_search commented//
//ril_search commented//    observer->notify_state(q, refstring(act_ref), result, level);
//ril_search commented//
//ril_search commented//    for (enabled_trans_container_t::iterator i = selected_trans.begin(); i!= selected_trans.end(); ++i) {
//ril_search commented//      state_t r;
//ril_search commented//      state_ref_t next_ref;
//ril_search commented//      system->get_enabled_trans_succ(q, *i, r);
//ril_search commented//      
//ril_search commented//      if (! visited.is_stored(r,next_ref)) {
//ril_search commented//	visited.insert(r, next_ref);
//ril_search commented//	if (parameters->paths)
//ril_search commented//	  observer->notify_succ(refstring(next_ref), refstring(act_ref), i->to_string());
//ril_search commented//	Wait.push(next_ref);
//ril_search commented//	next_level++;
//ril_search commented//      }
//ril_search commented//      
//ril_search commented//      observer->notify_transition(*i);
//ril_search commented//      
//ril_search commented//      delete_state(r);
//ril_search commented//    }
//ril_search commented//    delete_state(q);
//ril_search commented//    cur_level--;
//ril_search commented//    if (cur_level == 0) {level++; cur_level = next_level; next_level = 0; }
//ril_search commented//  }
//ril_search commented//
//ril_search commented//}

// }}}

// {{{ alpha search

alpha_search_t::alpha_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o) : search_t(s,p,o) {  
}

bool alpha_search_t::initialize() {
  if (parameters->special_file == "") {
    //    cerr << "This search needs a 'special' file with predicates to use for abstraction."<<endl;
    cerr << "No 'special' file with predicates was given, using just control part as abstraction."<<endl;
    return true;
  }
  
  ifstream input(parameters->special_file.c_str());
  string ex;
  while (getline(input, ex)) {
    expression_t* expr = new dve_expression_t(ex, system);
    predicates.push_back(expr);
  }

  return true;
}

string alpha_search_t::abstraction(state_t q) {
  stringstream abs;
  bool ble;
  dve_explicit_system_t * dve_system =
    dynamic_cast<dve_explicit_system_t *>(system);
  dve_expression_t * expr = 0;
  for (size_t i=0; i<dve_system->get_process_count(); i++) 
    abs << dve_system->get_state_of_process(q,i)<<",";
  for (size_t i=0; i<predicates.size(); i++) {
    expr = dynamic_cast<dve_expression_t*>(predicates[i]);
    abs << dve_system->eval_expr(expr, q, ble) << ",";
  }
  //  system->DBG_print_state_CR(q,cerr, 0);
  //  cerr << "Abs: "<<abs.str()<<endl;
  return abs.str();
}

void alpha_search_t::do_search() {
  queue<state_t> Wait;
  state_t q,r, init = system->get_initial_state();
  int cur_level=1, next_level=0, level=0;

  if (!initialize()) {cerr << "I need a file with predicates (option '--special') "; return;}
  
  Wait.push(init);
  visited[abstraction(init)] = true;
  while (! Wait.empty() &&
	 !(parameters->first_goal && observer->goal_found() > 0) &&
	 ! observer->limit_reached()) {
    q = Wait.front(); Wait.pop();
    
    enabled_trans_container_t act_trans(*system);   
    int result = system->get_enabled_trans(q, act_trans);

    observer->notify_state(q, abstraction(q), result, level);

    for (enabled_trans_container_t::iterator i = act_trans.begin(); i!= act_trans.end(); ++i) {
      system->get_enabled_trans_succ(q, *i, r);
      string abs = abstraction(r);
      
      if (! visited[abs]) {
	visited[abs] = true;;
	if (parameters->paths) observer->notify_succ(abs, abstraction(q),i->to_string());
	Wait.push(r);
	next_level++;
      } else {
	//protoze to strkam do fronty tak mazu jen kdyz to nedavam do fronty, ze...
	delete_state(r);
      }
      
      observer->notify_transition(*i);
      
    }
    delete_state(q);
    cur_level--;
    if (cur_level == 0) {level++; cur_level = next_level; next_level = 0; }
  }

}

// }}}

// {{{ por search

por_search_t::por_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o) : search_t(s,p,o) {
  visited.set_ht_size(parameters->htsize);
  visited.init();
}

void por_search_t::do_search() {
  cerr << "POR search NOT IMPLEMENTED... :-("<<endl;
}

// }}}

// {{{ rw search

rw_search_t::rw_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o) : search_t(s,p,o) {
  srand(time(NULL));
}

void rw_search_t::do_search() {
  state_t init = system->get_initial_state();
  state_t act = duplicate_state(init);
  int num = 0, dist = 0;
  
  while (!(parameters->first_goal && observer->goal_found() > 0) &&
	 ! observer->limit_reached()) {
    num++;
    enabled_trans_container_t act_trans(*system);   
    int result = system->get_enabled_trans(act, act_trans);
    observer->notify_state(act, myint2string(num), result, dist);    
    if (result == SUCC_DEADLOCK ||  (parameters->x != 0 && observer->get_states() % parameters->x == 0) ) {
      act = duplicate_state(init);
      dist = 0;
    } else {
      int choose = rand() % act_trans.size();	
      for (enabled_trans_container_t::iterator i = act_trans.begin(); i!= act_trans.end(); ++i) {
	observer->notify_transition(*i);
	if (choose==0) {
	  state_t r;
	  system->get_enabled_trans_succ(act, *i, r);
	  delete_state(act);
	  act = r;
	  dist++;
	  if (parameters->paths) observer->notify_succ(myint2string(num+1), myint2string(num),i->to_string());
	}
	choose--;
      }
    }
    
  }
  
}

// }}}

// {{{ bithash search

bithash_search_t::bithash_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o) : search_t(s,p,o) {
}

void bithash_search_t::initialize() {
  visited = new bit_hash_t(parameters->htsize);
}

void bithash_search_t::do_search() {
  queue<state_t> Wait;
  state_t q,r, init = system->get_initial_state();
  int cur_level=1, next_level=0, level=0;

  initialize();
  
  Wait.push(init);
  visited->check(init);
  while (! Wait.empty() &&
	 !(parameters->first_goal && observer->goal_found() > 0) &&
	 ! observer->limit_reached()) {
    q = Wait.front(); Wait.pop();
    
    enabled_trans_container_t act_trans(*system);   
    int result = system->get_enabled_trans(q, act_trans);

    //need toString
    observer->notify_state(q, "todo", result, level);

    for (enabled_trans_container_t::iterator i = act_trans.begin(); i!= act_trans.end(); ++i) {
      system->get_enabled_trans_succ(q, *i, r);
      
      if (! visited->check(r)) {
	//	if (parameters->paths) observer->notify_succ(abs, abstraction(q),get_enabled_trans_string(system, *i, false));
	// NEED state.toString
	Wait.push(r);
	next_level++;
      } else {
	//protoze to strkam do fronty tak mazu jen kdyz to nedavam do fronty, ze...
	delete_state(r);
      }
      
      observer->notify_transition(*i);
      
    }
    delete_state(q);
    cur_level--;
    if (cur_level == 0) {level++; cur_level = next_level; next_level = 0; }
  }
}

// }}}

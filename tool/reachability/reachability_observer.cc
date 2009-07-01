
#include "reachability_observer.hh"
#include <signal.h>
#include <sstream>

using namespace divine;

reachability_observer_t* search_limit_ptr; // toto je dost hack, okopiroval jsem to od Jirika, je to kvuli tomu, ze signal() chce asi nejakou global funkci a ne metodu... :-(

void signal_handler(int a) {
  search_limit_ptr->update();
  alarm(3);
}

void reachability_observer_t::initialize() {
  proc_state_visited.resize(System->get_process_count());
  proc_trans_visited.resize(System->get_process_count());
  states_total = trans_total = 0;
  for(size_t i=0;i<System->get_process_count(); i++) {
    dve_process_t * ith_process =
       dynamic_cast<dve_process_t*>(System->get_process(i));
    proc_state_visited[i].resize(ith_process->get_state_count(), false);
    states_total += ith_process->get_state_count();
    proc_trans_visited[i].resize(ith_process->get_trans_count(), false);
    trans_total += ith_process->get_trans_count();
  }    
  states_covered = trans_covered = 0;
  error_state_id = ""; deadlock_state_id = "";
  error_distance = -1; deadlock_distance = -1;
  states = transitions = 0;
  _found = false;
  _goal_found = 0;
  out = &cerr;
  trail_cnt=0;
  verbose=false;

  _limit_reached = false;
  time_info.reset();
  search_limit_ptr = this;
  signal(SIGALRM, signal_handler);
  alarm(3);
}

reachability_observer_t::reachability_observer_t(explicit_system_t* sys) {
  System = sys;
  time_limit = 0;
  vm_limit = 0;
  initialize();
}

reachability_observer_t::reachability_observer_t(explicit_system_t* sys, int t, int vm) {
  System = sys;
  time_limit = t;
  vm_limit = vm;
  initialize();
}

void reachability_observer_t::update() {
  if (vm_limit != 0 && vm_info.getvmsize() > vm_limit) _limit_reached = true;
  if (time_limit != 0 && time_info.gettime() > time_limit) _limit_reached = true;  
}


void reachability_observer_t::set_goals(vector<expression_t*> v) {
  goals = v;
  goal_state_id.resize(goals.size(),"");
  goal_distance.resize(goals.size(),0);
}

void reachability_observer_t::set_predicates(vector<expression_t*> p) {
  predicates = p;
}


bool reachability_observer_t::notify_state(state_t q, string state_id, bool deadlocked, int distance) {
  bool something_found = false;
  
  states++;
  if (verbose && (states %10000 ==0) ) cerr << "States searched: "<<states<<"  Time: "<<time_info.gettime()<< "  Speed: "<< (int) states/time_info.gettime()<<" \r";
    
  if (System->is_erroneous(q) && (error_state_id == "" || distance < error_distance)) {
    error_state_id = state_id;
    error_distance = distance;
    something_found = _found = true;
  }

  if (deadlocked && (deadlock_state_id == "" || distance < deadlock_distance)) {
    deadlock_state_id = state_id;
    deadlock_distance = distance; 
    something_found = _found = true;
  }
  
  dve_explicit_system_t *DVE_System =
      dynamic_cast<dve_explicit_system_t*>(System);
  
  for(size_t i=0 ;i<DVE_System->get_process_count(); i++) {
    if (!proc_state_visited[i][DVE_System->get_state_of_process(q, i)]) {
      proc_state_visited[i][DVE_System->get_state_of_process(q, i)] = true;
      states_covered++;
    }
  }
  
  bool ble;
  for (size_t i=0; i<goals.size(); i++ ) {
    if (DVE_System->eval_expr(dynamic_cast<dve_expression_t*>(goals[i]), q, ble) && (goal_state_id[i] == "" || distance < goal_distance[i]) ) {
      goal_state_id[i] = state_id;
      goal_distance[i] = distance;
      something_found = _found = true;
      _goal_found = distance;
    }
  }

  string predicate = "";
  for (size_t i=0; i<predicates.size(); i++) {
    if (DVE_System->eval_expr(dynamic_cast<dve_expression_t*>(predicates[i]), q, ble)) {
      predicate += "1";
    } else {
      predicate += "0";
    }
  }
  pred_seen[predicate]++;

  return something_found;
}

void reachability_observer_t::register_path(string state_id, path_t* path) {

  path_to_state[state_id] = path;
}

void reachability_observer_t::notify_succ(string newid, string oldid, string step) {
  if (out == 0) {
    cerr << "Error: ostream not initialized."<<endl;
  } else {    
    (*out) << newid << "\t" << oldid<<"\t"<<step<<endl;
  }
}

void reachability_observer_t::notify_transition(const enabled_trans_t & t) {
  transitions++;
  for (std::size_t i = 0; i!= t.get_count(); ++i)
   {
    dve_transition_t * const trans = dynamic_cast<dve_transition_t*>(t[i]);
    if (!System->get_with_property() ||
        trans->get_process_gid()!=System->get_property_gid())
      if (!proc_trans_visited[trans->get_process_gid()][trans->get_lid()])
       {
        proc_trans_visited[trans->get_process_gid()][trans->get_lid()] = true;
        trans_covered++;
       }
   }
}

void reachability_observer_t::print_reach_info(bool paths,  string state_id, int length) {
  if (state_id == "") {
    cout << "No"<<endl;
  } else {
    cout << "Yes"<<endl;
    if (path_to_state[state_id] != NULL) {
      if (verbose) {
	cout << " ------------- "<<endl;
	path_to_state[state_id]->write_trans(cerr);
	cout << " ------------- "<<endl;
      } else {
	stringstream tmp;
	tmp<< "trail"<<trail_cnt; trail_cnt++;
	cout << "Path length:"<<path_to_state[state_id]->length()<< "\nPath stored to file "<< tmp.str()<<endl;
	ofstream out(tmp.str().c_str());
	path_to_state[state_id]->write_trans(out);
      }
    } else {
      cout << "Path length:"<<length<<endl;
      if (paths) cout << "(you can get the path via 'reconstruct_path.pl  "<<state_id<< "' (well, this is not in bin at the moment...))" << endl<<endl;
    }    
  }
}

void reachability_observer_t::print(bool paths) {

  cerr <<"                                                                                           \r";
  cout <<"-----------------------------------------------"<<endl;
  cout <<"States: "<<states<<endl;
  cout <<"Transitions: "<<transitions<<endl<<endl;
  
  cout <<"Error state: "; print_reach_info(paths, error_state_id, error_distance);
  cout <<"Deadlock state: "; print_reach_info(paths, deadlock_state_id, deadlock_distance);
  
  if (goals.size()>0) {
    cout <<endl <<"Goals:"<<endl;
    for (size_t i=0; i<goals.size();i++) {
      goals[i]->write(cout);
      cout  << ": ";
      print_reach_info(paths, goal_state_id[i], goal_distance[i]);
    }
  }
  if (states_total) cout <<endl <<"Statement coverage: "<<100*states_covered/states_total<<"%"<<endl;
  if (trans_total) cout <<"Branch coverage: "<<100*trans_covered/trans_total<<"%"<<endl;
  if (verbose && (states_covered != states_total || trans_covered != trans_total)) {
    cout << "Unreachable code:"<<endl;
    for(size_t i=0;i<System->get_process_count(); i++) {
      const dve_process_t* proc =
        dynamic_cast<dve_process_t*>(System->get_process(i));
      bool some_output=false;

      for(size_t j=0; j< proc->get_state_count();j++)
	if (! proc_state_visited[i][j]) some_output=true;
      
      for(size_t j=0; j< proc->get_trans_count();j++)
	if (! proc_trans_visited[i][j]) some_output=true;
      
      dve_explicit_system_t * DVE_System =
         dynamic_cast<dve_explicit_system_t*>(System);
   
      
      if (some_output) {
	cout << "Process " << DVE_System->get_symbol_table()->get_process(proc->get_gid())->get_name()<<endl;
      
	for(size_t j=0; j< proc->get_state_count();j++)
	  if (! proc_state_visited[i][j]) cout <<"\tstate "<< DVE_System->get_symbol_table()->get_state(proc->get_state_gid(j))->get_name()<<endl;
	
	for(size_t j=0; j< proc->get_trans_count();j++)
	  if (! proc_trans_visited[i][j]) {
	    cout <<"\t";
	    proc->get_transition(j)->write(cout);
	    cout<<endl;
	  }
      }
    }
  }
  if (predicates.size()) {
    cout << endl<<"Predicates covered: "<< pred_seen.size() <<endl;
    if (verbose) {
      for (map<string,int>::iterator i = pred_seen.begin(); i!=pred_seen.end(); i++) 
	cout << "\t"<<(*i).first<<"\t"<<(*i).second <<endl;
    }
  }
  cout <<"-----------------------------------------------"<<endl;
}

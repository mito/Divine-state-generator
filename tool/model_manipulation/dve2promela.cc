#include <iostream>
#include "sevine.h"

using namespace std;
using namespace divine;

dve_system_t System(gerr);
dve_symbol_table_t* table;
bool have_init = false;

string spin_safe(string s) {
  string::size_type loc;
  
  do {    loc = s.find("active",0);
    if (loc != string::npos) {
      s.replace(loc, 6, "actv");
    }
  }  while (loc != string::npos);

  do {    loc = s.find("len",0);
    if (loc != string::npos) {
      s.replace(loc, 3, "ln");
    }
  }  while (loc != string::npos);

  
  do {    loc = s.find("chan",0);
    if (loc != string::npos) {
      s.replace(loc, 4, "chnl");
    }
  }  while (loc != string::npos);
  
  do {    loc = s.find("timeout",0);
    if (loc != string::npos) {
      s.replace(loc, 7, "timeisout");
    }
  }  while (loc != string::npos);

  do {    loc = s.find("active",0);
    if (loc != string::npos) {
      s.replace(loc, 6, "isactv");
    }
  }  while (loc != string::npos);  
  
  do {
    loc = s.find(" and ",0);
    if (loc != string::npos) {
      s.replace(loc, 5, " && ");
    }
  }  while (loc != string::npos);

  do {
    loc = s.find(" not ",0);
    if (loc != string::npos) {
      s.replace(loc, 5, " ! ");
    }
  }  while (loc != string::npos);

  do {
    loc = s.find(" or ",0);
    if (loc != string::npos) {
      s.replace(loc, 4, " || ");
    }
  }  while (loc != string::npos);
  
  return s;
}

string print_variable(size_t v_gid) { // vraci init string
  dve_symbol_t* sym = table->get_variable(v_gid);
  string s = "";
  char buff[33];
  cout << ((sym->get_var_type()==VAR_BYTE)?"byte":"int")<<" "<< spin_safe(sym->get_name());
  if (sym->is_vector()) {
    cout << "["<< sym->get_vector_size()<<"]";
    for (int i=0; i< sym->get_vector_size() && i <sym->get_init_expr_count(); i++) {
      stringstream ss;
      ss << i;
      s += spin_safe(sym->get_name()) + "[" + ss.str() + "] =" + sym->get_init_expr(i)->to_string()+"; ";
    }
  } else {
    dve_expression_t* e = sym->get_init_expr();
    cout << "="<< (e!=NULL?spin_safe(e->to_string()):"0");
  }
  cout <<";"<<endl;
  return s;
}

void print_header() {
  string init_string = "";
  for(size_t i=0; i<System.get_global_variable_count(); i++) {
    string si = print_variable(System.get_global_variable_gid(i));
    init_string += si;
  }    
  cout <<endl;
  
  for(size_t i=0; i<System.get_channel_count(); i++) 
    cout << "chan "<< spin_safe(table->get_channel(i)->get_name())<<" =[0] of {int};"<<endl;
  cout <<endl;

  if (init_string != "") {
    have_init = true;
    cout << "init { \n d_step { \n"+init_string<< "};" <<endl << "atomic { "<<endl;
    for(size_t i=0; i<System.get_process_count(); i++) {
      cout << "run "<< spin_safe(table->get_process(System.get_process(i)->get_gid())->get_name())<<"();"<<endl;
    }  
    cout << "}; };" <<endl<<endl;
  }
}

string get_transition_string(dve_process_t* proc, dve_transition_t* tr) {
  int step_num = 0;
  string s, res;
  if (tr->get_guard()) {
    tr->get_guard_string(s);
    res += spin_safe(s) + ";";
    step_num++;
  }
  s = "";
  if (! tr->is_without_sync()) {
    res += spin_safe(table->get_channel(tr->get_channel_gid())->get_name());
    res += (tr->is_sync_ask()?'?':'!');
    tr->get_sync_expr_string(s);
    if (s != "") 
      res += spin_safe(s) + ";";
    else res += "0;";
    step_num++;
  }
  //  if (tr->get_effect_count()) {
  s = "";
  for (std::size_t i=0; i!=tr->get_effect_count(); i++) {
    tr->get_effect_expr_string(i,s);
    res +=  spin_safe(s) + ";";
    step_num++;
  }

  if (step_num>1) {
    if (tr->is_without_sync())
      res = " d_step {" + res + "}; ";
    else {
      res = " atomic {" + res + "}; ";
      cerr<<"Not exact!"<<endl;
    }
  }  
  res += " goto " + spin_safe(tr->get_state2_name()) + "; \n";
  return res;
}

bool print_state_and_trans(dve_process_t* proc, size_t act_state) { // vraci jestli neco vypsal
  string s = "";

  for(size_t j=0; j<proc->get_trans_count(); j++) {
    dve_transition_t* tr = dynamic_cast<dve_transition_t *> (proc->get_transition(j));
    if (act_state == tr->get_state1_gid()) {
      s += ":: "+ get_transition_string(proc,tr) + "\n";
    }
  }

  if (s != "") {  
    cout << spin_safe(table->get_state(act_state)->get_name())<<": if"<<endl;
    cout << s;
    cout << "fi;"<<endl;
    return true;
  }
  return false;
}

void print_process(dve_process_t* proc) {
  string no_trans_state = "";
  if (!have_init) cout << "active ";
  cout << "proctype " << spin_safe(table->get_process(proc->get_gid())->get_name()) << "() { "<<endl;
  for(size_t i=0; i<proc->get_variable_count(); i++)
    print_variable(proc->get_variable_gid(i));
  cout <<endl;

  // jako prvni musim init stav!
  
  for(size_t i=0; i<proc->get_state_count();i++) {
    size_t act_state = proc->get_state_gid(i);
    if (i == proc->get_initial_state()) print_state_and_trans(proc, act_state);
  }

  // pak ostatni co maji aspon jeden prechod
  for(size_t i=0; i<proc->get_state_count();i++) {
    size_t act_state = proc->get_state_gid(i);
    if (i != proc->get_initial_state())
      if (!print_state_and_trans(proc,act_state)) {
	no_trans_state += spin_safe(table->get_state(act_state)->get_name()) + ": \n";
      }
  }

  if (no_trans_state != "") {
    cout << no_trans_state << " false; ";
  }
  cout <<  "}"<<endl<<endl;
}

int main(int argc, char** argv) {

  System.read(argv[1]);
  table = System.get_symbol_table();

  print_header();
  for(size_t i=0; i<System.get_process_count(); i++) {
    print_process(dynamic_cast<dve_process_t *>(System.get_process(i)));
  }  
  
  return 0;
}

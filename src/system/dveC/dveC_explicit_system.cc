#include <exception>
#include <cassert>

#include "system/dveC/dveC_explicit_system.hh"
#include "system/state.hh"
#include "common/bit_string.hh"
#include "common/error.hh"
#include "common/deb.hh"

#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <string>
#include <map>
#include <vector>


#ifndef DOXYGEN_PROCESSING
using namespace divine;
using namespace std;
#endif
using std::ostream;
using std::vector;
                                

struct ext_transition_t
  {
    int synchronized;
    dve_transition_t *first;
    dve_transition_t *second; //only when first transition is synchronized;
    dve_transition_t *property; // transition of property automaton
  };

//Declarations of external functions of compiled generator
static int (*lib_get_succ)(divine::state_t, divine::succ_container_t &);
static bool (*lib_is_accepting)(divine::state_t);
static divine::state_t (*lib_get_initial_state)();
static void (*lib_print_state)(divine::state_t, std::ostream & );

// =================================================================================================================================
// =================================================================================================================================
// Protected methods:
// =================================================================================================================================


void dveC_explicit_system_t::write_C(dve_expression_t & expr, std::ostream & ostr, std::string state_name)
{
  //DEBFUNC(cerr << "BEGIN of dve_expression_t::write" << endl;)
 dve_symbol_table_t * parent_table = expr.get_symbol_table();
 if (!parent_table) gerr << "Writing expression: Symbol table not set" << thr();
 switch (expr.get_operator())
  {
   case T_ID:
    { ostr<<state_name<<".";
      if(parent_table->get_variable(expr.get_ident_gid())->get_process_gid() != NO_ID)
      {
        ostr << parent_table->get_process(parent_table->get_variable(expr.get_ident_gid())->get_process_gid())->get_name(); //name of process
        ostr<<".";
      }
      ostr << parent_table->get_variable(expr.get_ident_gid())->get_name();
      break; }
   case T_FOREIGN_ID:
    { ostr << parent_table->get_process(parent_table->get_variable(expr.get_ident_gid())->get_process_gid())->get_name(); //name of process
      ostr<<"->";
      ostr << parent_table->get_variable(expr.get_ident_gid())->get_name();
      break; }
   case T_NAT:
    { char * aux_char = create_string_from<all_values_t>(expr.get_value());
      ostr << aux_char; dispose_string(aux_char); break; }
   case T_PARENTHESIS:
    { ostr << "("; write_C(*expr.left(), ostr, state_name); ostr << ")"; break; }
   case T_SQUARE_BRACKETS:
    { ostr<<state_name<<".";
      if(parent_table->get_variable(expr.get_ident_gid())->get_process_gid() != NO_ID)
      {
        ostr << parent_table->get_process(parent_table->get_variable(expr.get_ident_gid())->get_process_gid())->get_name(); //name of process
        ostr<<".";
      }
      ostr << parent_table->get_variable(expr.get_ident_gid())->
                 get_name(); ostr<<"["; write_C(*expr.left(), ostr, state_name); ostr<<"]" ;break; }
   case T_FOREIGN_SQUARE_BRACKETS:
    { ostr << parent_table->get_process(parent_table->get_variable(expr.get_ident_gid())->get_process_gid())->get_name(); //name of preocess
      ostr<<"->";
      ostr << parent_table->get_variable(expr.get_ident_gid())->get_name();
      ostr<<"["; write_C(*expr.left(), ostr, state_name); ostr<<"]" ;break; }
   case T_LT: { write_C(*expr.left(), ostr, state_name); ostr<<"<"; write_C(*expr.right(), ostr, state_name); break; }
   case T_LEQ: { write_C(*expr.left(), ostr, state_name); ostr<<"<="; write_C(*expr.right(), ostr, state_name); break; }
   case T_EQ: { write_C(*expr.left(), ostr, state_name); ostr<<"=="; write_C(*expr.right(), ostr, state_name); break; }
   case T_NEQ: { write_C(*expr.left(), ostr, state_name); ostr<<"!="; write_C(*expr.right(), ostr, state_name); break; }
   case T_GT: { write_C(*expr.left(), ostr, state_name); ostr<<">"; write_C(*expr.right(), ostr, state_name); break; }
   case T_GEQ: { write_C(*expr.left(), ostr, state_name); ostr<<">="; write_C(*expr.right(), ostr, state_name); break; }
   case T_PLUS: { write_C(*expr.left(), ostr, state_name); ostr<<"+"; write_C(*expr.right(), ostr, state_name); break; }
   case T_MINUS: { write_C(*expr.left(), ostr, state_name); ostr<<"-"; write_C(*expr.right(), ostr, state_name); break; }
   case T_MULT: { write_C(*expr.left(), ostr, state_name); ostr<<"*"; write_C(*expr.right(), ostr, state_name); break; }
   case T_DIV: { write_C(*expr.left(), ostr, state_name); ostr<<"/"; write_C(*expr.right(), ostr, state_name); break; }
   case T_MOD: { write_C(*expr.left(), ostr, state_name); ostr<<"%"; write_C(*expr.right(), ostr, state_name); break; }
   case T_AND: { write_C(*expr.left(), ostr, state_name); ostr<<"&"; write_C(*expr.right(), ostr, state_name); break; }
   case T_OR: { write_C(*expr.left(), ostr, state_name); ostr<<"|"; write_C(*expr.right(), ostr, state_name); break; }
   case T_XOR: { write_C(*expr.left(), ostr, state_name); ostr<<"^"; write_C(*expr.right(), ostr, state_name); break; }
   case T_LSHIFT: { write_C(*expr.left(), ostr, state_name); ostr<<"<<"; write_C(*expr.right(), ostr, state_name); break; }
   case T_RSHIFT: { write_C(*expr.left(), ostr, state_name); ostr<<">>"; write_C(*expr.right(), ostr, state_name); break; }
   case T_BOOL_AND: {write_C(*expr.left(), ostr, state_name); ostr<<" && ";write_C(*expr.right(), ostr, state_name); break;}
   case T_BOOL_OR: {write_C(*expr.left(), ostr, state_name); ostr<<" || "; write_C(*expr.right(), ostr, state_name); break;}
   case T_DOT:
    { ostr<<state_name<<".";
      ostr<<parent_table->get_process(parent_table->get_state(expr.get_ident_gid())->get_process_gid())->get_name(); ostr<<".state"<<" == ";
      ostr<<parent_table->get_state(expr.get_ident_gid())->get_lid(); break; }
   case T_IMPLY: { write_C(*expr.left(), ostr, state_name); ostr<<" -> "; write_C(*expr.right(), ostr, state_name); break; }
   case T_UNARY_MINUS: { ostr<<"-"; write_C(*expr.right(), ostr, state_name); break; }
   case T_TILDE: { ostr<<"~"; write_C(*expr.right(), ostr, state_name); break; }
   case T_BOOL_NOT: { ostr<<" ! ("; write_C(*expr.right(), ostr, state_name); ostr<< " )"; break; }
   case T_ASSIGNMENT: { write_C(*expr.left(), ostr, state_name); ostr<<" = "; write_C(*expr.right(), ostr, state_name); break; }
   default: { gerr << "Problem in expression - unknown operator"
                      " number " << expr.get_operator() << psh(); }
  }
 //DEBFUNC(cerr << "END of dve_expression_t::write_C" << endl;)
}


// =================================================================================================================================
// =================================================================================================================================
// Public methods:
// =================================================================================================================================

slong_int_t dveC_explicit_system_t::pure_read(const char * const filename)
{
  slong_int_t result;
  result = dve_explicit_system_t::read(filename);
  return result;
}


slong_int_t dveC_explicit_system_t::read(const char * const filename)
{
  void *handle;
  char *error;

  slong_int_t result;
  if (strlen(filename)>=4 && strcmp(filename+strlen(filename)-4,".dve")==0)
  {
    result = dve_explicit_system_t::read(filename);
    string name = filename;
    if(filename[0] == '/')
     name = name.substr(0,strlen(filename)-3);
    else
     name = "./" + name.substr(0,strlen(filename)-3);
    //C code generation
    ofstream ofstr;
    int len = sysconf(_SC_HOST_NAME_MAX) + 1;
    char host_name[len];
    gethostname(host_name,len); 
    string file_template = host_name + string(create_string_from(getpid()));
    name = name + file_template;
    ofstr.open((name + ".cc").c_str());
    if (ofstr)
    {
      dveC_explicit_system_t::print_include(ofstr);
      dveC_explicit_system_t::print_state_struct(ofstr);
      dveC_explicit_system_t::print_print_state(ofstr);
      dveC_explicit_system_t::print_dveC_compiler(ofstr);
      ofstr.close();
   }
   else
   {
       cerr << "Unable to create a file " + name + ".cc"<< endl;
       exit(2);
   }

   //C kode compilation
#define CPP_MakeString_aux(s) #s
#define CPP_MakeString(s) CPP_MakeString_aux(s)
   system((string("g++ -O2 -g -shared -fPIC ")
#ifdef INSTALL_PREFIX
	   + string("-I ") + string(CPP_MakeString(INSTALL_PREFIX)) + string("/include/divine-cluster ")
#else
	   + string("-I ~/divine/src/ ")
#endif
	   + name + ".cc -o " + name + ".dveC -lc "
#ifdef INSTALL_PREFIX
	   + string("-L ") + string(CPP_MakeString(INSTALL_PREFIX)) + string("/lib/ ")
#else
	   + string("-L ~/divine/lib/ ")
#endif
	   + string("-lsevine -lbymoc_vm")).c_str());

   //library opening
   handle = dlopen ((name + ".dveC").c_str(), RTLD_LAZY);
   if (!handle) 
   {
     fputs (dlerror(), stderr);
     system(("rm " + name + ".cc").c_str());
     system(("rm " + name + ".dveC").c_str());
     return 1;
   }

   //delete temporery files
   system(("rm " + name + ".cc").c_str());
   system(("rm " + name + ".dveC").c_str());

 }
 else{
   string name = filename;
   if(filename[0] == '/')
     name = name.substr(0,strlen(filename)-4);
    else
     name = "./" + name.substr(0,strlen(filename)-4);

   result = dve_explicit_system_t::read((name + "dve").c_str());

   //library opening
   handle = dlopen ((name + "dveC").c_str(), RTLD_LAZY);
   if (!handle) 
   {
     fputs (dlerror(), stderr);
     return 1;
   }
 }
 
  lib_get_succ = (int (*)(state_t, succ_container_t &)) dlsym(handle, "lib_get_succ");
  lib_is_accepting = (bool (*)(state_t)) dlsym(handle, "lib_is_accepting");
  lib_get_initial_state = (state_t (*)()) dlsym(handle, "lib_get_initial_state");
  lib_print_state  = (void (*)(state_t, ostream & outs)) dlsym(handle, "lib_print_state");

 if ((error = dlerror()) != NULL)  
  {
     fputs(error, stderr);
     return 1;
  }

 return result;

}

dveC_explicit_system_t::dveC_explicit_system_t(error_vector_t & evect):
  system_t(evect), explicit_system_t(evect), dve_system_t(evect), dve_explicit_system_t(evect)
{
}


bool dveC_explicit_system_t::is_accepting(state_t state, size_int_t acc_group, size_int_t pair_member)
{
  if (acc_group==0 && pair_member==1)
    return lib_is_accepting(state);
  else
    return dve_explicit_system_t::is_accepting(state,acc_group,pair_member);
}

// void dveC_explicit_system_t::print_state(state_t state,
//                                           std::ostream & outs)
// {
//    dve_explicit_system_t::print_state(state,outs);
//   //lib_print_state(state,outs);
// }

// state_t dveC_explicit_system_t::get_initial_state()
// {
//   //return lib_get_initial_state();
//   return dve_explicit_system_t::get_initial_state();
// }

int dveC_explicit_system_t::get_succs(state_t state, succ_container_t & succs)
{
 return lib_get_succ(state, succs);
}

dveC_explicit_system_t::~dveC_explicit_system_t()
{
}


// =================================================================================================================================
// =================================================================================================================================
// Functions for generating C code
// =================================================================================================================================


void dveC_explicit_system_t::print_include(ostream & ostr)
{
  ostr << "#include <string.h>" <<endl;
  ostr << "#include "<< '"' <<"sevine.h"<< '"' <<endl;
  ostr << endl;
  ostr << "using namespace std;"<<endl;
  ostr << "using namespace divine;"<<endl;
  ostr << endl;
}

void dveC_explicit_system_t::print_state_struct(ostream & ostr)
{
 bool global = true;
 string spaces = "  ";
 string orig_spaces = spaces;
 string name;
 string process_name = "UNINITIALIZED";
 ostr << "struct state_struct_t" << endl;
 ostr << " {" << endl;

 for (size_int_t i=0; i!=state_creators_count; ++i)
  {
   switch (state_creators[i].type)
    {
     case state_creator_t::VARIABLE:
      {
       name=get_symbol_table()->get_variable(state_creators[i].gid)->get_name();
       if (state_creators[i].array_size)
        {
         if (state_creators[i].var_type==VAR_BYTE)
           ostr << spaces << "byte_t ";
         else if (state_creators[i].var_type==VAR_INT)
           ostr << spaces << "sshort_int_t ";
         else gerr << "Unexpected error" << thr();
         ostr << name << "[" <<state_creators[i].array_size<< "];" << endl;
        }
       else
        {
         if (state_creators[i].var_type==VAR_BYTE)
           ostr << spaces << "byte_t " << name << ";" << endl;
         else if (state_creators[i].var_type==VAR_INT)
           ostr << spaces << "sshort_int_t " << name << ";" << endl;
         else gerr << "Unexpected error" << thr();
        }
      }
     break;
     case state_creator_t::PROCESS_STATE:
      {
       if (global)
        {
         spaces += "  ";
         global = false;
        }
       else
        {
         ostr << orig_spaces << " } __attribute__((__packed__)) " << process_name << ";" << endl;
        }
       ostr << orig_spaces << "struct" << endl;
       ostr << orig_spaces << " {" << endl;
       process_name=
         get_symbol_table()->get_process(state_creators[i].gid)->get_name();
       ostr << spaces << "ushort_int_t state;" << endl; 
      }
     break;
     case state_creator_t::CHANNEL_BUFFER:
      {
       name=get_symbol_table()->get_channel(state_creators[i].gid)->get_name();
       ostr << spaces << "struct" << endl;
       ostr << spaces << " {" << endl;
       ostr << spaces << "  ushort_int_t number_of_items;" << endl;
       ostr << spaces << "  struct" << endl;
       ostr << spaces << "   {" << endl;
       string extra_spaces = spaces + "    ";
       dve_symbol_t * symbol =
         get_symbol_table()->get_channel(state_creators[i].gid);
       size_int_t item_count = symbol->get_channel_type_list_size();
       for (size_int_t j=0; j<item_count; ++j)
         if (symbol->get_channel_type_list_item(j)==VAR_BYTE)
           ostr << extra_spaces << "byte_t x" << j << ";" << endl;
         else if (symbol->get_channel_type_list_item(j)==VAR_INT)
           ostr << extra_spaces << "sshort_int_t x" << j << ";" << endl;
         else gerr << "Unexpected error" << thr();
       ostr << spaces << "   } __attribute__((__packed__)) content["<< symbol->get_channel_buffer_size() << "];"<< endl;
       ostr << spaces << " } __attribute__((__packed__)) " << name << ";" <<endl;
      }
     break;
     default: gerr << "Unexpected error" << thr();
     break;
    };
  }
 if (!global)
  {
   ostr << orig_spaces << " } __attribute__((__packed__)) " << process_name << ";" << endl;
  }
 ostr << " } __attribute__((__packed__));" << endl;
 ostr << endl;
}

void dveC_explicit_system_t::print_dveC_compiler(ostream & ostr)
{

  map<size_int_t,map<size_int_t,vector<ext_transition_t> > > transition_map;
  map<size_int_t,vector<dve_transition_t*> > channel_map;

  vector<dve_transition_t*> property_transitions;
  vector<dve_transition_t*>::iterator iter_property_transitions;

  map<size_int_t,vector<dve_transition_t*> >::iterator iter_channel_map;
  vector<dve_transition_t*>::iterator iter_transition_vector;
  map<size_int_t,vector<ext_transition_t> >::iterator iter_process_transition_map;
  map<size_int_t,map<size_int_t,vector<ext_transition_t> > >::iterator iter_transition_map;
  vector<ext_transition_t>::iterator iter_ext_transition_vector;

  string state_name = "c_state";

  dve_transition_t * transition;
  bool sytem_with_property = this->get_with_property();

  // obtain transition with synchronization of the type SYNC_EXCLAIM and property transitions
  for(size_int_t i = 0; i < this->get_trans_count(); i++)
  {
    transition = dynamic_cast<dve_transition_t*>(this->get_transition(i));
    if(transition->is_sync_exclaim())
    {
      iter_channel_map = channel_map.find(transition->get_channel_gid());
      if(iter_channel_map == channel_map.end()) //new channel
      {
         vector<dve_transition_t*> transition_vector;
         transition_vector.push_back(transition);
         channel_map.insert(pair<size_int_t,vector<dve_transition_t*> >(transition->get_channel_gid(),transition_vector));
      }
      else{
       iter_channel_map->second.push_back(transition);
      }
    }

    if(sytem_with_property && transition->get_process_gid() == this->get_property_gid())
    {
      property_transitions.push_back(transition);
    }
  }

  // obtain map of transitions
  for(size_int_t i = 0; i < this->get_trans_count(); i++)
  {
    transition = dynamic_cast<dve_transition_t*>(this->get_transition(i));
    if(!transition->is_sync_exclaim() && (!sytem_with_property || transition->get_process_gid() != this->get_property_gid()) )
    {
      iter_transition_map = transition_map.find(transition->get_process_gid());
      if( iter_transition_map == transition_map.end()) //new process it means that new state in process is also new
      {

         map<size_int_t,vector<ext_transition_t> >  process_transition_map;
         vector<ext_transition_t> ext_transition_vector;
         if(!transition->is_sync_ask())
         {
           if(!sytem_with_property)
           { 
             ext_transition_t ext_transition;
             ext_transition.synchronized = false;
             ext_transition.first = transition;
             ext_transition_vector.push_back(ext_transition);
           }
           else
           {
             for(iter_property_transitions = property_transitions.begin();iter_property_transitions != property_transitions.end();
                 iter_property_transitions++)
             {
               ext_transition_t ext_transition;
               ext_transition.synchronized = false;
               ext_transition.first = transition;
               ext_transition.property = (*iter_property_transitions);
               ext_transition_vector.push_back(ext_transition);
             }
           }
         }
         else
         {
           iter_channel_map = channel_map.find(transition->get_channel_gid());
           if(iter_channel_map != channel_map.end())
           {
             for(iter_transition_vector = iter_channel_map->second.begin();iter_transition_vector != 
                 iter_channel_map->second.end();iter_transition_vector++)
             {
               if(!sytem_with_property)
               {
                 ext_transition_t ext_transition;
                 ext_transition.synchronized = true;
                 ext_transition.first = transition;
                 ext_transition.second = (*iter_transition_vector);
                 ext_transition_vector.push_back(ext_transition);
               }
               else
               {
                 for(iter_property_transitions = property_transitions.begin();iter_property_transitions != property_transitions.end();
                    iter_property_transitions++)
                 {
                    ext_transition_t ext_transition;
                    ext_transition.synchronized = true;
                    ext_transition.first = transition;
                    ext_transition.second = (*iter_transition_vector);
                    ext_transition.property = (*iter_property_transitions);
                    ext_transition_vector.push_back(ext_transition);
                 }
               }
             }
           }
         }
         process_transition_map.insert(pair<size_int_t,vector<ext_transition_t> >(transition->get_state1_lid(),ext_transition_vector));
         transition_map.insert(pair<size_int_t,map<size_int_t,vector<ext_transition_t> > >(transition->get_process_gid(),process_transition_map));
      }
      else{

         iter_process_transition_map = iter_transition_map->second.find(transition->get_state1_lid());
         if( iter_process_transition_map == iter_transition_map->second.end()) //new state in current process
         {
           vector<ext_transition_t> ext_transition_vector;
           if(!transition->is_sync_ask())
           {
             if(!sytem_with_property)
             { 
               ext_transition_t ext_transition;
               ext_transition.synchronized = false;
               ext_transition.first = transition;
               ext_transition_vector.push_back(ext_transition);
             }
             else
             {
               for(iter_property_transitions = property_transitions.begin();iter_property_transitions != property_transitions.end();
                   iter_property_transitions++)
               {
                 ext_transition_t ext_transition;
                 ext_transition.synchronized = false;
                 ext_transition.first = transition;
                 ext_transition.property = (*iter_property_transitions);
                 ext_transition_vector.push_back(ext_transition);
               }
             }
           }
           else
           {
             iter_channel_map = channel_map.find(transition->get_channel_gid());
             if(iter_channel_map != channel_map.end())
             {
               for(iter_transition_vector = iter_channel_map->second.begin();iter_transition_vector != 
                   iter_channel_map->second.end();iter_transition_vector++)
               {
                 if(!sytem_with_property)
                 {
                   ext_transition_t ext_transition;
                   ext_transition.synchronized = true;
                   ext_transition.first = transition;
                   ext_transition.second = (*iter_transition_vector);
                   ext_transition_vector.push_back(ext_transition);
                 }
                 else
                 {
                   for(iter_property_transitions = property_transitions.begin();iter_property_transitions != property_transitions.end();
                      iter_property_transitions++)
                   {
                      ext_transition_t ext_transition;
                      ext_transition.synchronized = true;
                      ext_transition.first = transition;
                      ext_transition.second = (*iter_transition_vector);
                      ext_transition.property = (*iter_property_transitions);
                      ext_transition_vector.push_back(ext_transition);
                   }
                 }
               }
             }
           }
           iter_transition_map->second.insert(pair<size_int_t,vector<ext_transition_t> >(transition->get_state1_lid(),ext_transition_vector));
         }
         else{
           if(!transition->is_sync_ask())
           {
             if(!sytem_with_property)
             { 
               ext_transition_t ext_transition;
               ext_transition.synchronized = false;
               ext_transition.first = transition;
               iter_process_transition_map->second.push_back(ext_transition);
             }
             else{
               for(iter_property_transitions = property_transitions.begin();iter_property_transitions != property_transitions.end();
                   iter_property_transitions++)
               {
                 ext_transition_t ext_transition;
                 ext_transition.synchronized = false;
                 ext_transition.first = transition;
                 ext_transition.property = (*iter_property_transitions);
                 iter_process_transition_map->second.push_back(ext_transition);
               }
             }
           }
           else
           {
             iter_channel_map = channel_map.find(transition->get_channel_gid());
             if(iter_channel_map != channel_map.end())
             {
               for(iter_transition_vector = iter_channel_map->second.begin();iter_transition_vector != 
                   iter_channel_map->second.end();iter_transition_vector++)
               {
                 if(!sytem_with_property)
                 {
                   ext_transition_t ext_transition;
                   ext_transition.synchronized = true;
                   ext_transition.first = transition;
                   ext_transition.second = (*iter_transition_vector);
                   iter_process_transition_map->second.push_back(ext_transition);
                 }
                 else{
                   for(iter_property_transitions = property_transitions.begin();iter_property_transitions != property_transitions.end();
                       iter_property_transitions++)
                   {
                      ext_transition_t ext_transition;
                      ext_transition.synchronized = true;
                      ext_transition.first = transition;
                      ext_transition.second = (*iter_transition_vector);
                      ext_transition.property = (*iter_property_transitions);
                      iter_process_transition_map->second.push_back(ext_transition);
                   }
                 }
               }
             }
           }
         }
      }
    }
  }


  /* for debuging
  for(iter_transition_map = transition_map.begin(); iter_transition_map != transition_map.end(); iter_transition_map++)
  {
     for(iter_process_transition_map = iter_transition_map->second.begin();
         iter_process_transition_map != iter_transition_map->second.end();iter_process_transition_map++)
     {
       for(iter_ext_transition_vector = iter_process_transition_map->second.begin();
           iter_ext_transition_vector != iter_process_transition_map->second.end();iter_ext_transition_vector++)
       {
         cout<<"-----------------------------------------------------------"<<endl;
         iter_ext_transition_vector->first->write(cout);
         cout<<endl;
         if(iter_ext_transition_vector->first->get_guard()!= 0 )
         {
           write_C(*iter_ext_transition_vector->first->get_guard(), cout, state_name);
           cout<<endl;
         }
         if(iter_ext_transition_vector->synchronized)
         {
           iter_ext_transition_vector->second->write(cout);
           cout<<endl;
           if(iter_ext_transition_vector->second->get_guard()!= 0 )
           {
            write_C(*iter_ext_transition_vector->second->get_guard(), cout, state_name);
            cout<<endl;
           }
         }
         if(sytem_with_property)
         {
           iter_ext_transition_vector->property->write(cout);
           cout<<endl;
           if(iter_ext_transition_vector->property->get_guard()!= 0 )
           {
            write_C(*iter_ext_transition_vector->property->get_guard(), cout, state_name);
            cout<<endl;
           }
         }
         cout<<"-----------------------------------------------------------"<<endl;
       }
    }
  }
  for(size_int_t i = 0; i < this->get_process_count(); i++)
  {
    cout<<get_symbol_table()->get_process(i)->get_name()<<endl;
  }*/

  // get_succ
  string space = "   ";
  bool some_commited_state = false;
  ostr << "extern "<< '"' << "C" << '"' << " int lib_get_succ(state_t state, succ_container_t & succ_container)" <<endl;
  ostr << " {"<<endl;
  ostr << space << "succ_container.clear();"<<endl;
  ostr << space << "bool processes_in_deadlock = true;"<<endl;
  ostr << space << "state_struct_t *p_state_struct = reinterpret_cast<state_struct_t*>(state.ptr);"<<endl;
  ostr << space << "state_struct_t "<<state_name<<" = *p_state_struct;"<<endl;
  ostr << space << "if( ";
  for(size_int_t i = 0; i < this->get_process_count(); i++)
   for(size_int_t j = 0; j < dynamic_cast<dve_process_t*>(this->get_process(i))->get_state_count(); j++)
   {
     if(dynamic_cast<dve_process_t*>(this->get_process(i))->get_commited(j))
     {
       if(some_commited_state)
         ostr << " || ";
       else
         some_commited_state = true;
       ostr << state_name << "." << get_symbol_table()->get_process(i)->get_name() << ".state" << " == "<<j;
     }
   }
   if(some_commited_state) 
    ostr << " )" << endl;
   else
    ostr << "false )" << endl;
   ostr << space << " { " << endl;  // in commited state
   space = space + "    ";

   for(size_int_t i = 0; i < this->get_process_count(); i++)
   {
     if(!sytem_with_property || i != this->get_property_gid())
     {
       ostr << space <<"switch ( "<< state_name <<"." << get_symbol_table()->get_process(i)->get_name() << ".state )" <<endl;
       ostr << space <<" {"<<endl;
       if(transition_map.find(i) != transition_map.end())
         for(iter_process_transition_map = transition_map.find(i)->second.begin();
             iter_process_transition_map != transition_map.find(i)->second.end();iter_process_transition_map++)
         {
           if(dynamic_cast<dve_process_t*>(this->get_process(i))->get_commited(iter_process_transition_map->first))
           {
             ostr << space << "    case " << iter_process_transition_map->first<<" : "<<endl;
             ostr << space << "      {"<<endl;
             for(iter_ext_transition_vector = iter_process_transition_map->second.begin();
               iter_ext_transition_vector != iter_process_transition_map->second.end();iter_ext_transition_vector++)
             {
               if( !iter_ext_transition_vector->synchronized || 
                  dynamic_cast<dve_process_t*>(this->get_process(iter_ext_transition_vector->second->get_process_gid()))->
                  get_commited(iter_ext_transition_vector->second->get_state1_lid()) ) // !! jak je to s property synchronizaci v comitted stavech !!
               {
                 ostr << space << "        if( ";
                 bool has_guard = false;
                 if(iter_ext_transition_vector->first->get_guard()!= 0 )
                 {
                   ostr << "( ";
                   write_C(*iter_ext_transition_vector->first->get_guard(), ostr, state_name);
                   has_guard = true;
                   ostr << ") ";
                 }
                 if(iter_ext_transition_vector->synchronized)
                 {
                   if(has_guard)
                     ostr <<" && "; 
                   else
                     has_guard = true;
                   ostr << state_name << "." << get_symbol_table()->get_process(iter_ext_transition_vector->second->get_process_gid())->get_name()
                        << ".state == "<< iter_ext_transition_vector->second->get_state1_lid();
                   if(iter_ext_transition_vector->second->get_guard()!= 0 )
                   {
                     if(has_guard)
                       ostr <<" && "; 
                     else
                       has_guard = true;
                    ostr << "( ";
                    write_C(*iter_ext_transition_vector->second->get_guard(), ostr, state_name);
                    ostr << ") ";
                   }
                 }
                 else
                 {
                   if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_EXCLAIM_BUFFER)
                   {
                     if(has_guard)
                       ostr <<" && "; 
                     else
                       has_guard = true;
                     ostr << "( ";
                     ostr << state_name << "." <<get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                          << "." <<"number_of_items != "
                          << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_channel_buffer_size();
                     ostr << ") ";
                   }
                   if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_ASK_BUFFER)
                   {
                     if(has_guard)
                       ostr <<" && "; 
                     else
                       has_guard = true;
                     ostr << "( ";
                     ostr << state_name << "." <<get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                          << "." <<"number_of_items != 0";
                     ostr << ") ";
                   }
                 }
                 if(sytem_with_property)
                 {
                   if(has_guard)
                     ostr <<" && "; 
                   else
                     has_guard = true;
                   ostr << state_name << "." << get_symbol_table()->get_process(iter_ext_transition_vector->property->get_process_gid())->get_name()
                        << ".state == "<< iter_ext_transition_vector->property->get_state1_lid();
                   if(iter_ext_transition_vector->property->get_guard()!= 0 )
                   {
                     if(has_guard)
                       ostr <<" && "; 
                     else
                      has_guard = true;
                    ostr << "( ";
                    write_C(*iter_ext_transition_vector->property->get_guard(), ostr, state_name);
                    ostr << ") ";
                   }
                 }
                 if(has_guard)
                   ostr <<" )"<<endl; 
                 else
                   ostr <<"true )"<<endl;
                 ostr << space << "          {" <<endl;

               /*  // for debuging 
                 ostr << space << "             cout << " << '"' << "------------------------------------------------------------------" << '"' << " << endl;" <<endl;
                 ostr << space << "             cout << " << '"';
                 ostringstream p_ostr;
                 iter_ext_transition_vector->first->write(p_ostr);
                 string s = p_ostr.str();
                 const string crlf = "\n";
                 unsigned int i=s.find(crlf);
                 while(i != string::npos)
                 {
                   s.replace(i,crlf.size(),"");
                   i=s.find(crlf);
                 }
                 ostr << s;
                 ostr << '"' << "<< endl;" << endl;
                 if(iter_ext_transition_vector->synchronized)
                 {
                   ostr << space << "             cout << " << '"';
                   ostringstream p_ostr;
                   iter_ext_transition_vector->second->write(p_ostr);
                   string s = p_ostr.str();
                   const string crlf = "\n";
                   unsigned int i=s.find(crlf);
                   while(i != string::npos)
                   {
                     s.replace(i,crlf.size(),"");
                     i=s.find(crlf);
                   }
                   ostr << s;  
                   ostr << '"' << "<< endl;" << endl;
                 }
                 if(sytem_with_property)
                 {
                   ostr << space << "             cout << " << '"';
                   ostringstream p_ostr;
                   iter_ext_transition_vector->property->write(p_ostr);
                   string s = p_ostr.str();
                   const string crlf = "\n";
                   unsigned int i=s.find(crlf);
                   while(i != string::npos)
                   {
                     s.replace(i,crlf.size(),"");
                     i=s.find(crlf);
                   }
                   ostr << s;  
                   ostr << '"' << "<< endl;" << endl;
                 }
                 ostr << space << "             cout << " << '"' << "------------------------------------------------------------------" << '"' << " << endl;" <<endl;
                 // for debuging */

                 ostr << space << "             processes_in_deadlock = false;" <<endl;
                 ostr << space << "             state_t new_state = duplicate_state(state);" <<endl;
                 ostr << space << "             state_struct_t *p_new_c_state = reinterpret_cast<state_struct_t*>(new_state.ptr);"<<endl;
                 //synchronization effect
                 if(iter_ext_transition_vector->synchronized)
                 {
                    for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                    {
                      ostr << space << "             ";
                      write_C(*iter_ext_transition_vector->first->get_sync_expr_list_item(s), ostr, "(*p_new_c_state)");
                      ostr << " = ";
                      write_C(*iter_ext_transition_vector->second->get_sync_expr_list_item(s), ostr, "(c_state)");
                      ostr << ";" <<endl;
                    }
                 }
                 else
                 {
                   if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_EXCLAIM_BUFFER)
                   {
                     for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                     {
                        ostr << space << "             ";
                        ostr << "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name()
                             << ".content[(c_state)."<< get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                             << ".number_of_items - 1].x" << s << " = ";
                        write_C(*iter_ext_transition_vector->first->get_sync_expr_list_item(s), ostr, "(c_state)");
                        ostr << ";" << endl;
                     }
                     ostr << space << "             ";
                     ostr << "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                          << ".number_of_items++;"<<endl;
                   }
                   if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_ASK_BUFFER)
                   {
                     for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                     {
                        ostr << space << "             ";
                        write_C(*iter_ext_transition_vector->first->get_sync_expr_list_item(s), ostr, "(*p_new_c_state)");
                        ostr << " = (c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name()
                             << ".content[0].x" << s <<";" <<endl;
                     }
                     ostr << space << "             ";
                     ostr << "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                          << ".number_of_items--;"<<endl;
                     ostr << space << "             ";
                     ostr << "for(size_int_t i = 1 ; i <= (*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                          << ".number_of_items; i++)"<<endl;
                     ostr << space << "               {" <<endl;
                     for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                     {
                       ostr << space << "                 ";
                       ostr <<  "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() << ".content[i-1].x" << s
                            << " = (c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() << ".content[i].x" << s << ";" <<endl;
                       ostr << space << "                 ";
                       ostr <<  "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() << ".content[i].x" << s
                            << " = 0;" <<endl;
                     }
                     ostr << space << "               }" <<endl;
                   }
                 }
                 //first transition effect 
                 ostr << space << "             (*p_new_c_state)."<<get_symbol_table()->get_process(iter_ext_transition_vector->first->get_process_gid())->get_name()
                      << ".state = "<< iter_ext_transition_vector->first->get_state2_lid()<< ";" <<endl;
                 for(size_int_t e = 0;e < iter_ext_transition_vector->first->get_effect_count();e++)
                 {
                    ostr << space << "             ";
                    write_C(*iter_ext_transition_vector->first->get_effect(e), ostr, "(*p_new_c_state)");
                    ostr <<";"<< endl;
                 }
                 if(iter_ext_transition_vector->synchronized) //second transiton effect
                 {
                   ostr << space << "             (*p_new_c_state)."<<get_symbol_table()->get_process(iter_ext_transition_vector->second->get_process_gid())->get_name()
                        << ".state = "<< iter_ext_transition_vector->second->get_state2_lid()<< ";" <<endl;
                   for(size_int_t e = 0;e < iter_ext_transition_vector->second->get_effect_count();e++)
                   {
                      ostr << space << "             ";
                      write_C(*iter_ext_transition_vector->second->get_effect(e), ostr, "(*p_new_c_state)");
                      ostr <<";"<< endl;
                   }
                 }
                 if(sytem_with_property) //change of the property process state
                 {
                   ostr << space << "             (*p_new_c_state)."<<get_symbol_table()->get_process(iter_ext_transition_vector->property->get_process_gid())->get_name()
                        << ".state = "<< iter_ext_transition_vector->property->get_state2_lid()<< ";" <<endl;
                 }
                 ostr << space << "             succ_container.push_back(new_state);"<<endl;
                 ostr << space << "          }" <<endl;
               }
             }
             ostr << space << "        break;"<<endl;
             ostr << space << "      }"<<endl;
           }
         }
       ostr << space <<" }"<<endl;
     }
   }
   space = "   ";
   ostr << space << " } " <<endl;


   ostr << space << " else" << endl; // no in commited state
   ostr << space << " {" << endl;
   space = space + "    ";

   for(size_int_t i = 0; i < this->get_process_count(); i++)
   {
     if(!sytem_with_property || i != this->get_property_gid())
     {
       ostr << space <<"switch ( "<< state_name <<"." << get_symbol_table()->get_process(i)->get_name() << ".state )" <<endl;
       ostr << space <<" {"<<endl;
       if(transition_map.find(i) != transition_map.end())
         for(iter_process_transition_map = transition_map.find(i)->second.begin();
             iter_process_transition_map != transition_map.find(i)->second.end();iter_process_transition_map++)
         {
           ostr << space << "    case " << iter_process_transition_map->first<<" : "<<endl;
           ostr << space << "      {"<<endl;
           for(iter_ext_transition_vector = iter_process_transition_map->second.begin();
               iter_ext_transition_vector != iter_process_transition_map->second.end();iter_ext_transition_vector++)
           {
             ostr << space << "        if( ";
             bool has_guard = false;
             if(iter_ext_transition_vector->first->get_guard()!= 0 )
             {
               ostr << "( ";
               write_C(*iter_ext_transition_vector->first->get_guard(), ostr, state_name);
               has_guard = true;
               ostr << " ) ";
             }
             if(iter_ext_transition_vector->synchronized)
             {
               if(has_guard)
                 ostr <<" && "; 
               else
                 has_guard = true;
               ostr << state_name << "." << get_symbol_table()->get_process(iter_ext_transition_vector->second->get_process_gid())->get_name()
                    << ".state == "<< iter_ext_transition_vector->second->get_state1_lid();
               if(iter_ext_transition_vector->second->get_guard()!= 0 )
               {
                 if(has_guard)
                   ostr <<" && "; 
                 else
                   has_guard = true;
                 ostr << "( ";
                 write_C(*iter_ext_transition_vector->second->get_guard(), ostr, state_name);
                 ostr << ") ";
               }
             }
             else
             {
               if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_EXCLAIM_BUFFER)
               {
                 if(has_guard)
                   ostr <<" && "; 
                 else
                   has_guard = true;
                 ostr << "( ";
                 ostr << state_name << "." <<get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                      << "." <<"number_of_items != "
                      << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_channel_buffer_size();
                 ostr << ") ";
               }
               if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_ASK_BUFFER)
               {
                 if(has_guard)
                   ostr <<" && "; 
                 else
                   has_guard = true;
                 ostr << "( ";
                 ostr << state_name << "." <<get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                      << "." <<"number_of_items != 0";
                 ostr << ") ";
               }
             }
             if(sytem_with_property)
             {
               if(has_guard)
                 ostr <<" && "; 
               else
                 has_guard = true;
               ostr << state_name << "." << get_symbol_table()->get_process(iter_ext_transition_vector->property->get_process_gid())->get_name()
                    << ".state == "<< iter_ext_transition_vector->property->get_state1_lid();
               if(iter_ext_transition_vector->property->get_guard()!= 0 )
               {
                 if(has_guard)
                   ostr <<" && "; 
                 else
                   has_guard = true;
                ostr << "( ";
                write_C(*iter_ext_transition_vector->property->get_guard(), ostr, state_name);
                ostr << ") ";
               }
             }
             if(has_guard)
               ostr <<" )"<<endl; 
             else
               ostr <<"true )"<<endl;
             ostr << space << "          {" <<endl;

           /*  // for debuging 
             ostr << space << "             cout << " << '"' << "------------------------------------------------------------------" << '"' << " << endl;" <<endl;
             ostr << space << "             cout << " << '"';
             ostringstream p_ostr;
             iter_ext_transition_vector->first->write(p_ostr);
             string s = p_ostr.str();
             const string crlf = "\n";
             unsigned int i=s.find(crlf);
             while(i != string::npos)
             {
               s.replace(i,crlf.size(),"");
               i=s.find(crlf);
             }
             ostr << s;
             ostr << '"' << "<< endl;" << endl;
             if(iter_ext_transition_vector->synchronized)
             {
               ostr << space << "             cout << " << '"';
               ostringstream p_ostr;
               iter_ext_transition_vector->second->write(p_ostr);
               string s = p_ostr.str();
               const string crlf = "\n";
               unsigned int i=s.find(crlf);
               while(i != string::npos)
               {
                 s.replace(i,crlf.size(),"");
                 i=s.find(crlf);
               }
               ostr << s;  
               ostr << '"' << "<< endl;" << endl;
             }
             if(sytem_with_property)
             {
               ostr << space << "             cout << " << '"';
               ostringstream p_ostr;
               iter_ext_transition_vector->property->write(p_ostr);
               string s = p_ostr.str();
               const string crlf = "\n";
               unsigned int i=s.find(crlf);
               while(i != string::npos)
               {
                 s.replace(i,crlf.size(),"");
                 i=s.find(crlf);
               }
               ostr << s;  
               ostr << '"' << "<< endl;" << endl;
             }
             ostr << space << "             cout << " << '"' << "------------------------------------------------------------------" << '"' << " << endl;" <<endl;
             // for debuging */

             ostr << space << "             processes_in_deadlock = false;" <<endl;
             ostr << space << "             state_t new_state = duplicate_state(state);" <<endl;
             ostr << space << "             state_struct_t *p_new_c_state = reinterpret_cast<state_struct_t*>(new_state.ptr);"<<endl;
             //synchronization effect
             if(iter_ext_transition_vector->synchronized)
             {
                for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                {
                  ostr << space << "             ";
                  write_C(*iter_ext_transition_vector->first->get_sync_expr_list_item(s), ostr, "(*p_new_c_state)");
                  ostr << " = ";
                  write_C(*iter_ext_transition_vector->second->get_sync_expr_list_item(s), ostr, "(c_state)");
                  ostr << ";" <<endl;
                }
             }
             else
             {
               if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_EXCLAIM_BUFFER)
               {
                 for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                 {
                    ostr << space << "             ";
                    ostr << "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name()
                         << ".content[(c_state)."<< get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                         << ".number_of_items].x" << s << " = ";
                    write_C(*iter_ext_transition_vector->first->get_sync_expr_list_item(s), ostr, "(c_state)");
                    ostr << ";" << endl;
                 }
                 ostr << space << "             ";
                 ostr << "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                         << ".number_of_items++;"<<endl;
               }
               if(iter_ext_transition_vector->first->get_sync_mode() == SYNC_ASK_BUFFER)
               {
                 for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                 {
                    ostr << space << "             ";
                    write_C(*iter_ext_transition_vector->first->get_sync_expr_list_item(s), ostr, "(*p_new_c_state)");
                    ostr << " = (c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name()
                         << ".content[0].x" << s <<";" <<endl;
                 }
                 ostr << space << "             ";
                 ostr << "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                      << ".number_of_items--;"<<endl;
                 ostr << space << "             ";
                 ostr << "for(size_int_t i = 1 ; i <= (*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() 
                      << ".number_of_items; i++)"<<endl;
                 ostr << space << "               {" <<endl;
                 for(size_int_t s = 0;s < iter_ext_transition_vector->first->get_sync_expr_list_size();s++)
                 {
                   ostr << space << "                 ";
                   ostr <<  "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() << ".content[i-1].x" << s
                        << " = (c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() << ".content[i].x" << s << ";" <<endl;
                   ostr << space << "                 ";
                   ostr <<  "(*p_new_c_state)." << get_symbol_table()->get_channel(iter_ext_transition_vector->first->get_channel_gid())->get_name() << ".content[i].x" << s
                        << " = 0;" <<endl;
                 }
                 ostr << space << "               }" <<endl;
               }
             }
             //first transition effect 
             ostr << space << "             (*p_new_c_state)."<<get_symbol_table()->get_process(iter_ext_transition_vector->first->get_process_gid())->get_name()
                  << ".state = "<< iter_ext_transition_vector->first->get_state2_lid()<< ";" <<endl;
             for(size_int_t e = 0;e < iter_ext_transition_vector->first->get_effect_count();e++)
             {
                ostr << space << "             ";
                write_C(*iter_ext_transition_vector->first->get_effect(e), ostr, "(*p_new_c_state)");
                ostr <<";"<< endl;
             }
             if(iter_ext_transition_vector->synchronized) //second transiton effect
             {
               ostr << space << "             (*p_new_c_state)."<<get_symbol_table()->get_process(iter_ext_transition_vector->second->get_process_gid())->get_name()
                    << ".state = "<< iter_ext_transition_vector->second->get_state2_lid()<< ";" <<endl;
               for(size_int_t e = 0;e < iter_ext_transition_vector->second->get_effect_count();e++)
               {
                  ostr << space << "             ";
                  write_C(*iter_ext_transition_vector->second->get_effect(e), ostr, "(*p_new_c_state)");
                  ostr <<";"<< endl;
               }
             }
             if(sytem_with_property) //change of the property process state
             {
               ostr << space << "             (*p_new_c_state)."<<get_symbol_table()->get_process(iter_ext_transition_vector->property->get_process_gid())->get_name()
                    << ".state = "<< iter_ext_transition_vector->property->get_state2_lid()<< ";" <<endl;
             }
             ostr << space << "             succ_container.push_back(new_state);"<<endl;
             ostr << space << "          }" <<endl;
           }
           ostr << space << "        break;"<<endl;
           ostr << space << "      }"<<endl;
         }
       ostr << space <<" }"<<endl;
     }
   }
   space = "   ";
   ostr << space << " } " <<endl;
   ostr << space << "if( processes_in_deadlock )" <<endl;
   ostr << space << " {" <<endl;
   for(iter_property_transitions = property_transitions.begin();iter_property_transitions != property_transitions.end();
       iter_property_transitions++)
   {
        ostr << space << "   if( ";
        ostr << state_name << "." << get_symbol_table()->get_process((*iter_property_transitions)->get_process_gid())->get_name()
                    << ".state == "<< (*iter_property_transitions)->get_state1_lid();
        if( (*iter_property_transitions)->get_guard()!= 0 )
        {
          ostr << space << " && ( ";
          write_C(*(*iter_property_transitions)->get_guard(), ostr, state_name);
          ostr << " )"<<endl;
        }
        ostr << " )"<<endl;

        ostr << space << "    {" <<endl;
        ostr << space << "      state_t new_state = duplicate_state(state);" <<endl;
        ostr << space << "      state_struct_t *p_new_c_state = reinterpret_cast<state_struct_t*>(new_state.ptr);"<<endl;
        ostr << space << "      (*p_new_c_state)."<<get_symbol_table()->get_process((*iter_property_transitions)->get_process_gid())->get_name()
                    << ".state = "<< (*iter_property_transitions)->get_state2_lid()<< ";" <<endl;
        ostr << space << "      succ_container.push_back(new_state);"<<endl;
        ostr << space << "    }"<<endl;
   }
   ostr << space << " }" <<endl;
   ostr << space << "return 0;" << endl;
   ostr << " }"<<endl;

   ostr<<endl;

  // is_accepting
  ostr << "extern "<< '"' << "C" << '"' << " bool lib_is_accepting(state_t state)" <<endl; // only Buchi acceptance condition
  ostr << " {"<<endl;
  if(sytem_with_property)
  {
    ostr << space << "state_struct_t *p_state_struct = reinterpret_cast<state_struct_t*>(state.ptr);"<<endl;
    ostr << space << "state_struct_t "<<state_name<<" = *p_state_struct;"<<endl;
    for(size_int_t i = 0; i < dynamic_cast<dve_process_t*>(this->get_process((this->get_property_gid())))->get_state_count(); i++)
    {
      if (dynamic_cast<dve_process_t*>(this->get_process((this->get_property_gid())))->get_acceptance(i, 0, 1) )
      {
         ostr << space << "if(" << state_name << "." << get_symbol_table()->get_process(this->get_property_gid())->get_name() << ".state == " 
              <<  i  <<" ) return true;" << endl;
      }
    }
    ostr << space << "return false;" << endl;
  }
  else{
  ostr << "    return false;"<<endl;
  }
  ostr << " }"<<endl;

  ostr<<endl;

  // get_initial_state
  ostr << "extern "<< '"' << "C" << '"' << " state_t lib_get_initial_state()" <<endl;
  ostr << " {"<<endl;
  ostr << "   state_t initial_state = new_state(" <<this->get_space_sum()<< ");"<<endl;
  ostr << "   return initial_state;"<<endl;
  ostr << " }"<<endl;

}

void dveC_explicit_system_t::print_print_state(ostream & ostr)
{
 string state_name = "c_state.";
 string state_name_p = "c_state";
 string space = "   ";
 string name;
 ostr << "extern "<< '"' << "C" << '"' << " void lib_print_state(state_t state, std::ostream & outs)" <<endl;
 ostr << " {"<<endl;
 ostr << space << "state_struct_t *p_state_struct = reinterpret_cast<state_struct_t*>(state.ptr);"<<endl;
 ostr << space << "state_struct_t "<<state_name_p<<" = *p_state_struct;"<<endl;
 for (size_int_t i=0; i!=state_creators_count; ++i)
  {
   switch (state_creators[i].type)
    {
     case state_creator_t::VARIABLE:
      {
       if (i) 
         ostr << space <<" outs << "<< '"' << ", " << '"' << ";" <<endl;
       else 
	 ostr << space <<" outs << "<< '"' << "[" << '"'<< ";" <<endl;
       name=get_symbol_table()->get_variable(state_creators[i].gid)->get_name();
       ostr << space << " outs << " << '"' << name << '"' << " << " << '"' << ":" << '"' << ";" << endl;
       if (state_creators[i].array_size)
        {
          ostr << space <<" outs << "<< '"' << "{" << '"' << ";" <<endl;
          for(size_int_t j=0; j!= state_creators[i].array_size; j++)
          {
            ostr << space << " outs << (int)" << state_name << name << "[" <<  j << "]" << ";" <<endl;
            if (j!=(state_creators[i].array_size-1)) 
              ostr << space << " outs << "<< '"' << "|" << '"' << ";" <<endl;
          }
         ostr << space <<" outs << "<< '"' << "}" << '"' << ";" << endl;
        }
       else
        {
         ostr << space << " outs << (int)" << state_name << name << ";" <<endl;
        }
      }
     break;
     case state_creator_t::PROCESS_STATE:
      {
       state_name = "c_state." +  string(get_symbol_table()->get_process(state_creators[i].gid)->get_name()) + ".";
       if (i) 
         ostr << space << " outs << "<< '"' << "]" << '"' << " << endl;" <<endl;
       else 
	 ostr << space <<" outs << "<< '"' << "[]"  << '"' << " << endl;"<<endl;
       ostr << space <<" outs << "<< '"' << get_symbol_table()->get_process(state_creators[i].gid)->get_name() << ":[" << '"' <<";"<<endl;
       ostr << space << " switch ( " << state_name <<"state )" << endl ;
       ostr << space << "   {" <<endl;
       for(size_int_t j = 0; j < dynamic_cast<dve_process_t*>(this->get_process(state_creators[i].gid))->get_state_count(); j++)
       {  
         ostr << space << "     case " << j << " : " <<  "{ outs << " <<  '"'
              << get_symbol_table()->get_state(dynamic_cast<dve_process_t*>(this->get_process(state_creators[i].gid))->get_state_gid(j))->get_name()
              << '"' << "; break; }"<<endl;
       }
       ostr << space << "   }" <<endl;
      }
     break;
     case state_creator_t::CHANNEL_BUFFER:
      {
        if (i) 
         ostr << space <<" outs << "<< '"' << ", " << '"' << ";" <<endl;
       else 
        ostr << space <<" outs << "<< '"' << "[" << '"'<< ";" <<endl;
       name=get_symbol_table()->get_channel(state_creators[i].gid)->get_name();
       ostr << space << " outs << " << '"' << name << '"' << " << " << '"' << ":" << '"' << ";" << endl;
       ostr << space << " if(c_state. " << name << ".number_of_items )" << endl;
       ostr << space << "   {" <<endl;
       ostr << space << "      outs << "<< '"' << "{" << '"' << ";" <<endl;
       ostr << space << "      for(size_int_t j=0; j!= c_state." << name << ".number_of_items; j++)" <<endl;
       ostr << space << "      {" <<endl;
       ostr << space << "         outs << (int)c_state." << name << ".content[j].x0 ; " <<endl; // only one value 
       ostr << space << "         if (j!=c_state." << name << ".number_of_items-1)" <<endl; 
       ostr << space << "           outs << "<< '"' << "|" << '"' << ";" <<endl;
       ostr << space << "      }" <<endl;
       ostr << space << "      outs << "<< '"' << "}" << '"' << ";" << endl;
       ostr << space << "   }" <<endl;
       ostr << space << "   else{ outs << "<< '"' << "EMPTY" << '"' << ";}" <<endl;
      }
     break;
     default: gerr << "Unexpected error" << thr();
     break;
    };
  }
 ostr << space << " outs << " << '"' << "]" << '"'<<" << endl;" <<endl;
 ostr << " }"<<endl;
}



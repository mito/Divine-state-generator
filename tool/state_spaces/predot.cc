#include <getopt.h>
#include <iostream>
#include <queue>
#include <sstream>
#include <unistd.h>

#include "sevine.h"

#define ES_FMT_DIVIDE_PROCESSES_BY_BACKSLASH_N 16

using namespace std;
using namespace divine;

explicit_system_t *sys;
process_decomposition_t *property_decomposition;

dve_prob_explicit_system_t psys(gerr);
explicit_storage_t st;
unsigned int max_b=0;
bool draw_accepting = false, draw_components = false, show_deadlocks = false;
bool prob=false;
bool use_print_state=false;


struct appendix_t {
  int depth;
};

appendix_t app,app1;

void print_transition(const transition_t * const trans)
{
 const dve_transition_t * const dve_trans =
   dynamic_cast<const dve_transition_t*>(trans);
 
 static const char * state1 = 0, * state2 = 0, * synchro_channel = 0;
 string synchro_value, guard;
 bool guard_b=false,synchro_value_b=false;
 // cout << dve_trans->get_lid() << ": ";
 state1 = dve_trans->get_state1_name();
 state2 = dve_trans->get_state2_name();

 guard_b = dve_trans->get_guard_string(guard);
 synchro_value_b = dve_trans->get_sync_expr_string(synchro_value);
 synchro_channel = dve_trans->get_sync_channel_name();

 cout << state1 << "->" << state2 <<"\\n";

 if (guard_b) cout << guard;
 if (guard_b && synchro_channel) cout <<";";

 if (synchro_channel) cout << "sync " << synchro_channel <<
                      (dve_trans->is_sync_ask()?'?':'!') <<
                      (synchro_value_b?(synchro_value):"");
 if (dve_trans->get_effect_count())
  {
    cout <<"\\n";
    //cout << "effect ";
    for (std::size_t i=0; i!=dve_trans->get_effect_count(); i++)
      {
	cout << dve_trans->get_effect(i)->to_string()
	     << ((i==(dve_trans->get_effect_count()-1)) ? "" : ", ");
      }
  }
 // cout << "}";
 //cout << endl;
}


void print_state(state_t state,int format)
{
  if (use_print_state)
    {
      sys->print_state(state, cout);
      return;
    }
  format &= ~ES_FMT_DIVIDE_PROCESSES_BY_CR;
  ostringstream mystream;
  if (prob)
    psys.DBG_print_state(state,mystream,format);
  else
    dynamic_cast<dve_explicit_system_t*>(sys)->DBG_print_state(state,mystream,format);
  string str = mystream.str();
  size_t i=0;

  if (max_b!=0)
    {
      while ((i=str.find('[',i)) != string::npos)
	{
	  str.replace(i,1,"");
	}
      
      i=0;
      while ((i=str.find('{',i)) != string::npos)
	{
	  str.replace(i,1,"");
	}
      i=0;
      while ((i=str.find('}',i)) != string::npos)
	{
	  str.replace(i,1,"");
	}
      i=0;
      while ((i=str.find('|',i)) != string::npos)
	{
	  str.replace(i,1,",");
	}
      i=0;
      while ((i=str.find(']',i)) != string::npos)
	{
	  str.replace(i,1,"");
	}
      i=0;
      while ((i=str.find(';',i)) != string::npos)
	{
	  str.replace(i,1,",");
	}
      i=0;
      while ((i=str.find(' ',i)) != string::npos)
	{
	  str.replace(i,1,"");
	}
      
      int count=0;
      for (unsigned int j=0;j<str.length();j++)
	{
	  i=str.find(',',j);
	  if (i==j)
	    {
	      count ++;
	      if (count%max_b == 0)
		str.replace(i,1,"\\n");
	    }
	}
    }
  else
    {
      
      i=0;  
      while ((i=str.find(';',i)) != string::npos)
	{
	  str.replace(i,2,"\\n");
	  i=i+3;
	}
    }
  cout << str;
}

void version()
{
  cout <<"predot ";
  cout <<"1.0 build 9 (2007/Jun/04 12:50)"<<endl;
}

void usage()
{
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"-----------------------------------------------------------------"<<endl;
  version();
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<endl;
  cout <<"Predot transforms a model file to format readable by 'dot'."<<endl;
  cout <<endl;
  cout <<"Usage: predot [switches] input_file"<<endl;
  cout <<"Switches:"<<endl;
  cout <<" -a    draw accepting states (Buchi acceptance only)" <<endl ;
  cout <<" -b    colorize backlevel edges" <<endl;
  cout <<" -c    colorize components" <<endl;
  cout <<" -d    colorize deadlocks" <<endl;
  cout <<" -p    colorize probabilistic transitions" <<endl;
  cout <<" -P    print only probabilities of probabilistic transitions" <<endl;
  cout <<" -D n  show only 1st n levels of the graph" <<endl;
  cout <<" -h    show this help" <<endl;
  cout <<" -l    landscape" <<endl;
  cout <<" -t    show transition labels" <<endl ;
  cout <<" -v    show predot version"<<endl;
  cout <<" -w n  bound line in a state to n characters" <<endl;
  return;
}

int main(int argc, char **argv)
{
  MCRL2_ATERM_INIT(argc,argv);

  
  bool recognized = false;
  int file_opening=0;
  
  char *filename = argv[1];
  int filename_length = strlen(filename);

  if (filename_length>=4 && strcmp(filename+filename_length-4,".dve")==0)
    {
      sys = new dve_explicit_system_t(gerr);
      file_opening = sys->read(argv[optind]);
      recognized = true;
    }

  if (!recognized)
    {
      cerr << "File type not recognized. Supported extensions are .dve, .probdve, .b and .lps" << endl;
      return 1;
    }


  if (file_opening)
    {
      cerr <<"Filename "<<argv[optind]<<" does not exist."<<endl;
      return 1;
    }

    state_t state;
    state = sys->get_initial_state();
    print_state(state, 0);


  //cout <<ref.page_id<<","<<ref.hres<<","<<ref.id<<endl<<flush;
    int succs_result;
    succ_container_t succs(*sys);
    succs_result = sys->get_succs(state,succs);	

    for (std::size_t info_index=0; info_index!=succs.size(); info_index++) {
        print_state(state,0);
        cout <<"\"->\"";

        //sys.DBG_print_state(succs[info_index],cout,16);
        print_state(succs[info_index],0);
        cout <<"\"";
    }
      
    return 0;
}

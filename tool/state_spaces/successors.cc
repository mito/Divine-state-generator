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

  bool show_probtrans = false;
  bool color_prob = false;
  bool backlevel_edges=false; 
  bool print_trans=false,landscape=false;
  bool use_succs_only=false;
  char c;
  int d=0,max_d=0;

  static const option longopts[]={
    {"version",0,0,'V'},
    {NULL,0,0,0}
  };

  opterr = 0;
  while ((c = getopt_long(argc, argv, "abcdpPD:hltVw:", longopts, NULL)) != -1) {
    switch (c) {
    case 'a': draw_accepting = true; break;
    case 'b': backlevel_edges = true; break;
    case 'c': draw_components = true; break;
    case 'd': show_deadlocks = true; break;
    case 'p': color_prob = true; break;
    case 'P': show_probtrans = true; break;
    case 'D': max_d = atoi(optarg); break;
    case 'h': usage(); return 0;break;
    case 'l': landscape = true; break;
    case 't': print_trans = true;break;
    case 'V': version();return 0;break;
    case 'w': max_b = atoi(optarg); break;
    case '?': cerr <<"skipping unknown switch -"<<(char)optopt<<endl;break;
    }
  }

  if (argc<optind+1)
    {
      usage();
      return 0;
    }
  
  bool recognized = false;
  int file_opening=0;
  
  char *filename = argv[optind];
  int filename_length = strlen(filename);
  if (filename_length>=8 && strcmp(filename+filename_length-8,".probdve")==0)
    {
      file_opening = psys.read(argv[optind]);
      recognized = true;
      prob=true;
    }

  if (filename_length>=4 && strcmp(filename+filename_length-4,".dve")==0)
    {
      sys = new dve_explicit_system_t(gerr);
      file_opening = sys->read(argv[optind]);
      recognized = true;
    }

  if (filename_length>=2 && strcmp(filename+filename_length-2,".b")==0)
    {
      sys = new bymoc_explicit_system_t(gerr);
      file_opening = sys->read(argv[optind]);
      recognized = true;
      if (print_trans)
	{
	  cerr<<"Cannot print transitions for bymoc state spaces."<<endl;
	  print_trans = false;
	}
      use_succs_only = true;
      use_print_state = true;
    }

#ifdef HAVE_MCRL2
  if (filename_length>=4 && strcmp(filename+filename_length-4,".lps")==0)
    {
      sys = new mcrl2_explicit_system_t(gerr);
      file_opening = sys->read(argv[optind]);
      recognized = true;
      use_print_state = true;
    }
#endif

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

  property_decomposition = sys->get_property_decomposition();

  if (prob)
    {
      if (psys.get_with_property() && (psys.get_property_type()==BUCHI) && (draw_components || draw_accepting) )
	{
//  	  prop.set_system(&psys);
//  	  prop.parse_process(psys.get_property_gid());
	}
      else
	{
	  draw_accepting = false;
	  draw_components = false;
	}
    }
  else
    {

      if (sys->get_with_property() && (sys->get_property_type()==BUCHI) && (draw_components || draw_accepting) )
	{
//  	  prop.set_system(sys);
//  	  prop.parse_process(sys->get_property_gid());
	}
      else
	{
	  draw_accepting = false;
	  draw_components = false;
	}
    }

  st.set_appendix(app);
  st.init();
  
  state_ref_t ref;
  state_t state;

  if (prob)
    state = psys.get_initial_state();
  else
    state = sys->get_initial_state();

  st.insert(state,ref);

  //cout <<ref.page_id<<","<<ref.hres<<","<<ref.id<<endl<<flush;


  app.depth = d;
  st.set_app_by_ref(ref,app);

  queue<state_t> current,next,temp;
  next.push(state);

  cout <<"digraph G {";
  if (landscape)
    {
      cout <<"  orientation=\"landscape\" size=\"10.8,7.2\""<<endl;
    }
  else
    {
      cout <<"  size=\"7.2,10.8\""<<endl;
    }
  //cout <<"  size=\"7.2,10.8\""<<endl;

  while (!next.empty())
    {
      d++;

      // current -> temp
      while (!current.empty())
	{
	  state_t a;
	  a = current.front();
	  current.pop();
	  temp.push(a);
	}

      // next -> current
      cout <<"{ rank = same; ";
      while(!next.empty())
	{
	  state = next.front();
	  cout<<"\"";
	  //sys.DBG_print_state(state,cout,16);
	  print_state(state,0);
	  cout<<"\"; ";
	  next.pop();
	  current.push(state);
	}
      cout <<"}"<<endl;

      // temp -> next
//        while (!temp.empty())
//  	{
//  	  state_t a;
//  	  a = temp.front();
//  	  temp.pop();
//  	  next.push(a);
//  	}

      app.depth = d;
      while (!current.empty())
	{
	  prob_succ_container_t psuccs_cont(psys);
	  succ_container_t succs(*sys);
	  
	  
	  enabled_trans_container_t *enabled_trans;
	  enabled_trans_container_t penabled_trans(psys);	  

	  if (!use_succs_only)
	    {
	      enabled_trans = new enabled_trans_container_t(*sys);
	    }

	  state = current.front();
	  state_ref_t stref;
	  int succs_result;
	  if (prob)
	    {
	      succs_result = psys.get_succs(state,psuccs_cont,penabled_trans);		    
	      if (draw_accepting && psys.is_accepting(state))
		{
		  cout <<"\"";
		  print_state(state,0);
		  cout <<"\" [peripheries=2,style=bold];";
		}
	    }
	  else
	    {
	      if (use_succs_only)
		{
		  succs_result = sys->get_succs(state,succs);	
		}		
	      else
		{
		  succs_result = sys->get_succs(state,succs,*enabled_trans);	
		}

	      if (draw_accepting && sys->is_accepting(state))
		{
		  cout <<"\"";
		  print_state(state,0);
		  cout <<"\" [peripheries=2,style=bold];";
		}
	    }

	  if (draw_components || (show_deadlocks && succs_result))
	    {
	      cout <<"\"";
	      print_state(state,0);
	      cout <<"\" ";	      
	      cout <<"[color=";

	      if (succs_result && show_deadlocks)
		{		  
		  cout <<"red,style=filled";
		}
	      else
		{
		  switch (property_decomposition->get_process_scc_type(state))  {
		  case 0:
		    cout<<"black";
		    break;
		  case 1:
		    cout<<"brown";
		    break;
		  case 2:
		    cout<<"green";
		    break;
		  }
		}
	      //cout<<",style=bold";
	      cout<<"];"<<endl;
	    }
	  
	  current.pop();	 	

	  if (prob)
	    {
	      std::size_t ii=0;
  	      for (prob_succ_container_t::iterator info_index=psuccs_cont.begin();
  		   info_index!=psuccs_cont.end();
  		   info_index++, ii++)
		{
		  cout <<"\"";
		  print_state(state,0);
		  cout <<"\"->\"";

		  print_state((*info_index).state,0);		  
		  cout <<"\"";

		  if (color_prob)
		    {
		      if ((*info_index).sum != 0) //prob transition
			{
			  cout <<" [color=red] ";
			}
		    }
		  

		  if (backlevel_edges)
		    {
		      if (st.is_stored((*info_index).state,stref))
			{
			  st.get_app_by_ref(stref,app1);
			  if (app1.depth<d)
			    {
			      if ((color_prob) && ((*info_index).sum != 0))
				{
				  cout <<" [color=purple] ";
				}			      
			      else
				{
				  cout <<" [color=blue] ";
				}
			    }
			}
		    }

		  if (print_trans)
  		    {
  		      cout <<" [label=\"";
		      if ((*info_index).sum == 0) //simple nondeterministic transition
			{
			  enabled_trans_t & enabled = penabled_trans[ii];
			  for (std::size_t i=0; i!=enabled.get_count(); ++i)
			    {
			      print_transition(enabled[i]);
			      if (i!=enabled.get_count()-1) cout << "\\n";
			    }
			}
		      else
			{
			  cout <<(*info_index).weight <<"/"<<(*info_index).sum<<"\\n("
			       <<(signed int)(*info_index).prob_and_property_trans.prob_trans_gid
			       <<","
			       <<(signed int)(*info_index).prob_and_property_trans.property_trans_gid
			       <<")";
			}      
		      cout <<"\"]";
  		    }

		  if (show_probtrans)
		    {
		      if ((*info_index).sum != 0) //prob transition
			{
			  cout <<" [label=\"";
			  cout <<(*info_index).weight <<"/"<<(*info_index).sum<<"\\n("
			       <<(signed int)(*info_index).prob_and_property_trans.prob_trans_gid
			       <<","
			       <<(signed int)(*info_index).prob_and_property_trans.property_trans_gid
			       <<")";
			  
			  cout <<"\"]";
			}      

		    }
		  
  		  cout <<endl;		 
		  }

	      for (prob_succ_container_t::iterator pi = psuccs_cont.begin(); 
		   pi!= psuccs_cont.end(); ++pi) 
		{
		  state_t r=(*pi).state;
		  
		  if ((st.is_stored(r)) || (d==max_d))
		    {
		      delete_state(r);
		    }
		  else
		    {
		      next.push(r);
		      st.insert(r,ref);
		      st.set_app_by_ref(ref,app);
		    }
		}	      
	    }
	  else
	    {
	      for (std::size_t info_index=0; info_index!=succs.size(); info_index++)
		{
		  cout <<"\"";
		  //sys.DBG_print_state(state,cout,16);
		  print_state(state,0);
		  cout <<"\"->\"";
		  
		  //sys.DBG_print_state(succs[info_index],cout,16);
		  print_state(succs[info_index],0);
		  cout <<"\"";
		  
		  if (backlevel_edges)
		    {
		      if (st.is_stored(succs[info_index],stref))
			{
			  st.get_app_by_ref(stref,app1);
			  if (app1.depth<d)
			    {
			      cout <<" [color=blue] ";
			    }
			}
		    }
		  
		  if (print_trans)
		    {
		      cout <<" [label=\"";
		      enabled_trans_t & enabled = (*enabled_trans)[info_index];
		      for (std::size_t i=0; i!=enabled.get_count(); ++i)
			{
			  print_transition(enabled[i]);
			  if (i!=enabled.get_count()-1) cout << "\\n";
			}
		      if (enabled.get_count() == 0)
			cout << enabled.to_string();
		      cout <<"\""; cout<<"]";
		    }
		  cout <<endl;
		}

	      for (succ_container_t::iterator i 
		     = succs.begin(); i!= succs.end(); ++i) 
		{
		  state_t r=*i;
		  
		  if ((st.is_stored(r)) || (d==max_d))
		    {
		      delete_state(r);
		    }
		  else
		    {
		      next.push(r);
		      st.insert(r,ref);
		      st.set_app_by_ref(ref,app);
		    }
		}
	    }

	  delete_state(state);
	}
      //  cout <<temp.size()<<" "<<current.size()<<" "<<next.size()<<endl;  
    }

  cout <<"}"<<endl;
  return 0;

}

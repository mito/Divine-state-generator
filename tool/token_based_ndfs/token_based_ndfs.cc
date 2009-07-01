#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include "divine.h"

using namespace divine;
using namespace std;

explicit_system_t * p_sys;
explicit_storage_t st;
distributed_t distributed;
distr_reporter_t reporter(&distributed);


const int TAG_TOKEN = DIVINE_TAG_USER + 0;
const int TAG_FINISHED = DIVINE_TAG_USER + 1;
const int TAG_STATE = DIVINE_TAG_USER + 2;
const int TAG_STATE_NESTED = DIVINE_TAG_USER + 3;
const int TAG_FINISHED_CYCLE = DIVINE_TAG_USER + 4;
const int TAG_CE = DIVINE_TAG_USER + 5;
const int TAG_CE_NESTED = DIVINE_TAG_USER + 6;


struct appenix_t 
{
  bool nested_dfs;
  bool on_stack;
} appendix;


struct info_t {
  int allstates;
  int alltrans;
  int alltrans_nested;
  int allsuccs;
  int allcross;
  int allcross_nested;
  long int allmem;
  virtual void update();
  virtual ~info_t() {}
};
 
updateable_info_t<info_t> info;

const char * input_file_ext = 0;
int depth,nid,nnn,where_found;
double init_time;
string ids,file_name,set_base_name;
bool finished=false,token=false, seed=false, cycle_found = false;
bool nested_allowed = false;
bool trail=false,reach_only=false,show_ce=false,report=false;
bool compiled_generator = false;
bool statistics=false;
bool quietmode=false;
bool logging=false;
bool base_name = false;
size_t htsize=0;
int pfunc=0;


int fsuccscalls=0;
int trans=0, transcross=0;
int trans_nested=0, transcross_nested=0;

timeinfo_t timer;
vminfo_t vm;
logger_t logger;

struct stack_member_t
{
  state_t state;
  int came_from;
  bool nested;
  bool backtracking;
};

stack <stack_member_t> gstack,gstack1,gstack2;

void info_t::update()
{
  if (nid == 0)
    {
      allstates = st.get_states_stored();
      allsuccs = fsuccscalls;
      alltrans = trans;
      alltrans_nested = trans_nested;
      allcross = transcross;
      allcross_nested = transcross_nested;
      allmem = vm.getvmsize();
    }
  else
    {
      allstates += st.get_states_stored();
      allsuccs += fsuccscalls;
      alltrans += trans;
      alltrans_nested += trans_nested;
      allcross += transcross;
      allcross_nested += transcross_nested;
      allmem += vm.getvmsize();
    }
}

size_t partition_function(state_t state)
{
  int tmp=0,start=0,end=state.size;
  int increase = 1;
  size_t result=73;
 
  unsigned char *tmp_ptr = reinterpret_cast<unsigned char*>(state.ptr);
 
  switch (pfunc) {
  case 0:
    break;
  case 1:
    increase = 2;
    break;
  case 2:
    start = start + state.size/2;
    break;
  case 3:
    end = end - state.size/4;
    break;
  }
 
  for(tmp=start; tmp<end;tmp+=increase)
    {
      if (*tmp_ptr==0)
        {
          result *= 6781;
        }
      else
        {
          result += 9311 ^ (*tmp_ptr) ;
          result *= 8713 ;
        }
      tmp_ptr ++;
    }
  return (result);
}

void process_message(char *buf, int size, int src, int tag, bool urgent)
{
  stack_member_t sm;
  switch(tag) 
    {
    case TAG_TOKEN:
      token = true;
      break;
    case TAG_FINISHED:
      finished = true;
      break;
    case TAG_FINISHED_CYCLE:
      finished = true;
      cycle_found = true;
      where_found = src;
      break;
    case TAG_CE:
    case TAG_CE_NESTED:
      sm.backtracking=true;
      sm.nested = (tag == TAG_CE_NESTED);
      sm.state = new_state(buf,size);
      gstack1.push(sm);
      distributed.network.send_message(NULL,0,src,TAG_TOKEN);
      break;
    case TAG_STATE_NESTED:
    case TAG_STATE:
      token = true;
      sm.came_from = src;
      sm.nested = (tag == TAG_STATE_NESTED);
      sm.backtracking = false;
      sm.state = new_state(buf,size);
      gstack.push(sm);	
      break;
    }
}

void version()
{
  cout <<"version 1.0 build 21 (2006/10/05 16:40)"<<endl;
}

void usage()
{
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"Token Based Nested DFS ";
  version();
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"Usage: [mpirun -np N] tbndfs [options] input_file"<<endl;
  cout <<"Options: "<<endl;
  cout <<" -v\tshow tbndfs version"<<endl;
  cout <<" -h\tshow this help"<<endl;
  cout <<" -r\tproduce report (file.report)"<<endl;
  cout <<" -p\tDVE file is precompiled prior verification"<<endl;
  cout <<" -s\tperform reachability only (do not run nested search)"<<endl;
  cout <<" -t\tproduce trail file (file.trail)"<<endl;
  cout <<" -c\tshow counterexample states (in reverse order!)"<<endl;
  cout <<" -H x\tset the size of hash table to ( x<33 ? 2^x : x )"<<endl;
  cout <<" -S\tprint some statistics"<<endl;
  cout <<" -q\tquiet mode (do not print anything "
       <<"(overrides all except -h and -v)"<<endl;
  cout <<" -P x\tset part of the state used by partition function:"<<endl;
  cout <<"\t 0 - full state (default)"<<endl;
  cout <<"\t 1 - every second byte"<<endl;
  cout <<"\t 2 - the second half"<<endl;
  cout <<"\t 3 - the first three quarters"<<endl;
  cout <<" -L\tperform logging"<<endl;
  cout <<" -X w\tsets base name of produced files to w (w.trail,w.report,w.00-w.N)"<<endl;
  cout <<" -Y\treserved for GUI"<<endl;
  cout <<" -Z\treserved for GUI"<<endl;
}


int main(int argc, char **argv)
{
  try 
    {
      distributed.process_user_message = process_message;
      distributed.set_proc_msgs_buf_exclusive_mem(false);
      distributed.network_initialize(argc, argv);

      ostringstream oss,oss1;
      oss1<<"tbndfs";
      int c;
      opterr=0;
      while ((c = getopt(argc, argv, "LX:YZP:H:Sqhtprsvc")) != -1) {
	oss1 <<" -"<<(char)c;
	switch (c) {
	case 'P': pfunc=atoi(optarg); break;
	case 'h': if (distributed.network_id == 0) 
	  { 
	    usage(); 
	  }
	  return 0;
	  break;
	case 'H': htsize=atoi(optarg);break;
	case 'S': statistics = true;break;
	case 'q': quietmode = true;break;
	case 't': trail=true;break;
	case 'r': report = true;break;
	case 's': reach_only=true; break;
        case 'p': compiled_generator = true; break;
	case 'c': show_ce = true; break;
	case 'L': logging = true; break;
	case 'X': set_base_name = optarg; base_name = true; break;
	case 'v': if (distributed.network_id == 0) 
	  { 
	    version();
	  }
	  return 0;
	  break;
	case '?': cerr <<"unknown switch -"<<(char)optopt<<endl;
	}
      }

      if (quietmode)
	{
	  statistics = show_ce = false;
	}
      
      if (argc < optind+1)
	{	
	  if (distributed.network_id == 0)
	    {
	      usage();
	    }
	  return 0;
	}
      
      if (reach_only && trail)
	{
	  cerr <<"Reachability cannot produce trail, switch -t skipped."<<endl;
	  trail=false;
	}
      
      nid = distributed.network_id;
      nnn = distributed.cluster_size;

      oss << (nid<10?" ":"") <<nid <<": ";
      ids = oss.str();

    /* decisions about the type of an input */
      system_description_t system_desc;
      p_sys = system_desc.open_system_from_file(argv[optind],
					      compiled_generator,
					      (nid==0)&&
					      statistics
					      );
      input_file_ext = system_desc.input_file_ext.c_str();


      if (htsize != 0)
        {	  
          if (htsize < 33)
            {
              int z = htsize;
              htsize = 1;
              for (;z>0; z--)
                {
                  htsize = 2* htsize;
                }
            }
          st.set_ht_size(htsize);
        }

      st.set_appendix(appendix);

      
      
//      /**************************/
//      /* VYRAZENO KVULI STORAGE */
//      /**************************/
//      st.set_pages(nnn);
//        if (pfunc !=0)
//  	{
//  	  st.user_get_page_id = partition_function;
//  	}
//        st.set_active_pages(nid,1);
      st.init();
      
      //      distributed.initialize(st);
      distributed.initialize();
    
      //    cout <<ids<<distributed.processor_name<<endl;
      
      int file_opening;
      try
	{
	  if ((file_opening=p_sys->read(argv[optind]))&&(distributed.network_id==NETWORK_ID_MANAGER))
	    {
	      if (file_opening==system_t::ERR_FILE_NOT_OPEN)
		gerr << distributed.network_id << ": " << "Cannot open file ...";
	      else 
		gerr << distributed.network_id << ": " << "Syntax error ...";
	    }
	  if (file_opening)
	    gerr << thr();
	}
      catch (ERR_throw_t & err)
	{ 
	  distributed.finalize();
	  return err.id;
	}

      file_name = argv[optind];
      int position = file_name.find(input_file_ext,0);
      if (position != -1)
	{
	  file_name.erase(position,4);
	}

      if (p_sys->get_with_property() && !reach_only)
	{
	  nested_allowed = true;
	}

      if (p_sys->get_with_property() && p_sys->get_property_type()!=BUCHI)
	{
	  if (nid==0)
	    cerr<<  "Cannot work with other than standard Buchi accepting condition."<<endl;
	  distributed.finalize();
	  return 1;	  
	}
      
//       if ((trail) && (!p_sys->get_abilities().system_can_transitions) && (distributed.network_id == 0))
// 	{
// 	  cout << "Model of system can't handle transitions, trail won't be produced." << endl;
// 	  trail = false;
// 	}

      reporter.set_info("InitTime", timer.gettime());
      reporter.start_timer();

      if (logging)
	{
	  logger.set_storage(&st);
	  if (base_name)
	    {
	      logger.init(&distributed,set_base_name);
	    }
	  else
	    {
	      logger.init(&distributed,file_name);
	    }
	  logger.use_SIGALRM(1);
	}

    // =============================
    // === Nested DFS ==============
    
      depth = 0;
      
      if (nid == 0 && statistics)
	{
	  cout <<ids<<"Computation init:  "<<timer.gettime()<<" s"<<endl;
	  //timer.reset(); //computation time includes initialization
	}
      
      state_t state;
      stack_member_t st_member;
      
      state = p_sys->get_initial_state();
      //p_sys->print_state(state,cout);
      
      if (distributed.get_state_net_id(state) != nid)
	{
	  delete_state(state);
	}
      else
	{	
	  st_member.state = state;
	  //p_sys->print_state(st_member.state,cout);
	  st_member.came_from = -1;
	  st_member.nested = false;
	  st_member.backtracking = false;
	  gstack.push(st_member);	
	  token = true;
	}
      
      while (!finished)
	{
	  distributed.process_messages();
	  if (token)
	    {
	      if (gstack.size() == 0)
		{		
		  cerr <<ids<<"Oops ..."<<endl;
		  throw;
		}
	      st_member = gstack.top();
	      gstack.pop();
	      
	      if (distributed.get_state_net_id(st_member.state)==nid) // local
		{
		  state_ref_t ref;
		  bool stored = st.is_stored(st_member.state,ref);
		  appendix.nested_dfs = false;
		  appendix.on_stack = false;
		  if (stored)
		    {
		      st.get_app_by_ref(ref,appendix);
		    }
		  
		  if ((st_member.backtracking) ||
		      (stored && !st_member.nested) || 
		      (appendix.nested_dfs)) //already visited
		    {		    		    
		      if (st_member.backtracking) //do we backtrack?
			{
			  if (!st_member.nested && 
			      (nested_allowed && 
			       p_sys->is_accepting(st_member.state)))
			    {
			      st_member.nested = true;
			      st_member.backtracking = false;
			      gstack.push(st_member);
			      seed = true;
			      continue;
			    }
			  else
			    {
			      appendix.on_stack = false;
			      st.set_app_by_ref(ref,appendix);
			    }						
			}
		      
		      if (st_member.came_from != nid) 
			//came from network and is already visited
			{
			  token = false;
			  if (st_member.came_from == -1)
			    {
			      for (int x = 0; x<nnn; x++)
				{
				  distributed.network.send_message
				    (NULL,0,x,TAG_FINISHED);
				  distributed.network.flush_buffer(x);
				}
			    }
			  else
			    {
			      distributed.network.send_message
				(NULL,0,st_member.came_from,TAG_TOKEN);
			      distributed.network.flush_buffer(st_member.came_from);
			    }
			}

		      // !!! very suspicious delete !!! check it !!!
		      // !!! possible bug !!! check it !!!
		      delete_state(st_member.state);
		    }
		  else //not visited
		    {
		      succ_container_t succs(*p_sys);
		      stack_member_t sm;
		      
		      if (st_member.nested && appendix.on_stack && !seed) 
			// it is an accepting cycle?
			{
			  if (!quietmode)
			    {
			      cout <<ids<<"======================"<<endl;
			      cout <<ids<<"Accepting cycle found!"<<endl;
			      cout <<ids<<"======================"<<endl;
			    }
			  st_member.backtracking = true;
			  gstack.push(st_member);
			  for (int x = 0; x<nnn; x++)
			    {
			      if (x!=nid)
				{
				  distributed.network.send_message
				    (NULL,0,x,TAG_FINISHED_CYCLE);
				  distributed.network.flush_buffer(x);
				}
			    }
			  where_found = nid;
			  cycle_found = true;
			  finished = true;
			  token = false;
			  continue;
			}

		      st_member.backtracking = true;  
		      gstack.push(st_member);
		      
		      if (!st_member.nested)
			{
			  st.insert(st_member.state,ref);
			  appendix.on_stack = true;
			}
		      else
			{
			  // to prevent the seed to be considered  
			  // as visited when it is reached in the 2nd search
			  if (!seed)
			    {
			      appendix.nested_dfs = true;
			    }
			}
		      st.set_app_by_ref(ref,appendix);
		      seed = false;
		      
		      p_sys->get_succs(st_member.state,succs);
		      fsuccscalls++;
		      for (size_t i = succs.size(); i>0; i--)
			{
			  trans++;
			  if (st_member.nested)
			    {
			      trans_nested++;
			    }
			  sm.state = succs[i-1];
			  sm.came_from = nid;
			  sm.nested = st_member.nested;
			  sm.backtracking = false;
			  gstack.push(sm);
			}
		    }
		}
	      else // remote
		{
		  transcross++;
		  if (st_member.nested)
		    {
		      transcross_nested++;
		    }
		  token = false;		
		  distributed.network.send_message
		    (st_member.state.ptr,st_member.state.size,
		     distributed.get_state_net_id(st_member.state),
		     (st_member.nested?TAG_STATE_NESTED:TAG_STATE));
		  distributed.network.flush_buffer
		    (distributed.get_state_net_id(st_member.state));
		  delete_state(st_member.state);
		}
	    }
	}
            
      
      if (!trail && ! show_ce)
	{
	  while( gstack.size() != 0)	
	    {		
	      st_member = gstack.top();
	      delete_state(st_member.state);
	      gstack.pop();
	    }
	}

      if (logging)
	{
	  logger.log_now();
	  logger.stop_SIGALRM();
	}

      // ==============================
      // == finalizace ================

      distributed.network.barrier(); 
      //to prevent other receive messages from CE within the 1st while cycle

      if (cycle_found && where_found == nid)
	{
	  token = true;
	}
      else
	{
	  token = false;
	}
      
      if (trail || show_ce)
	{
	  if (nnn>1 && cycle_found)
	    {
	      finished = false;
	      while(!finished)
		{
		  distributed.process_messages();
		  if (token)
		    {
		      st_member = gstack.top();
		      gstack.pop();
		      if (st_member.backtracking)
			{
			  if (nid == 0)
			    {
			      gstack1.push(st_member);
			    }
			  else
			    {
			      //cout <<ids<<"Sending state to master"<<endl;
			      distributed.network.send_message
				(st_member.state.ptr,st_member.state.size,
				 0, (st_member.nested?TAG_CE_NESTED:TAG_CE));
			      distributed.network.flush_buffer(0);
			      delete_state(st_member.state);
			      // wait for acknowledgement from master, 
			      // this ensures message ordering 
			      token = false; 
			      while (!token)
				{
				  distributed.process_messages();
				}
			    }		   
			  if (st_member.came_from == -1)
			    {
			      for (int x = 0; x<nnn; x++)
				{
				  distributed.network.send_message
				    (NULL,0,x,TAG_FINISHED_CYCLE);
				  distributed.network.flush_buffer(x);
				}
			      token = false;
			      continue;
			    }
			  if (st_member.came_from != nid)
			    {
			      token = false;
			      distributed.network.send_message
				(NULL,0,st_member.came_from,TAG_TOKEN);
			      distributed.network.flush_buffer(st_member.came_from);
			    }		    
			}
		      else
			{
			  if (nid >0)
			    {
			      delete_state(st_member.state);
			    }
			  else
			    {
			      delete_state(st_member.state);
			    }
			}
		    }
		}
	    }
	  
	  if (nid == 0 && nnn >1 && cycle_found)
	    {
	      if (!gstack.empty())
		{
		  cout <<ids<<"ERROR!"<<endl;
		}
	      while (!gstack1.empty())
		{
		  st_member = gstack1.top();
		  gstack1.pop();
		  gstack.push(st_member);
		}
	    }      

	  if (nid==0 && cycle_found)
	    {
	      bool first=true;
	      bool laso_printed = false;
	      path_t ce(p_sys);

	      bool tmpbool=false,inserttop=false;
	      stack_member_t topstack,sm;
	      //at first we complete the accepting cycle on the stack
	      //we need to duplicate part of the 1st dfs stack

	      topstack=gstack.top();
	      gstack.pop();
	      gstack1.push(topstack);
	      sm=gstack.top();		  
	      // localize the needed part of the 1st dfs stack
	      while (!sm.backtracking || 
		     !(memcmp(sm.state.ptr,topstack.state.ptr,sm.state.size)==0))
		{
		  if (sm.backtracking) //consider only relevant states
		    {
		      gstack1.push(sm);
		    }
		  else
		    {
		      delete_state(sm.state);
		    }
		  gstack.pop();
		  sm=gstack.top();
		}
		
	      if (!sm.nested)   // 1st stack hit above the seed
		{
		  inserttop = true;
		}
		
	      // copy the relevant path into gstack2
	      while (!gstack1.empty())
		{
		  sm=gstack1.top();
		  if (sm.nested == true && !tmpbool)
		    {
		      topstack = gstack1.top();   //topstack holds the seed
		      tmpbool = true;
		    }
		  gstack1.pop();
		  gstack.push(sm);
		  if (sm.nested==false)
		    {
		      sm.nested=true;
		      sm.state=duplicate_state(sm.state);
		      gstack2.push(sm);
		    }
		}
	      
	      while (!gstack2.empty()) //copy the gstack2 to gstack
		{
		  sm=gstack2.top();
		  gstack2.pop();
		  gstack1.push(sm);
		}
	      while (!gstack1.empty())
		{
		  sm=gstack1.top();
		  gstack1.pop();
		  gstack.push(sm);
		}
	      	  	  
	      if (inserttop) //insert the seed if the 1stdfs stack was hitted above it
		{
		  stack_member_t smtemp=topstack;
		  smtemp.state=duplicate_state(topstack.state);
		  gstack.push(smtemp);
		}

	      while (!gstack.empty()) // now process the gstack, and fullfil the distributed path_t
		{	    
		  st_member = gstack.top();
		  gstack.pop();
		  if (st_member.backtracking)
		    {
		      if (st_member.nested == false && !laso_printed)
			{
			  laso_printed = true;
			  ce.mark_cycle_start_front();
			}
		      if(!first)
			{
			  ce.push_front(st_member.state);
			}
		      first = false;
		    }
		  delete_state(st_member.state);
		}

	      if (trail)
		{
		  ofstream ce_out;
		  string pom_fn;
		  if (base_name)
		    {
		      pom_fn = set_base_name+".trail";
		    }
		  else
		    {
		      pom_fn = file_name+".trail";
		    }
		  ce_out.open(pom_fn.c_str());
		  if (p_sys->can_system_transitions())
		    ce.write_trans(ce_out);
		  else
		    ce.write_states(ce_out);
		  ce_out.close();
		}
	      if (show_ce)
		{
		  ofstream ce_out;
		  string pom_fn;
		  if (base_name)
		    {
		      pom_fn = set_base_name+".ce_states";
		    }
		  else
		    {
		      pom_fn = file_name+".ce_states";
		    }
		  ce_out.open(pom_fn.c_str());
		  ce.write_states(ce_out);
		  ce_out.close();
		}
	    }

	}

      if (report)
	{
	  ofstream report_out;
	  string pom_fn;
	  if (base_name)
	    {
	      pom_fn = set_base_name+".report";
	    }
	  else
	    {
	      pom_fn = file_name+".report";
	    }
	  report_out.open(pom_fn.c_str());

	  reporter.set_alg_name(oss1.str());
	  reporter.set_file_name(argv[optind]);
	  if (reach_only || !nested_allowed)
	    {
	      reporter.set_problem("reachability");
	    }
	  else
	    {
	      reporter.set_problem("cycle detection");
	    }
	  string problem=(reach_only || !nested_allowed)?"SSGen":"LTL MC";
	  reporter.set_obligatory_keys(oss1.str(), argv[optind], problem, st.get_states_stored(), fsuccscalls);
	  reporter.set_info("States", st.get_states_stored());
	  reporter.set_info("Trans", trans-trans_nested);
	  reporter.set_info("CrossTrans", transcross-transcross_nested);      
	  if  (nid == 0)
	    {
	      if (cycle_found)
		{
		  reporter.set_global_info("IsValid","No");
		  if (trail || show_ce)
		    reporter.set_global_info("CEGenerated", "Yes");
		  else
		    reporter.set_global_info("CEGenerated", "No");
		}
	      else
		{
		  reporter.set_global_info("IsValid","Yes");
		  reporter.set_global_info("CEGenerated", "No");
		}
	    }
	  reporter.stop_timer();
	  reporter.collect_and_print(REPORTER_OUTPUT_LONG,report_out);
	  if (nid==0)
	    report_out.close();
	}      

      if (statistics)
	{
	  while (!distributed.synchronized(info))
	    {
	      distributed.process_messages();
	    }
	}
      if (statistics && nid==0)
	{
	  cout <<ids<<"state size:        "<<state.size<<endl;
          cout <<ids<<"states generated:  "<<info.data.allstates<<endl;
          if (htsize != 0)
            cout <<ids<<"hashtable size     "<<htsize<<endl;
          else
            cout <<ids<<"hashtable size     "<<65536<<endl;
          cout <<ids<<"get_succs called:  "<<info.data.allsuccs<<endl;
          cout <<ids<<"all transitions:   "<<info.data.alltrans
	       <<" ("<<info.data.alltrans-info.data.alltrans_nested
	       <<"/"<<info.data.alltrans_nested<<")"<<endl;
          cout <<ids<<"cross transitions: "<<info.data.allcross
	       <<" ("<<info.data.allcross-info.data.allcross_nested
	       <<"/"<<info.data.allcross_nested<<")"<<endl;
          cout <<ids<<"all memory         "<<info.data.allmem/1024.0
	       <<" MB"<<endl;
          cout <<ids<<"Computation done:  "<<timer.gettime()<<" s"<<endl;
          cout <<ids<<"-------------------"<<endl;
	}

      if (statistics)
        {
          distributed.network.barrier();
          cout <<ids<<"local states:      "<<st.get_states_stored()<<endl;
        }

      if (statistics)
        {
          distributed.network.barrier();
          cout <<ids<<"local memory:      "<<vm.getvmsize()/1024.0<<endl;
        }

      distributed.finalize();
      delete p_sys;
    }
  catch (...)
    {
      cout <<endl<<flush;
      distributed.network.abort();
    }
    return 0;
}





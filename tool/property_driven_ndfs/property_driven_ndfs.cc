#include <getopt.h>
#include <sys/resource.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include "divine.h"
#include <queue>
#include <vector>

using namespace divine;
using namespace std;

const int TAG_FINISHED = DIVINE_TAG_USER + 1;
const int TAG_STATE = DIVINE_TAG_USER + 2;
const int TAG_FINISHED_CYCLE = DIVINE_TAG_USER + 3;
const int TAG_CE_CYCLE_STATE = DIVINE_TAG_USER + 4;
const int TAG_CE_REQ_STATE = DIVINE_TAG_USER + 5;
const int TAG_CE_STATE = DIVINE_TAG_USER + 6;
const int TAG_CE_STATE_SIMPLE = DIVINE_TAG_USER + 7;

rlimit rlp;

struct global_state_ref_t
{
public:
  global_state_ref_t();
  state_ref_t state_ref;
  size_int_t network_id;
  bool is_valid();
  void invalidate();
};

global_state_ref_t::global_state_ref_t()
{
  invalidate();
}

bool global_state_ref_t::is_valid()
{
  return ((network_id != MAX_ULONG_INT) && (state_ref.is_valid()));
}

void global_state_ref_t::invalidate()
{
  network_id = MAX_ULONG_INT;
  state_ref.invalidate();
}

struct appenix_t 
{
  bool nested_dfs;
  bool on_stack;
  global_state_ref_t predecessor;
} appendix;

struct queue_member_t
{
  state_t state;
  global_state_ref_t predecessor;
} qm;

struct info_t {
  int allstates;
  int alltrans;
  int allsuccs;
  int allcross;
  long int allmem;
  virtual void update();
  virtual ~info_t() {}
};                                                                                
updateable_info_t<info_t> info;

bool quiet=false;

explicit_system_t * p_sys;
process_decomposition_t *property_decomposition;
explicit_storage_t st;
distributed_t distributed;
distr_reporter_t reporter(&distributed);
path_t ce;

const char * input_file_ext = 0;
logger_t logger;

int nid,nnn,xx,fsuccscalls=0;
string ids,file_name;
bool finished=false,  cycle_found = false;
bool trail=false,show_ce=false,report=false, reach_only=false;
bool in_cycle = false, next_ce_state = false;
bool no_partition_function = false;
bool logging = false;
bool base_name = false;
string set_base_name;
size_t htsize=0;
state_t state;
//char buffer[4096];
bool compiled_generator = false;



message_t message;
message_t received_message;

timeinfo_t timer;

int slow_down = 500;
bool do_slow_down = true;

queue<queue_member_t> q;
global_state_ref_t cycle_point,ce_path_start;

int trans=0,transcross = 0;
vminfo_t vm;
bool statistics = false;
unsigned int depth=0,maxdepth=0,ndepth=0,maxndepth=0;

succ_container_t succs_cont;

void info_t::update()
{
  if (nid == 0)
    {
      allstates = st.get_states_stored();
      allsuccs = fsuccscalls;
      alltrans = trans;
      allcross = transcross;
      allmem = vm.getvmsize();
    }
  else
    {
      allstates += st.get_states_stored();
      allsuccs += fsuccscalls;
      alltrans += trans;
      allcross += transcross;
      allmem += vm.getvmsize();
    }
}

// {{{ process messages, partition function, version, usage 

void process_message(char *buf, int size, int src, int tag, bool urgent)
{
  queue_member_t qm;
  switch(tag) 
    {
    case TAG_FINISHED:
      finished = true;
      distributed.network.flush_all_buffers();
      break;
    case TAG_STATE:
      if (!finished)
	{ 
          received_message.load_data((byte_t*)buf, size);
	  received_message.read_state_ref(qm.predecessor.state_ref);
          received_message.read_state(qm.state);
///////	  memcpy(&qm.predecessor,buf,sizeof(global_state_ref_t));
///////	  memcpy(qm.state.ptr,buf+sizeof(global_state_ref_t),qm.state.size);
	  q.push(qm);
	  distributed.set_busy();
	}
      break;
    case TAG_FINISHED_CYCLE:
      cycle_found = true;
      finished = true;
      distributed.network.flush_all_buffers();
      break;
    case TAG_CE_CYCLE_STATE:
       received_message.load_data((byte_t*)buf, size);
       received_message.read_state_ref(ce_path_start.state_ref);
       received_message.read_size_int(ce_path_start.network_id);
//////      memcpy(&ce_path_start,buf,sizeof(global_state_ref_t));
      break;
    case TAG_CE_REQ_STATE:
      received_message.load_data((byte_t*)buf, size);
      received_message.read_state_ref(ce_path_start.state_ref);
//////      memcpy(&ce_path_start,buf,sizeof(global_state_ref_t));
      qm.state = st.reconstruct(ce_path_start.state_ref);
      st.get_app_by_ref(ce_path_start.state_ref,appendix);
      message.rewind();
      message.append_state_ref(appendix.predecessor.state_ref);
      message.append_state(qm.state);
//////      memcpy (buffer,&(appendix.predecessor),sizeof(global_state_ref_t));
//////      memcpy (buffer+sizeof(global_state_ref_t),qm.state.ptr,qm.state.size);
      distributed.network.send_message
	(message,0,TAG_CE_STATE);
      distributed.network.flush_buffer(0);
      delete_state(qm.state);
      break;
    case TAG_CE_STATE:
      received_message.load_data((byte_t*)buf, size);
      received_message.read_state_ref(ce_path_start.state_ref);
      received_message.read_state(qm.state);
//////      memcpy(&ce_path_start,buf,sizeof(global_state_ref_t));
//////      memcpy(qm.state.ptr,buf+sizeof(global_state_ref_t),qm.state.size);
      next_ce_state = true;
      ce.push_front(qm.state);
      delete_state(qm.state);
      break;
    case TAG_CE_STATE_SIMPLE:
      qm.state = new_state(buf,size);
//////      memcpy(qm.state.ptr,buf,qm.state.size);
      ce.push_front(qm.state);
      delete_state(qm.state);
      break;
    }
}

int partition_function(state_t &st)
{
  if (no_partition_function) 
    {
      return distributed.partition_function(st);
    }
  else
    {
      return ((property_decomposition->get_process_scc_id(st))%distributed.cluster_size);
    }
}

void version()
{
  cout <<"version 1.1 build 14 (2007/06/10 22:22)" <<endl;
}

void usage()
{
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"Property Driven Nested DFS ";
  version();
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"Usage: [mpirun -np N] property_driven_ndfs [options] input_file"<<endl;
  cout <<"Options: "<<endl;
  cout <<" -V,--version\t\tshow pdndfs version"<<endl;
  cout <<" -h,--help\t\tshow this help"<<endl;
  cout <<" -r,--report\t\tproduce report file"<<endl;
  cout <<" -t,--trail\t\tproduce trail file"<<endl;
  cout <<" -p,--precompile\tDVE file is precompiled prior verification"<<endl;
  cout <<" -c\t\t\tshow counterexample states (in reverse order!)"<<endl;  
  cout <<" -s,--simple\t\tperform simple reachability only (do not run nested search)"<<endl;
  cout <<" -S,-v,--verbose\tprint statistics"<<endl;
  cout <<" -Hx,--htsize x\t\tset the size of hash table to ( x<33 ? 2^x : x )"<<endl;
  cout <<" -q,--quiet\t\tquiet mode "
       <<"(overrides all except -h, --help, -V, and --version)"<<endl;
  cout <<" -d\t\t\tdo not slowdown workstations working on "
       <<"non-accepting components"<<endl;
  cout <<" -Dx,--delay x\t\tslowdown after every x cross-transisions (default x=500)"
       <<endl;
  cout <<" -L,--log\t\tperform logging"<<endl;
  cout <<" -X w,--basename w\tsets base name of produced files to w (w.trail,w.report,w.00-w.N)"<<endl;
  cout <<" -Y\t\t\treserved for GUI"<<endl;
  cout <<" -Z\t\t\treserved for GUI"<<endl;
}

// }}}

// {{{ NestedDFS 

void NestedDFS(state_t state)
{
  global_state_ref_t stref;
  if ((++ndepth)>maxndepth)
    {
      maxndepth = ndepth;
    }

  distributed.process_messages();
  if (finished)
    {
      ndepth--;
      return;
    }

  st.is_stored(state,stref.state_ref);
  st.get_app_by_ref(stref.state_ref,appendix);
  if (!appendix.nested_dfs)
    {
      appendix.nested_dfs = true;
      st.set_app_by_ref(stref.state_ref,appendix);
      p_sys->get_succs(state,succs_cont);

      vector<state_t> succs_vec(succs_cont.size());
      for (size_t i = 0; i<succs_cont.size(); i++)
	{
	  succs_vec[i]=succs_cont[i];
	}
      fsuccscalls++;
      
      for (size_t i = 0; i<succs_vec.size(); i++)
	{
	  state_t s=succs_vec[i];
	  if (!cycle_found && partition_function(state)==partition_function(s))
	    {
	      st.is_stored(s,stref.state_ref);
	      st.get_app_by_ref(stref.state_ref,appendix);
	      if (appendix.on_stack)
		{
		  // cycle found
		  for (xx = 0; xx<nnn; xx++)
		    {
		      if (xx!=nid)
			{
			  distributed.network.send_urgent_message
			    (NULL,0,xx,TAG_FINISHED_CYCLE);
			  distributed.network.flush_buffer(xx);
			}
		    }
		  if (!quiet)
		    {	
		      cout <<ids<<"======================"<<endl;
		      cout <<ids<<"Accepting cycle found!"<<endl;
		      cout <<ids<<"======================"<<endl;
		    }

		  cycle_found = true;
		  finished = true;
		  memcpy (&cycle_point,&stref,sizeof(global_state_ref_t));
		  cycle_point.network_id=distributed.network_id; //THIS WAS MISSING SOONER, however it is _necessary_
		  in_cycle = true;
		}
	      else
		{
		  NestedDFS(s);
		  if (cycle_found && (trail||show_ce))
		    distributed.network.send_message
		      (s.ptr, s.size,0,TAG_CE_STATE_SIMPLE);
		}
	    }
	  delete_state(s);
	}   
    }
  ndepth --;
}

// }}}

// {{{ FirstDFS 

void FirstDFS(state_t state, global_state_ref_t predecessor)
{
//    cout <<distributed.network_id<<": FirstDFS called from "<<predecessor.network_id<<endl;
  global_state_ref_t stref;

  if ((++depth)>maxdepth)
    {
      maxdepth = depth;
    }
  
  distributed.process_messages();
  if (finished) 
    {
      depth --;
      return;
    }

  if (!st.is_stored(state,stref.state_ref))
    {
      st.insert(state,stref.state_ref);
      stref.network_id = distributed.network_id;
      appendix.nested_dfs = false;
      appendix.on_stack = true;
      appendix.predecessor = predecessor;
      st.set_app_by_ref(stref.state_ref,appendix);

      p_sys->get_succs(state,succs_cont);
      vector<state_t> succs_vec(succs_cont.size());
      for (size_t i = 0; i<succs_cont.size(); i++)
	{
	  succs_vec[i]=succs_cont[i];
	}
      fsuccscalls++;

      for (size_t i = 0; i<succs_vec.size(); i++)
	{
	  trans++;
	  state_t s=succs_vec[i];
	  if (nid == partition_function(s))
	    {
	      FirstDFS(s,stref);
	    }
	  else
	    {
	      transcross++;
	      if (do_slow_down &&
		  transcross%slow_down == 0 && nnn >= property_decomposition->get_scc_count() &&
		  property_decomposition->get_scc_type(nid)==0)
		{	      
		  usleep (10);
		}
              message.rewind();
              message.append_state_ref(stref.state_ref);
              message.append_state(s);
//	      memcpy (buffer,&stref,sizeof(stref));
//	      memcpy (buffer+sizeof(stref),s.ptr,s.size);
	      distributed.network.send_message
		(message, partition_function(s),
		 TAG_STATE);
	    }
	  delete_state(s);
	}   
      
      if (p_sys->is_accepting(state) && !reach_only)
	{
	  NestedDFS(state);
	}
      
      if (cycle_found && (trail || show_ce))
	{
	  if (in_cycle)
	    {
	      distributed.network.send_message
		(state.ptr, state.size,0,TAG_CE_STATE_SIMPLE);
	      //  	      cout << " cycle closing";
	      if ((memcmp(&stref,&cycle_point,sizeof(global_state_ref_t))==0))
		{
		  in_cycle = false;
		  st.get_app_by_ref(stref.state_ref,appendix);
		  message.rewind();
		  message.append_state_ref(appendix.predecessor.state_ref);
		  message.append_size_int(appendix.predecessor.network_id);
		  distributed.network.send_message(message, 0, TAG_CE_CYCLE_STATE);
		  
		  /*		  distributed.network.send_message(
		    reinterpret_cast<char *>(&(appendix.predecessor)),
		    sizeof(global_state_ref_t),
		    0,TAG_CE_CYCLE_STATE);*/
		  distributed.network.flush_buffer(0);
		}
	    }
	  return; 
	  
	}      

      st.get_app_by_ref(stref.state_ref,appendix);
      appendix.on_stack = false;
      st.set_app_by_ref(stref.state_ref,appendix);
    }
  depth--;
}

// }}}

int main(int argc, char **argv)
{
  try 
    {
      // {{{ initialization 

      distributed.network.set_buf_msgs_cnt_limit(200);
      distributed.network.set_buf_size_limit(65536);
      //      distributed.network.set_buf_time_limit(int limit_sec, int limit_msec);


      distributed.process_user_message = process_message;
      distributed.set_proc_msgs_buf_exclusive_mem(false);
      distributed.network_initialize(argc, argv);

      nid = distributed.network_id;
      nnn = distributed.cluster_size;

      getrlimit(RLIMIT_STACK, &rlp);
//        cout <<"cur = "<<rlp.rlim_cur/1024.0<<endl;
//        cout <<"max = "<<rlp.rlim_max/1024.0<<endl;
      rlp.rlim_cur = 32*rlp.rlim_cur;
      if (setrlimit(RLIMIT_STACK, &rlp))
	{
	  cerr <<"*** setrlimit failed !!! "<<endl;
	}
//        getrlimit(RLIMIT_STACK, &rlp);
//        cout <<"cur = "<<rlp.rlim_cur/1024.0<<endl;
//        cout <<"max = "<<rlp.rlim_max/1024.0<<endl;
      

      ostringstream oss,oss1;
      oss1<<"pdndfs";
      int c;
      opterr=0;

      static struct option longopts[] = {
	{ "help",       no_argument, 0, 'h'},
	{ "quiet",      no_argument, 0, 'q'},
	{ "trail",      no_argument, 0, 't'},
	{ "report",     no_argument, 0, 'r'},
	{ "simple",     no_argument, 0, 's' },
	{ "verbose",    no_argument, 0, 'v' },
	{ "precompile", no_argument, 0, 'p' },
	{ "htsize",     required_argument, 0, 'H' },
	{ "delay",      required_argument, 0, 'D'},
	{ "basename",   required_argument, 0, 'X'},
	{ "log",        no_argument, 0, 'L'},
	{ "version",    no_argument, 0, 'V'},
	{ NULL, 0, NULL, 0 }
      };

      while ((c = getopt_long(argc, argv, "SX:YZLdD:pqhtrscvH:V", longopts, NULL)) != -1)
	{
	  oss1 <<" -"<<(char)c;
	  switch (c) {
	  case 'L': logging = true; break;
	  case 'X': set_base_name = optarg; base_name = true; break;	    
	  case 'd': do_slow_down = false; break;
	  case 'D': slow_down = atoi(optarg); break;
	  case 'q': quiet = true; break;
	  case 'p': compiled_generator = true; break;
	  case 'h': 
	    if (nid == 0) 
	      {
		usage();
	      }
	    distributed.network.barrier();
	    distributed.finalize();
	    return 0;
	    break;
	  case 't': trail=true;break;
	  case 'r': report = true;break;
	  case 's': reach_only=true; break;
	  case 'c': show_ce = true; break;
	  case 'S':
	  case 'v': statistics = true; break;
	  case 'H': htsize=atoi(optarg); break;	  
	  case 'V': 
	    if (nid == 0) 
	      {
		version();
	      }
	    distributed.finalize();
	    return 0;
	    break;
	  case '?': cerr <<"unknown option -"<<(char)optopt<<endl;
	  }
	}

      if (quiet)
	{
	  statistics = false; show_ce = false;
	}

      if (argc < optind+1)
	{
	  if (nid == 0)
	    {
	      usage();
	    }
	  distributed.finalize();
	  return 0;
	}
      
      /* decisions about the type of an input */
      system_description_t system_desc;
      p_sys = system_desc.open_system_from_file(argv[optind],
                                                compiled_generator,
                                                (nid==0)&&
						statistics
						);
      input_file_ext = system_desc.input_file_ext.c_str();
      ce.set_system(p_sys);


      property_decomposition = p_sys->get_property_decomposition();
      
//       if ((trail) && (!p_sys->get_abilities().system_can_transitions) && (distributed.network_id == 0))
// 	{
// 	  cout << "Model of system can't handle transitions, trail won't be produced." << endl;
// 	  trail = false;
// 	}
      
      oss << (nid<10?" ":"") <<nid <<": ";
      ids = oss.str();

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
      st.init();
      
      distributed.initialize();      
      
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
      file_name.erase(position,4);

      if (!p_sys->get_with_property())
	{
	  if (nid==0)
	    {
	      cerr <<"*** Verified model without LTL property,"
		   <<" performing reachability only!"<<endl<<flush;
	      cerr <<"*** No partition function can be defined! Using default."<<endl<<flush;
	    }
	  reach_only = true;
	  no_partition_function = true;
	}
      else
	{
          if (!p_sys->get_abilities().system_can_decompose_property
              || property_decomposition == 0)
	    {
	      if (nid==0)
		cerr << "System is not able to provide informations "
		  "needed to decompose property process" << endl;
	      distributed.finalize();
	      return 1;
	    }
	  
	  if (p_sys->get_with_property() && p_sys->get_property_type()!=BUCHI)
	    {
	      if (nid==0)
		cerr<<"Cannot work with other than standard Buchi accepting condition."<<endl;
	      distributed.finalize();
	      return 1;
	    }	  
	}
      

      if (reach_only && trail)
	{
	  cerr <<"Reachability cannot produce trail, switch -t skipped."<<endl;
	  trail=false;
	}

      if (report)
	{
	  reporter.set_info("InitTime", timer.gettime());
	  reporter.start_timer();
	}

      if (logging)
	{
	  logger.set_storage(&st);
	  if (base_name)
	    {
	      logger.init(&distributed,set_base_name,0);
	    }
	  else
	    {
	      logger.init(&distributed,file_name,0);
	    }
	  logger.use_SIGALRM(1);
	}

      // }}}

      state = p_sys->get_initial_state();
      
      if (partition_function(state) != nid)
	{
	  delete_state(state);
	  distributed.set_idle();
	}
      else
	{
	  qm.state = state;
	  qm.predecessor.invalidate();
	  q.push(qm);
	  distributed.set_busy();
	}


      while (!distributed.synchronized())
	{
	  distributed.process_messages();
	      
	  if (!q.empty())
	    {
	      queue_member_t tmp_qm = q.front();
	      q.pop();
	      if (!finished)
		{
		  FirstDFS(tmp_qm.state,tmp_qm.predecessor);
		  distributed.network.flush_all_buffers();		  
		  delete_state(tmp_qm.state);
		}
	      else
		{
		  delete_state(tmp_qm.state);
		}
	    }
	  else
	    {
	      distributed.set_idle();
	    }
	}

      if (logging)
        {
          logger.log_now();
          logger.stop_SIGALRM();
        }     
//        cout <<ids<<"****"<<endl;
      if (cycle_found && (trail || show_ce))
	{
	  finished = false;
	  if (nid == 0)
	    {
	      ce.mark_cycle_start_front();

	      while (ce_path_start.is_valid())
		{
		  distributed.network.send_message(
						   reinterpret_cast<char *>(&ce_path_start),sizeof(global_state_ref_t),
						   ce_path_start.network_id,
						   TAG_CE_REQ_STATE);
		  distributed.network.flush_buffer(ce_path_start.network_id);
		  next_ce_state = false;	      
		  while (!next_ce_state)
		    { 
		      distributed.process_messages();
		    }
		}
	      for (int xx=0; xx<nnn; xx++)
		{
		  if (xx!=nid)
		    distributed.network.send_message(NULL,0,xx,TAG_FINISHED);
		}
	      distributed.network.flush_all_buffers();
	    }
	  else
	    {
	      while (!finished)
		{
		  distributed.process_messages();
		}
	    }
	}

      distributed.network.barrier();

      if (trail && cycle_found)
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

      if (show_ce && cycle_found)
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
      
      if (statistics)
	{
	  while (!distributed.synchronized(info))
	    {
	      distributed.process_messages();
	    }
	  if (nid == 0)
	    {
	      cout <<"state size:        "<<state.size<<endl;
	      cout <<"states generated:  "<<info.data.allstates<<endl;
	      if (htsize != 0)
		cout <<"hashtable size     "<<htsize<<endl;
	      else
		cout <<"hashtable size     "<<65536<<endl;
	      cout <<"get_succs called:  "<<info.data.allsuccs<<endl;
	      cout <<"all transitions:   "<<info.data.alltrans<<endl;
	      cout <<"cross transitions: "<<info.data.allcross<<endl;
	      cout <<"all memory         "<<info.data.allmem/1024.0 
		   <<" MB"<<endl;
	      cout <<"runtime:           "<<timer.gettime()<<" s"<<endl;
	      cout <<"max depth:         "<<maxdepth+maxndepth
		   <<" ("<<maxdepth<<"+"<<maxndepth<<")"<<endl;
	      cout <<"-------------------"<<endl;
	    }
	  distributed.network.barrier();
	  cout <<ids<<"local states: "<<st.get_states_stored()<<endl;
	  cout <<ids<<"local trans:  "<<trans<<endl;
	  cout <<ids<<"local mem:    "<<vm.getvmsize()/1024.0<<" MB"<<endl;  
	  cout <<ids<<"max depth:    "<<maxdepth+maxndepth
	       <<" ("<<maxdepth<<"+"<<maxndepth<<")"<<endl;
	  cout <<ids<<"sent buffs:   ";
   	  int val;
//  	  val=distributed.network.get_all_sent_msgs_cnt(val);
//  	  cout <<val<<" ";
	  for (int i=0; i<nnn; i++)
	    {
	      distributed.network.get_buffer_flushes_cnt(i,val);
	      cout <<"("<<val<<")";
		if (i<nnn-1)
		  cout <<", ";
		else
		  cout <<endl;
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
	  string problem=reach_only?"SSGen":"LTL MC";
	  reporter.set_obligatory_keys(oss1.str(), argv[optind], problem, st.get_states_stored(), fsuccscalls);
	  reporter.set_info("States", st.get_states_stored());
	  reporter.set_info("Trans", trans);
	  reporter.set_info("CrossTrans", transcross);      
	  if  ((nid == 0) && (!reach_only))
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
      distributed.finalize();
      delete p_sys;
    }
  catch (...)
    {
      cout <<endl<<flush;
      distributed.network.abort();
    }
  
//    if (nid == 0)
//      {
//        cout <<ids<<"Computation time "<<timer.gettime()<<" sec."<<endl;
//        cout <<ids<<"Initialization time "<<init_time<<" sec."<<endl;
//      }
//    return 0;
}

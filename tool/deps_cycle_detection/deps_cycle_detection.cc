//  Notes to understand this source code.
//  First, read "Jiri Barnat: Distributed memory LTL model checking (2004)"
//
//  Acquaint yourself with DiVinE to understand network primitives
//  such as synchronized(), flush_all_buffers(), process_messages(), TAGS,
//  explicit_storage_t, path_t, reporter_t, etc.
//  
//  Used abbreviations:
//  -------------------
//  ce = counterexample
//  tb = tbndfs = token based distributed memory Nested DFS
//  

//=======================================================
// suggested optimizations
//-------------------------------------------------------
// - build the DepS from global_state_ref_t and not from state_t
//
//   ****** if the same state is stored on different workstations 
//          te reference neednot be the same
//           
// - do not send messages to generate CE when they are not needed
// - include TAG_CE_CYCLE_STATE in TAG_FINISHED_CYCLE


#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <queue>
#include <vector>
#include <list>
#include <sys/resource.h>
#include <stack>
#include <fstream>

#include "divine.h"

using namespace divine;
using namespace std;

const int TAG_FINISHED =          DIVINE_TAG_USER + 1;
const int TAG_STATE =             DIVINE_TAG_USER + 2;
const int TAG_FINISHED_CYCLE =    DIVINE_TAG_USER + 3;
const int TAG_CE_CYCLE_STATE =    DIVINE_TAG_USER + 4;
const int TAG_CE_REQ_STATE =      DIVINE_TAG_USER + 5;
const int TAG_CE_STATE =          DIVINE_TAG_USER + 6;
const int TAG_CE_STATE_SIMPLE =   DIVINE_TAG_USER + 7;
const int TAG_ACK_STATE =         DIVINE_TAG_USER + 8;
const int TAG_COLLECT_DEPS_SIZE = DIVINE_TAG_USER + 9;
const int TAG_TB_NEXTTURN =       DIVINE_TAG_USER + 10;
const int TAG_TB_ALL_FINISHED =   DIVINE_TAG_USER + 11;
const int TAG_TB_RUN =            DIVINE_TAG_USER + 12;
const int TAG_TB_FINISHED =       DIVINE_TAG_USER + 13;
const int TAG_TB_FINISHED_CYCLE = DIVINE_TAG_USER + 14;
const int TAG_TB_TOKEN =          DIVINE_TAG_USER + 15;
const int TAG_TB_STATE_NESTED =   DIVINE_TAG_USER + 16;
const int TAG_TB_STATE =          DIVINE_TAG_USER + 17;
const int TAG_TB_CE =             DIVINE_TAG_USER + 18;
const int TAG_TB_CE_NESTED =      DIVINE_TAG_USER + 19;


rlimit rlp;

struct global_state_ref_t
{
public:
  global_state_ref_t();
  state_ref_t state_ref;
  int network_id;
  bool is_valid();
  void invalidate();
  void print();
};

global_state_ref_t::global_state_ref_t()
{
  invalidate();
}

bool global_state_ref_t::is_valid()
{
  return ((network_id != -1) && (state_ref.is_valid()));
}

void global_state_ref_t::invalidate()
{
  network_id = -1;
  state_ref.invalidate();
}

void global_state_ref_t::print()
{
  if (is_valid())
    {
      cout <<"("<<state_ref.hres<<","<<state_ref.id<<","<<network_id<<")";
    }
  else
    {
      cout <<"(invalid)";
    }
}


typedef struct appendix_t 
{
  bool V,PV,DepS,in_stack;
  bool tb_nested,tb_in_stack,tb_stored;
  global_state_ref_t predecessor;
  list<state_t> *preds,*succs;
  int predscount,succscount;
};

typedef struct queue_member_t
{
  state_t state;
  global_state_ref_t predecessor;
};

dve_explicit_system_t sys(gerr);
explicit_storage_t st;
distributed_t distributed;
distr_reporter_t reporter(&distributed);
path_t ce(&sys);
logger_t logger;

appendix_t appendix;
int nid,nnn;
int xx,fsuccscalls=0,states_overhead=0;
double init_time;
string ids,file_name,set_base_name;
bool finished=false,  cycle_found = false;
bool trail=false,show_ce=false,report=false, reach_only=false;
bool show_deps = false,tbndfs_needed = false;
bool in_cycle = false, next_ce_state = false;
bool optimized = true,cycle_proved = false;
bool ce_path_first = true;
bool skip_first_push_to_ce = true;
bool logging = false;
bool base_name = false;
bool should_delete = false;


state_t state;
message_t message;
message_t received_message;
timeinfo_t timer;
vminfo_t vm;
long int bmem,rmem,tmem;
int depth=0,maxdepth=0,ndepth=0,maxndepth=0;
double timeafterdepsbuilt,timeafterdepsrem,timeaftertbndfs;
queue<queue_member_t> q,acks;
global_state_ref_t cycle_point,ce_path_start,ce_path_start1,ce_path_start_predecessor,ce_path;
int fromall=0;

ostringstream /* show_ce_out,show_ce_out1, */ deps_out;
ostringstream oss, oss1;

size_t htsize=0;
bool quietmode=false;

state_t last;
vector<list<state_t> > leaves;
vector<list<state_t> > roots;
vector<int> depssize,depsedge,rootsall,leavesall;
vector<int> maxdepssize,maxdepsedge;
vector<int> totaldepssize,totaldepsedge;

bool all_tb_finished=false,tb_finished=false,run_tbndfs = false;
int onturn=0;

int where_found=0;
bool token=false, seed=false, tb_cycle_found = false;
bool statistics = false;

struct stack_member_t
{
  state_t state;
  int came_from;
  bool nested;
  bool backtracking; 
};

stack <stack_member_t> gstack,gstack1,gstack2;

int trans=0, transcross=0;
int trans_nested=0, transcross_nested=0;
int pfunc=0;

succ_container_t succs_cont(sys);

struct info_t {
  int allstates;
  size_t maxstates;
  int alltrans;
  int alltrans_nested;
  int allsuccs;
  int allcross;
  int allcross_nested;
  long int allmem,maxbmem,maxrmem,maxtmem;
  virtual void update();
  virtual ~info_t() {}
};

updateable_info_t<info_t> info;

void info_t::update()
{
  if (nid == 0)
    {
      maxstates = allstates = st.get_states_stored();      
      allsuccs = fsuccscalls;
      alltrans = trans;
      alltrans_nested = trans_nested;
      allcross = transcross;
      allcross_nested = transcross_nested;
      allmem = vm.getvmsize();
      maxbmem = bmem;
      maxrmem = rmem;
      maxtmem = tmem;
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
      if (st.get_states_stored() > maxstates)
	{
	  maxstates = st.get_states_stored();
	}
      if (bmem > maxbmem)
	{
	  maxbmem = bmem;
	}
      if (rmem > maxrmem)
	{
	  maxrmem = rmem;
	}
      if (tmem > maxtmem)
	{
	  maxtmem = tmem;
	}
    }
}

// {{{ process messages 

void process_message(char *buf, int size, int src, int tag, bool urgent)
{
//    cout <<ids<<"received "<<tag-DIVINE_TAG_USER<<" from "<<src<<" ";
//    cout<<endl;
  queue_member_t qm;
  stack_member_t sm;
  slong_int_t tempint;
  //  global_state_ref_t tmpstate_ref;
  switch(tag) 
    {
    case TAG_FINISHED:
      finished = true;
      distributed.network.flush_all_buffers();
      break;
    case TAG_STATE:
      if (!finished)
	{
          received_message.load_data((byte_t *)(buf),size);
          received_message.read_data(reinterpret_cast<char *>(&qm.predecessor),sizeof(global_state_ref_t));
          received_message.read_state(qm.state);
//////	  memcpy(&qm.predecessor,buf,sizeof(global_state_ref_t));
//////	  memcpy(qm.state.ptr,buf+sizeof(global_state_ref_t),qm.state.size);
	  q.push(qm);
	  distributed.set_busy();
//  	  cout <<ids<<"received state to be explored ";
//  	  sys.DBG_print_state(qm.state,cout,0);
//  	  cout <<endl;
	}
      break;
    case TAG_FINISHED_CYCLE:
      cycle_found = true;
      finished = true;
      distributed.network.flush_all_buffers();
      break;
    case TAG_CE_CYCLE_STATE:
      // a state on the cycle of the CE from which the path to 
      // the initial state of the graph is constructed
      received_message.load_data((byte_t *)(buf),size);
      received_message.read_data(reinterpret_cast<char *>(&ce_path_start),sizeof(global_state_ref_t));
//////      memcpy(&ce_path_start,buf,sizeof(global_state_ref_t));
      break;
    case TAG_CE_REQ_STATE:
      // sending states from a path to the discovered accepting cycle to master
      received_message.load_data((byte_t *)(buf),size);
      received_message.read_data(reinterpret_cast<char *>(&ce_path_start),sizeof(global_state_ref_t));
//////      memcpy(&ce_path_start,buf,sizeof(global_state_ref_t));
      message.rewind();
      qm.state = st.reconstruct(ce_path_start.state_ref);
      st.get_app_by_ref(ce_path_start.state_ref,appendix);
      message.append_data(reinterpret_cast<byte_t *>(&appendix.predecessor),sizeof(global_state_ref_t));
      message.append_state(qm.state);
/////      memcpy (buffer,&(appendix.predecessor),sizeof(global_state_ref_t));
/////      memcpy (buffer+sizeof(global_state_ref_t),qm.state.ptr,qm.state.size);
      distributed.network.send_message
	(message,0,TAG_CE_STATE);
      distributed.network.flush_buffer(0);
      delete_state(qm.state);
      break;
    case TAG_CE_STATE:
      // building a path to the discovered accepting cycle
      received_message.load_data((byte_t *)(buf),size);
      received_message.read_data(reinterpret_cast<char *>(&ce_path_start),sizeof(global_state_ref_t));
      received_message.read_state(qm.state);
//////      memcpy(&ce_path_start,buf,sizeof(global_state_ref_t));
//////      memcpy(qm.state.ptr,buf+sizeof(global_state_ref_t),qm.state.size);
/*      if (show_ce)
	{
	  sys.DBG_print_state(qm.state,show_ce_out,0);
	  show_ce_out <<" path"<<endl;
	  }*/
      ce.push_front(qm.state);
//        cout <<"path f ";
//        sys.DBG_print_state_CR(qm.state,cout,0);
//        ce.print_trans_ids(cout);
      next_ce_state = true;
      break;
    case TAG_CE_STATE_SIMPLE:
      // building CE from a locally discovered accepting cycle
      qm.state = new_state(buf,size);
//////      memcpy(qm.state.ptr,buf,qm.state.size);
/*      if (show_ce)
	{
	  sys.DBG_print_state(qm.state,show_ce_out,0);
	  show_ce_out <<" cycle"<<endl;
	  }*/
      if (!skip_first_push_to_ce)
	{
  	  ce.push_front(qm.state);
//  	  cout <<"cycl f ";      
//  	  sys.DBG_print_state_CR(qm.state,cout,0);
//  	  ce.print_trans_ids(cout);
	}
      else
	{
	  skip_first_push_to_ce = false;
	}	
      break;
    case TAG_ACK_STATE:
      if (!finished)
	{
  	  global_state_ref_t stref;
  	  appendix_t app1;
	  qm.predecessor.invalidate();
	  qm.state = new_state(buf, size);
//////	  memcpy(qm.state.ptr,buf,qm.state.size);

	  if (st.is_stored(qm.state,stref.state_ref))
	    {
	      // this if is necessary because the initial state is not stored 
	      // on all workstations, but may be removed
	      st.get_app_by_ref(stref.state_ref,app1);
	      if (app1.DepS)
		{
//  		  cout <<ids<<"received state to be removed ";
//  		  sys.DBG_print_state(qm.state,cout,0);
//  		  cout <<endl;
		  acks.push(qm);
		}
	      else
		{
		  delete_state(qm.state);
		}	  
	    } 
	}
      break;
    case TAG_COLLECT_DEPS_SIZE:
      fromall--;
      received_message.load_data((byte_t *)(buf),size);
      for (int i=0; i<sys.get_property_scc_count(); i++)
	{
	  size_int_t local_size;

//////	  memcpy(&tempint,bufaddr,sizeof(int));
//////	  bufaddr += sizeof(int);
          received_message.read_slong_int(tempint);
	  totaldepssize[i] += tempint;

//////	  memcpy(&tempint,bufaddr,sizeof(int));
//////	  bufaddr += sizeof(int);
          received_message.read_slong_int(tempint);
	  totaldepsedge[i] += tempint;

//////	  memcpy(&local_size,bufaddr,sizeof(size_t));
//////	  bufaddr += sizeof(size_t);
          received_message.read_size_int(local_size);
	  rootsall[i] += local_size;

//////	  memcpy(&local_size,bufaddr,sizeof(size_t));
//////	  bufaddr += sizeof(size_t);
          received_message.read_size_int(local_size);
	  leavesall[i] += local_size;
	}
      break;
    case TAG_TB_NEXTTURN:
      if (src==nnn-1)
	{
	  onturn = nnn;
	  all_tb_finished = true;
	  for (int i=1; i<nnn; i++)
	    {
	      distributed.network.send_message(NULL,0,i,TAG_TB_ALL_FINISHED);
	      distributed.network.flush_buffer(i);
	    }
	}
      else
	{
	  onturn = nid;
	}
      break;
    case TAG_TB_ALL_FINISHED:
      all_tb_finished = true;
      break;
    case TAG_TB_RUN:
      run_tbndfs = true;
      break;
    case TAG_TB_FINISHED:
      tb_finished = true;
      break;
    case TAG_TB_FINISHED_CYCLE:
      tb_finished = true;
      tb_cycle_found = true;
      where_found = src;
      received_message.load_data((byte_t *)(buf),size);
      received_message.read_data(reinterpret_cast<char *>(&ce_path_start),sizeof(global_state_ref_t));
      received_message.read_data(reinterpret_cast<char *>(&ce_path_start_predecessor),sizeof(global_state_ref_t));
//////      memcpy(&ce_path_start,buf,sizeof(global_state_ref_t));
//////      memcpy(&ce_path_start_predecessor,buf+sizeof(global_state_ref_t),sizeof(global_state_ref_t));

//        cout <<ids; ce_path_start.print();cout<<" ";
//        ce_path_start_predecessor.print();cout <<endl;

      break;
    case TAG_TB_TOKEN:
      token = true;
      break;
    case TAG_TB_STATE_NESTED:
    case TAG_TB_STATE:
      token = true;
      sm.came_from = src;
      sm.nested = (tag == TAG_TB_STATE_NESTED);
      sm.state = new_state(buf, size);
//////      memcpy(sm.state.ptr,buf,sm.state.size);
      sm.backtracking = false;
      gstack.push(sm);
      break;
    case TAG_TB_CE:
    case TAG_TB_CE_NESTED:
      sm.came_from = src; //!!!
      sm.backtracking=true;
      sm.nested = (tag == TAG_TB_CE_NESTED);
      sm.state = new_state(buf, size);
//////      memcpy(sm.state.ptr,buf,sm.state.size);
      gstack1.push(sm);
      distributed.network.send_message(NULL,0,src,TAG_TB_TOKEN);
      break;
    }
}

// }}}

// {{{ partition function, usage, version 

inline
int partition_function(state_t state)
{
  int tmp=0,start=0,end=state.size;
  int increase = 1;
  int result=73;
 
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
  if (result <0)
    {
      result = -result;
    }
  return result%nnn;
}
 
inline
int partition_function(global_state_ref_t stref)
{
  return stref.network_id;
}

void version()
{
  cout <<"1.0 build 11 (2006/11/21 15:51)"<<endl;
}

void usage()
{
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"DepS-based Cycle Detection ";
  version();
  cout <<"-----------------------------------------------------------------"<<endl;
  cout <<"Usage: [mpirun -np N] deps [options] file.dve"<<endl;
  cout <<"Options: "<<endl;
  cout <<" -v\tshow deps version"<<endl;
  cout <<" -h\tshow this help"<<endl;
  cout <<" -r\tproduce report (file.deps.report)"<<endl;
  cout <<" -o\tdo not employ property decomposition optimizations"<<endl;
  cout <<" -R\tperform delete on data structures that are no more needed"<<endl;
  cout <<" -z\tshow built dependency structure"<<endl;
  cout <<" -t produce trail file (file.deps.trail)"<<endl;
//    cout <<" -s perform reachability only (do not run nested search)"<<endl;
  cout <<" -c\tshow counterexample states (in reverse order!)"<<endl;  
  cout <<" -q\tquiet mode (override all except -v and -h)"<<endl;
  cout <<" -H x\tset the size of hash table to ( x<33 ? 2^x : x )"<<endl;
  cout <<" -S\tprint some statistics"<<endl;
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

// }}}

// {{{ tbndfs 

void tbndfs()
{
  stack_member_t st_member;
  while (!tb_finished)
    {
      distributed.process_messages();
      if (token)
	{

	  st_member = gstack.top();	  
//    	  cout <<ids<<"popped state";
//    	  sys.DBG_print_state(st_member.state,cout,0);
//    	  cout <<endl;	    
	  gstack.pop();
	      
	  if (partition_function(st_member.state)==nid) // local
	    {
	      global_state_ref_t ref;
	      if (!st.is_stored(st_member.state,ref.state_ref))
		{
		  cout <<ids<<"Not stored!!!"<<endl;
		}
	      ref.network_id = distributed.network_id;
	      st.get_app_by_ref(ref.state_ref,appendix);
	      bool stored=appendix.tb_stored;
	      if (!stored)
		{
		  appendix.tb_stored = true;
		  st.set_app_by_ref(ref.state_ref,appendix);
		}
	      
	      if ((st_member.backtracking) ||
		  (stored && !st_member.nested) || 
		  (appendix.tb_nested)) //already visited
		{		    		    
		  if (st_member.backtracking) //do we backtrack?
		    {
		      if (!st_member.nested && 
			  (sys.is_accepting(st_member.state)))
			{
//    			  cout <<ids<<"starting nested ";
//    			  sys.DBG_print_state(st_member.state,cout,0);
//    			  cout <<endl;
 			  st_member.nested = true;
			  st_member.backtracking = false;
			  gstack.push(st_member);
			  seed = true;
			  continue;
			}
		      else
			{
			  appendix.tb_in_stack = false;
			  st.set_app_by_ref(ref.state_ref,appendix);
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
				(NULL,0,x,TAG_TB_FINISHED);
			      distributed.network.flush_buffer(x);
			    }
			}
		      else
			{
			  distributed.network.send_message
			    (NULL,0,st_member.came_from,TAG_TB_TOKEN);
			  distributed.network.flush_buffer(st_member.came_from);
			}
		      // came from network thus new instance was created 
		      // and can be deleted 
		      delete_state(st_member.state);		      
		    }
		}
	      else //not visited
		{
//  		  cout <<ids<<"("<<ref.page_id<<","<<ref.hres<<","<<ref.id<<")";
//  		  cout<<(st_member.nested?" nested":" primary")<<endl;		    
				      
		  if (st_member.nested && appendix.tb_in_stack && !seed) 
		    // it is an accepting cycle?
		    {
//  		      cout <<ids<<"Accepting cycle found!";
//  		      cout <<endl;
		      st_member.backtracking = true;
		      gstack.push(st_member);
		      for (int x = 0; x<nnn; x++)
			{
			  global_state_ref_t stateref_pair[2];
			  stateref_pair[0]=ref;
			  stateref_pair[1]=appendix.predecessor;
			  if (x!=nid)
			    {
			      message.rewind();
			      message.append_data(reinterpret_cast<byte_t *>(stateref_pair),
						  2*sizeof(global_state_ref_t));
			      distributed.network.send_message(message,x,TAG_TB_FINISHED_CYCLE);
			      distributed.network.flush_buffer(x);
			    }
			  else
			    {
			      ce_path_start = ref;
			      ce_path_start_predecessor = appendix.predecessor;

//  			      cout <<ids; ce_path_start.print();cout<<" ";
//  			      ce_path_start_predecessor.print();cout <<endl;
			    }
			}
		      where_found = nid;
		      tb_cycle_found = true;
		      tb_finished = true;
		      token = false;

//  		      cout <<ids<<"TB ";
//  		      sys.DBG_print_state(st_member.state,cout,0);
//  		      cout <<" from ";
//  		      appendix.predecessor.print();
//  		      cout <<endl;
		      continue;
		    }

		  st_member.backtracking = true;  
		  gstack.push(st_member);
		      
		  if (!st_member.nested)
		    {
		      appendix.tb_stored = true;
		      appendix.tb_in_stack = true;
		    }
		  else
		    {
		      // to prevent the seed to be considered  
		      // as visited when it is reached in the 2nd search
		      if (!seed)
			{
			  appendix.tb_nested = true;
			}
		    }
		  st.set_app_by_ref(ref.state_ref,appendix);
		  seed = false;
		      
		  for (list<state_t>::iterator iter = appendix.succs->begin();
		       iter != appendix.succs->end(); iter++)
 		    {
//  		      cout <<ids<<"Generated successor ";
//  		      sys.DBG_print_state(*iter,cout,0);
//  		      cout <<endl;		      

		      //!! no new instances of the states are created
		      //!! states must not be deleted when sending out
		      st_member.state =*iter;
		      st_member.came_from = nid;
		      st_member.backtracking = false;
		      gstack.push(st_member);
		    }
		}
	    }
	  else // remote
	    {
	      token = false;		
//  	      cout <<ids<<"sending state ";
//  	      sys.DBG_print_state(st_member.state,cout,0);
//  	      cout <<endl;
	      distributed.network.send_message
		(st_member.state.ptr,st_member.state.size,
		 partition_function(st_member.state),
		 (st_member.nested?TAG_TB_STATE_NESTED:TAG_TB_STATE));
	      distributed.network.flush_buffer
		(partition_function(st_member.state));
	    }
  	}

    }  
}

// }}}

// {{{ remove 

void remove(state_t &state)
{

//    cout <<ids<<"Remove called for the state ";
//    sys.DBG_print_state(state,cout,0);
//    cout <<endl;

  global_state_ref_t stref;
  st.is_stored(state,stref.state_ref);
  st.get_app_by_ref(stref.state_ref,appendix);
  if (!appendix.DepS)
    {
      return;
    }

  list<state_t> *tmp = appendix.preds;
  //  int tmpcount = appendix.predscount;
  
  if (appendix.succscount != 0)
    {
      cout <<ids<<"something is wrong, removing state that has successors"
	   <<endl;
    }

  if (appendix.succs != NULL)
    {
      if (should_delete)
	{
	  delete appendix.succs;
	}
      appendix.succs = NULL;
    }

  depssize[sys.get_property_scc_id(state)] --;

  appendix.preds = NULL;
  appendix.predscount = 0;
  appendix.DepS = false;
  st.set_app_by_ref(stref.state_ref,appendix);

  
  for (int i=0;i<nnn;i++)
    {
      if (i!=nid)
	{
	  distributed.network.send_message(state.ptr,state.size,
				      i,TAG_ACK_STATE);
	}
    }

  if (tmp==NULL)
    {
//         cout <<ids<<"removing root"<<endl;

      list<state_t>::iterator k=roots[sys.get_property_scc_id(state)].begin();
      while (k!=roots[sys.get_property_scc_id(state)].end())
	{
	  state_t s = *k;
	  if (state == s)
	    {
	      k=roots[sys.get_property_scc_id(state)].erase(k);
	      delete_state(s);
	    }
	  else
	    {
	      k++;
	    }
	}      
    }
  else
    {
      for (list<state_t>::iterator i=tmp->begin(); i!=tmp->end(); i++)
	{
	  global_state_ref_t s_ref;
	  st.is_stored(*i,s_ref.state_ref);
	  st.get_app_by_ref(s_ref.state_ref,appendix);
	  if (appendix.succscount == 0 || appendix.succs==NULL)
	    {
	      cout <<ids<<"something is wrong, inconsistence in DepS"<<endl;
	    }      

	  list<state_t>::iterator k=appendix.succs->begin();
	  while (k!=appendix.succs->end())
	    {
	      state_t s = *k;
//  	  cout <<ids<<"comparing states ";
//  	  sys.DBG_print_state(state,cout,0);
//  	  cout <<" and ";
//  	  sys.DBG_print_state(s,cout,0);
//  	  cout <<endl;
	  
	      if (state == s)
		{
		  k=appendix.succs->erase(k);
		  delete_state(s);
		  depsedge[sys.get_property_scc_id(state)] --;
		}
	      else
		{
		  k++;
		}
	    }
	  
	  appendix.succscount --;      
	  if (appendix.succscount == 0)
	    {
	      if (should_delete)
		{
		  delete appendix.succs;
		}
	      appendix.succs = NULL;
	    }	    
	  st.set_app_by_ref(s_ref.state_ref,appendix);
	}
      

      while (!tmp->empty())
	{
	  global_state_ref_t s_ref;
	  state_t s = tmp->front();
	  tmp->pop_front();
	  st.is_stored(s,s_ref.state_ref);
	  st.get_app_by_ref(s_ref.state_ref,appendix);
	  if (appendix.succscount == 0)
	    {
	      remove(s);
	    }
	  delete_state(s);
	}

      if (should_delete)
	{
	  delete tmp;
	}
    }
}

// }}}

// {{{ draw_deps 

// appendix.in_stack is used for marking already drawn states !!!
void draw_deps(state_t state,int depth)
{  
  global_state_ref_t stref;
  st.is_stored(state,stref.state_ref);
  st.get_app_by_ref(stref.state_ref,appendix);
  if (!appendix.in_stack)
    {
      appendix.in_stack=true;
      st.set_app_by_ref(stref.state_ref,appendix);
      deps_out <<ids;
      for (int i=0; i<depth; i++)
	{
	  deps_out <<" ";
	}
      sys.DBG_print_state(state,deps_out,0);
      deps_out <<endl;
      if (appendix.succscount >0)
	{
	  for (list<state_t>::iterator i=appendix.succs->begin();
	       i!=appendix.succs->end();
	       i++)
	    {
	      draw_deps(*i,depth+3);
	      st.get_app_by_ref(stref.state_ref,appendix);
	    }
	}
    }
  else
    {
      deps_out <<ids;
      for (int i=0; i<depth-3; i++)
	{
	  deps_out <<" ";
	}
      deps_out <<"<- ";
      sys.DBG_print_state(state,deps_out,0);
      deps_out <<endl;
    }
    
}
// }}}

// {{{ NewDep 

void newdep(state_t &last, state_t &state)
{
  global_state_ref_t sref;
  state_t ss;
  if (last.ptr==NULL) // new root
    {
      st.is_stored(state,sref.state_ref);
      st.get_app_by_ref(sref.state_ref,appendix);
      if (appendix.DepS == false)
	{
	  depssize[sys.get_property_scc_id(state)]++;
	}
      appendix.DepS=true;
      st.set_app_by_ref(sref.state_ref,appendix);
    }
  else
    {
      st.is_stored(last,sref.state_ref);
      st.get_app_by_ref(sref.state_ref,appendix);
      if (appendix.succscount==0)
	{
	  appendix.succs = new list<state_t>;
	}
      appendix.succscount++;
      ss=duplicate_state(state);
      appendix.succs->push_front(ss);
      st.set_app_by_ref(sref.state_ref,appendix);

      st.is_stored(state,sref.state_ref);
      st.get_app_by_ref(sref.state_ref,appendix);
      if (appendix.predscount==0)
	{
	  appendix.preds = new list<state_t>;
	}
      appendix.predscount++;
      ss=duplicate_state(last);
      appendix.preds->push_front(ss);
      if (appendix.DepS == false)
	{
	  depssize[sys.get_property_scc_id(state)]++;
	}
      appendix.DepS = true;
      st.set_app_by_ref(sref.state_ref,appendix);
      depsedge[sys.get_property_scc_id(state)]++;
    }
}

// }}}

// {{{ NestedDFS 

void NestedDFS(state_t state)
{
//    cout <<ids<<"nested ";
//    sys.DBG_print_state(state,cout,0);
//    cout <<endl;

  global_state_ref_t stref;

  if ((++ndepth)>maxndepth)
    {
      maxndepth = ndepth;
    }

  distributed.process_messages();
  if (finished) 
    {
      cout <<"skipping - finished"<<endl;
      ndepth --;
      return;
    }

  if (!st.is_stored(state,stref.state_ref))
    {
      cout <<"NDFS: searching through a nonexplored state"<<endl;
    }
  stref.network_id = distributed.network_id;
  st.get_app_by_ref(stref.state_ref,appendix);
  if (!appendix.V)
    {
      appendix.V = true;
      st.set_app_by_ref(stref.state_ref,appendix);
      
      sys.get_succs(state,succs_cont);
      vector<state_t> succs_vec(succs_cont.size());
      for (size_t i = 0; i<succs_cont.size(); i++)
	{
	  succs_vec[i]=succs_cont[i];
	}
      fsuccscalls++;
      
      for (size_t i = 0; i<succs_vec.size(); i++)
  	{
	  trans_nested ++;
  	  state_t s=succs_vec[i];
  	  
	  if (!cycle_found &&   // more immediate successors may produce cycle
	                        // and also when backtracking skip unexplored immediate successors
	      partition_function(state)==partition_function(s) &&
	      (!optimized || 
	       (optimized && sys.get_property_scc_id(state)==sys.get_property_scc_id(s))))
  	    {
	      st.is_stored(s,stref.state_ref);
	      stref.network_id = distributed.network_id;
	      st.get_app_by_ref(stref.state_ref,appendix);
  	      if (appendix.in_stack)
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
//  		  if (!quietmode)
//  		    {	
//  		      cout <<ids<<"======================"<<endl;
//  		      cout <<ids<<"Accepting cycle found!"<<endl;
//  		      cout <<ids<<"======================"<<endl;
//  		    }
  		  cycle_found = true;
  		  finished = true;
  		  memcpy (&cycle_point,&stref,sizeof(global_state_ref_t));
  		  in_cycle = true;
  		  distributed.network.send_message
  		    (s.ptr, s.size,0,TAG_CE_STATE_SIMPLE);
  		}
 	      else
 		{
  		  NestedDFS(s);
 		  if (cycle_found)
  		    {
  		      distributed.network.send_message
  			(s.ptr, s.size,0,TAG_CE_STATE_SIMPLE);
  		    }
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
//    cout <<ids<<"first  ";
//    sys.DBG_print_state(state,cout,0);
//    cout <<" from ";
//    predecessor.print();
//    cout<<endl;

  global_state_ref_t stref;
  
  if ((++depth)>maxdepth)
    {
      maxdepth = depth;
    }

  //cout <<"."<<flush;

  distributed.process_messages();
  if (finished) 
    {
      depth --;
      return;
    }

//    cout <<ids<<"FirstDFS processes state ";
//    sys.DBG_print_state(state,cout,0);
//    cout <<" came from ("<<predecessor.page_id<<","<<predecessor.hres<<","
//         << predecessor.id<<")";
//    cout<<endl;

//    if (!st.is_stored(state,stref.state_ref))
//      {
//        cout <<"something is wrong, FirstDFS called for a new state,"
//  	   <<"(last.ptr = "<< (int)(last.ptr)
//  	   <<")"
//  	   <<endl;
//      }

  stref.network_id = distributed.network_id;
  st.get_app_by_ref(stref.state_ref,appendix);                        
  appendix.in_stack = true;           //in_stack[i] := in_stack[i] + {state}
  st.set_app_by_ref(stref.state_ref,appendix);

  // push(last) -- not necessary

  if (optimized && sys.get_property_scc_type(state)==0)
    {
      appendix.PV = true;
      st.set_app_by_ref(stref.state_ref,appendix);
    }
  else
    {
      if (last.ptr==NULL || sys.is_accepting(state))
	{
	  appendix.PV = true;
	  st.set_app_by_ref(stref.state_ref,appendix);
	  newdep(last,state);
	  last = state;	    
	  state_t s;
	  s = duplicate_state(state);
	  roots[sys.get_property_scc_id(state)].push_back(s);
	}
    }

  sys.get_succs(state,succs_cont);
  vector<state_t> succs_vec(succs_cont.size());
  for (size_t i = 0; i<succs_cont.size(); i++)
    {
      succs_vec[i]=succs_cont[i];
    }
  fsuccscalls++;

  for (size_t i = 0; i<succs_vec.size(); i++)   //foreach s \in F_succs(state)
    {
      if (cycle_found) continue; // skip unexplored immediate successors 
                                 // when backtracking after cycle was found
      trans++;
      state_t s=succs_vec[i];
      global_state_ref_t s_ref;
      if (!st.is_stored(s,s_ref.state_ref))
	{
	  st.insert(s,s_ref.state_ref);
	  s_ref.network_id = distributed.network_id;
	  appendix.V = false;
	  appendix.PV = false;
	  appendix.DepS = false;
	  appendix.in_stack = false;
	  appendix.preds = NULL;
	  appendix.succs = NULL;
	  appendix.predscount=0;
	  appendix.succscount=0;
	  appendix.predecessor = stref;
	  appendix.tb_nested = false;
	  appendix.tb_stored = false;
	  appendix.tb_in_stack = false;

	  st.set_app_by_ref(s_ref.state_ref,appendix);
	  if (nid != partition_function(s))
	    {
	      states_overhead ++;
	    }
	}
      else
	{
	  st.get_app_by_ref(s_ref.state_ref,appendix);
	}
      
      if (nid != partition_function(s)||
	  (optimized && sys.get_property_scc_id(state)!=sys.get_property_scc_id(s)))
	//if (Partition(s)!=i)
	{
	  transcross++;
          message.rewind();
          message.append_data(reinterpret_cast<byte_t *>(&stref),sizeof(global_state_ref_t));
          message.append_state(s);
//////	  memcpy (buffer,&stref,sizeof(global_state_ref_t));
//////	  memcpy (buffer+sizeof(global_state_ref_t),s.ptr,s.size);
	  distributed.network.send_message   // SendTo(....)
	    (message, 
	     partition_function(s),
	     TAG_STATE);
	  if ( !optimized || 
	       (optimized &&
		(sys.get_property_scc_id(state)==sys.get_property_scc_id(s) &&
		 sys.get_property_scc_type(s) != 0)))
	    {
	      newdep(last,s);               //NewDep(DepS[i],last,s)
	    }
	}
      else
	{	  
	  if (!appendix.PV && !appendix.in_stack) 
	    {
//  	      if ((last.ptr == NULL && !optimized) ||
//  		  (optimized && sys.get_property_scc_type(state)!=0 && last.ptr == NULL && 
//  		   sys.get_property_scc_id(state)==sys.get_property_scc_id(s)))
//  		{
//  		  cout <<ids<<"something is wrong, last.ptr == NULL"<<endl;
//  		}
//	      cout <<ids<<"FirstDFS call from inside"<<endl;
	      FirstDFS(s,stref);
	    }
	  else
	    {
	      if (!appendix.in_stack && sys.get_property_scc_type(s)!=0)
		{
		  newdep(last,s);
		}
	    }
	}
      delete_state(s);
    }
      
  if (sys.is_accepting(state) && !cycle_found)   // && !reach_only)
    {
      NestedDFS(state);
    }
 
  // {{{ local counterexample reconstruction 

  if (cycle_found)
    {
      if (in_cycle)
	{
	  distributed.network.send_message
	    (state.ptr, state.size,0,TAG_CE_STATE_SIMPLE);
//    	  sys.DBG_print_state(state,cout,0); 
//    	  cout << " cycle closing" << endl;
	  if ((memcmp(&stref,&cycle_point,sizeof(global_state_ref_t))==0))
	    {
	      in_cycle = false;
	      st.get_app_by_ref(stref.state_ref,appendix);
	      message.rewind();
	      message.append_data(reinterpret_cast<byte_t *>(&(appendix.predecessor)),
				  sizeof(global_state_ref_t));
	      distributed.network.send_message(message,0,TAG_CE_CYCLE_STATE);
	      distributed.network.flush_buffer(0);
	    }
	}
      return;       
    }      

  // }}}

  st.get_app_by_ref(stref.state_ref,appendix);

  if (appendix.DepS)
    {
      if (appendix.predscount > 1)
	{
	  cout <<ids<<"something is wrong, |preds| != 1"<<endl;
	}

      if (appendix.predscount == 0)         // pop(last)
	{
	  last.ptr=NULL;
	}
      else
	{
	  last = appendix.preds->front();
	}
  
      if (appendix.succscount == 0)
	{
	  state_t s=duplicate_state(state);
	  leaves[sys.get_property_scc_id(state)].push_front(s);
	}
    }

  appendix.in_stack = false;           // in_stack[i]:=in_stack[i]\{state}
  st.set_app_by_ref(stref.state_ref,appendix);
  depth --;
}

// }}}

// {{{ initialization 

void init_all(int argc, char **argv)
{
  distributed.process_user_message = process_message;
  distributed.set_proc_msgs_buf_exclusive_mem(false);
  distributed.network_initialize(argc, argv);


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
  nid = distributed.network_id;
  nnn = distributed.cluster_size;

  oss1<<argv[0];
  int c;
  opterr=0;
  while ((c = getopt(argc, argv, "RLX:YZhrvVozstcH:SqP:")) != -1) {
    oss1 <<" -"<<(char)c;
    switch (c) {
    case 'L': logging = true; break;
    case 'X': set_base_name = optarg; base_name = true; break;
    case 'h': if (nid == 0) usage();
      distributed.network.barrier();
      distributed.finalize();
      exit(0);break;
    case 't': trail=true;break;
    case 'r': report = true;break;
    case 's': reach_only=true; break;
    case 'c': show_ce = true; break;
    case 'V': if (nid == 0) version();
      distributed.network.barrier();
      distributed.finalize();
      exit(0);break;
    case 'R': should_delete = true; break;
    case 'z': show_deps = true; break;
    case 'o': optimized = false; break;
    case '?': cerr <<"unknown switch -"<<(char)optopt<<endl;break;
    case 'H': htsize=atoi(optarg);break;
    case 'v':
    case 'S': statistics = true;break;
    case 'q': quietmode = true;break;
    case 'P': pfunc=atoi(optarg); break;
    }
  }

  if (quietmode)
    {
      statistics = show_ce = false;
    }     

  if (argc < optind+1)
	{	  
	  if (nid == 0)
	    {
	      usage();
	    }
	  distributed.finalize();
	  exit(0);
	}
  
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

  // Since we want to store states that are owned by other workstations
  // locally we do not let distributed take control of partitioning but we use
  // the method explicit_storage_t::get_page_id() instead.  We wrap it in a
  // new "partition_function()" in case of future modification of the
  // partitioning scheme.
  
  st.set_appendix(appendix);
  st.init();

  distributed.initialize();


  
  // cout <<ids<<distributed.processor_name<<endl;
  
  //reading of input:
  int file_opening;
  try
    {
      if ((file_opening=sys.read(argv[optind]))&&(nid==NETWORK_ID_MANAGER))
	if (file_opening==system_t::ERR_FILE_NOT_OPEN)
	  gerr << nid << ": " << "Cannot open file ...";
	else gerr << nid << ": " << "Syntax error ...";
      if (file_opening)
	gerr << thr();
    }
  catch (ERR_throw_t & err)
    { 
      distributed.finalize();
      exit(err.id);
    }

  file_name = argv[optind];
  int position = file_name.find(".dve",0);
  file_name.erase(position,4);

  if (!sys.get_with_property())
    {
      if (nid==0)
	{
	  cerr <<"Verified model without LTL property."<<endl;
	}
      distributed.finalize();
      exit (1);
    }
  else
    {
      if (sys.get_property_type()!=BUCHI)
	{
	  if (nid==0)
	    {
	      cerr <<"Cannot work with other than standard Buchi accepting condition."<<endl;
	    }
	  distributed.finalize();
	  exit (1);
	}
    }

  if ((trail) && (!sys.get_abilities().system_can_transitions) && (distributed.network_id == 0))
    {
      cout << "Model of system can't handle transitions, trail won't be produced." << endl;
      trail = false;
    }

  if (reach_only && trail)
    {
      if (nid==0)
	cerr <<"Reachability cannot produce trail, switch -t skipped."<<endl;
      trail=false;
    }

  reporter.start_timer();
  
  leaves.resize(sys.get_property_scc_count());
  roots.resize(sys.get_property_scc_count());
  depssize.resize(sys.get_property_scc_count());
  depsedge.resize(sys.get_property_scc_count());
  maxdepssize.resize(sys.get_property_scc_count());
  maxdepsedge.resize(sys.get_property_scc_count());
  totaldepssize.resize(sys.get_property_scc_count());
  totaldepsedge.resize(sys.get_property_scc_count());
  rootsall.resize(sys.get_property_scc_count());
  leavesall.resize(sys.get_property_scc_count());
  

  
  if (nid==0) 
    {
      init_time = timer.gettime();
      //timer.reset(); //computation time includes initialization
    }

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
}

// }}}








// {{{ workstation[i] 

void workstation()
{
  global_state_ref_t stref;  
  if (nid == 0 && !quietmode)
    {
      cout<<ids<<"Building DepS ... "<<flush;
    }
  
  state = sys.get_initial_state();
  
  if (partition_function(state) != nid)
    {
      delete_state(state);
      distributed.set_idle();
    }
  else
    {
      queue_member_t qm;
      qm.state = state;
      q.push(qm);
      distributed.set_busy();
    }
  
  while (!distributed.synchronized())
    {
      distributed.process_messages();

      //cout <<"."<<flush;
      
      if (!q.empty())
	{
	  queue_member_t qm1 = q.front();
	  q.pop();
	  if (!finished)
	    {
	      if (!st.is_stored(qm1.state,stref.state_ref))
		{
		  st.insert(qm1.state,stref.state_ref);
		  stref.network_id = distributed.network_id;
		  appendix.V = false;
		  appendix.PV = false;
		  appendix.DepS = false;
		  appendix.in_stack= false;
		  appendix.preds = NULL;
		  appendix.succs = NULL;
		  appendix.predscount=0;
		  appendix.succscount=0;
		  appendix.predecessor = qm1.predecessor;
		  appendix.tb_nested = false;
		  appendix.tb_stored = false;
		  appendix.tb_in_stack = false;
		  st.set_app_by_ref(stref.state_ref,appendix);
		}
	      else
		{
		  st.get_app_by_ref(stref.state_ref,appendix);
		}		  
	      
	      if (!appendix.PV)
		{
		  if (last.ptr!=NULL)
		    {
		      cout <<ids<<"something is wrong,(last.ptr is not NULL)"
			   <<endl;
		    }
		  FirstDFS(qm1.state,qm1.predecessor);
		  distributed.network.flush_all_buffers();
		}
	    }
	  delete_state(qm1.state);
	}
      else
	{
	  distributed.set_idle();
	}
    }
  
  if (nid == 0 && !quietmode)
    {
      cout <<" done"<<endl;
      if (!cycle_found)
	{
	  cout <<ids<<"No local accepting cycles found."<<endl;
	}
      else
	{
	  cout <<ids<<"Local accepting cycle found."<<endl;	  
	}
    }
}

// }}}

// {{{ complete_path 


// this procedure search a path from a state given in 'q' to the given state 'state'
// uses appendix.deps to mark already explored states
// the path is reconstructed where??? 

// problem: one state on the cycle has to have 2 predecessors!

void complete_path(state_t state)
{
  bool found = false;
  global_state_ref_t stref;
  queue_member_t qm,qm1;

  while (!q.empty())
    {
      qm = q.front();
      q.pop();

      if (found)
	{
	  continue;
	}
      	      
      if (!qm.predecessor.is_valid())  //only for the first time
	{
	  if (!st.is_stored(qm.state,qm.predecessor.state_ref))
	    {
	      //cout <<ids<<"This is somehow strange!"<<endl;
    	      st.insert(qm.state,qm.predecessor.state_ref);
	    }	  
	  qm.predecessor.network_id = distributed.network_id;
	}
    
//        cout <<ids<<"expanding state ";
//        sys.DBG_print_state(qm.state,cout,0);
//        cout <<" (while searching for ";
//        sys.DBG_print_state(state,cout,0);
//        cout <<")"<<endl;

      succ_container_t succs(sys);
      sys.get_succs(qm.state,succs);  
      
      for (succ_container_t::iterator i = succs.begin(); i!=succs.end(); i++)
	{	  
	  qm1.state = *i;
	  if (!found)
	    {
	      if (!st.is_stored(qm1.state,stref.state_ref))
		{
		  st.insert(qm1.state,stref.state_ref);
		}
	      stref.network_id = distributed.network_id;
	      qm1.predecessor = stref;

	      st.get_app_by_ref(stref.state_ref,appendix);
	      appendix.predecessor = qm.predecessor;
	      st.set_app_by_ref(stref.state_ref,appendix);
	      
	      if (qm1.state == state)
		{
		  found = true;
		}      
	      else
		{
//  		  if (!appendix.DepS)
//  		    {
		      q.push(qm1);
//  		    }
		}
	    }
	  delete_state(qm.state);
	}
    }    
}
// }}}

int main(int argc, char **argv)
{
  try 
    {
      int rootssum=0;
      int tbndfscount = 0;
      //      global_state_ref_t stref;
      
      init_all(argc, argv);
      
      workstation(); //perform the deps construction phase ...

      bmem = vm.getvmsize();      
      timeafterdepsbuilt = timer.gettime();
      
      // {{{ clearing roots and draw deps 


      if (show_deps || !cycle_found)
	{
	  // some roots are roots no more !!!
	  for (int j=0; j<sys.get_property_scc_count(); j++)
	    {      
	      list<state_t>::iterator k=roots[j].begin();
	      while (k!=roots[j].end())
		{
		  global_state_ref_t stref;
		  state_t s=*k;
		  st.is_stored(s,stref.state_ref);
		  st.get_app_by_ref(stref.state_ref,appendix);
		  if (appendix.predscount != 0)
		    {
		      k=roots[j].erase(k);
		      delete_state(s);
		      //cout <<ids<<"invalid root in roots["<<j<<"]"<<endl;
		    }
		  else
		    {
		      k++;
		    }
		}      
	    }
	  
	  if (show_deps)
	    {
	      for (int j=0; j<sys.get_property_scc_count(); j++)
		{      
		  for (list<state_t>::iterator i=roots[j].begin();
		       i!=roots[j].end(); i++)
		    {
		      draw_deps(*i,4);
		    }
		}
	    }
	}

      // }}}

      // {{{ removing phase 

      if (!cycle_found)    //Distributed cycle detection
  	{

  	  if (nid==0 && !quietmode)
  	    {
	      cout <<ids<<"Removing states from DepS ... "<<flush;
	    }

	  // is it important to call synchronized here?
	  // YES  -- the following remove generate some messages 
	  //      -- that are processed in process_messages()
	  distributed.synchronized();  

//  	  if (nid== 0) cout <<endl;
//  	  for (int j=0; j<sys.get_property_scc_count(); j++)
//  	    {
//    	      cout <<ids<<"leaves["<<j<<"].size()="<<leaves[j].size()<<endl;
//    	      cout <<ids<<"roots["<<j<<"].size()="<<roots[j].size()<<endl;
//  	      cout <<ids<<"depssize["<<j<<"]="<<depssize[j]<<endl;
//  	      cout <<ids<<"depsedge["<<j<<"]="<<depsedge[j]<<endl;
//  	    }

	  for (int j=0; j<sys.get_property_scc_count(); j++)
	    {
	      maxdepsedge[j]=depsedge[j];
	      maxdepssize[j]=depssize[j];
	      while (!leaves[j].empty())
		{
		  state_t s=leaves[j].front();
		  leaves[j].pop_front();
//  		  cout <<ids<<"removing ";
//  		  sys.DBG_print_state(s,cout,0);
//  		  cout <<endl;
		  remove(s);
		  delete_state(s);
		  distributed.process_messages();
		}
	    }

//  	  for (int j=0; j<sys.get_property_scc_count(); j++)
//  	    {
//    	      cout <<ids<<"leaves["<<j<<"].size()="<<leaves[j].size()<<endl;
//    	      cout <<ids<<"roots["<<j<<"].size()="<<roots[j].size()<<endl;
//  	      cout <<ids<<"depssize["<<j<<"]="<<depssize[j]<<endl;
//  	      cout <<ids<<"depsedge["<<j<<"]="<<depsedge[j]<<endl;
//  	    }

	  if (!acks.empty())
	    {
	      distributed.set_busy();
	    }

	  while (!distributed.synchronized())
	    {
	      distributed.process_messages();
	      if (!acks.empty())
		{
		  queue_member_t qm2 = acks.front();
		  acks.pop();
		  if (!finished)
		    {
		      global_state_ref_t stref;
		      st.is_stored(qm2.state,stref.state_ref);
		      st.get_app_by_ref(stref.state_ref,appendix);
		      if (appendix.DepS)
			{
			  remove(qm2.state);
			}
		      distributed.network.flush_all_buffers();
		    }
		  delete_state(qm2.state);
		}
	      else
		{
		  distributed.set_idle();
		}
	    }

	  // now collect info about DepS remainders
	  fromall=nnn;
//////	  char *bufaddr = buffer;
          message.rewind();
	  for (int i=0; i<sys.get_property_scc_count(); i++)
	    {
	      size_int_t local_size;
              
              message.append_slong_int(depssize[i]);
              message.append_slong_int(depsedge[i]);
//////	      memcpy(bufaddr,&(depssize[i]),sizeof(int));
//////	      bufaddr += sizeof(int);
//////	      memcpy(bufaddr,&(depsedge[i]),sizeof(int));
//////	      bufaddr += sizeof(int);
	      
	      local_size = roots[i].size();
              message.append_size_int(local_size);
//////	      memcpy(bufaddr,&local_size,sizeof(size_t));
//////	      bufaddr += sizeof(size_t);
	      local_size = leaves[i].size();
              message.append_size_int(local_size);
//////	      memcpy(bufaddr,&local_size,sizeof(size_t));
//////	      bufaddr += sizeof(size_t);
	    }
	  distributed.network.send_message(message,
				      0,TAG_COLLECT_DEPS_SIZE);
	  distributed.network.flush_all_buffers();
	  
	  if (nid == 0)
	    {
	      int depssizesum=0;
	      if (!quietmode)
		{
		  cout <<"done"<<endl;
		}
	      while (!fromall==0)
		{
		  distributed.process_messages();
		}

	      for (int j=0; j<sys.get_property_scc_count(); j++)
		{
		  rootssum += rootsall[j];		  
		  if (optimized)
		    {
		      if (!quietmode) cout <<ids<<"Component "<<j<<" ";
		      switch (sys.get_property_scc_type(j))
			{
			case 0:
			  if (!quietmode) cout <<"(N) ";
			  break;
			case 1:
			  if (!quietmode) cout <<"(P) ";
			  if (totaldepssize[j]>0)
			    {
			      tbndfs_needed = true;
			    }				    
			  break;
			case 2:
			  if (!quietmode) cout <<"(F) ";
			  if (totaldepssize[j]>0)
			    {
			      cycle_proved = true;
			    }
			  break;
			}
		       if (!quietmode)
			 {
			   cout <<" -- "
			   <<totaldepssize[j]<<" states remain in DepS."
			   <<endl;
			 }
		    }
		  else
		    {
		      if (totaldepssize[j]>0)
			{
			  tbndfs_needed = true;
			  depssizesum += totaldepssize[j];
			}		  
		    }
		}
	      if (!optimized && !quietmode)
		{
		  cout <<ids<<depssizesum<<" states remain in DepS."<<endl;
		}
	    }
	}     

      distributed.network.barrier();

      // }}}

      rmem = vm.getvmsize();      
      timeafterdepsrem = timer.gettime();

      // {{{ detecting accepting cycles in DepS remainders

      if (cycle_proved && nid == 0 && optimized)
	{
	  if (!quietmode) cout <<ids<<"Accepting cycle exists!"<<endl;
	  if (nid == 0 && (trail || show_ce))
	    {
	      tbndfs_needed = true;
	    }      
	}
      
      if (!cycle_found)
	{
	  all_tb_finished = false;
	  if (nid == 0)
	    {
	      if (!tbndfs_needed)
		{
		  all_tb_finished = true;
		  for (int i=1; i<nnn; i++)
		    {
		      distributed.network.send_message(NULL,0,i,TAG_TB_ALL_FINISHED);
		    }
		  distributed.network.flush_all_buffers();		      		  
		}
	      else
		{
		  if (!quietmode)
		    {
		      cout <<ids<<"Performing token based "
			   <<"Nested DFS on DepS remainders ... 0/"
			   << rootssum << flush;
		      printf("\r");
		      cout<<flush;
		    }
		}
	    }

	  while (!all_tb_finished)
	    {
	      distributed.process_messages();
	      if (run_tbndfs)
		{
		  run_tbndfs = false;
		  tb_finished = false;
		  distributed.network.barrier();
		  tbndfs();
		  tbndfscount ++;
		  if (nid == 0  && !quietmode)
		    {
		      cout <<ids<<"Performing token based "
			   <<"Nested DFS on DepS remainders ... "
			   << tbndfscount <<"/"
			   << rootssum << flush;
		      printf("\r");
		      cout<<flush;
		    }
		}
	      if (onturn == nid)
		{
		  for (int j=0; j<sys.get_property_scc_count();j++)
		    {
		      for (list<state_t>::iterator k=roots[j].begin();
			   k!=roots[j].end(); k++)
			{
			  if (!tb_cycle_found)
			    {
			      stack_member_t st_member;
			      st_member.state = *k;
			      st_member.came_from = -1;
			      st_member.nested = false;
			      st_member.backtracking = false;
			      gstack.push(st_member);	
			      token = true;
			      for (int l=0;l<nnn;l++)
				{
				  if (l!=nid)
				    {
				      distributed.network.send_message
					(NULL,0,l,TAG_TB_RUN);
				    }
				}
			      distributed.network.flush_all_buffers();
			      tb_finished = false;
//  			      cout <<ids<<"running tbndfs (";
//  			      sys.DBG_print_state(*k,cout,0);
//  			      cout <<")"<<endl;
			      distributed.network.barrier();			  
			      tbndfs();
			      tbndfscount ++;
			      if (nid == 0 && !quietmode)
				{
				  cout <<ids<<"Performing token based "
				       <<"Nested DFS on DepS remainders ... "
				       << tbndfscount <<"/"
				       << rootssum << flush;
				  printf("\r");
				  cout<<flush;
				}
			    }
			}
		    }
		  onturn++;
		  distributed.network.send_message(NULL,0,onturn%nnn,
					      TAG_TB_NEXTTURN);
		  distributed.network.flush_buffer(onturn%nnn);		  
		}
	    }
	  
	  if (nid== 0 && tbndfs_needed && !quietmode)
	    {
	      cout <<ids<<"Performing token based "
		   <<"Nested DFS on DepS remainders ... "
		   << tbndfscount <<"/"
		   << rootssum
		   <<" done."<<endl;	 

	      if (!tb_cycle_found)
		{
		  cout <<ids<<"No distributed accepting cycles found."<<endl;
		}
	      else
		{
		  cout <<ids<<"Accepting cycle found!"<<endl;
		}
	    }
	}

      // }}}

      tmem = vm.getvmsize();
      timeaftertbndfs = timer.gettime();

      if (logging)
        {
          logger.log_now();
          logger.stop_SIGALRM();
        }
      
      distributed.network.barrier(); // do not ask me why :-)  but this is necessary

      // {{{ counterexample generation 

      if (!(show_ce||trail) )
	{
	  tb_cycle_found = false;
	  cycle_found = false;
	}

      if ((tb_cycle_found || cycle_found) && nid == 0)
	{
	   if (!quietmode) 
	     {
	       cout <<ids<<"Generating counterexample ... "<<flush;
	     }
	}

      // we reconstruct the path from the initial state to a state on cycle
      // NOTE: in the case of local cycle we already have the cycle in "ce"
      // !!!! if there wasnot local cycle, we have to extract it from DepS :-(
      // {{{ cycle reconstruction from DEPS 

      if (tb_cycle_found)	
	{
	  if (nid == 0)
	    {
	      while (!gstack2.empty())
		{
		  gstack2.pop();
		}
	      while (!gstack1.empty())
		{
		  gstack1.pop();
		}
	    }


	  stack_member_t st_member;
	  bool first=true;
	  distributed.network.barrier(); 
	  if (where_found == nid)
	    {
	      token = true;
	    }
	  else
	    {
	      token = false;
	    }

	  //First, we collect all states of a cycle from gstack (tbndfs) to gstack1 on master.

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
			     0, (st_member.nested?TAG_TB_CE_NESTED:TAG_TB_CE));
			  distributed.network.flush_buffer(0);
			  // wait for acknowledgement from master, 
			  // this ensures message ordering 
			  token = false; 
			  while (!token)
			    {
			      distributed.process_messages();
			    }
			}   

		      global_state_ref_t s_ref;
		      st.is_stored(st_member.state,s_ref.state_ref);
		      if (where_found == nid && 
			  (memcmp(&ce_path_start.state_ref,&s_ref.state_ref,
				  sizeof(state_ref_t))==0))
			{ 
			  ce_path_start.network_id = distributed.network_id;
			  if (first)
			    {
			      first = false;
			    }
			  else
			    {
			      for (int x = 0; x<nnn; x++)
				{
				  distributed.network.send_message
				    (NULL,0,x,TAG_FINISHED);
				  distributed.network.flush_buffer(x);
				}
			      token = false;
			      continue;
			    }
			}

		      if (st_member.came_from != nid)
			{
			  token = false;
			  distributed.network.send_message
			    (NULL,0,st_member.came_from,TAG_TB_TOKEN);
			  distributed.network.flush_buffer(st_member.came_from);
			}    
		    }
		}
	    }
	
	  // all states of the cycle that were in DepS should now be on master in gstack1
	  // this might not be a valid path as DepS need not contain all states of the cycle
	  // now we complete its missing parts (completed cycle will be stored in gstack)
	
	  if (nid == 0)
	    {
	      while (!gstack.empty())
		{
		  gstack.pop();
		}

	      while (!gstack1.empty())
		{
		  st_member = gstack1.top();
		  gstack1.pop();
		  gstack.push(st_member);		  
		  if (!gstack1.empty())
		    {
		      queue_member_t qmtmp;
		      stack_member_t sm;
		      global_state_ref_t stref;
		      state_t s;
		      sm = gstack.top();
		      qmtmp.state = duplicate_state(sm.state); // deleted in complete_path()
		      q.push(qmtmp);
		      qmtmp.state = sm.state;
		      sm = gstack1.top();
		      
//  		      cout <<ids;
//  		      sys.DBG_print_state(qmtmp.state,cout,0);
//  		      cout <<" -> ";
//  		      sys.DBG_print_state(sm.state,cout,0);
//  		      cout <<endl;
	
		      
//  		      if (where_found == nid)
//  			{
//  			  st.get_app_by_ref(ce_path_start.state_ref,appendix);
//  			  cout <<ids<<"pre CP:";
//  			  appendix.predecessor.print();
//  			  cout <<endl;
//  			}

		      complete_path(sm.state);		      

//  		      if (where_found == nid)
//  			{
//  			  st.get_app_by_ref(ce_path_start.state_ref,appendix);
//  			  cout <<ids<<"post CP:";
//  			  appendix.predecessor.print();
//  			  cout <<endl;
//  			}

		      st.is_stored(sm.state,stref.state_ref);
		      st.get_app_by_ref(stref.state_ref,appendix);
		      s = st.reconstruct(appendix.predecessor.state_ref);
		      while (s != qmtmp.state)
			{
			  stack_member_t sm1;
			  sm1.state = s;
			  gstack2.push(sm1);
			  st.is_stored(sm1.state,stref.state_ref);
			  st.get_app_by_ref(stref.state_ref,appendix);
			  s = st.reconstruct(appendix.predecessor.state_ref);
			}
		      delete_state(s);
		      while(!gstack2.empty())
			{
			  stack_member_t sm2;
			  sm2 = gstack2.top();
			  gstack2.pop();
			  gstack.push(sm2);
			}
		    }
		}

	      // now we move the complete cycle in the path_t structure (ce)

	      while (!gstack.empty())
		{	    
		  st_member = gstack.top();
		  gstack.pop();
		  if (st_member.backtracking)
		    {
		      if(!skip_first_push_to_ce)
			{
  			  ce.push_front(st_member.state);
			  //			  ce.push_back(st_member.state);
//  			  cout <<"cycl f ";      
//  			  sys.DBG_print_state_CR(st_member.state,cout,0);
//  			  ce.print_trans_ids(cout);
			}
		      skip_first_push_to_ce = false;
		      
		      /*		      if (show_ce)
			{
			  sys.DBG_print_state(st_member.state,show_ce_out1,0);
			  show_ce_out1 <<" cycle"<<endl;
			  }*/
		    }
		}
	    } 	  
	}
      
      // }}}
      
      // we suppose that the cycle is already in "ce" here
      // so it remains to produce the path to the cycle
      // we do it by following "appendix.predecessor"
      
      if (tb_cycle_found)
  	{
  	  ce_path_start1 = ce_path_start;
  	  ce_path_start = ce_path_start_predecessor;	 
	}      

      if (cycle_found || tb_cycle_found)
	{
	  finished = false;
	  if (nid == 0)
	    {
	      ce.mark_cycle_start_front();
	      /*	      if (show_ce)
		{
		  show_ce_out <<"=============="<<endl;
		  }				      */

	      while (ce_path_start.is_valid())
		{
		  message.rewind();
		  message.append_data(reinterpret_cast<byte_t *>(&ce_path_start),
				      sizeof(global_state_ref_t));
  		  distributed.network.send_message(message,
					      partition_function(ce_path_start),
					      TAG_CE_REQ_STATE);
		  distributed.network.flush_buffer(partition_function(ce_path_start));
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
      //        cout <<ids<<"bar2"<<endl;

      if (nid== 0 && (tb_cycle_found || cycle_found))
	{
	  if (!quietmode)
	    {
	      cout <<"done"<<endl;
	    }
	  /*	  if (show_ce)
	    {
	      cout <<show_ce_out1.str();
	      cout <<show_ce_out.str();
	      }*/
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
	      ce.write_trans(ce_out);
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

      // }}}

      // {{{ reporting and finalizing 

      if (show_deps)
	{
	  cout <<ids<<"DepS on workstation "<<nid<<" (before removal phase)"
	       <<endl;
	  cout <<deps_out.str();
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

	  string problem = ((reach_only)?("SSGen"):("LTL MC"));
	  reporter.set_obligatory_keys(oss1.str(), argv[optind], problem, st.get_states_stored(), fsuccscalls);
	  if  ((!reach_only) && (nid == 0))
	    {
	      if (tb_cycle_found || cycle_found)
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

	  reporter.set_info("States",st.get_states_stored()-states_overhead);
	  reporter.set_info("Trans",trans);
	  reporter.set_info("CrossTrans",transcross);
	  reporter.set_info("States_overhead",states_overhead);
	  reporter.set_info("Init time",init_time);

	  for (int xx=0; xx<sys.get_property_scc_count(); xx++)
	    {
	      ostringstream xxx;
	      xxx <<xx;
	      reporter.set_info("Leaves["+xxx.str()+"]",leaves[xx].size());
  	      reporter.set_info("Roots["+xxx.str()+"]",roots[xx].size());
  	      reporter.set_info("DepS["+xxx.str()+"]size",depssize[xx]);
  	      reporter.set_info("DepS["+xxx.str()+"]sizemax",maxdepssize[xx]);
  	      reporter.set_info("DepS["+xxx.str()+"]edge",depsedge[xx]);
  	      reporter.set_info("DepS["+xxx.str()+"]edgemax",maxdepsedge[xx]);
	    }
	  reporter.stop_timer();
	  reporter.collect_and_print(REPORTER_OUTPUT_LONG,report_out);
	  report_out.close();
	}      

      if (statistics)
	{
	  while (!distributed.synchronized(info))
	    {
	      distributed.process_messages();
	    }
	  if (nid == 0)
	    cout <<ids<<"======================"<<endl;
	}
      if (statistics && nid==0)
	{
          cout <<ids<<"Memory:            "<<info.data.allmem/1024.0
	       <<" MB"<<endl;
	  cout <<ids<<"Memory phases:     "
	       <<info.data.maxbmem/1024.0<<", "
	       <<info.data.maxrmem/1024.0<<", "
	       <<info.data.maxtmem/1024.0
	       <<endl;
	  double timenow = timer.gettime();
          cout <<ids<<"Runtime            "<<timenow<<" sec"<<endl;
	  cout <<ids<<"Runtime phases:    "
	       <<timeafterdepsbuilt<<", "
	       <<timeafterdepsrem-timeafterdepsbuilt<<", "
	       <<timeaftertbndfs-timeafterdepsrem
	       <<" sec"<<endl;
  	  cout <<ids<<"State size:        "<<state.size<<endl;
	  cout <<ids<<"Appendix size:     "<<sizeof(appendix)<<endl;
          cout <<ids<<"States generated:  "<<info.data.allstates
	       <<" (max per workstation: "<<info.data.maxstates<<")"
	       <<endl;	  
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
          cout <<ids<<"local memory:      "<<vm.getvmsize()/1024.0
	       <<" (before removing phase: "<<bmem/1024.0<<")"<<endl;
        }


      distributed.finalize();

      // }}}

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


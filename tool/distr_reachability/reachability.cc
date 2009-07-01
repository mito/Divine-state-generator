#include <iostream>
#include <fstream>
#include <queue>
#include <getopt.h>
#include <ext/algorithm>
#include "sevine.h"
#include "divine.h"
#include <cmath>

using namespace std;
using namespace divine;

const int TAG_SEND_STATE = DIVINE_TAG_USER;
const int TAG_SEND_UNREACHED = DIVINE_TAG_USER + 1;
const int TAG_SEND_CE_STATE = DIVINE_TAG_USER + 2;
const int TAG_ADD_TO_CE = DIVINE_TAG_USER + 3;
const int TAG_DEADLOCK_FOUND = DIVINE_TAG_USER + 4;
const int TAG_ASSERTION_VIOLATED = DIVINE_TAG_USER + 5;
const int TAG_ASSERTION_STRING = DIVINE_TAG_USER + 6;
const int TAG_ERROR_STATE = DIVINE_TAG_USER + 7;
//const int TAG_SEND_FINISHED = DIVINE_TAG_USER + 8;

struct global_ref_t
{
  state_ref_t state_ref;
  size_int_t network_id;
};

struct appendix_t
{
  global_ref_t predecessor;
  size_int_t depth;
};

static message_t message;
static message_t recv_message;
static size_t max_queue_size = 0;

const char *fbasename = "distr_reachability";
string input_name="";

bool find_unreachable_code = true;
bool find_deadlock = true;
bool test_assertions = true;
bool trail = false;
bool ce_states = false;
bool base_name = false;
bool htsize_set = false;
bit_string_t reachable_trans;
explicit_storage_t visited; 

ostringstream oss1;

vminfo_t vm;
timeinfo_t timer;
logger_t logger;



bool help = false;
bool quiet = false;
bool compiled_generator = false;
bool do_log = false;
bool report = false;
bool version = false;
size_t htsize=0;

#if defined(ORIG_POLL)
int freqency=50;
#else
/* process_messages now limits poll rate itself */
int freqency=3;
#endif
int buffer_messages = 100;
int buffer_size = 8192;



void process_message_bad_state_path( char *buf, int size, int src, int tag, bool urgent );

struct search_t {
  queue<state_ref_t> S;
  explicit_system_t *system;
  distributed_t *distributed;
  int results;
  int trans;
  int cross_trans;
  int states;
  int total_edges, total_states;
  succ_container_t succ_cont;
  appendix_t appendix;
  size_int_t ce_length;
  state_t *ce_path;
  bool deadlock_found;
  bool assertion_violated;
  string violated_assertion_string;
  state_t error_state;
  state_ref_t bad_state_ref;
  
  void start() {
    if ( find_unreachable_code )
      {
	if ( system->can_transitions() )
	  {
	    reachable_trans.alloc_mem(system->get_trans_count());
	    reachable_trans.clear();
	  }
	else
	  {
	    find_unreachable_code=false;
	    if ( distributed->is_manager() )
	      {
		cout <<"WARNING: Unable to find unreachable code since the system"<<endl;
		cout <<"         interface cannot work with transitions. Turning the" << endl;
		cout <<"         unreachable code detection mode off."<<endl<<endl;
	      }
	  }
      }
    
    if ( find_deadlock && !system->can_system_transitions() )
      {
	if (distributed->is_manager())
	  {
	    cout <<"WARNING: Cannot perform deadlock detection for promela"<<endl;
	    cout <<"         bytecodes. Turning the deadlock detection mode off." << endl<<endl;
	  }
	find_deadlock = false;
      }


    if ( test_assertions && !system->can_transitions() )
      {
	if (distributed->is_manager())
	  {
	    cout <<"WARNING: Test for assertion violation is not fully supported "<<endl;
	    cout <<"         for promela bytecodes. If performed and an assetion is"<<endl;
	    cout <<"         violated, NIPS RUNTIME ERROR occurs." <<endl<<endl;
	  }
      }
    
    state_t initial = system->get_initial_state();
    state_ref_t initial_ref;
    
    if ( distributed->partition_function( initial ) == distributed->network_id ) {
      visited.insert( initial, initial_ref);
      distributed->set_busy();
      S.push( initial_ref );

      if ( find_deadlock || test_assertions )
	{
	  appendix.predecessor.network_id = -1;
	  appendix.predecessor.state_ref.invalidate();
	  appendix.depth=0;
	  visited.set_app_by_ref( initial_ref, appendix );
	}
      max_queue_size = 1;
    }
    deadlock_found=false;
    assertion_violated=false;
    violated_assertion_string="";    
    bad_state_ref.hres=MAX_ULONG_INT;
    
    while ( !distributed->synchronized() ) {
      distributed->process_messages();
      if ( S.empty() )
	{
	  distributed->set_idle();
	}
      else
	{
	  reach();
	}
    }
    
    states = visited.get_states_stored();
    if (find_unreachable_code)
      {
	size_int_t result=0;
	for (size_int_t i=0; i<reachable_trans.get_bit_count(); i++)
	  result=2*result+(reachable_trans.get_bit(i)?1:0);
	message.rewind();
	message.append_size_int(result);
	distributed->network.send_message(message, NETWORK_ID_MANAGER, TAG_SEND_UNREACHED);
	distributed->network.flush_buffer(NETWORK_ID_MANAGER);
	while ( !distributed->synchronized() )
	  distributed->process_messages();
	if (distributed->is_manager())
	  {
	    cout << "---------------------------------------------"<<endl;
	    cout << "Unreachable code: " << endl;
	    bool unreachable_exists=false;
	    for (size_int_t i=0; i<reachable_trans.get_bit_count(); i++)
	      if ((reachable_trans.get_bit(i)==false)&&
		  ((!system->get_with_property())||(dynamic_cast<dve_transition_t*>(system->get_transition(i))->get_process_gid()!=system->get_property_gid()))) 
		{ //an unreachable transition of non-property process detected
		  unreachable_exists=true;
		  dve_transition_t * trans = dynamic_cast<dve_transition_t*>(system->get_transition(i));
		  dve_explicit_system_t * dve_sys = dynamic_cast<dve_explicit_system_t*>(system);
		  cout << '\t';
		  cout << dve_sys->get_symbol_table()->get_process(trans->get_process_gid())->get_name();
		  cout << '.' << trans->get_lid() << ": ";
		  trans->write(cout);
		  cout << endl;
		}
	    if (unreachable_exists == false) {	      
	      cout << "\tNone." <<endl;
	    }
	    cout << "---------------------------------------------"<<endl;

	  }
      }

    if ( deadlock_found || assertion_violated )
      {
	if ( bad_state_ref.hres!=MAX_ULONG_INT )
	  process_ce_state( bad_state_ref );
	while ( !distributed->synchronized() )
	  {
	    distributed->process_messages();
	    /*	    distributed->network.get_all_sent_msgs_cnt(sent);
	    distributed->network.get_all_received_msgs_cnt(recv);
	    distributed->network.get_user_sent_msgs_cnt(sent2);
	    distributed->network.get_user_received_msgs_cnt(recv2);*/
	    //	    cout << distributed->network_id << ' ' << recv << ' ' << sent << ' ' << recv2 << ' ' << sent2 << endl;
	  }
      }

    if (( find_deadlock || test_assertions ) && ( distributed->network_id == NETWORK_ID_MANAGER ))
      {
	if ( deadlock_found )
	  {
	    cout << "Deadlock found." << endl;
	    path_t ce(system);
	    ofstream ce_out;
	    for (size_int_t i=0; i<ce_length; i++)
	      ce.push_back(ce_path[i]);
	    string pom_fn, pom_fn_2;
	    if ( base_name )
	      pom_fn = fbasename;
	    else
	      pom_fn = input_name;
	    if (trail)
	      {
		pom_fn_2 = pom_fn+".trail";
		ce_out.open(pom_fn_2.c_str());
		if (system->can_system_transitions())
		  ce.write_trans(ce_out);
		else
		  ce.write_states(ce_out);		
		ce_out.close();
	      }
	    if (ce_states)
	      {
		pom_fn_2 = pom_fn+".ce_states";
		ce_out.open(pom_fn_2.c_str());
		ce.write_states(ce_out);
		ce_out.close();
	      }
	    
	    for (size_int_t i=0; i<ce_length; i++)
	      delete_state(ce_path[i]);
	    delete[] ce_path;
	  }
	else
	  if ( find_deadlock )
	    cout << "No deadlock." << endl;       
	if ( assertion_violated )
	  {
	    if (violated_assertion_string != "") //real assertion violated, not erroneous state detected
	      cout << "Assertion violated:" << violated_assertion_string << endl;
	    else //an erroneous state detected
	      {
		cout << "Erroneous state reached:" << endl;
		system->print_state(error_state, cout);
	      }
	    path_t ce(system);
	    ofstream ce_out;
	    for (size_int_t i=0; i<ce_length; i++)
	      ce.push_back(ce_path[i]);
	    string pom_fn, pom_fn_2;
	    if ( base_name )
	      pom_fn = fbasename;
	    else
	      pom_fn = input_name;
	    if (trail)
	      {
		pom_fn_2 = pom_fn+".trail";
		ce_out.open(pom_fn_2.c_str());
		ce.write_trans(ce_out);
		ce_out.close();
	      }
	    if (ce_states)
	      {
		pom_fn_2 = pom_fn+".ce_states";
		ce_out.open(pom_fn_2.c_str());
		ce.write_states(ce_out);
		ce_out.close();
	      }
	    
	    for (size_int_t i=0; i<ce_length; i++)
	      delete_state(ce_path[i]);
	    delete[] ce_path;
	  }
	else
	  if ( test_assertions )
	  cout << "No assertion violated." << endl;
      }
  }

  void reach() {
        
    if (freqency==0) 
      {
	freqency=1;
      }
    int freq=freqency;
    state_t x;
    state_ref_t i_ref, x_ref;    
    enabled_trans_container_t *enb_trans_cont = 0;
    int succs_result;
    
    if (system->can_transitions())
      {
	enb_trans_cont = new enabled_trans_container_t(*system);
      }
    
    while ( !S.empty() ) {
      x_ref = S.front();
      x = visited.reconstruct( x_ref );
      S.pop();
      
      if (find_unreachable_code)
	{
	  succs_result = system->get_succs(x, succ_cont, *enb_trans_cont);
	  for (size_int_t i=0; i<enb_trans_cont->size(); i++)
	    {
	      for (size_int_t j=0; j<(*enb_trans_cont)[i].get_count(); j++)
		reachable_trans.enable_bit((*enb_trans_cont)[i][j]->get_gid());
	    }
	}
      else
	{
	  succs_result = system->get_succs( x, succ_cont );
	}
    
      if ( ((find_deadlock) && (succs_deadlock(succs_result))) || ((test_assertions) && ((system->violates_assertion(x)) || (succs_error(succs_result)))) ) //a deadlock detected or an assertion violated, report a trail to it
	{
	  while ( !S.empty() )
	    S.pop();
	  visited.get_app_by_ref( x_ref, appendix );
	  message.rewind();
	  message.append_size_int(appendix.depth+1);
	  int tmp_tag=((find_deadlock) && (succs_deadlock(succs_result)))?TAG_DEADLOCK_FOUND:TAG_ASSERTION_VIOLATED;
	  for (size_int_t i=0; i<(size_int_t)distributed->cluster_size; i++)
	    distributed->network.send_message(message, i, tmp_tag );
	  bad_state_ref = x_ref;
	  if ((test_assertions) && (system->violates_assertion(x)))
	    {
	      string s;
	      size_int_t nr=system->violated_assertion_count(x);
	      for (size_int_t i=0; i<nr; i++)
		{
		  s=system->violated_assertion_string(x, i);
		  distributed->network.send_message((char*)s.c_str(), s.length(), NETWORK_ID_MANAGER, TAG_ASSERTION_STRING);
		}
	    }
	  if ((test_assertions) && (succs_error(succs_result)))
	    {
	      message.rewind();
	      message.append_state(x);
	      distributed->network.send_message(message, NETWORK_ID_MANAGER, TAG_ERROR_STATE );
	    }
	  distributed->network.flush_all_buffers();
	}
      else
	{	  
	  if ( find_deadlock || test_assertions ) //to further possible reconstruction of trail to a bad state, store predecessor state and depth of current state
	    {
	      visited.get_app_by_ref( x_ref, appendix );
	      appendix.depth++;
	      appendix.predecessor.state_ref=x_ref;
	      appendix.predecessor.network_id=distributed->network_id;
	    }
	  succ_container_t::iterator i;
	  for ( i = succ_cont.begin(); i != succ_cont.end(); ++i ) {
	    int dest = distributed->partition_function( *i );
	    if ( dest == distributed->network_id ) {
	      if ( !visited.is_stored( *i ) ) {
		visited.insert( *i, i_ref );
		if ( find_deadlock || test_assertions )
		  visited.set_app_by_ref( i_ref, appendix );
		S.push( i_ref );
		if (S.size()>max_queue_size)
		  max_queue_size=S.size();
	      }
	    }
	    else {
	      cross_trans++;
	      message.rewind();
	      message.append_state( *i );
	      if ( find_deadlock || test_assertions )
		{
		  message.append_state_ref( appendix.predecessor.state_ref );
		  message.append_size_int( appendix.depth );
		}
	      distributed->network.send_message( message, dest, TAG_SEND_STATE );
	    }
	    delete_state( *i );
	  }
	}
    
      trans += succ_cont.size();
      delete_state( x );
      
      if (freq==0 || S.empty())
	{
#if !defined(ORIG_POLL)
	  if (S.empty()) {
	      distributed->force_poll();
	  }
#endif
	  distributed->process_messages();
	  freq=freqency;
	}
      freq --;
    }
#if defined(ORIG_FLUSH)
    distributed->network.flush_all_buffers();
#else
    distributed->network.flush_some_buffers();
#endif
  }
        
  search_t( explicit_system_t *sys, distributed_t *distr, size_t htsize )
    : system( sys ), distributed( distr ), results( 1 ), trans( 0 ), cross_trans( 0 ), states( 0 ), succ_cont( *sys )
  {
    if (htsize_set)
      visited.set_ht_size( htsize );
    if ( find_deadlock || test_assertions )
      {
	visited.set_appendix(appendix);
      }    
    visited.init();
  }

  void process_ce_state ( state_ref_t ref ) //processing a state on a trail to a bad state (when the trail is recontructed)
  {    
    visited.get_app_by_ref( ref, appendix );
    if ( appendix.depth>0 )
      {
	message.rewind();
	message.append_state_ref( appendix.predecessor.state_ref );
	distributed->network.send_message( message, appendix.predecessor.network_id, TAG_SEND_CE_STATE );
      }
    state_t state = visited.reconstruct( ref );    
    message.rewind();
    message.append_size_int( appendix.depth );
    message.append_state( state );
    distributed->network.send_message( message, NETWORK_ID_MANAGER, TAG_ADD_TO_CE );
    distributed->network.flush_all_buffers();
  }

};

search_t *_search;
distributed_t distributed;

unsigned current_states( void ) 
{
  return visited.get_states_stored();
}

unsigned current_trans( void ) 
{
  return _search->trans;
}

unsigned current_cross_trans( void ) 
{
  return _search->cross_trans;
}

struct info_t {
  size_t allstored;
  size_t allstates;
  size_t alltrans;
  size_t allcross;
  long int allmem;
  virtual void update();
  virtual ~info_t() {}
};

void info_t::update()
{
  if (distributed.network_id == 0)
    {
      allstates = allstored = current_states();
      alltrans = current_trans();
      allcross = current_cross_trans();
      allmem = vm.getvmsize();
    }
  else
    {
      allstored += current_states();
      allstates += current_states();
      alltrans += current_trans();
      allcross += current_cross_trans();
      allmem += vm.getvmsize();
    }
}

updateable_info_t<info_t> info;


static bool print_stats = false;

void read_options(int argc, char** argv) {
  int c;
  
  static struct option longopts[] = {
    { "printstats",    no_argument, 0, 'S'},
    { "htsize", required_argument, 0, 'H' },
    { "basename", required_argument, 0, 'X' },
    { "bufslimit", required_argument, 0, 'M' },
    { "bufmlimit", required_argument, 0, 'N' },
    { "freqency", required_argument, 0, 'F' },
    { "report", no_argument,0, 'r' },
    { "version", no_argument,0, 'V' },
    { "unreachable", no_argument,0, 'u' },
    { "deadlock", no_argument,0, 'd' },
    { "precompile", no_argument, 0, 'p'},
    { "assertion", no_argument,0, 'a' },
    { "safety", no_argument,0, 's' },
    { "help", no_argument, 0 , 'h' },
    { "quiet", no_argument, 0 , 'q' },
    { "log", no_argument,0, 'L' },
    { "trail",      no_argument, 0, 't'},
    { "statelist",  no_argument, 0, 'c'},
    { NULL, 0, NULL, 0 }
  };

  oss1<<"distr_reachability";
  while ((c = getopt_long(argc, argv, "tcparSVhdquM:N:F:H:X:L", longopts, NULL)) != -1)
    {
      oss1 <<" -"<<(char)c;
      switch (c) {
      case 'h': help = true; break;
      case 'q': quiet = true; break;
      case 'r': report = true; break;
      case 't': trail = true; break;
      case 'p': compiled_generator = true; break;
      case 'c': ce_states = true; break;
      case 'd': find_deadlock = false; break;
      case 'a': test_assertions = false; break;
	//      case 's': find_deadlock = true; test_assertions = true; break;
      case 'u': find_unreachable_code = false; break;
      case 'X': base_name = true; fbasename = optarg; break;
      case 'M': buffer_messages = atoi(optarg); break;
      case 'N': buffer_size = atoi(optarg); break;
      case 'F': freqency = atoi(optarg); break;
      case 'H': htsize_set = true; 
	        c = atoi(optarg);
		htsize = htsize < 33 ? size_t(pow( 2.0, c )) : htsize;  
		break;
      case 'S': print_stats = true; break;
      case 'L': do_log = true; break;
      case 'V': version = true; break;
      default:
	cerr << "Unknown option: "<<c<<endl;
	break;
      }
    }

  if (optind < argc) input_name = argv[optind]; 
  
}

void print_version(){
  cout << "Reachability version 1.0 build 9 (2006/09/20 17:00)" << endl;
}

void print_usage() {
  cout << "-----------------------------------------------------------------"<<endl;
  cout << "DiVinE Tool Set"<<endl;
  cout << "-----------------------------------------------------------------"<<endl;
  print_version();
  cout << "-----------------------------------------------------------------"<<endl;
  cout << "Usage: [mpirun -np n] divine.distr_reachability [options] input_file"<<endl;
  cout <<"Options:"<<endl;
  cout << "-S, --printstats\tprint some statistics"<<endl;
  cout << "-h, --help \t\thelp"<<endl;
  cout << "-L, --log\t\tproduce logfiles (log period is 1 second)"<<endl;
  cout << "-q, --quiet\t\tquiet (suppress standard output)"<<endl;
  cout << "-u, --unreachable\tturn off unreachable code detection"<<endl;
  cout << "-d, --deadlock\t\tturn off deadlock detection"<<endl;
  cout <<" -p, --precompile \tDVE file is precompiled prior verification"<<endl;
  cout << "-a, --assertion\t\tturn off assertions checking"<<endl;
  cout << "-t, --trail \t\tproduce trail file"<<endl;
  cout << "-c, --statelist \tshow counterexample states"<<endl;  
  //  cout << "-s, --safety\t\tsafety checking (assertions and deadlocks)"<<endl;
  cout << "-H x, --htsize=x\tset the size of the hash table to ( x < 33 ? 2^x : x )"<<endl;
  cout << "-M x, --bufslimit=x \tlimit to the number of messages in the network buffer"<<endl;
  cout << "-N x, --bufmlimit=x \tlimit to the size of messages (in chars) in the network buffer"<<endl;
  cout << "-F x, --freqency=x \tnumber of states to be processed before incoming messages are checked"<<endl;
  cout << "-X w \t\t\tsets base name of produced files to w (w.trail, w.report, w.00-w.N)"<<endl;
  cout << "-Y\t\t\treserved for GUI"<<endl;
  cout << "-Z\t\t\treserved for GUI"<<endl;
}


void process_message( char *buf, int size, int src, int tag, bool urgent ) //serves messages sent during state space traversal and to the detection of unreached code
{
    state_t state;
    state_ref_t state_ref;
    appendix_t app;

    if ( _search->deadlock_found==false && _search->assertion_violated==false )
      switch (tag)
	{
	case TAG_SEND_STATE:
	  {
	    recv_message.load_data( (byte_t*)(buf), size );
	    recv_message.read_state( state );
	    if ( ! visited.is_stored( state ) ) 
	      {
		visited.insert( state, state_ref );
		if ( find_deadlock || test_assertions )
		  {
		    app.predecessor.network_id=src;
		    recv_message.read_state_ref( app.predecessor.state_ref );
		    recv_message.read_size_int( app.depth );
		    visited.set_app_by_ref( state_ref, app );
		  }
		_search->S.push( state_ref );
		distributed.set_busy();
		if (_search->S.size()>max_queue_size)
		  max_queue_size=_search->S.size();
	      }
	    delete_state( state );
	    break;
	  }
	case TAG_DEADLOCK_FOUND:
	  {
	    distributed.process_user_message = process_message_bad_state_path;
	    if ( distributed.network_id==NETWORK_ID_MANAGER )
	      {
		recv_message.load_data( (byte_t*)(buf), size );
		recv_message.read_size_int( _search->ce_length );
		_search->ce_path=new state_t[_search->ce_length];
	      }
	    while ( _search->S.size()>0 )
	      _search->S.pop();
	    distributed.set_idle();
	    _search->deadlock_found=true;
	    break;
	  }	
	case TAG_ASSERTION_VIOLATED:
	  {
	    distributed.process_user_message = process_message_bad_state_path;
	    if ( distributed.network_id==NETWORK_ID_MANAGER )
	      {
		recv_message.load_data( (byte_t*)(buf), size );
		recv_message.read_size_int( _search->ce_length );
		_search->ce_path=new state_t[_search->ce_length];
	      }
	    while ( _search->S.size()>0 )
	      _search->S.pop();
	    distributed.set_idle();
	    _search->assertion_violated=true;
	    break;
	  }
	case TAG_SEND_UNREACHED:
	  {
	    size_int_t tmp;
	    recv_message.load_data((byte_t*)(buf),size);
	    recv_message.read_size_int(tmp);
	    for (size_int_t i=reachable_trans.get_bit_count(); i>0; i--)
	      {
		if (tmp%2==1)
		  reachable_trans.enable_bit(i-1);
		tmp/=2;
	      }
	    break;
	  }
	default: gerr << "Unknown message tag" << thr();
	}
}

void process_message_bad_state_path( char *buf, int size, int src, int tag, bool urgent ) //serves messages during trail to bad_state reconstruction
{
  switch (tag)
    {
    case TAG_SEND_CE_STATE:
      {
	state_ref_t state_ref;
	recv_message.load_data( (byte_t*)(buf), size );
	recv_message.read_state_ref( state_ref );
	_search->process_ce_state( state_ref );
	break;
      }
    case TAG_ADD_TO_CE:
      {
	size_int_t depth;
	recv_message.load_data( (byte_t*)(buf), size );
	recv_message.read_size_int( depth );
#ifdef ORIG
	recv_message.read_state( _search->ce_path[depth] );
#else
	if (depth < _search->ce_length)
	  {
	    recv_message.read_state( _search->ce_path[depth] );
	  }
        else
	  {
	    cout << "BUG: ignoring CE for depth " << depth
		 << " exceeding alloced length " << _search->ce_length << endl;
	  }
#endif
	break;
      }
    case TAG_ASSERTION_STRING:
      {
	_search->violated_assertion_string=_search->violated_assertion_string+"\n\t"+(string)buf;
	break;
      }
    case TAG_ERROR_STATE:
      {
	recv_message.load_data( (byte_t*)(buf), size );
	recv_message.read_state( _search->error_state );
	break;
      }
      //      default: gerr << "Unknown message tag" << thr(); -- CAN'T BE HERE, since messages with TAG_SEND_STATE may be processed (and must be ignored)
    }
}

int main(int argc, char** argv) {
    MCRL2_ATERM_INIT(argc,argv);
  
    explicit_system_t *system = 0;

    distributed.process_user_message = process_message;
    distributed.network.set_buf_time_limit(0,0);
    distributed.set_proc_msgs_buf_exclusive_mem(false);
    distributed.network.set_buf_size_limit(buffer_size);
    distributed.network.set_buf_msgs_cnt_limit(buffer_messages);
    distributed.network_initialize(argc, argv);
    //distributed.buffers_initialize();

    distributed.initialize();

    read_options(argc, argv);

    if ( version && !help )
      {
	if (distributed.is_manager())
	  print_version();
	return 0;
      }

    if (input_name == "" || help) {
      if (distributed.is_manager())
        print_usage();
      return 0;
    }

    try {
	const char * input_file_ext; 
        system_description_t system_desc;
#ifdef HAVE_MCRL2
      	if (filename_length>=4 && strcmp(filename+filename_length-4,".lps")==0) {
            if (distributed.is_manager() && !quiet)
	      {
		cout << "Reading mCRL2 source..." << endl;
	      }
            system = new mcrl2_explicit_system_t(gerr);
	    input_file_ext = ".lps";
        }
	else
	{
#endif
	system = system_desc.open_system_from_file(argv[optind],
						   compiled_generator,
						   (distributed.network_id==NETWORK_ID_MANAGER)&&
						   print_stats
						   );
	input_file_ext= system_desc.input_file_ext.c_str();

#ifdef HAVE_MCRL2
	}
#endif
	int file_opening;
	try
	  {
	    if ((file_opening=system->read(input_name.c_str()))&&(distributed.network_id==NETWORK_ID_MANAGER))
             {
	      if (file_opening==system_t::ERR_FILE_NOT_OPEN)
		gerr << distributed.network_id << ": " << "Cannot open file ...";
	      else gerr << distributed.network_id << ": " << "Syntax error ...";
             }
	    if (file_opening)
	      gerr << thr();
	  }
	catch (ERR_throw_t & err)
	  { 
	    distributed.finalize();
	    exit(err.id);
	  }

	int position = input_name.find(input_file_ext,0);
	input_name.erase(position,4);

        distr_reporter_t reporter( &distributed );
	if (report) {	  	    
	  reporter.set_info("InitTime",timer.gettime());
	  reporter.start_timer();
	}
	

        search_t search( system, &distributed, htsize );
        _search = &search;

        if (do_log) {
	  logger.set_storage(&visited);
	  logger.register_unsigned( current_states, "states" );
	  logger.register_unsigned( current_trans, "trans" );
	  logger.init( &distributed, (base_name?fbasename:input_name) );
	  logger.use_SIGALRM(1);
	}
        search.start();

	if (do_log) {
	  logger.stop_SIGALRM();
	  logger.log_now();      // in order to log final values
	}
	
	if ((report) || ((!quiet && print_stats)))
	  {
	    reporter.set_alg_name("distr_reachability");
	    reporter.set_file_name(input_name);
	    reporter.set_problem("reachability");
	    string problem = ( find_deadlock || test_assertions )?"Safety":"SSGen";
	    reporter.set_obligatory_keys(oss1.str(), argv[argc-1], problem, current_states(), current_states());

	    reporter.stop_timer();
	    reporter.set_info( "States", current_states() , REPORTER_SUM );
	    reporter.set_info( "Trans", current_trans() , REPORTER_SUM );
	    reporter.set_info( "CrossTrans", current_cross_trans() , REPORTER_SUM );
	    reporter.set_info( "Maximal length of BFS queue(s)", max_queue_size, REPORTER_SUM );
	    // reporter.set_info( "State size", system.get_space_sum(), REPORTER_SUM );
	    reporter.set_info( "Speed", (int) current_states()/reporter.get_time(), REPORTER_AVG );

	    if (distributed.is_manager())
	      {
		reporter.set_global_info( "Buffer size", buffer_size);
		reporter.set_global_info( "Buffer msg limit", buffer_messages);	    
		int s1,s2;
		distributed.network.get_buf_time_limit(s1,s2);
		reporter.set_global_info( "Buffer time limit", s1*1000+s2);
		reporter.set_global_info( "Freqency of polling", freqency );
	      }
	  }
	if (report)
	  {
	    if (distributed.is_manager())
	      {//only manager writes down the report
		string pom_fn;
		if (base_name)
		  {
		    pom_fn = fbasename;
		  }
		else
		  {
		    pom_fn = input_name;
		  }
		fstream repf;
		repf.open(string(pom_fn+".report").c_str(), fstream::out );

		if ( find_deadlock || test_assertions )
		  {
		    if ( _search->deadlock_found || _search->assertion_violated )
		      {
			reporter.set_global_info("SafetyViolated","Yes");
			if (trail || ce_states)
			  reporter.set_global_info("CEGenerated", "Yes");
			else
			  reporter.set_global_info("CEGenerated", "No");
			if ( _search->assertion_violated )
			  reporter.set_global_info("Violated assertion",(_search->violated_assertion_string != "")?_search->violated_assertion_string:"Erroneous state reached.");
		      }
		    else
		      {
			reporter.set_global_info("SafetyViolated","No");
			reporter.set_global_info("CEGenerated", "No");
		      }
		  }
		
		reporter.collect_and_print( REPORTER_OUTPUT_LONG, repf );
		repf.close();
	      }
	    else //the others only send messages (std::cerr is not used actually)
	      reporter.collect_and_print( REPORTER_OUTPUT_LONG, std::cerr );
	  }

	while(!distributed.synchronized(info))
	  {
	    distributed.process_messages();
	  }

	if (!quiet && print_stats && distributed.is_manager())
	  {
	    state_t s = system->get_initial_state();
	    cout << "States:\t\t\t" << info.data.allstates << endl;
	    cout << "transitions:\t\t" << info.data.alltrans << endl;
	    cout << "cross transitions:\t" << info.data.allcross << endl;
	    cout << "size of initial state:\t" << s.size << endl;
	    cout << "all memory:\t\t" << info.data.allmem/1024.0<<" MB" << endl;
	    cout << "time:\t\t\t" << timer.gettime() <<" s"<< endl;
	    cout << "---------------------------------------------"<<endl;
	  }

	while(!distributed.synchronized())
	  {
	    distributed.process_messages();
	  }


	if (!quiet && print_stats)
	  {
	    cout << distributed.network_id<<": local states:\t" << current_states() << endl;
	    cout << distributed.network_id<<": local memory:\t" << vm.getvmsize()/1024.0 << endl;
	  }

        distributed.finalize();

        delete system;
    } catch (ERR_throw_t & err_type) {
        return 1;
    } catch (...) {
        return 2;
    }  
  
    return 0;
}



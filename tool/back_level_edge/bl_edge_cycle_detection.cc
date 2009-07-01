#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <queue>
#include <stack>
#include <string.h>
#include <sys/times.h>
#include <sys/time.h>
#include <getopt.h>
#include "divine.h" 

using namespace std;
using namespace divine;

//bit_values has been yet declared in divine.h (see bit_string_t class)
/*static const unsigned long int bit_values[32] =
   {1, 2, 4, 8, 16, 32, 64, 128, 256, 1<<9, 1<<10, 1<<11, 1<<12, 1<<13, 1<<14,
    1<<15, 1<<16, 1<<17, 1<<18, 1<<19, 1<<20, 1<<21, 1<<22, 1<<23, 1<<24,
    1<<25, 1<<26, 1<<27, 1<<28, 1<<29, 1<<30, 1<<31};*/
           
// Parametr last_search_bl_source se do storage neuklada, tam se uklada pouze pointer.
// Jeho posledni uvolneni je ponechano na OS (jinak bych musel prochazet celou storage)
struct appendix_t {
  int level;
  state_t last_search_bl_source;
  state_t parent_bfs;
  state_t parent_bl_search;
  int last_search_bl_nid;
  int last_search_bl_level;
  int last_search_acc_bl_passed;
  bool last_search_acc_passed;
  bool ce_search_visited; // navstiveno pri hledani protiprikladu
  bool ce_gen_visited; // navstiveno pri generovani protiprikladu
  bit_string_t *ample_set; //pro p.o.r.
} appendix;

enum por_mode_t { pm_none, pm_first_ample, pm_smallest_ample };
enum por_expansion_completion_mode_t { pecm_enabled_minus_ample, pecm_all };
// pro testovaci ucely, vcet_dest_accepting neni korektni
enum VCL_check_edge_type_t { vcet_standard, vcet_dest_accepting };


const int TAG_SEND_STATE = DIVINE_TAG_USER;
const int TAG_SEND_BL_SEARCH = DIVINE_TAG_USER + 1;
const int TAG_CYCLE_FOUND = DIVINE_TAG_USER + 2;
const int TAG_CE_SEARCH_BL_SOURCE = DIVINE_TAG_USER + 3;
const int TAG_CE_GEN_INITIATE = DIVINE_TAG_USER + 4;
const int TAG_CE_GEN_SEND_STATE = DIVINE_TAG_USER + 5;
const int TAG_CE_GEN_INITIATE_BFS_PART = DIVINE_TAG_USER + 6;
const int TAG_POR_COMPLETE_STATE_EXPANSION = DIVINE_TAG_USER + 7;

const char* COUNTER_EXAMPLE_FILE_EXTENSION = ".ce_states";
const char* COUNTER_EXAMPLE_PATH_FILE_EXTENSION = ".trail";
const char* REPORT_FILE_EXTENSION = ".report";
const int PAGES_PER_MACHINE = 1; // uz neni nutne aby jich bylo vic, testovani skoncilo
const int INITIAL_CE_VECTOR_SIZE = 10;
const por_expansion_completion_mode_t POR_EXPANSION_COMPLETION_MODE = pecm_enabled_minus_ample;
const VCL_check_edge_type_t VCL_check_edge_type = vcet_standard;
const bool BL_SEARCH_PARENT_optimization = true; 
const bool REVERSE_ORDERING_OF_STATES = false;


const char * input_file_ext = 0;
por_mode_t POR_MODE = pm_none;
bool synchronized_reachability_only = false;
bool produce_ce_trail_file = false;
bool produce_ce_states_file = false;
bool produce_report = false;
bool produce_log = false;
bool basename_specified = false;
bool print_detailed_comm_info = false;
bool print_statistics = false;
bool compiled_generator = false;
bool VCL_optimization = true;

string file_basename="";

int pfunc=0;

int net_id, cluster_size;
bool cycle_found;
timeinfo_t init_timer;
vminfo_t vm;
double init_time,cycle_found_time;

enabled_trans_container_t **active_trans;

distributed_t distributed;
distr_reporter_t reporter(&distributed);

explicit_storage_t st;
explicit_system_t * p_sys;
process_decomposition_t *property_decomposition;
logger_t logger;

por_t por;

void interchange_queues(queue<state_ref_t> *&q1, queue<state_ref_t> *&q2)
{
  queue<state_ref_t> *aux=q1;
  q1 = q2;
  q2 = aux;
}

struct bl_search_t { 
  state_t bl_source;
  slong_int_t bl_source_nid;
  state_ref_t state_ref;  
  slong_int_t limit_level;
  state_t parent;
  slong_int_t parent_level;
  slong_int_t acc_bl_passed;
  bool acc_passed;
  bool ce_search; // je to hledani protiprikladu
} bl_search, tmp_bl_search;

struct bl_send_struct_t {
  state_t bl_source;
  slong_int_t bl_source_nid;
  state_t state;
  slong_int_t limit_level;
  state_t parent;
  slong_int_t parent_level;
  slong_int_t acc_bl_passed;
  bool acc_passed;
  bool ce_search;
} bl_send_struct;

struct send_struct_t {
  state_t state;
  state_t predecessor;
  bool full_expand;
} send_struct;

struct ce_gen_send_struct_t {
  state_t state;
  int state_level;
  int state_order_no;
  // stavy se budou posilat, jak pocitacum, ktere budou v generovani protiprikladu pokracovat,
  // tak managerovi, ktery je bude zatrizovat do vektoru.
  bool go_on;
  // stav je z casti protiprikladu, ktera byla vyskladana z bfs parentu
  bool bfs_parent_part;
  bool repeated_state;
} ce_gen_send_struct;

struct ce_member_t {
  state_t state;
  slong_int_t state_level;
  slong_int_t state_order_no;
  bool bfs_parent_part;
  bool go_on;
  bool repeated_state;
} ce_member, tmp_ce_member;


static message_t message;
static message_t received_message;

//MESSAGE//char *bl_send_buf;
//MESSAGE//int bl_send_buf_size;
//MESSAGE//int bl_send_buf_size_without_parent;
//MESSAGE//char *send_buf;
//MESSAGE//int send_buf_size;
//MESSAGE//char *ce_gen_send_buf;
//MESSAGE//int ce_gen_send_buf_size;
bool ce_search_started = false;

void send_bl_search(bl_send_struct_t &blss, int dest)
{
  
  message.rewind();
  message.append_state(blss.state);
  message.append_state(blss.bl_source);
  if (!BL_SEARCH_PARENT_optimization || ce_search_started)
      // parent se posila pouze v pripade ce_search
      message.append_state(blss.parent);
  message.append_slong_int(blss.bl_source_nid);
  message.append_slong_int(blss.limit_level);
  message.append_slong_int(blss.parent_level);
  message.append_slong_int(blss.acc_bl_passed);
  message.append_bool(blss.acc_passed);
  message.append_bool(blss.ce_search);

  distributed.network.send_message(message,
			      dest,
			      TAG_SEND_BL_SEARCH);
}

void send_state(send_struct_t &ss, int dest)
{

  message.rewind();
  message.append_state(ss.state);
  
  message.append_state(ss.predecessor);
  message.append_bool(ss.full_expand);

  distributed.network.send_message(message,
			      dest,
			      TAG_SEND_STATE);
}

void send_ce_gen_state(ce_gen_send_struct_t& cgss, int dest)
{
  message.rewind();
  message.append_state(cgss.state);
  message.append_slong_int(cgss.state_level);
  message.append_slong_int(cgss.state_order_no);
  message.append_bool(cgss.bfs_parent_part);
  message.append_bool(cgss.go_on);
  message.append_bool(cgss.repeated_state);

  distributed.network.send_urgent_message(message,
				     dest,
				     TAG_CE_GEN_SEND_STATE);
}

queue<state_ref_t> *current_level_queue = new queue<state_ref_t>;
queue<state_ref_t> *next_level_queue = new queue<state_ref_t>;
stack<bl_search_t> bl_search_stack;
vector<ce_member_t> ce_vector(INITIAL_CE_VECTOR_SIZE);
queue<ce_member_t> ce_member_queue;


int bl_at_level = 0;
int relevant_bl_at_level = 0;
int states_count = 0;
int states_without_bl_edges = 0;
int all_states_count = 0;
int succs_count = 0;
int all_succs_count = 0;
int full_expands = 0;
int all_full_expands = 0;
int proviso_forced_expands = 0;
int all_proviso_forced_expands = 0;
int trans = 0;
int cross_trans = 0;
int bl_edges_cnt = 0; // pocita pocet bl hran na jednom pocitaci
int all_bl_edges_cnt = 0;
int all_relevant_bl_edges_cnt = 0;
long real_time_sec;
long real_time_usec;
clock_t processor_times;
clock_t ticks;
// znaci, ze byl prijat zdroj bl hran pro zahajeni hledani protiprikladu, na true se
// muze nastavit pouze u managera
bool ce_search_bl_source_received = false;
int ce_search_bl_source_sender = -1;
state_t ce_search_bl_source;
bool ce_gen_starting_state_valid = false;
state_t ce_gen_starting_state;
bool ce_gen_initiate_bfs_part = false;

struct info_t {
  int next_level_states;
  int bl_edges_cnt;
  int relevant_bl_edges_cnt;
  int all_states_count;
  void update(void);
};

struct time_info_t {
  long max_real_time_sec;
  long max_real_time_usec;
  clock_t max_processor_times;
  clock_t max_ticks;
  int all_succs_count;
  int all_full_expands;
  int all_proviso_forced_expands;
  int states_without_bl_edges_cnt;
  long int mem,maxmem;
  void update(void);
};

void info_t::update(void)
{
  if (net_id == NETWORK_ID_MANAGER)
    {
      next_level_states = next_level_queue->size();
      bl_edges_cnt = bl_at_level; 
      relevant_bl_edges_cnt = relevant_bl_at_level;
      all_states_count = states_count;
    }
  else
    {
      next_level_states += next_level_queue->size();
      bl_edges_cnt += bl_at_level;
      relevant_bl_edges_cnt += relevant_bl_at_level;
      all_states_count += states_count;
    }
}

void time_info_t::update(void)
{
  if (net_id == NETWORK_ID_MANAGER)
    {
      max_real_time_sec = real_time_sec;
      max_real_time_usec = real_time_usec;
      max_processor_times = processor_times;
      max_ticks = ticks;
      all_succs_count = succs_count;
      all_full_expands = full_expands;
      all_proviso_forced_expands = proviso_forced_expands;
      states_without_bl_edges_cnt = states_without_bl_edges;
      mem = vm.getvmsize();
      maxmem = mem;
    }
  else
    {
      if (real_time_sec > max_real_time_sec ||
	  ((real_time_sec == max_real_time_sec) &&
	   (real_time_usec > max_real_time_usec)))
	{
	  max_real_time_sec = real_time_sec;
	  max_real_time_usec = real_time_usec;
	}

      if (processor_times > max_processor_times)
	{
	  max_processor_times = processor_times;
	}

      if (ticks > max_ticks)
	{
	  max_ticks = ticks;
	}
      all_succs_count += succs_count;
      all_full_expands += full_expands;
      all_proviso_forced_expands += proviso_forced_expands;
      states_without_bl_edges_cnt += states_without_bl_edges;
      mem += vm.getvmsize();
      if (vm.getvmsize() >maxmem)
	{
	  maxmem = vm.getvmsize();
	}
    }
} 


int current_level;
updateable_info_t<info_t> info;
updateable_info_t<time_info_t> time_info;

// pokud VCL_check_edge_type = vcet_standard, tak vrati true pokud src i dest patri do stejne 
// komponenty a tato komponenta je typu P nebo F
bool VCL_check_edge(state_t& src, state_t& dest)
{

  if (src.ptr == NULL)
    {
      if (!BL_SEARCH_PARENT_optimization)
	{
	  gerr << net_id << ": "
	       << "VCL_check_edge: BL_SEARCH_PARENT_optimization = false and"
	       << "src.ptr == NULL!" << thr();
	}
      else
	{
	  // Pri hlavnim hledani cyklu se pri zapnuti jiste volby parenti do bl_search
	  // neukladaji, takze optimalizace nejde pouzit
	  return true;
	}
    }

  if (dest.ptr == NULL)
    {
      gerr << net_id << ": "
	   << "VCL_check_edge: dest.ptr == NULL!" << thr();
    }

  if (!p_sys->get_with_property())
    {
      return true;
    }


  switch (VCL_check_edge_type) {
  case vcet_standard:
    {
      int src_scc_id = property_decomposition->get_process_scc_id(src);
      int dest_scc_id = property_decomposition->get_process_scc_id(dest);
      
      return !(src_scc_id != dest_scc_id || 
	       property_decomposition->get_scc_type(dest_scc_id) == 0);

    }
  case vcet_dest_accepting:
    {
      return (p_sys->is_accepting(dest));
    }
  default:
    {
      return true;
    }
  }
}

void process_message(char *buf, int size, int src, int tag, bool urgent)
{
  state_t recv_state, parent_state;
  state_ref_t recv_state_ref;
  
  switch (tag) {
  case TAG_SEND_STATE:
    {
      bool full_expand;
      
      
      received_message.load_data((byte_t *)(buf), size);
      
      received_message.read_state(recv_state);
      

      if (st.is_stored(recv_state, recv_state_ref))
	{
	  st.get_app_by_ref(recv_state_ref, appendix);

          received_message.read_state(parent_state);
          received_message.read_bool(full_expand);
          
	  // Pokud ce_search_started = true, pak tato hrana uz je prijmana podruhe,
	  // proro nesmi byt zapocitana
	  if (!ce_search_started && appendix.level <= current_level)
	    {
	      bl_at_level++;
	      bl_edges_cnt++;

	      // je nutne poslat odesilateli zpravu, ze ma stav doexpandovat, a to pouze
	      // v pripade ze nebezi ce_search (coz je osetreno vyse) a ze to uz nebylo
	      // udelano
	      if (!full_expand)
		{
		  distributed.network.send_urgent_message(parent_state.ptr,
						     parent_state.size,
						     src,
						     TAG_POR_COMPLETE_STATE_EXPANSION);
		}
	    }

	  if (appendix.level <= current_level && !synchronized_reachability_only &&
	      (!VCL_optimization ||
	       (VCL_check_edge(parent_state, recv_state))))
	    {
	      bl_search.bl_source = parent_state;

	      if (!ce_search_started)
		{
		  bl_search.bl_source_nid = src;
		}
	      else
		{
		  // toto by samozrejme fungovalo i pro druhou vetev if, ale tam se to da
		  // udelat jednoduseji pres src
		  bl_search.bl_source_nid = distributed.get_state_net_id(bl_search.bl_source);
		}
		
	      bl_search.state_ref = recv_state_ref;  
	      bl_search.limit_level = current_level;

	      // parent se uklada pouze v pripade ce_search nebo pri vypnuti teto optimalizace
	      if (!BL_SEARCH_PARENT_optimization || ce_search_started)
		  // zde je parent stejny jako bl_source
                  bl_search.parent=duplicate_state(parent_state);
	      else
		  bl_search.parent.ptr = NULL;
	      
	      bl_search.parent_level = current_level;
	      bl_search.acc_bl_passed = 0;
	      bl_search.acc_passed =  !p_sys->get_with_property() || 
		p_sys->is_accepting(bl_search.bl_source);
	      bl_search.ce_search = ce_search_started;
      
	      bl_search_stack.push(bl_search);

	      // Pokud ce_search_started = true, pak tato hrana uz je prijmana podruhe,
	      // proro nesmi byt zapocitana
	      if (!ce_search_started)
		relevant_bl_at_level++;
	    }
	  else
	    {
	      delete_state(parent_state);
	    }
	  
//  	  cout << net_id << ": "
//  	       << "Old state received." << endl;
	}
      else
	{
	  // Pokud ce_search_started = true, pak by nemely byt objeveny zadne nove stavy
	  if (ce_search_started)
	    gerr << net_id << ": "
		 << "RCV_STATE: State that should be stored is not stored!" << thr();

	  states_count++;
	  st.insert(recv_state, recv_state_ref);
	  appendix.level = current_level + 1;
	  appendix.last_search_bl_nid = -1;
	  appendix.last_search_bl_level = -1;
	  appendix.last_search_acc_bl_passed = -1;
	  appendix.last_search_acc_passed = false;
	  
          received_message.read_state(appendix.parent_bfs);
	  
          appendix.parent_bl_search.ptr = NULL;
	  appendix.ce_search_visited = false;
	  appendix.ce_gen_visited = false;
	  if (POR_MODE != pm_none)
	    {
	      appendix.ample_set = new bit_string_t(p_sys->get_process_count());
	      appendix.ample_set->clear();
	    }
	  else
	    appendix.ample_set = NULL;
	  
	  st.set_app_by_ref(recv_state_ref, appendix);
	  next_level_queue->push(recv_state_ref);


//  	  cout << net_id << ": "
//  	       << "New state received." << endl;

	}

      // !!!delete_state(recv_state) - OK
      delete_state(recv_state);
      
      break;
    }    
  case TAG_SEND_BL_SEARCH:
    {
      received_message.load_data((byte_t *)(buf), size);
      
      received_message.read_state(recv_state);

      if (st.is_stored(recv_state, recv_state_ref))
	{
	  bl_search.state_ref = recv_state_ref;
	  received_message.read_state(bl_search.bl_source);
	  if (!BL_SEARCH_PARENT_optimization || ce_search_started)
	      // parent se prijima pouze v pripade ce_search nebo vypnuti optimalizace
              received_message.read_state(bl_search.parent);
	  else
	      bl_search.parent.ptr = NULL;
 	  received_message.read_slong_int(bl_search.bl_source_nid);
 	  received_message.read_slong_int(bl_search.limit_level);
 	  received_message.read_slong_int(bl_search.parent_level);
 	  received_message.read_slong_int(bl_search.acc_bl_passed);
 	  received_message.read_bool(bl_search.acc_passed);
 	  received_message.read_bool(bl_search.ce_search);

	  bl_search_stack.push(bl_search);
	}
      else
	{
	  gerr << net_id << ": "
	       << "RCV_BL_SEARCH_STATE: State that should be stored is not stored" << thr ();
	}

      delete_state(recv_state);
      
      break;
    }
  case TAG_CYCLE_FOUND:
    {
      cycle_found_time = init_timer.gettime();
      cycle_found = true;
      break;
    }
  case TAG_CE_SEARCH_BL_SOURCE:
    {
      if (!ce_search_bl_source_received)
	{
	  ce_search_bl_source_received = true;

	  ce_search_bl_source = new_state(buf, size);

	  ce_search_bl_source_sender = src;
	}
      break;
    }
  case TAG_CE_GEN_INITIATE:
    {
      if (ce_gen_starting_state_valid)
	{
	  ce_member.state = ce_gen_starting_state;
	  // spravne ce_member.state_level doplnime pozdeji, ale pred posilanim stavu managerovi
	  ce_member.state_level = -1;
	  ce_member.state_order_no = 0;
	  ce_member.bfs_parent_part = false;
	  ce_member.go_on = true;
	  ce_member.repeated_state = false;

	  ce_member_queue.push(ce_member);
	}
      else
	{
	  gerr << net_id << ": "
	       << "Ce_ge_starting_state invalid when it should be valid!" << thr();
	}

      break;
    }
  case TAG_CE_GEN_SEND_STATE:
    {
      received_message.load_data((byte_t *)(buf), size);
                  
      received_message.read_state(ce_member.state);
      received_message.read_slong_int(ce_member.state_level);
      received_message.read_slong_int(ce_member.state_order_no);
      received_message.read_bool(ce_member.bfs_parent_part);
      received_message.read_bool(ce_member.go_on);
      received_message.read_bool(ce_member.repeated_state);

      ce_member_queue.push(ce_member);

      break;
    }
  case TAG_CE_GEN_INITIATE_BFS_PART:
    {
      ce_gen_initiate_bfs_part = true;

      break;
    }
  case TAG_POR_COMPLETE_STATE_EXPANSION:
    {
      recv_state = new_state(buf, size);

      if (st.is_stored(recv_state, recv_state_ref))
	{
	  st.get_app_by_ref(recv_state_ref, appendix);
	  bool perform_full_expansion=false;
	  for (size_int_t i=0; i<p_sys->get_process_count(); i++)
	    if (appendix.ample_set->get_bit(i)==false)
	      {
		appendix.ample_set->enable_bit(i);
		perform_full_expansion=true;
	      }
	  if (perform_full_expansion)
	    {
	      // je nutne stav doexpandovat
              full_expands++;
	      proviso_forced_expands++;
	      st.set_app_by_ref(recv_state_ref, appendix);
	      current_level_queue->push(recv_state_ref);
	    }
	}
      else
	{
	  gerr << net_id << ": "
	       << "RCV_STATE_COMPLETE_EXPANSION: State that should be stored is not stored!" 
	       << thr();
	}

      break;
    }
  }
}

void broadcast_cycle(bl_search_t& bl_search, state_t state)
{
  int i;
  if (ce_search_started)
    {
      // zahajime operace nutne pro nalezeni protiprikladu

      if (appendix.last_search_bl_level >= 0)
	{
	  // last_search_bl_source je definovano -> uvolni pamet, aby
	  // se to mohlo prepsat
	  delete_state(appendix.last_search_bl_source);
	}

      if (appendix.parent_bl_search.ptr != NULL)
	{
	  // parent_bl_search je definovano -> uvolni pamet, aby
	  // se to mohlo prepsat

	  delete_state(appendix.parent_bl_search);
	  appendix.parent_bl_search.ptr = NULL;
	}
      
      appendix.last_search_bl_source = duplicate_state(bl_search.bl_source);
      appendix.last_search_bl_level = bl_search.limit_level;
      appendix.last_search_bl_nid = bl_search.bl_source_nid;
      appendix.last_search_acc_bl_passed = bl_search.acc_bl_passed;
      appendix.last_search_acc_passed = bl_search.acc_passed;
     
      if (appendix.parent_bfs.ptr == NULL ||
	  appendix.parent_bfs != bl_search.parent)
	{
	  // drobna optimalizace - pokud by byly parent_bfs a
	  // parent_bl_search stejne, tak bude ulozeno jenom jedno,
	  // a to v parent_bfs
	  appendix.parent_bl_search = duplicate_state(bl_search.parent);
	}

      appendix.ce_search_visited = bl_search.ce_search;
      appendix.ce_gen_visited = false;

      st.set_app_by_ref(bl_search.state_ref, appendix);

      ce_gen_starting_state = duplicate_state(state);
      ce_gen_starting_state_valid = true;
    }
 
  for (i = 0; i < cluster_size; i++)
    {
      if (i != net_id)
	{
	  distributed.network.send_urgent_message(NULL, 0, i, TAG_CYCLE_FOUND);
	}
    }

  distributed.network.send_urgent_message(bl_search.bl_source.ptr, 
				     bl_search.bl_source.size, 
				     NETWORK_ID_MANAGER,
				     TAG_CE_SEARCH_BL_SOURCE);
				     
}

void initiate_ce_search(state_t& bl_source)
{
  succ_container_t successors(*p_sys);
  state_t successor;
  state_ref_t successor_ref;
  int state_net_id;

  // zde je plna expanze v poradku, protoze se jedna o bl_source

  p_sys->get_succs(bl_source, successors);
	      
  for (size_t i = 0; i < successors.size(); i++)
    {
      succs_count++;

      successor = successors[i];
      state_net_id = distributed.get_state_net_id(successor);
 
      if (state_net_id == net_id)
	{
	  if (st.is_stored(successor, successor_ref))
	    {
	      st.get_app_by_ref(successor_ref, appendix);

	      if (appendix.level <= current_level && !synchronized_reachability_only  &&
		  (!VCL_optimization ||
		   (VCL_check_edge(bl_source, successor))))

		{
		  bl_search.bl_source = duplicate_state(bl_source);
		  bl_search.bl_source_nid = distributed.get_state_net_id(bl_source);
		  bl_search.state_ref = successor_ref;  
		  bl_search.limit_level = current_level;

		  // parent a bl_source jsou nyni stejne
		  bl_search.parent = duplicate_state(bl_source);
		  
		  bl_search.parent_level = current_level;
		  bl_search.acc_bl_passed = 0;
		  bl_search.acc_passed =  !p_sys->get_with_property() || 
		    p_sys->is_accepting(bl_source);
		  bl_search.ce_search = true;
		       
		  bl_search_stack.push(bl_search);
		}
	    }
	  else
	    {
	      gerr << net_id << ": " 
		   << "INITIATE_CE_SEARCH: State that should be stored is not stored!" << thr();
	    }
	}
      else
	{
	  send_struct.state = successor;
	  send_struct.predecessor = bl_source;
	  send_struct.full_expand = true; // zde je uplne jedno, co se do full_expand da
	  send_state(send_struct, state_net_id);
	}

      delete_state(successor);
    }

  distributed.network.flush_all_buffers();


  // pri opetovnem hledani protiprikladu jiz sice nemuze prijit jiny ce_search_bl_source (protoze
  // uz je jenom jediny), ale muze prijit z jineho pocitace  
  delete_state(ce_search_bl_source);
  ce_search_bl_source.ptr = NULL;
  ce_search_bl_source_received = false;
  ce_search_bl_source_sender = -1;

  cout << net_id << ": " 
       << "Counterexample searching..." << endl;
}

bool bl_search_can_go_on(state_t& current_state, bl_search_t& bl_search, appendix_t& appendix)
{
  bool ret = false;

  // Sichrovaci test
  if (appendix.ce_search_visited && 
      bl_search.bl_source != appendix.last_search_bl_source)
    {
      gerr << net_id << ": "
	   << "Some state was visited by ce_search with erronous bl_source"
	   << thr();
      
    }
  
  // Sichrovaci test
  if (appendix.ce_search_visited && 
      bl_search.limit_level != appendix.last_search_bl_level)
    {
      gerr << net_id << ": "
	   << "Some state was visited by ce_search with erronous limit_level"
	   << thr();
      
    }

  // Sichrovaci test
  if (appendix.ce_search_visited && !bl_search.ce_search)
    {
      gerr << net_id << ": "
	   << "Non-ce_seach bl_search found after ce search had started"
	   << thr();
      
    }

//    Chybna verze, je zde pro jiste testovaci ucely
//
//    if (bl_search.ce_search)
//      {
//        if (!appendix.ce_search_visited)
//  	{
//  	  ret = true;
//  	}
//        else
//  	{
//  	  if (bl_search.bl_source > appendix.last_search_bl_source)
//  	    {
//  	      ret = true;
//  	    }
//  	  else
//  	    {
//  	      if (bl_search.acc_bl_passed > appendix.last_search_acc_bl_passed)
//  		{
//  		  ret = true;
//  		}
//  	      else
//  		{
//  		  ret = bl_search.acc_passed && !appendix.last_search_acc_passed;
//  		}
//  	    }
//  	}
//      }
//    else
//      {
//        if (bl_search.limit_level > appendix.last_search_bl_level)
//  	{
//  	  ret = true;
//  	}
//        else
//  	{
//  	  if (bl_search.bl_source > appendix.last_search_bl_source)
//  	    {
//  	      ret = true;
//  	    }
//  	  else
//  	    {
//  	      if (bl_search.acc_bl_passed > appendix.last_search_acc_bl_passed)
//  		{
//  		  ret = true;
//  		}
//  	      else
//  		{
//  		  ret = bl_search.acc_passed && !appendix.last_search_acc_passed;
//  		}
//  	    }
//  	}
//      }


  if (!REVERSE_ORDERING_OF_STATES)
    {
      if (bl_search.ce_search)
	{
	  if (!appendix.ce_search_visited)
	    {
	      ret = true;
	    }
	  else
	    {
	      if ((bl_search.bl_source > appendix.last_search_bl_source) ||
		  (bl_search.bl_source == appendix.last_search_bl_source &&
		   bl_search.acc_bl_passed > appendix.last_search_acc_bl_passed) ||
		  (bl_search.bl_source == appendix.last_search_bl_source &&
		   bl_search.acc_bl_passed == appendix.last_search_acc_bl_passed &&
		   bl_search.acc_passed && !appendix.last_search_acc_passed))
		{
		  ret = true;
		}
	    }
	}
      else
	{
	  if ((bl_search.limit_level > appendix.last_search_bl_level) ||
	      (bl_search.limit_level == appendix.last_search_bl_level &&
	       bl_search.bl_source > appendix.last_search_bl_source) ||
	      (bl_search.limit_level == appendix.last_search_bl_level &&
	       bl_search.bl_source == appendix.last_search_bl_source &&
	       bl_search.acc_bl_passed > appendix.last_search_acc_bl_passed) ||
	      (bl_search.limit_level == appendix.last_search_bl_level &&
	       bl_search.bl_source == appendix.last_search_bl_source &&
	       bl_search.acc_bl_passed == appendix.last_search_acc_bl_passed &&
	       bl_search.acc_passed && !appendix.last_search_acc_passed))
	    {
	      ret = true;
	    }
	}
    }
  else
    {
      if (bl_search.ce_search)
	{
	  if (!appendix.ce_search_visited)
	    {
	      ret = true;
	    }
	  else
	    {
	      if ((bl_search.bl_source < appendix.last_search_bl_source) ||
		  (bl_search.bl_source == appendix.last_search_bl_source &&
		   bl_search.acc_bl_passed > appendix.last_search_acc_bl_passed) ||
		  (bl_search.bl_source == appendix.last_search_bl_source &&
		   bl_search.acc_bl_passed == appendix.last_search_acc_bl_passed &&
		   bl_search.acc_passed && !appendix.last_search_acc_passed))
		{
		  ret = true;
		}
	    }
	}
      else
	{
	  if ((bl_search.limit_level > appendix.last_search_bl_level) ||
	      (bl_search.limit_level == appendix.last_search_bl_level &&
	       bl_search.bl_source < appendix.last_search_bl_source) ||
	      (bl_search.limit_level == appendix.last_search_bl_level &&
	       bl_search.bl_source == appendix.last_search_bl_source &&
	       bl_search.acc_bl_passed > appendix.last_search_acc_bl_passed) ||
	      (bl_search.limit_level == appendix.last_search_bl_level &&
	       bl_search.bl_source == appendix.last_search_bl_source &&
	       bl_search.acc_bl_passed == appendix.last_search_acc_bl_passed &&
	       bl_search.acc_passed && !appendix.last_search_acc_passed))
	    {
	      ret = true;
	    }
	}
    }

  return (ret && (!VCL_optimization || 
		  (VCL_check_edge(bl_search.parent, current_state))));
}

// funkce vrati pro dany stav ample set, a to pouze pokud je ruzny od enabled set
// result = (ample set != enabled set)
// enabled_trans se predavaji proto, aby se nemusely pri kazdem volani funkce vytvaret
// jako ample se standardne bere 1. mozny proces, dalsi moznost je vzit ten s nejmensim 
// poctem transitionu (nastavuje se konstantou POR_MODE)
bool get_ample_succs(state_t& state, succ_container_t& succs_cont)
{
  
  dve_explicit_system_t * p_dve_sys =
    dynamic_cast<dve_explicit_system_t*>(p_sys);
  
  if (POR_MODE == pm_none || (!p_dve_sys))
    {
      p_sys->get_succs(state, succs_cont);
      return false;
    }

  enabled_trans_container_t all_enabled_trans(*p_sys);
  state_t succ_state;
  state_ref_t ref, state_ref;

  //nejdriv zkus jestli uz nemas ample_set napocitanou; pokud jo rovnou vygeneruj nasledniky a zkonci, pokud ne, najdi composed_ample_set
  st.is_stored(state, state_ref);
  st.get_app_by_ref(state_ref, appendix);
  p_sys->get_enabled_trans(state, all_enabled_trans);
  succs_cont.clear();
  size_int_t i,j,k;
  for (i=j=0; j<p_sys->get_process_count(); j++)
    {
      if (appendix.ample_set->get_bit(j))
	{
	  for (k=0; k<all_enabled_trans.get_count(j); k++)
	    {
	      p_sys->get_enabled_trans_succ(state, all_enabled_trans[i+k], succ_state);
	      succs_cont.push_back(succ_state);
	    }
	}
      i+=all_enabled_trans.get_count(j);
    }
  if (succs_cont.size()>0) //ample_set jiz napocitana nekdy driv
    {
      return true;
    }

  bool can_be_ample[p_sys->get_process_count()];
  std::size_t best_ample_size, best_ample_proc_id;
  int state_owner;
  bool ample_without_proviso_exists=false;

  por.generate_composed_ample_sets(state, can_be_ample, active_trans, all_enabled_trans);
  best_ample_size=MAX_ULONG_INT;
  for (size_t proc_id = 0; proc_id < p_sys->get_process_count(); proc_id++)
    {
      if ((can_be_ample[proc_id])&&(active_trans[proc_id]->size()<all_enabled_trans.size()))
	{
	  ample_without_proviso_exists=true;
	  succs_cont.clear();
	  p_sys->get_enabled_trans_succs(state, succs_cont, *(active_trans[proc_id]));
	  size_int_t j = 0;
	  for (; j < succs_cont.size(); j++) //jsou vsichni naslednici OK? Neuzavira nektery z nich cyklus?
	    {
	      succ_state=succs_cont[j];
	      state_owner = distributed.partition_function(succ_state);
	      if ((state_owner==distributed.network_id)&&(st.is_stored(succ_state, ref)))
		{
		  st.get_app_by_ref(ref,appendix);
		  if (appendix.level <= current_level) //BL-hrana muze uzavrit cyklus => zamitni kandidata na ample set
		    {
		      for (size_int_t k=j+1; k<succs_cont.size(); k++)
			delete_state(succs_cont[k]);
		      j=succs_cont.size()+1;
		    }
		}
	      delete_state(succ_state);
	    }
	  if (j==succs_cont.size()) //a candidate found
	    {
	      if (POR_MODE == pm_first_ample)
		{
		  p_sys->get_enabled_trans_succs(state, succs_cont, *(active_trans[proc_id]));
		  return true;
		}
	      if (best_ample_size>succs_cont.size()) //has it lower number of transitions?
		{
		  best_ample_size = succs_cont.size();
		  best_ample_proc_id = proc_id;
		}
	    }
	}
    }
  st.is_stored(state, state_ref);
  st.get_app_by_ref(state_ref, appendix);
  if (best_ample_size!=MAX_ULONG_INT) //some proper ample set has been found
    {
      p_sys->get_enabled_trans_succs(state, succs_cont, *(active_trans[best_ample_proc_id]));
      for (size_int_t j=0; j<p_sys->get_process_count(); j++)
	if ((j!=p_sys->get_property_gid())&&(active_trans[best_ample_proc_id]->get_count(j)>0))
	  appendix.ample_set->enable_bit(j);
    }
  else
    {
      full_expands++;
      if (ample_without_proviso_exists)
	proviso_forced_expands++;
      for (size_int_t j=0; j<p_sys->get_process_count(); j++)
	appendix.ample_set->enable_bit(j);
      p_sys->get_succs(state, succs_cont);
    }
  st.set_app_by_ref(state_ref, appendix);
  return (best_ample_size!=MAX_ULONG_INT);
}

void broadcast_ce_gen_initiate_bfs_part(void)
{
  ce_gen_initiate_bfs_part = true;

  for (int i = 0; i < cluster_size; i++)
    {
      if (i != net_id)
	{
	  distributed.network.send_urgent_message(NULL, 0, i, TAG_CE_GEN_INITIATE_BFS_PART);
	}
    }
}

void print_state(state_t& state)
{
  cout << "--------------------------------------" << endl;
  
//  std::size_t print_mode = ES_FMT_PRINT_VAR_NAMES |
//    ES_FMT_PRINT_STATE_NAMES |  ES_FMT_PRINT_PROCESS_NAMES;
  
  p_sys->print_state(state, cout/*, print_mode*/);
  cout << endl;

  cout << "--------------------------------------" << endl;
}

void print_comm_matrix(pcomm_matrix_t& cm)
{
  for (int i = 0; i < cluster_size; i++)
    {
      for (int j = 0; j < cluster_size; j++)
	{
	  cout << (*cm)(i, j) << " ";
	}
      cout << endl;
    }
}

void gather_ints(int& n, int* gath_buf)
{
  distributed.network.gather(static_cast<char *>(static_cast<void *>(&n)),
			sizeof(int),
			static_cast<char *>(static_cast<void *>(gath_buf)),
			sizeof(int),
			NETWORK_ID_MANAGER);
}

void print_gathered_ints(int* gath_buf)
{
  int total_cnt = 0;
  
  for (int i = 0; i < cluster_size; i++)
    {
      cout << gath_buf[i] << " ";
      total_cnt += gath_buf[i];
    }
  cout << endl;
  cout << "Total: " << total_cnt << endl;
}


inline
size_t get_state_net_id(state_t state)
{
  int tmp=0,start=0,end=state.size;
  int increase = 1;
  int result=73;

  if (!state.ptr)
    {
      gerr << "get_state_page(): called for invalid state.ptr"
	   << thr();
      return 0;
    }                     
  
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
  case 4:
    increase = 3;
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
  return(result);
}


//  size_t get_state_net_id(state_t state)
//  {
//    int tmp=0;
//    size_t result=73;
 
//     if (!state.ptr)
//      {
//        gerr << "get_state_page(): called for invalid state.ptr"
//  	   << thr();
//        return 0;
//      }                     

//    unsigned char *tmp_ptr = reinterpret_cast<unsigned char*>(state.ptr);

//    for(tmp=0;tmp<state.size;tmp+=3)
//      {
//        if (*tmp_ptr==0)
//  	{
//  	  result *= 6781;
//  	}
//        else
//  	{
//  	  result += 9311 ^ (*tmp_ptr) ;
//  	  result *= 8713 ;
//  	}
//        tmp_ptr ++;
//      }
//    return (result);
//  }

void version()
{
  cout <<"BLedge-based cycle detection 1.0 build ";
  cout <<"15";
  cout <<" (";
  cout <<"2006/09/20 17:00";
  cout <<")";
  cout <<endl;
}

void usage()
{

  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"--------------------------------------------------------------"<<endl;
  version();
  cout<<"--------------------------------------------------------------"<<endl;
  //  cout <<"bledge: ltl model-checker based on back-level edges"<<endl;
  cout <<"Usage: [mpirun -np N] distributed.bledge [options] input_file"<<endl;
  cout <<"Options: "<<endl;
  cout <<" -v, --version\t\tshow bledge version"<<endl;
  cout <<" -S, --printstats\tprint some statistics"<<endl;
  cout <<" -h, --help\t\tshow this help"<<endl;
  cout <<" -Hx, --htsize x\tset the size of hash table to ( x<33 ? 2^x : x )"<<endl;
  cout <<" -d, --commdetails\tprint detailed communication information at the end"<<endl;
  cout <<" -r, --report\t\tproduce report (file" << REPORT_FILE_EXTENSION << ")" << endl;
  cout <<" -s, --syncreachonly\tperform reachability with bl. edges counting only" << endl;
  cout <<" -P, --por\t\tenable partial order reduction" <<endl; 
  cout <<" -p, --precompile\tDVE file is precompiled prior verification"<<endl;
  cout <<" -t, --trail\t\tproduce ce. trail file (file" 
       << COUNTER_EXAMPLE_PATH_FILE_EXTENSION << ")" <<endl;
  cout <<" -c, --statelist\tproduce ce. list of states file (file" 
       << COUNTER_EXAMPLE_FILE_EXTENSION << ")" <<endl;  
  cout <<" -Dx, --partfunc x\tset part of the state used by partition function:"<<endl;
  cout <<"\t\t 0 - full state (default)"<<endl;
  cout <<"\t\t 1 - every second byte"<<endl;
  cout <<"\t\t 2 - the second half"<<endl;
  cout <<"\t\t 3 - the first three quarters"<<endl;
  cout <<"\t\t 4 - every third byte"<<endl;
  cout <<" -L\tperform logging"<<endl;
  cout <<" -X w\tsets base name of produced files to w (w.trail,w.report,w.00-w.N)"<<endl;
  cout <<" -Y\treserved for GUI"<<endl;
  cout <<" -Z\treserved for GUI"<<endl;
}


int main(int argc, char **argv)
{
  state_t initial_state;
  state_t current_state;
  state_t successor;
  state_t parent_state;
  state_ref_t state_ref;
  state_ref_t current_state_ref;
  state_ref_t successor_ref;
  appendix_t curr_state_appendix;
  bool finished;
  string processor_name;
  int state_net_id;
  struct tms tms_start, tms_end;
  clock_t ticks_start, ticks_end;
  timeval tv_start, tv_end;
  bool processed; 
  bool finish_after_bl_search;
  bool state_has_bl_edges;
  bool bl_search_finished;
  int c;
  string file_base_name;
  ostringstream cmd_line;
  int ce_length;
  size_t hashtable_size = 0;

  distributed.process_user_message = process_message;
  distributed.set_proc_msgs_buf_exclusive_mem(false);
  distributed.network_initialize(argc, argv);

  net_id = distributed.network_id;
  cluster_size = distributed.cluster_size;
  processor_name = distributed.processor_name;
  
  try {
    cmd_line << "bledge";
    
    static struct option longopts[] = {
      { "printstats",    no_argument, 0, 'S'},
      { "partfunc",      required_argument, 0, 'D'},
      { "help",          no_argument, 0, 'h'},
      { "htsize",        required_argument, 0, 'H'},
      { "por",           no_argument, 0, 'P'},
      { "precompile",    no_argument, 0, 'p'},
      { "trail",         no_argument, 0, 't'},
      { "report",        no_argument, 0, 'r'},
      { "syncreachonly", no_argument, 0, 's'},
      { "statelist",     no_argument, 0, 'c'},
      { "version",       no_argument, 0, 'v'},
      { "commdetails",   no_argument, 0, 'd'},
      { "log",           no_argument, 0, 'L'},
      { "basename",      no_argument, 0, 'X'},
      { NULL, 0, NULL, 0 }
    };

    while ((c = getopt_long(argc, argv, "LX:SD:hH:pPtrsvcd", longopts, NULL)) != -1) {
      cmd_line << " -" << static_cast<char>(c);
      switch (c) {
      case 'L': {
	produce_log = true;
	break;
      }
      case 'X': {
	basename_specified = true;
	file_basename = optarg;
	break;
      }
      case 'S': { 
	print_statistics = true; 
	break;
      }
      case 'D': {
	pfunc = atoi(optarg); 
	break;
      }
      case 'h': {
	if (net_id == NETWORK_ID_MANAGER)
	  usage();
        distributed.finalize();
	return 0;
	break;
      }
      case 'H': {
	hashtable_size = atoi(optarg);
	break;
      }
      case 'P': {
	POR_MODE = pm_smallest_ample;
	break;
      }
      case 'p': {
	compiled_generator = true;
	break;
      }
      case 't': {
	produce_ce_trail_file = true;
	break;
      }
      case 'r': {
	produce_report = true;
	break;
      }
      case 's': {
	synchronized_reachability_only=true; 
	break;
      }
      case 'c': {
	produce_ce_states_file = true; 
	break;
      }
      case 'v': {
	version();
	distributed.finalize();
	return 0;
	break;
      }
      case 'd': {
	print_detailed_comm_info = true;
	break;
      }
      case '?': {
	cerr << "unknown switch -"<< static_cast<char>(optopt) <<endl;
	break;
      }
      }
    }

    if (argc <= optind)
      {
	if (net_id == NETWORK_ID_MANAGER)
	  {
//  	    gerr << net_id << ": "
//  		 << "No input file." << thr();
	    usage();
	  }
	distributed.finalize();
	return 0;
      }
    
    /* decisions about the type of an input */
    system_description_t system_desc;
    p_sys = system_desc.open_system_from_file(argv[optind],
					      compiled_generator,
					      (distributed.network_id==NETWORK_ID_MANAGER)&&
					      print_statistics
					      );
    input_file_ext = system_desc.input_file_ext.c_str();

    if (p_sys->get_with_property())
      {
	property_decomposition = p_sys->get_property_decomposition();
      }
    else
      {
	VCL_optimization = false;
      }
    
    //reading of input:
    int file_opening;
    try
      {
	if ((file_opening=p_sys->read(argv[optind]))&&(net_id==NETWORK_ID_MANAGER))
	  {
	    if (file_opening==system_t::ERR_FILE_NOT_OPEN)
	      gerr << net_id << ": " << "Cannot open file ...";
	    else
	      gerr << net_id << ": " << "Syntax error ...";
	  }
	if (file_opening)
	  gerr << thr();
      }
    catch (ERR_throw_t & err)
      { 
	distributed.finalize();
	return err.id; 
      }

    file_base_name = argv[optind];
    size_t pos = file_base_name.find(input_file_ext,
                                     file_base_name.length() - 4);
    if (pos != string::npos)
      file_base_name.erase(pos, 4);

    if (POR_MODE != pm_none)
      {
	if (p_sys->get_abilities().system_can_processes)
	  por.init(p_sys);
	else
	  gerr << "Model of system is not able to provide informations about processes necessary for p.o.r." << thr();
      }
//     if ((produce_ce_trail_file) && (!p_sys->get_abilities().system_can_transitions) && (distributed.network_id == 0))
//       {
// 	cout << "Model of system can't handle transitions, trail won't be produced." << endl;
// 	produce_ce_trail_file = false;
//       }

    if (p_sys->get_with_property())
      {
        if (p_sys->get_abilities().system_can_decompose_property)
         {
         }
        else 
	  {
 	    // 	  gerr << "Model of system is not able to provide informations "
	    // 	    "about processes and a property process" << thr();
	    cout <<"Model of system is not able to provide informations "
		 <<"about processes and a property process."
		 <<endl<<"Turning off VCL optimization."<<endl;
	      VCL_optimization = false;
	  }
	if (p_sys->get_property_type() != BUCHI)
	  gerr << "Cannot work with other than standard Buchi accepting condition."<<thr();
      }
    else
      {
	if (!synchronized_reachability_only)
	  { 
	    if (distributed.network_id == 0)
	      {
		cout <<"WARNING: ---------------------------------------------------------------------"<<endl;
		cout <<"WARNING: No property process specified."<<endl;
	      }
	    
	    if (distributed.network_id == 0 && !synchronized_reachability_only)
	      {
		cout <<"WARNING: Performing non-accepting cycle detection. (Can be supressed with -s.)"<<endl;
	      }
	    
	    /*	    if (POR_MODE != pm_none) -- ted uz p.o.r. jede i bez property, nastavi si visibility na emptyset
	      {
		if (distributed.network_id == 0)
		  {
		    cout <<"WARNING: Cannot perform partial order reduction. Switching to nonPOR mode."<<endl;	       
		  }
		POR_MODE=pm_none;
		}*/
	    
	    if (distributed.network_id == 0)
	      {
		cout <<"WARNING: ---------------------------------------------------------------------"<<endl;
	      }
	  }
      }
    
    succ_container_t successors(*p_sys), tmp_succs(*p_sys);
    if (POR_MODE !=pm_none)
      {
	active_trans = new enabled_trans_container_t*[p_sys->get_process_count()];
	for (std::size_t i=0; i<p_sys->get_process_count(); i++)
	  active_trans[i] = new enabled_trans_container_t(*p_sys);
      }
      
     if (hashtable_size != 0)
        {
          if (hashtable_size < 33)
            {
              int z = hashtable_size;
              hashtable_size = 1;
              for (;z>0; z--)
                {
                  hashtable_size = 2* hashtable_size;
                }
            }
          st.set_ht_size(hashtable_size);
        }

    st.set_appendix(appendix);
    st.init();

    distributed.initialize();

    init_time = init_timer.gettime();

    reporter.start_timer();

    if (produce_log)
      {
	logger.set_storage(&st);
	if (basename_specified)
	  {
	    logger.init(&distributed,file_basename,0);
	  }
	else
	  {
	    logger.init(&distributed,file_base_name,0);	    
	  }
	logger.use_SIGALRM(1);
      }
    
//      if (net_id == NETWORK_ID_MANAGER)
//        {
//  	cout << net_id << ": "
//  	     << "State size = " << initial_state.size << "." << endl;
	
//  	cout << net_id << ": "
//  	     << "Appendix size = " << sizeof(appendix_t) << "." << endl;

//  	if (p_sys->get_with_property())
//  	  {
//  	    cout << net_id << ": "
//  		 << "System is with property." << endl;
//  	  } else {
//  	    cout << net_id << ": "
//  		 << "System is without property." << endl;
//  	  }
//        }

//      cout << "net_id = " << net_id 
//  	 << ", cluster_size = " << cluster_size 
//  	 << ", processor_name = " << processor_name << endl;
    
//MESSAGE//    bl_send_buf_size = 3 * initial_state.size + 4*sizeof(int) + 2*sizeof(bool);
//MESSAGE//    bl_send_buf_size_without_parent = 2 * initial_state.size + 4*sizeof(int) + 2*sizeof(bool);
//MESSAGE//    bl_send_buf = new char[bl_send_buf_size];

//MESSAGE//    send_buf_size = 2 * initial_state.size + sizeof(bool);
//MESSAGE//    send_buf = new char[send_buf_size];

//MESSAGE//    ce_gen_send_buf_size = initial_state.size + 2*sizeof(int) + 3*sizeof(bool);
//MESSAGE//    ce_gen_send_buf = new char[ce_gen_send_buf_size];

//      cout << net_id << ": "
//  	 << "Current memory load = " << st.get_mem_used() << " bytes, maximum = "
//  	 << st.get_mem_max_used() << " bytes." << endl;  

//      cout << net_id << ": "
//  	 << "Currently states stored = " << st.get_states_stored() << "." << endl
//  	 << net_id << ": "
//  	 << "Collision blocks allocated = " << st.get_coltables() << "." << endl;

    // record start times
    ticks_start = times(&tms_start);
    gettimeofday(&tv_start, NULL);

    initial_state = p_sys->get_initial_state();
    state_net_id = distributed.get_state_net_id(initial_state);

    current_level = 0;
    all_bl_edges_cnt = 0;
    all_relevant_bl_edges_cnt = 0;
    all_states_count = 0;
    states_count = 0;
    succs_count = 0;
    full_expands = 0;
    proviso_forced_expands = 0;

    if (state_net_id == net_id)
      {
	states_count++;
	st.insert(initial_state, state_ref);

	appendix.level = current_level;
	appendix.last_search_bl_nid = -1;
	appendix.last_search_bl_level = -1;
	appendix.last_search_acc_bl_passed = -1;
	appendix.parent_bfs.ptr = NULL;
	appendix.parent_bl_search.ptr = NULL;
	appendix.ce_search_visited = false;
	appendix.ce_gen_visited = false;
	if (POR_MODE != pm_none)
	  {
	    appendix.ample_set = new bit_string_t(p_sys->get_process_count());
	    appendix.ample_set->clear();
	  }
	else
	  appendix.ample_set = NULL;
	
	st.set_app_by_ref(state_ref, appendix);
	next_level_queue->push(state_ref);

        delete_state(initial_state);
      }

    finished = false;

    cycle_found = false;

    while (!finished)
      {
	interchange_queues(current_level_queue, next_level_queue);

	if (!current_level_queue->empty())
	  {
	    distributed.set_busy();
	  }

	bl_at_level = 0;
	relevant_bl_at_level = 0;

	while (!distributed.synchronized(info))
	  {
	    distributed.process_messages();

	    if (!current_level_queue->empty())
	      {
		distributed.set_busy();
	      }

	    while (!current_level_queue->empty())
	      {
		distributed.process_messages();

		current_state_ref = current_level_queue->front();
		current_level_queue->pop();
		current_state = st.reconstruct(current_state_ref);
		st.get_app_by_ref(current_state_ref, curr_state_appendix);
		state_has_bl_edges = false;
                
                get_ample_succs(current_state, successors);		
                st.get_app_by_ref(current_state_ref, curr_state_appendix);
		trans+=successors.size();
		for (size_t i = 0; i < successors.size(); i++)
		  {
		    successor = successors[i];
		    succs_count++;		    
		    state_net_id = distributed.get_state_net_id(successor);
 
		    // cout << net_id << ": "
		    //      << "Found state which belogs to workstation "
		    //      << state_net_id << "." << endl;

		    if (state_net_id == net_id)
		      {
			if (st.is_stored(successor, successor_ref))
			  {
			    st.get_app_by_ref(successor_ref, appendix);

			    if (appendix.level <= current_level)
			      {
				bl_at_level++;
				bl_edges_cnt++;
				state_has_bl_edges = true;
			      }

			    if (appendix.level <= current_level && 
				!synchronized_reachability_only &&
				(!VCL_optimization ||
				 (VCL_check_edge(current_state, successor))))

			      {
				bl_search.bl_source = duplicate_state(current_state);
				bl_search.bl_source_nid = net_id;
				bl_search.state_ref = successor_ref;  
				bl_search.limit_level = current_level;

				// zde je uklada, pouze pokud je vypnuta optimalizace
				if (!BL_SEARCH_PARENT_optimization)
				  {
				    bl_search.parent = duplicate_state(current_state);
				  }
				else
				  {
				    bl_search.parent.ptr = NULL;
				  }

				bl_search.parent_level = current_level;
				bl_search.acc_bl_passed = 0;
				bl_search.acc_passed =  !p_sys->get_with_property() || 
				  p_sys->is_accepting(current_state);
				bl_search.ce_search = false;

				// print_state(current_state);
				// print_state(successor);
		       
				bl_search_stack.push(bl_search);

				relevant_bl_at_level++;
			      }
			  }
			else
			  {
			    states_count++;
			    st.insert(successor, state_ref);
			    appendix.level = current_level + 1;
			    appendix.last_search_bl_nid = -1;
			    appendix.last_search_bl_level = -1;
			    appendix.last_search_acc_bl_passed = -1;
			    appendix.last_search_acc_passed = false;
			    appendix.parent_bfs = duplicate_state(current_state);
			    appendix.parent_bl_search.ptr = NULL;
			    appendix.ce_search_visited = false;
			    appendix.ce_gen_visited = false;
			    if (POR_MODE != pm_none)
			      {
				appendix.ample_set = new bit_string_t(p_sys->get_process_count());
				appendix.ample_set->clear();
			      }
			    else
			      appendix.ample_set = NULL;

			    st.set_app_by_ref(state_ref, appendix);
			    next_level_queue->push(state_ref);
			  }
		      }
		    else
		      {
			cross_trans++;
			send_struct.state = successor;
			send_struct.predecessor = current_state;
			send_struct.full_expand = true;
			if (POR_MODE != pm_none)
			  {
			    for (size_int_t j=0; j<p_sys->get_process_count(); j++)
			      send_struct.full_expand &= curr_state_appendix.ample_set->get_bit(j);
			  }
			send_state(send_struct, state_net_id);
		      }

		    // !!!delete_state(successor) - OK
		    delete_state(successor);
		  }

		if (!state_has_bl_edges)
		  states_without_bl_edges++;
	    
		// !!!delete_state(current_state) - OK
		delete_state(current_state);
	      }

	    distributed.network.flush_all_buffers();
	
	    distributed.set_idle();
	  }

	bl_at_level = info.data.bl_edges_cnt;
	relevant_bl_at_level = info.data.relevant_bl_edges_cnt;
	all_bl_edges_cnt += bl_at_level;
	all_relevant_bl_edges_cnt += relevant_bl_at_level;

	all_states_count = info.data.all_states_count;

	if (net_id == NETWORK_ID_MANAGER && print_statistics)
	  {
	    cout <<net_id<<": Level:" 
		 <<(current_level<10?" ":"")
		 <<(current_level<100?" ":"")
		 <<current_level <<"\t"
		 <<"relevant back-level edges: "
		 <<relevant_bl_at_level <<"/"
		 <<bl_at_level /*<< " states: " << states_count << '/' << full_expands << '/' << succs_count*/
		 <<endl;
//  	    cout << net_id << ": "
//  		 << "Synchronized on level " << current_level << "." << endl;

//  	    cout << net_id << ": "
//  		 << bl_at_level << " back edges found." << endl;

//  	    cout << net_id << ": "
//  		 << relevant_bl_at_level << " relevant back edges." << endl;
	  }

	finished = (info.data.next_level_states == 0 && bl_at_level == 0);
	finish_after_bl_search = info.data.next_level_states == 0;

	if (!finished)
	  {
	    bl_search_finished = false;
	    ce_search_started = false;
	    ce_search_bl_source_received = false;
	    ce_search_bl_source_sender = -1;
	    ce_gen_starting_state_valid = false;

	    if (cycle_found)
	      {
		gerr << net_id << ": " 
		     << "cycle_found = true when it should be false" << thr();
	      }

	    while (!bl_search_finished)
	      { 
		if (cycle_found)
		  {
		    if (!produce_ce_trail_file && !produce_ce_states_file)
		      break;

		    // byl nalezen cyklus a je nutne zahajit hledani protiprikladu
		    // ale to pouze za predpokladu, ze po nas nekdo protipriklad chce
		    ce_search_started = true;
		    
		    while (!bl_search_stack.empty())
		      {
			bl_search = bl_search_stack.top();
			delete_state(bl_search.bl_source);
			if (BL_SEARCH_PARENT_optimization)
			  {
			    if (bl_search.parent.ptr != NULL)
			      gerr << net_id << ": " 
				   << "Non-NULL parent in Non-ce_search!" << thr();
			  }
			else
			  {
			    delete_state(bl_search.parent);
			    bl_search.parent.ptr = NULL;
			  }

			bl_search_stack.pop();
		      }

		    if (net_id == NETWORK_ID_MANAGER)
		      {
			if (!ce_search_bl_source_received)
			  {
			    gerr << net_id << ": " 
				 << "Cycle found, but manager does not have ce_search_bl_source!"
				 << thr();
			  }

			initiate_ce_search(ce_search_bl_source);
		      }

		    while (!distributed.synchronized())
		      {
			distributed.process_messages();
		      }
		  }

		cycle_found = false;

		if (!bl_search_stack.empty())
		  distributed.set_busy();

		while (!distributed.synchronized())
		  {
		    distributed.process_messages();

		    if (!bl_search_stack.empty() && !cycle_found)
		      {
			distributed.set_busy();
		      }

		    while (!bl_search_stack.empty() && !cycle_found)
		      {
			distributed.process_messages();

			bl_search = bl_search_stack.top();
			bl_search_stack.pop();
		
			current_state = st.reconstruct(bl_search.state_ref);
			st.get_app_by_ref(bl_search.state_ref, appendix);
		
			processed = false;

			if (current_state == bl_search.bl_source &&
			    (bl_search.acc_passed || 
			     (!p_sys->get_with_property() || p_sys->is_accepting(current_state))))
			  {

			    cout << net_id << ": "
				 << "Cycle found by hitting the target" 
				 << "                     "
				 << endl;
			
			    if (ce_search_started)
			      cout << net_id << ": " 
				   << "Parent graph constructed" << endl;
			    
			    cycle_found = true;
			    processed = true;

//  			    if (ce_search_started)
//  			      print_state(bl_search.parent);

//  			    print_state(current_state);

			    broadcast_cycle(bl_search, current_state);
			  }

			if (!processed && appendix.level > bl_search.limit_level)
			  {
			    processed = true;
			  }

			// nasledujici test jsem mohl provest uz pri zarazovani
			// bl_search do fronty, ale to jsem jeste nevedel jestli z
			// bl_search.state_ref vedou nejake bl hrany.
			  
			if (!processed && 
			    bl_search.parent_level == bl_search.limit_level)
			  {
			    if (bl_search.acc_passed)
			      {
				bl_search.acc_bl_passed++;
				bl_search.acc_passed = false;
			      }
			    if (bl_search.acc_bl_passed > bl_at_level)
			      {
				cout << net_id << ": " 
				     << "Cycle found by exceeding the bl_at_level" << endl;

				if (ce_search_started)
				  cout << net_id << ": " 
				       << "Parent graph constructed" << endl;


				cycle_found = true;
				processed = true;
				broadcast_cycle(bl_search, current_state);      
			      }
			  }

			if (!processed &&
			    bl_search_can_go_on(current_state, bl_search, appendix))
			  {
			    if (appendix.last_search_bl_level >= 0)
			      {
				// last_search_bl_source je definovano -> uvolni pamet, aby
				// se to mohlo prepsat
				delete_state(appendix.last_search_bl_source);
			      }

			    if (appendix.parent_bl_search.ptr != NULL)
			      {
				// parent_bl_search je definovano -> uvolni pamet, aby
				// se to mohlo prepsat

				if (!ce_search_started)
				  gerr << net_id << ": "
				       << "Non-NULL parent_bl_search when ce_search is not "
				       << "running!" << thr();

				delete_state(appendix.parent_bl_search);
				appendix.parent_bl_search.ptr = NULL;
			      }

			    appendix.last_search_bl_source = duplicate_state(bl_search.bl_source);
			    appendix.last_search_bl_level = bl_search.limit_level;
			    appendix.last_search_bl_nid = bl_search.bl_source_nid;
			    appendix.last_search_acc_bl_passed = bl_search.acc_bl_passed;
			    appendix.last_search_acc_passed = bl_search.acc_passed;

			    if (ce_search_started)
			      {
				// parent graf se uklada pouze v pripade, ze bezi ce_search
				if (appendix.parent_bfs.ptr == NULL ||
				    appendix.parent_bfs != bl_search.parent)
				  {
				    // drobna optimalizace - pokud by byly parent_bfs a
				    // parent_bl_search stejne, tak bude ulozeno jenom jedno,
				    // a to v parent_bfs
				    appendix.parent_bl_search = duplicate_state(bl_search.parent);
				  }
			      }
			    

			    appendix.ce_search_visited = bl_search.ce_search;

			    st.set_app_by_ref(bl_search.state_ref, appendix);
			
			    bool full_expand=true;
			    if (POR_MODE != pm_none)
			      for (size_int_t j=0; j<p_sys->get_process_count(); j++)
				full_expand &= appendix.ample_set->get_bit(j);
			    if (POR_MODE == pm_none || (full_expand))
			      {
				p_sys->get_succs(current_state, successors);
			      }
			    else
			      {
				if (!get_ample_succs(current_state, successors))
				  {
				    gerr << net_id << ": "
					 << "State with enabled set = ample set has "
					 << "full_expand = false!" << thr();
				  }
			      }

		  
			    for (size_t i = 0; i < successors.size(); i++)
			      {
				succs_count++;

				successor = successors[i];

				state_net_id = distributed.get_state_net_id(successor);
			      
				if (state_net_id == net_id)
				  {
				    if (st.is_stored(successor, successor_ref))
				      {
					tmp_bl_search.bl_source = duplicate_state(bl_search.bl_source);
					tmp_bl_search.bl_source_nid = bl_search.bl_source_nid;
					tmp_bl_search.state_ref = successor_ref;  
					tmp_bl_search.limit_level = bl_search.limit_level;

					if (!BL_SEARCH_PARENT_optimization || 
					    ce_search_started)
					  {
					    // do bl_search se uklada parent pouze pokud bezi
					    // ce_search nebo je vypnuta optimalizace

					    tmp_bl_search.parent = duplicate_state(current_state);
					  }
					else
					  {
					    tmp_bl_search.parent.ptr = NULL;
					  }
					
					tmp_bl_search.parent_level = appendix.level;
					tmp_bl_search.acc_bl_passed = bl_search.acc_bl_passed;
					tmp_bl_search.acc_passed = bl_search.acc_passed ||
					  (!p_sys->get_with_property() ||
					   p_sys->is_accepting(current_state));
					tmp_bl_search.ce_search = bl_search.ce_search;
				      
					bl_search_stack.push(tmp_bl_search);
				      }
				    else
				      {
					gerr << net_id << ": "
					     << "BL_SEARCH: State that should be " 
					     << "stored is not stored, " 
					     << appendix.level << thr ();
				      }
				  }
				else
				  {
				    bl_send_struct.bl_source = bl_search.bl_source;
				    bl_send_struct.bl_source_nid = bl_search.bl_source_nid;
				    bl_send_struct.state = successor;
				    bl_send_struct.limit_level = bl_search.limit_level;
				    bl_send_struct.parent = current_state;
				    bl_send_struct.parent_level = appendix.level;
				    bl_send_struct.acc_bl_passed = bl_search.acc_bl_passed;
				    bl_send_struct.acc_passed = bl_search.acc_passed ||
				      (!p_sys->get_with_property() ||
				       p_sys->is_accepting(current_state));
				    bl_send_struct.ce_search = bl_search.ce_search;
				  
				    send_bl_search(bl_send_struct, state_net_id);
				  }

				delete_state(successor);
			      }
			  }
		    
			if (bl_search.parent.ptr != NULL)
			  delete_state(bl_search.parent);
			delete_state(bl_search.bl_source);
			delete_state(current_state);
		      }

		    distributed.set_idle();
	    
		    distributed.network.flush_all_buffers();
		  }
                                    
		bl_search_finished = !cycle_found || ce_search_started;
	      }
	  }
	
	if (finish_after_bl_search)
	  finished = true;

	if (ce_search_started && !cycle_found)
	  gerr << net_id << ": "
	       << "Cycle that should have been found was not found!" << thr();


	// vyprazdnime zasobnik, pokud neni prazdny
	if (cycle_found || finished)
	  {
	    finished = true;
	    while (!bl_search_stack.empty())
	      {
		bl_search = bl_search_stack.top();
		delete_state(bl_search.bl_source);
		if (bl_search.parent.ptr != NULL)
		  delete_state(bl_search.parent);
		bl_search_stack.pop();
	      }
	  }

	current_level++;
      }
 
    if (POR_MODE !=pm_none)
      {
	for (std::size_t i=0; i<p_sys->get_process_count(); i++)
	  delete active_trans[i];
	delete[] active_trans;
      }
          

    // promenne pro optimalizaci delky protiprikladu
    int cycle_state_on_lowest_level = -1;
    int ce_cycle_start = 0;
    int csoll_level = current_level + 1;
    
    state_t ce_repeated_state; // stav uzavirajici cyklus v protiprikladu
    bool repeated_state_found_in_ce_vector = false;
    int ce_members_in_ce_vector_cnt = 0;
    ce_repeated_state.ptr = NULL;


    // zajisti aby se protipriklad negeneroval v pripade nenalezeni cyklu
    ce_gen_initiate_bfs_part = false;

    if (cycle_found && (produce_ce_trail_file || produce_ce_states_file))
      {
	if (net_id == NETWORK_ID_MANAGER && ce_search_bl_source_received)
	  {
	    distributed.network.send_urgent_message(NULL, 
					       0, 
					       ce_search_bl_source_sender,
					       TAG_CE_GEN_INITIATE);

	    cout << net_id << ": " 
		 << "Counterexample generation..." << endl;
	  }
	
	for (int i = 0; i < static_cast<int>(ce_vector.size()); i++)
	  {
	    tmp_ce_member.state.ptr = NULL;
	    tmp_ce_member.state_level = -1;
	    tmp_ce_member.state_order_no = -1;
	    tmp_ce_member.bfs_parent_part = false;
	    tmp_ce_member.go_on = false;
	    tmp_ce_member.repeated_state = false;
	    
	    ce_vector[i] = tmp_ce_member;
	  }
	
	// lehky hack, na zacatku samozrejme hned po bfs parentech nejdeme, ale aby se vlezlo
	// do nasledujiciho cyklu, je nutne nastavit ce_gen_initiate_bfs_part na true
	ce_gen_initiate_bfs_part = true;
      }

    while (ce_gen_initiate_bfs_part)
      {
	ce_gen_initiate_bfs_part = false;

        while (!distributed.synchronized())
	  {
	    distributed.process_messages();

	    if (!ce_member_queue.empty())
	      {
		ce_member = ce_member_queue.front();

		ce_member_queue.pop();

		if (!ce_member.go_on)
		  {
		    if (net_id != NETWORK_ID_MANAGER)
		      {
			gerr << net_id << ": "
			     << "I have ce_member with go_on = false, which does not belong to me!"
			     << thr();
		      }

		    if (!ce_member.repeated_state)
 		      {
			// pokud se prvek do ce_vectoru vejde, pridame ho tam
			// pokud je ce_vector kratky, prodlouzime ho
			if (static_cast<int>(ce_vector.size()) - 1 < ce_member.state_order_no)
			  { 
			    tmp_ce_member.state.ptr = NULL;
			    tmp_ce_member.state_level = -1;
			    tmp_ce_member.state_order_no = -1;
			    tmp_ce_member.bfs_parent_part = false;
			    tmp_ce_member.go_on = false;
			    tmp_ce_member.repeated_state = false;

			    ce_vector.resize(ce_member.state_order_no + 1, tmp_ce_member);
			  }

			if (ce_vector[ce_member.state_order_no].state_level >= 0)
			  {
			    gerr << net_id << ": "
				 << "Two counterexample states in one position!" << thr();
			  }

			// zaradime stav protiprikladu do vektoru
			tmp_ce_member.state = duplicate_state(ce_member.state);
			tmp_ce_member.state_level = ce_member.state_level;
			tmp_ce_member.state_order_no = ce_member.state_order_no;
			tmp_ce_member.bfs_parent_part = ce_member.bfs_parent_part;
			tmp_ce_member.go_on = ce_member.go_on;
			tmp_ce_member.repeated_state = ce_member.repeated_state; 
			
			ce_vector[ce_member.state_order_no] = tmp_ce_member;

			if (ce_member.state_order_no + 1 > ce_members_in_ce_vector_cnt)
			  ce_members_in_ce_vector_cnt = ce_member.state_order_no + 1;
		      }
		    else
		      {
			if (!ce_member.bfs_parent_part)
			  {

			    if (ce_repeated_state.ptr != NULL)
			      {
				gerr << net_id << ": "
				     << "Second repeated state in ce!" << thr();
				
			      }
			    
			    ce_repeated_state = duplicate_state(ce_member.state);
			  }
		      }
		  }
		else
		  {
		    // pokud to jde, pokracujeme v generovani protiprikladu

		    if (st.is_stored(ce_member.state, state_ref))
		      {
			st.get_app_by_ref(state_ref, appendix);

			if (!ce_member.bfs_parent_part && !appendix.ce_search_visited)
			  {
			    gerr << net_id << ": " 
				 << "State visited during ce_gen was not visited during ce_search!"
				 << thr();
			  }

			ce_member.state_level = appendix.level;
 
			ce_gen_send_struct.state = ce_member.state;
			ce_gen_send_struct.state_level = ce_member.state_level;
			ce_gen_send_struct.state_order_no = ce_member.state_order_no;
			ce_gen_send_struct.bfs_parent_part = ce_member.bfs_parent_part;
			ce_gen_send_struct.go_on = false;
			ce_gen_send_struct.repeated_state = appendix.ce_gen_visited;
			
			send_ce_gen_state(ce_gen_send_struct, NETWORK_ID_MANAGER);

			if (ce_member.bfs_parent_part)
			  {
			    // tady jiz neni nutne appendix nastavovat
			    appendix.ce_gen_visited = true;

			    st.set_app_by_ref(state_ref, appendix);
			
			    if (appendix.parent_bfs.ptr == NULL)
			      {
				// protipriklad je kompletni
			      }
			    else
			      {
				parent_state = appendix.parent_bfs;
			    
				state_net_id = distributed.get_state_net_id(parent_state);

				if (state_net_id == net_id)
				  {
				    tmp_ce_member.state = duplicate_state(parent_state);
				    tmp_ce_member.state_level = -1;
				    tmp_ce_member.state_order_no = ce_member.state_order_no + 1;
				    tmp_ce_member.bfs_parent_part = true;
				    tmp_ce_member.go_on = true;
				    tmp_ce_member.repeated_state = false;

				    ce_member_queue.push(tmp_ce_member);
				  }
				else
				  {
				    ce_gen_send_struct.state = parent_state;
				    ce_gen_send_struct.state_level = -1;
				    ce_gen_send_struct.state_order_no = 
				      ce_member.state_order_no + 1;
				    ce_gen_send_struct.bfs_parent_part = true;
				    ce_gen_send_struct.go_on = true;
				    ce_gen_send_struct.repeated_state = false;
				
				    send_ce_gen_state(ce_gen_send_struct, state_net_id);
				  }
			      }
			  }
			else
			  {
			    if (appendix.ce_gen_visited)
			      {
				// Cyklus je zkompletovan, pokracujeme po BFS parentech, ale
				// od nejvyssiho stavu cyklu, je to trochu slozitejsi, musi
				// se to poslat pres managera a dat o tom vedet vsem pocitacum
				broadcast_ce_gen_initiate_bfs_part();
			      }
			    else
			      {
				appendix.ce_gen_visited = true;

				st.set_app_by_ref(state_ref, appendix);
			    
				if (appendix.parent_bl_search.ptr != NULL)
				  {
				    parent_state = appendix.parent_bl_search;
				  }
				else
				  {
				    if (appendix.parent_bfs.ptr == NULL)
				      gerr << net_id << ": "
					   << "counterexample cycle cannot be completed" << thr();

				    parent_state = appendix.parent_bfs;
				  }

				state_net_id = distributed.get_state_net_id(parent_state);

				if (state_net_id == net_id)
				  {
				    tmp_ce_member.state = duplicate_state(parent_state);
				    tmp_ce_member.state_level = -1;
				    tmp_ce_member.state_order_no = ce_member.state_order_no + 1;
				    tmp_ce_member.bfs_parent_part = false;
				    tmp_ce_member.go_on = true;
				    tmp_ce_member.repeated_state = false;

				    ce_member_queue.push(tmp_ce_member);
				  }
				else
				  {
				    ce_gen_send_struct.state = parent_state;
				    ce_gen_send_struct.state_level = -1;
				    ce_gen_send_struct.state_order_no = 
				      ce_member.state_order_no + 1;
				    ce_gen_send_struct.bfs_parent_part = false;
				    ce_gen_send_struct.go_on = true;
				    ce_gen_send_struct.repeated_state = false;
				
				    send_ce_gen_state(ce_gen_send_struct, state_net_id);
				  }

			      }
			  }
		      }
		    else
		      {
			gerr << net_id << ": "
			     << "CE_GEN: State that should be stored is not stored!" << thr();
		      }
		  }

		delete_state(ce_member.state);
	      }
	  }

	if (net_id == NETWORK_ID_MANAGER && ce_gen_initiate_bfs_part)
 	  {
	    int i;

	    for (i = 0; i < ce_members_in_ce_vector_cnt; i++)
	      {
		if (ce_vector[i].state_level < 0)
		  gerr << net_id << ": "
		       << "Incomplete ce_vector!" << thr();
	      }

	    i = ce_members_in_ce_vector_cnt - 1;

	    if (i < 0)
	      {
		gerr << net_id << ": "
		     << "No states in ce_vector (no. 1)!" << thr();
	      }

	    if (ce_repeated_state.ptr == NULL)
	      {
		gerr << net_id << ": "
		     << "Repeated state missing!" << thr();
	      }

	    ce_cycle_start = 0;
	    repeated_state_found_in_ce_vector = false;

	    while (i >= ce_cycle_start)
	      {
		tmp_ce_member = ce_vector[i];

		// pokud je na zatim nejnizsi urovni, uloz si jeho index
		
		if (tmp_ce_member.state_level < csoll_level)
		  {
		    csoll_level = tmp_ce_member.state_level;
		    cycle_state_on_lowest_level = i; // = tmp_ce_member.state_order_no
		  }

		if (ce_repeated_state == tmp_ce_member.state)
		  {
		    repeated_state_found_in_ce_vector = true;
		    ce_cycle_start = i;
		  }

		i--;
	      }

	    if (!repeated_state_found_in_ce_vector)
	      {
		gerr << net_id << ": "
		     << "Repeated state missing in ce_vector! (no. 1)" << thr();
	      }
	    

	    if (cycle_state_on_lowest_level < 0)
	      {
		gerr << net_id << ": "
		     << "No cycle states in ce_vector or incorrect state levels!" << thr();
	      }
	    
	    ce_gen_send_struct.state = 
	      ce_vector[cycle_state_on_lowest_level].state;
	    ce_gen_send_struct.state_level = 
	      ce_vector[cycle_state_on_lowest_level].state_level;
	    ce_gen_send_struct.state_order_no = 
	      ce_members_in_ce_vector_cnt - 1;
	    ce_gen_send_struct.bfs_parent_part = true;
	    ce_gen_send_struct.go_on = true;
	    ce_gen_send_struct.repeated_state = true;
	    
	    state_net_id = distributed.get_state_net_id(ce_gen_send_struct.state);
	    
	    send_ce_gen_state(ce_gen_send_struct, state_net_id);
	  }
      }

    ce_length = 0;
    
    if (cycle_found && (produce_ce_trail_file || produce_ce_states_file) && 
	net_id == NETWORK_ID_MANAGER)
      {
	// poskladej z ce_vector protipriklad a nejak ho vyplivni

	cout << net_id << ": "
	     << "Counterexample assembly..." << endl;
	    
	int i, first_ce_member_with_bfs_parent_part_false;

	for (i = 0; i < ce_members_in_ce_vector_cnt; i++)
	  {
	    if (ce_vector[i].state_level < 0)
	      gerr << net_id << ": "
		   << "Incomplete ce_vector!" << thr();
	  }

	if (!repeated_state_found_in_ce_vector)
	  {
	    gerr << net_id << ": "
		 << "Repeated state missing in ce_vector! (no. 2)" << thr();
	  }

	if (cycle_state_on_lowest_level < 0)
	  {
	    gerr << net_id << ": "
		 << "No cycle states in ce_vector" << thr();
	  }

	// protipriklad ziskame nasledovne. pujdeme od konce ce_vectoru dokud 
	// bfs_parent_part = true, zapamatujeme si cislo prvniho ce_memberu, ktery
	// ma bfs_parent_part = false, dale jdeme od cycle_state_on_lowest_level
	// az po ce_cycle_start, pokracujeme od zapamatovaneho mista az po ce_member
	// pred cycle_state_on_lowest_level

	i = ce_members_in_ce_vector_cnt - 1;
	
	if (i < 0)
	  {
	    gerr << net_id << ": "
		 << "No states in ce_vector (no. 2)!" << thr();
	  }

	path_t ce_path(p_sys);
	// pomocna struktura pro testovani, zda protipriklad je opravdu platnym behem
	vector<state_t> ce_path_vec;
        string ce_trail_file_name, ce_states_file_name;

	while (i >= ce_cycle_start && ce_vector[i].bfs_parent_part)
	  {
	    ce_path_vec.push_back(ce_vector[i].state);
	    ce_path.push_back(ce_vector[i].state);
	    i--;
	  }

 	first_ce_member_with_bfs_parent_part_false = i;
 
	if (first_ce_member_with_bfs_parent_part_false < ce_cycle_start)
	  {
	    gerr << net_id << ": "
		 << "No cycle states in ce_vector!" << thr();
	  }

	i = cycle_state_on_lowest_level;

	if (i < ce_cycle_start)
	  {
	    gerr << net_id << ": "
		 << "Erroneous cycle_state_on_lowest_level or ce_cycle_start!" << thr();
	  }

	bool first_path_cycle_state_marked = false;

	while (i >= ce_cycle_start)
	  {
	    if (ce_vector[i].bfs_parent_part)
	      {
		gerr << net_id << ": "
		     << "Cycle state has bfs_parent_part = true!" << thr();
	      }
	    

	    ce_path_vec.push_back(ce_vector[i].state);
    	    ce_path.push_back(ce_vector[i].state);
	    if (!first_path_cycle_state_marked)
	      {
		first_path_cycle_state_marked = true;
		ce_path.mark_cycle_start_back();
	      }
	    i--;
	  }


	i = first_ce_member_with_bfs_parent_part_false;

	while (i >= cycle_state_on_lowest_level)
	  {
	    if (ce_vector[i].bfs_parent_part)
	      {
		gerr << net_id << ": "
		     << "Cycle state has bfs_parent_part = true!" << thr();
	      }
	    

	    ce_path_vec.push_back(ce_vector[i].state);
	    if (i != cycle_state_on_lowest_level)
	      {
		// zacatek cyklu se na konec path_t znovu nepridava, path_t to pochopi
		// sama podle oznaceni zacatku cyklu a prislusnou transition tam dopise
		ce_path.push_back(ce_vector[i].state);
	      }
	    else
	      {
		// zde je vhodne misto na spocitani delky protiprikladu
		ce_length = ce_path.length();
		if (p_sys->get_succs(ce_vector[i].state, tmp_succs) == SUCC_DEADLOCK)
		  {
		    ce_length--;
		  }

		for (size_t j = 0; j < tmp_succs.size(); j++)
		  {
		    delete_state(tmp_succs[j]);
		  }
	      }
	    i--;
	  }

	// otestovani zda protipriklad je opravdu platnym behem
	for (size_t i = 1; i < ce_path_vec.size(); i++)
	  {
	    p_sys->get_succs(ce_path_vec[i - 1], successors);

	    bool succ_found = false;
	    
	    for (size_t j = 0; j < successors.size(); j++)
	      {
		successor = successors[j];

		if (successor == ce_path_vec[i])
		  {
		    succ_found = true;
		  }

		delete_state(successor);
	      }

	    if (!succ_found)
	      {
		gerr << net_id << ": "
		     << "Incorrect successor in ce_path_vec: " << i << "!" << thr();
	      }
	  }
	
	fstream ce_path_fs, ce_states_fs;

	if (produce_ce_trail_file)
	  {
	    if (basename_specified)
	      {
		ce_trail_file_name = file_basename + COUNTER_EXAMPLE_PATH_FILE_EXTENSION;
	      }
	    else
	      {
		ce_trail_file_name = file_base_name + COUNTER_EXAMPLE_PATH_FILE_EXTENSION;
	      }

	    ce_path_fs.open(ce_trail_file_name.c_str(), fstream::out);

	    if (!ce_path_fs)
	      {
		gerr << net_id << ": "
		     << "Unable to write the counterexample to file!" << thr();
	      }
	    
	    if (p_sys->can_system_transitions())
	      ce_path.write_trans(ce_path_fs);
	    else
	      ce_path.write_states(ce_path_fs);
	    
	    ce_path_fs.close();
	  }
	if (produce_ce_states_file)
	  {
	    if (basename_specified)
	      {
		ce_states_file_name = file_basename + COUNTER_EXAMPLE_FILE_EXTENSION;
	      }
	    else
	      {
		ce_states_file_name = file_base_name + COUNTER_EXAMPLE_FILE_EXTENSION;
	      }
	    ce_states_fs.open(ce_states_file_name.c_str(), fstream::out);
	    
	    if (!ce_states_fs)
	      {
		gerr << net_id << ": "
		     << "Unable to write the counterexample to file!" << thr();
	      }

	    ce_path.write_states(ce_states_fs);
	    
	    ce_states_fs.close();
	  }


	for (size_t i = 0; i < ce_path_vec.size(); i++)
	  {
	    delete_state(ce_path_vec[i]);
	  }

	delete_state(ce_repeated_state);
      }
    
    if (produce_log)
      {
	logger.stop_SIGALRM();
	logger.log_now();
      }

    
    // ulozime si koncove casy a vypocitame rozdily od pocatecnich casu
    ticks_end = times(&tms_end);
    gettimeofday(&tv_end, NULL);
    ticks = ticks_end - ticks_start;
    processor_times = (tms_end.tms_utime - tms_start.tms_utime) + 
      (tms_end.tms_stime - tms_start.tms_stime);

    if (tv_end.tv_usec > tv_start.tv_usec)
      {
	real_time_sec = tv_end.tv_sec - tv_start.tv_sec;
	real_time_usec = tv_end.tv_usec - tv_start.tv_usec;
      }
    else
      {
	real_time_sec = tv_end.tv_sec - tv_start.tv_sec - 1;
	real_time_usec = 1000000 - tv_start.tv_usec + tv_end.tv_usec;
      }

    distributed.set_idle();

    while (!distributed.synchronized(time_info))
      {
	distributed.process_messages();
      }

//MESSAGE//    delete bl_send_buf;
//MESSAGE//    delete send_buf;

    all_succs_count = time_info.data.all_succs_count;
    all_full_expands = time_info.data.all_full_expands;
    all_proviso_forced_expands = time_info.data.all_proviso_forced_expands;

    if (print_detailed_comm_info)
      {
    
	pcomm_matrix_t cm;

	distributed.network.get_comm_matrix_snm(cm, NETWORK_ID_MANAGER);

	if (net_id == NETWORK_ID_MANAGER)
	  {
	    cout << "Sent normal messages matrix:" << endl;
	    print_comm_matrix(cm);
	  }

	distributed.network.get_comm_matrix_rnm(cm, NETWORK_ID_MANAGER);

	if (net_id == NETWORK_ID_MANAGER)
	  {
	    cout << "Received normal messages matrix:" << endl;
	    print_comm_matrix(cm);
	  }

	distributed.network.get_comm_matrix_sum(cm, NETWORK_ID_MANAGER);

	if (net_id == NETWORK_ID_MANAGER)
	  {
	    cout << "Sent urgent messages matrix:" << endl;
	    print_comm_matrix(cm);
	  }

	distributed.network.get_comm_matrix_rum(cm, NETWORK_ID_MANAGER);

	if (net_id == NETWORK_ID_MANAGER)
	  {
	    cout << "Received urgent messages matrix:" << endl;
	    print_comm_matrix(cm);
	  }
 
	distributed.get_comm_matrix_ssm(cm, NETWORK_ID_MANAGER);

	if (net_id == NETWORK_ID_MANAGER)
	  {
	    cout << "Sent sync messages matrix:" << endl;
	    print_comm_matrix(cm);
	  }

	distributed.get_comm_matrix_rsm(cm, NETWORK_ID_MANAGER);

	if (net_id == NETWORK_ID_MANAGER)
	  {
	    cout << "Received sync messages matrix:" << endl;
	    print_comm_matrix(cm);
	  }

	{
	  int all_sync_barriers_cnt = distributed.get_all_sync_barriers_cnt();
	  int all_sent_sync_msgs_cnt = distributed.get_all_sent_sync_msgs_cnt();
	  int all_recv_sync_msgs_cnt = distributed.get_all_received_sync_msgs_cnt();
 
	  int all_barriers_cnt;
	  distributed.network.get_all_barriers_cnt(all_barriers_cnt);
	  int all_sent_msgs_cnt;
	  distributed.network.get_all_sent_msgs_cnt(all_sent_msgs_cnt);
	  int all_recv_msgs_cnt;
	  distributed.network.get_all_sent_msgs_cnt(all_recv_msgs_cnt);
	  int all_sent_normal_msgs_cnt;
	  distributed.network.get_user_sent_msgs_cnt(all_sent_normal_msgs_cnt);
	  int all_recv_normal_msgs_cnt;
	  distributed.network.get_user_received_msgs_cnt(all_recv_normal_msgs_cnt);

	  int gath_buf[cluster_size];

	  gather_ints(all_sent_msgs_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All sent msgs vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }

	  gather_ints(all_recv_msgs_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All received msgs vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }

	  gather_ints(all_sent_normal_msgs_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All sent normal msgs vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }

	  gather_ints(all_recv_normal_msgs_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All received normal msgs vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }

	  gather_ints(all_barriers_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All barriers vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }

	  gather_ints(all_sent_sync_msgs_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All sent sync msgs vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }

	  gather_ints(all_recv_sync_msgs_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All received sync msgs vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }

	  gather_ints(all_sync_barriers_cnt, gath_buf);

	  if (net_id == NETWORK_ID_MANAGER)
	    {
	      cout << "All sync barriers vector:" << endl;
	      print_gathered_ints(gath_buf);
	    }
	}
      }

//      cout << net_id << ": " 
//  	 << "Current memory load = " << st.get_mem_used() << " bytes, maximum = "
//  	 << st.get_mem_max_used() << " bytes." << endl;  

//      cout << net_id << ": "
//  	 << "Currently states stored = " << st.get_states_stored() << "." << endl;
//  	 << net_id << ": "
//  	 << "Collision blocks allocated = " << st.get_coltables() << "." << endl;

    if (print_statistics && net_id == NETWORK_ID_MANAGER)
      {	
	string ids="0: ";
	cout <<ids<<"=================================="<<endl;
	cout <<ids<<"State size = " << initial_state.size << "." << endl;	
	cout <<ids<<"Appendix size = " << sizeof(appendix_t) << "." << endl;

	if (cycle_found)
	  {
//   	    cout <<ids <<"Cycle found on level " << current_level - 1 << endl;
	  }
	else
	  {
	    cout <<ids <<"Levels count = " << current_level << endl;
	  }

	cout <<ids<<"Runtime = "<<init_timer.gettime()<<" sec"<<endl;
	if (cycle_found)
	  {
	    cout <<ids<<"Cycle found at "<<cycle_found_time<<" sec"<<endl;
	  }
	cout <<ids<<"Memory  = "<<time_info.data.mem/1024.0<<" MB"
	     <<"  (Max per workstation = "<<time_info.data.maxmem/1024.0<<" MB)"
	     <<endl;
      
	cout <<ids <<"Total number of back edges: " <<all_bl_edges_cnt << endl;
	cout <<ids <<"Total number of relevant back edges: " 
	     <<all_relevant_bl_edges_cnt << endl;
	cout <<ids
	     << "Total number of found states: " <<all_states_count << endl;
        if (POR_MODE != pm_none)
	  {
	    cout <<ids<<"Total number of fully expanded states: " <<all_full_expands << endl;
	    cout <<ids<<"Expansions forced by proviso:          " <<all_proviso_forced_expands << endl;
	  }
//  	if (cluster_size == 1)
//  	  {
//  	    // jinak hodnota neni platna
//  	    cout <<ids
//  		 << "Total number of states without bl_edges: " 
//  		 << time_info.data.states_without_bl_edges_cnt << endl;
//  	  }
	cout <<ids<<"Total number of edges explored (not distinct!!!): " 
	     <<all_succs_count << endl;
//  	cout <<ids
//  	     << "Max real time: " << time_info.data.max_real_time_sec << " seconds "
//  	     << time_info.data.max_real_time_usec	<< " microseconds." << endl;
//  	cout <<ids
//  	     << "Max processor times: " << time_info.data.max_processor_times
//  	     << " centiseconds." << endl;
//  	cout <<ids
//  	     << "Max ticks: " << time_info.data.max_ticks
//  	     << endl;
	cout <<ids<<"=================================="<<endl;
      }

    
    if (print_statistics)
      {
	distributed.network.barrier();
	cout <<net_id <<": Local Memory: "
	     <<vm.getvmsize()/1024.0 <<" MB" <<endl;
      }



    if (produce_report)
      {
	if (net_id != NETWORK_ID_MANAGER)
	  {
	    current_level = 0;
	    init_time = 0;
	    ce_length = 0;
	  }
	else
	  {
	    // Pokud se nasel cyklus tak je v current_level cislo o 1 vetsi nez cislo levelu,
            // na kterem se cyklus nasel. Pokud se cyklus nenasel, je tam pocet levelu, takze
	    // cislo posledniho dosazeneho levelu je opet o 1 mensi.
	    current_level = current_level - 1;
	  }

	fstream report_file;
	string report_fn;
	if (basename_specified)
	  {
	    report_fn = file_basename + REPORT_FILE_EXTENSION;
	  }
	else
	  {
	    report_fn = file_base_name + REPORT_FILE_EXTENSION;
	  }
	report_file.open(report_fn.c_str(), fstream::out);

	string problem = ((synchronized_reachability_only)?("SSGen (level-synchronized)"):("LTL MC"));
	reporter.set_obligatory_keys(cmd_line.str(), argv[optind], problem, st.get_states_stored(), succs_count);
	reporter.set_info("Back-level edges found", bl_edges_cnt);
	reporter.set_info("InitTime", init_time);
	reporter.set_info("States", st.get_states_stored());
	reporter.set_info("Trans", trans);
	reporter.set_info("CrossTrans", cross_trans);

	if (net_id == NETWORK_ID_MANAGER) //set global information to reporter
	  {
	    if (!synchronized_reachability_only)
	      {
		if (cycle_found)
		  {
		    reporter.set_global_info("IsValid", "No");
		    if (produce_ce_trail_file || produce_ce_states_file)
		      {
			reporter.set_global_info("Counterexample length (in edges)", ce_length);
			reporter.set_global_info("CEGenerated", "Yes");
		      }
		    else
		      reporter.set_global_info("CEGenerated", "No");
		  }
		else
		  {
		    reporter.set_global_info("IsValid", "Yes");
		    reporter.set_global_info("CEGenerated", "No");
		  }
	      }
	    reporter.set_global_info("Level reached (counted from 0)", current_level);
	  }
			  
	reporter.stop_timer();
	reporter.collect_and_print(REPORTER_OUTPUT_LONG, report_file);

	report_file.close();
      }
    
    delete p_sys;
    distributed.finalize();    
  }
  catch (...)
    {
      cout << net_id << ": "
	   << "Error!" << endl;
      
      try
	{
	  distributed.network.abort();
	}
      catch (...)
	{
	  return 1;
	}
      return 1;
    }
  
  return 0;
}

















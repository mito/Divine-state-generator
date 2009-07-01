#include <divine.h>
#include <getopt.h>
#include <sstream>
#include <fstream>
#include <string>
#include "shared_global.hh"
#include "process_messages.hh"
#include "update_distance.hh"
#include "statistics.hh"
#include "counterexample.hh"

using namespace divine;
using namespace std;

bool compiled_generator = false;

static struct option longopts[] = {
  { "printstats",    no_argument, 0, 'S'},
  { "partfunc",      required_argument, 0, 'P'},
  { "help",          no_argument, 0, 'h'},
  { "htsize",        required_argument, 0, 'H'},
  { "trail",         no_argument, 0, 't'},
  { "precompile",    no_argument, 0, 'p'},
  { "statelist",     no_argument, 0, 'c'},
  { "report",        no_argument, 0, 'r'},
  { NULL, 0, NULL, 0 }
};

static void print_version()
{
  cout <<"version 1.0 build 4 (2006/09/20 17:00)"<<endl;
}


static void print_usage(const char * const program_name)
{
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"Negative Cycle Detection ";
  print_version();
  cout <<"--------------------------------------------------------------"<<endl;
  
  cout << "Usage: [mpirun -np N] divine.negative_cycle_detection [parameters] "
    "input_file" << endl;
  cout <<
    "Options:\n"
    "-S, --printstats         print some statistics\n"
    "-h, --help               show this help\n"
    "-p, --precompile         DVE file is precompiled prior verification\n"
    "-Hx, --htsize x          set the size of hash table to ( x<33 ? 2^x : x )\n"
    "-t, --trail              produce counterexample trail file\n"
    "-c, --statelist          produce counterexample list of states\n"
    "-L,--log                 perform logging\n"
    "-r,--report              produce report\n"
    "-Xw,--basename w         sets base name of produced files to w\n"
    "                         (w.trail,w.report,w.00-w.N)\n"
    "-Y                       reserved for GUI\n"
    "-Z                       reserved for GUI"
       << endl;
}

//computes a global string variable file_name_base (it is set to input_file_name
//without an extension)
static void compute_file_name_base(const char * const input_file_name)
{
 file_name_base = input_file_name;
 size_t pos = 0;
 size_t last_pos = string::npos;
 do
  {
   last_pos = pos;
   pos = file_name_base.find(".", last_pos+1);
  }
 while (pos!=string::npos);
 
 if (last_pos!=0)
   file_name_base.erase(last_pos, file_name_base.size()-last_pos);
}

//parses an command line and sets various boolean options according
//to it
static void parse_command_line(int argc, char ** argv)
{
 char c;
 while ((c = getopt_long(argc, argv, "LX:YZSpP:hH:tcr", longopts, NULL)) != -1)
  {
   switch (c)
    {
    case 'L': should_produce_log = true; break;
    case 'X': file_name_base_specified = true; given_file_name_base = optarg; break;
    case 'S': should_print_statistics=true; break;
    case 'h':
      if (distributed.is_manager())
        {
	  print_usage(argv[0]);
	  distributed.finalize();
	  exit(0);
        }
      break;
    case 'H': hash_table_size = atol(optarg); break;
    case 'p': compiled_generator = true; break;
    case 't': search_for_counterexample = true; should_produce_trail = true;
      break;
    case 'c': search_for_counterexample = true;
      should_produce_state_list = true;break;
    case 'r': should_produce_report=true; break;
    }
  }
 compute_file_name_base(argv[argc-1]);
}

static void initialize_distributed(int & argc, char **& argv)
{
 distributed.process_user_message = process_message;
 distributed.network_initialize(argc,argv);
 distributed.initialize();
 distributed.set_proc_msgs_buf_exclusive_mem(false);
}

static void initialize_storage()
{
 if (hash_table_size > 0)
  {
   if (hash_table_size < 33) //sizes 1..32 interpret as 2^1..2^32
    {
     int z = hash_table_size;
     hash_table_size = 1;
     for (;z>0; z--)
      {
       hash_table_size = 2* hash_table_size;
      }
    }
   Storage.set_ht_size(hash_table_size);
  }
 else gerr << "Wrong size of a hash table: " << hash_table_size << thr();
 Storage.set_appendix(appendix);
 Storage.init();
}

static void initialize_logger()
{
  logger.set_storage(&Storage);
  if (file_name_base_specified)
    {
      logger.init(&distributed,given_file_name_base);
    }
  else
    {
      logger.init(&distributed,file_name_base);
    }
  logger.use_SIGALRM(1);
}

static slong_int_t weight_given_by_state(state_t v)
{
 if (p_System->is_accepting(v)) return -1;
 else return 0;
}

static void scan(state_t v, const state_ref_t v_ref,
                 const slong_int_t distance_of_v, const bool first_visit)
{
 succs_calls++;
 int succs_result = p_System->get_succs(v,*p_succs);
 slong_int_t new_distance = distance_of_v + weight_given_by_state(v);
 statist_number_of_transitions += p_succs->size();//statistics infomations
 
 if (!succs_normal(succs_result))
   if (succs_error(succs_result))
     gerr << "Found an error state during state space exploration" << thr();
 
 //for each (v,u) from E:
// debfile << "\n\nnumber of successors:" << p_succs->size() << endl;
 for (size_int_t i=0; i<p_succs->size(); ++i) //i. e. for all successors of v...
  {
   update_distance_or_resend_task((*p_succs)[i], distributed.network_id, v_ref,
                                  new_distance, first_visit);
   delete_state((*p_succs)[i]);
  }
}

int main(int argc, char ** argv)
{
 try
  {
   timeinfo_t timer;   

   initialize_distributed(argc, argv);
   parse_command_line(argc, argv);

   // let us be user friendly
   if (argc < optind+1)
     {
       if (distributed.is_manager())
	 {
	   print_usage("");
	 }
       distributed.finalize();
       return 0;
     }  

   if (should_produce_report)
     {
       reporter.set_info("InitTime", timer.gettime());
       reporter.start_timer();
     }

//   ostringstream debfile_name;
//   debfile_name << "debfile." << distributed.network_id;
//   debfile.open(debfile_name.str().c_str());
   
   /* decisions about the type of an input */
   system_description_t system_desc;
   p_System = system_desc.open_system_from_file(argv[optind],
						compiled_generator,
						(distributed.network_id==NETWORK_ID_MANAGER)&&
 						should_print_statistics
						);

   //reading of input:
   int file_opening;
   try
     {
       if ((file_opening=p_System->read(argv[argc-1]))&&(distributed.is_manager()))
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

   if (p_System->get_with_property() && p_System->get_property_type()!=BUCHI)
     gerr << "Cannot work with other than standard Buchi accepting condition."<< thr();
   
//    if ((should_produce_trail) && (!p_System->get_abilities().system_can_transitions) && (distributed.network_id == 0))
//      {
//        cout << "Model of system can't handle transitions, trail won't be produced." << endl;
//        should_produce_trail = false;
//      }

   //continuing in the initialization
   initialize_storage();
   
   if (should_produce_log) initialize_logger();

   
   succ_container_t succs(*p_System);
   p_succs = &succs;
   
   state_t initial_state = p_System->get_initial_state();
   initiator_id = distributed.partition_function(initial_state);
   
   if (distributed.network_id==initiator_id)
    {
     //inserting initial state to storage with its appendix
     Storage.insert(initial_state, initial_state_ref);
     statist_reached_states++;
     //setting parameters of initial state to the appendix:
     appendix.distance = 0;
     appendix.origins_process = NIL_STAMP;
     appendix.in_queue_or_first_fisit=3; //state in the queue and it is visited for the first time
     appendix.parents_process = NO_PARENT;
     appendix.parent = initial_state_ref;
     Storage.set_app_by_ref(initial_state_ref, appendix);
     
     //pushing a reference to the initial state to the queue of states:
     state_queue.push(initial_state_ref);
    }
   //else state queue stays empty until state from another computer arrives
   delete_state(initial_state);
   
   if (distributed.is_manager())
     cout << "Searching for accepting cycles..." << endl;

   succs_calls=0;

   distributed.set_busy();
   state_ref_t v_ref;
   state_t v;
   //main loop:
   while (!distributed.synchronized(statistics_info))
    {
     distributed.process_messages();
     
     if (state_queue.empty())
#if defined(ORIG_FLUSH)
       { if (!is_idle) { distributed.set_idle(); is_idle=true; }
#else
       { if (!is_idle) { distributed.set_idle(); is_idle=true; }
         // still need to do an explicit flush here, since process_messages()
	 // may have enqueued messages for other nodes
	 distributed.network.flush_some_buffers();
       }
#endif
     else
       if (is_idle && !message_finish_sent) 
         { distributed.set_busy(); is_idle=false; }

     if (!is_idle)
      {
  //       debfile << "queue size: " << state_queue.size();
       v_ref = pop_from_state_queue(appendix);
       v = Storage.reconstruct(v_ref);  //retrieving a state from storage using ref.
       scan(v, v_ref, appendix.distance, (appendix.in_queue_or_first_fisit>=2));
       if (appendix.in_queue_or_first_fisit>=2)
	 {
	   appendix.in_queue_or_first_fisit-=2;
	   Storage.set_app_by_ref(v_ref, appendix);
	 }
       delete_state(v); //`v' stays in a Storage, we delete its local copy
      }
    }
   distributed.set_busy();

   float report_here_consumed_memory;
   if (should_produce_report)
    {
      report_here_consumed_memory = get_consumed_memory();
      ostringstream ostr;
      ostr << "negative_cycle_detection";
      for (int i=1; i<argc-1; ++i) ostr << ' ' << argv[i];
      reporter.set_alg_name(ostr.str());
      reporter.set_file_name(argv[argc-1]);
      reporter.set_problem("LTL MC");
      reporter.set_obligatory_keys(ostr.str(), argv[argc-1], "LTL MC", statist_reached_states, succs_calls);

      reporter.set_info("States", statist_reached_states);
      reporter.set_info("Trans", statist_reached_trans);
      reporter.set_info("CrossTrans", statist_reached_cross_trans);
      reporter.set_info("Used transitions", statist_number_of_transitions);
      reporter.set_info("Number of walks to root", statist_number_of_walks_to_root);
      reporter.set_info("States returned to queue", statist_number_of_returning_states_to_queue);
      reporter.set_info("Number of updates of distances", statist_number_of_updates);
      reporter.set_info("Amortization constant", walk_to_root_amortization_bound);
      reporter.set_info("Size of state queue", statist_maximal_queue_size);
      if  (distributed.is_manager())
	{
	  if (negative_cycle_found)
	    {
	      reporter.set_global_info("IsValid","No");
	      if (search_for_counterexample)
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
    }
       
   
   //reconstruction of counterexample
   timeinfo_t counterexample_timer;
   if (search_for_counterexample && negative_cycle_found)
    {
     statist_maximal_queue_size=0;
     
     //collecting states of negative cycle:
     distributed.process_user_message =
       process_message_about_counterexample_reconstruction;
     negative_cycle_reconstruction = true;
     p_used_state_container = &negative_cycle;
     
     if (distributed.is_manager())
      {
       cout << "Creating the counterexample..." << endl;
       cout << "    ... reconstructing the counterexample's cycle" << endl;
       distributed.network.send_urgent_message(0,0,negative_cycle_founder_id,
                           MESSAGE_NEGATIVE_CYCLE_INITIATE_RECONSTRUCTION);
      }
     
     distributed.set_busy();
     while (!distributed.synchronized()) distributed.process_messages();
     
     //reachability of s cycle:
     distributed.process_user_message =
       process_message_about_counterexamples_path_to_cycle_search;
     if (distributed.is_manager())
      {
       if (negative_cycle_number_of_received_states!=
           negative_cycle.size())
         gerr << "Unexpected error: Manager has not received all states from "
                 "a negative cycle" << thr();
       cout << "    ... searching for the path to the counterexample's cycle"
            << endl;
      }
     
     distributed.set_busy();
     is_idle = false;
     message_finish_sent = false;
     
     while (!state_queue.empty()) pop_from_state_queue(appendix);
     if (distributed.network_id == initiator_id)
      {
       state_t initial_state = Storage.reconstruct(initial_state_ref);
       counterexample_push_to_queue(initial_state,NO_PARENT,initial_state_ref);
       delete_state(initial_state);
      }
      
     while (!distributed.synchronized())
      {
       distributed.process_messages();
     
       if (state_queue.empty())
         { if (!is_idle) { distributed.set_idle(); is_idle=true; } }
       else
         if (is_idle && !message_finish_sent) 
           { distributed.set_busy(); is_idle=false; }

       if (!is_idle)
        {
         v_ref = pop_from_state_queue(appendix);
         //retrieving a state from storage using ref.
         v = Storage.reconstruct(v_ref);
         counterexample_scan(v, v_ref);
         delete_state(v); //`v' stays in a Storage, we delete its local copy
        }
      }
     
     if (distributed.is_manager())
       if (!message_finish_sent)
         gerr << "Unexpected error: prematurely finished reachability of"
                 " a counterexample's cycle" << thr();
     
     //path to the cycle collection:
     negative_cycle_reconstruction=false;
     p_used_state_container = &path_to_negative_cycle;
     distributed.process_user_message =
       process_message_about_counterexample_reconstruction;
     distributed.set_busy();
     
     if (distributed.is_manager())
      {
       cout << "    ... collecting the path to the counterexample's cycle"
            << endl;
       distributed.network.send_urgent_message(0,0,
                         path_to_negative_cycle_founder_id,
                         MESSAGE_NEGATIVE_CYCLE_INITIATE_RECONSTRUCTION);
      }
     
     while (!distributed.synchronized(counterexample_info))
       distributed.process_messages();
     
     if (distributed.is_manager())
      {
       if (path_to_negative_cycle_number_of_received_states!=
           path_to_negative_cycle.size())
         gerr << "Unexpected error: Manager has not received all states from "
                 "a path to the negative cycle" << thr();
      }
      
     
     
     if (distributed.is_manager())
      {
       cout << "    ... printing a counterexample to file" << endl;
       path_t counterexample_path(p_System);
       array_t<state_t>
         counterexample_path_array(path_to_negative_cycle.size()+negative_cycle.size()-1);
       
       state_t intersection = duplicate_state(path_to_negative_cycle[0]);
       for (size_int_t i=path_to_negative_cycle.size(); i!=0; --i)
        {
         counterexample_path.push_back(path_to_negative_cycle[i-1]);
         counterexample_path_array.push_back(path_to_negative_cycle[i-1]);
        }
       counterexample_path.mark_cycle_start_back();
       bool intersection_found=false;
       size_int_t intersection_index=0;
       for (size_int_t i=0; i!=negative_cycle.size() && !intersection_found;++i)
         if (negative_cycle[i]==intersection)
          { intersection_found=true; intersection_index=i; }
       if (!intersection_found)
         gerr << "Unexpected error: An intersection of a path to cycle and"
                 " a cycle not found" << thr();
       
       delete_state(intersection);
       
       //in following 2 cycles we push all cycle's states except for an
       //intersection:
       for (size_int_t i=intersection_index; i!=0; --i)
        {
         counterexample_path.push_back(negative_cycle[i-1]);
         counterexample_path_array.push_back(negative_cycle[i-1]);
        }
       for (size_int_t i=negative_cycle.size(); i!=intersection_index+1; --i)
        {
         counterexample_path.push_back(negative_cycle[i-1]);
         counterexample_path_array.push_back(negative_cycle[i-1]);
        }
       
       delete_state(negative_cycle[intersection_index]); //the state is not used
                                                         //thus it is freed
       
       ofstream output_file;

       if (should_produce_trail)
        {
	  
	  if (file_name_base_specified)
	    {
	      output_file.open((given_file_name_base+TRAIL_EXTENSION).c_str());
	    }
	  else
	    {
	      output_file.open((file_name_base+TRAIL_EXTENSION).c_str());
	    }

         if (!output_file)
           gerr << "Unable to produce a trail to "
                << (file_name_base+TRAIL_EXTENSION) << psh();
         else
          {
	    if (p_System->can_system_transitions())
	      counterexample_path.write_trans(output_file);
	    else
	      counterexample_path.write_states(output_file);
           output_file.close();
          }
        }

       if (should_produce_state_list)
        {
	  if (file_name_base_specified)
	    {
	      output_file.open((given_file_name_base+STATE_LIST_EXTENSION).c_str());
	    }
	  else
	    {
	      output_file.open((file_name_base+STATE_LIST_EXTENSION).c_str());
	    }

         if (!output_file)
           gerr << "Unable to produce a state list to "
                << (file_name_base+TRAIL_EXTENSION) << psh();
         else
          {
	    counterexample_path.write_states(output_file);
	    output_file.close();
          }
        }
       
       counterexample_path.erase();
       for (size_int_t i=0; i!=counterexample_path_array.size(); ++i)
         delete_state(counterexample_path_array[i]);
      }

     if (should_produce_report)
      {
       float ce_here_consumed_memory = get_consumed_memory();

       if  (distributed.is_manager())
	 {
	   if (negative_cycle_found)
	     {
	       reporter.set_global_info("IsValid","No");
	       if (search_for_counterexample)
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
      
       reporter.set_info("Counterexample consumed memory", ce_here_consumed_memory - report_here_consumed_memory);
       reporter.set_info("Counterexample exploration queue size", statist_maximal_queue_size);
      }
    }
    
   if (should_produce_log)
     {
       logger.log_now();
       logger.stop_SIGALRM();
     }       
   
    //finally manager produces a report to the file
    if (should_produce_report)
     {
       string report_file_name;
       if (file_name_base_specified)
	 {
	   report_file_name = given_file_name_base + REPORT_EXTENSION;
	 }
       else
	 {
	   report_file_name = file_name_base + REPORT_EXTENSION;
	 }

      ofstream report_file;
      reporter.stop_timer(); //all workstations stop their timers
      if (distributed.is_manager())
       {
        report_file.open(report_file_name.c_str());
        if (!report_file)
          gerr << "Unable to produce a report to " << report_file_name
               << psh();
        else
         {
          cout << "    ... printing a report to file" << endl;
          reporter.collect_and_print(REPORTER_OUTPUT_LONG, report_file);
          report_file.close();
         }
       }
      else reporter.collect_and_print(REPORTER_OUTPUT_LONG, report_file);
     }
      
   //finishing a distributed computation
   distributed.finalize();
   
   //printing a result and statistics
   if (distributed.is_manager())
    {
     cout << "\n=================== Result: ====================\n" << endl;
     if (negative_cycle_found) cout << "Accepting cycle found." << endl;
     else cout << "No accepting cycle found." << endl;
   
     if (should_print_statistics)
      {
       cout << "\n============ Algorithm statistics: =============\n" << endl;
       cout << "Runtime                           = " << timer.gettime() << endl;
       cout << "Consumed memory                   = " << statistics_info.data.consumed_memory << " MB" << endl;
       cout << "Maximal memory on single computer = " << statistics_info.data.maximal_consumed_memory << " MB" << endl;
       cout << "Reached states                    = " << statistics_info.data.reached_states << endl;
       state_t s=p_System->get_initial_state();
       cout << "Size of the initial state         = " <<s.size<<endl;
       cout << "Size of appendix                  = " <<sizeof(appendix)<<endl;
       delete_state(s);
       cout << "Used transitions                  = " << statistics_info.data.number_of_transitions << endl;
       cout << "Number of walks to root           = " << statistics_info.data.number_of_walks_to_root << endl;
       cout << "States returned to queue          = " << statistics_info.data.number_of_returning_states_to_queue << endl;
       cout << "Number of updates of distances    = " << statistics_info.data.number_of_updates << endl;
       cout << "Maximal height of amort. const.   = " << statistics_info.data.maximal_amortization << endl;
       cout << "Maximal size of state queue       = " << statistics_info.data.maximal_queue_size << endl;
       if (search_for_counterexample && negative_cycle_found)
        {
         cout << "\n========== Counterexample statistics: ==========\n" << endl;
         cout << "Runtime                           = " << counterexample_timer.gettime() << endl;
         cout << "Additionally consumed memory      = " << counterexample_info.data.consumed_memory - statistics_info.data.consumed_memory << " MB" << endl;
         cout << "Maximal size of state queue       = " << counterexample_info.data.maximal_queue_size << endl;
         cout << "Length of counterexample's cycle  = " << negative_cycle.size() << endl;
         cout << "Length of entire counterexample   = " << negative_cycle.size() + path_to_negative_cycle.size() - 1  << endl;
        }
      }
     cout << "\n================================================\n" << endl;
    }
   delete p_System;
  }
 catch (ERR_throw_t & err_type)
  {
   cerr << "Program finishes." << endl;
   return err_type.type;
  }
 
 return 0;
}



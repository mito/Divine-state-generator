#include <iostream>
#include <fstream>
#include <getopt.h>
#include "sevine.h"
#include "reachability_observer.hh"
#include "parameters.hh"
#include "search.hh"

using namespace std;
using namespace divine;

dve_explicit_system_t System(gerr);
reachability_observer_t* observer;
reporter_t reporter;
parameters_t parameters;
search_t* search_alg=NULL;
string goal="", goal_file="", input_name="", coverage_predicates_file="", search_alg_name="", bfs_level_output="";
int time_limit=0, vm_limit=0;
bool help = false, quiet = false, report = false, verbose = false, concise = false;
ostringstream oss1;
timeinfo_t timer;

void read_options(int argc, char** argv) {
  int c;
  
  static struct option longopts[] = {
    { "htsize", required_argument, 0, 'H' },
    { "goal", required_argument, 0, 'g' },
    { "verbose", no_argument,0, 'v' },
    { "search", required_argument, 0, 's' },
    { "cov_predicates", required_argument, 0, 'c' },
    { "first_goal", no_argument, 0, 'f'},
    { "special", required_argument, 0, 1 },
    { "time_limit", required_argument, 0, 2},
    { "vm_limit", required_argument, 0, 3},
    { "order", required_argument, 0, 4},      
    { "goals", required_argument,  0, 5 },
    { "concise", no_argument, 0, 6},
    { "bfs_level_output", required_argument, 0, 7 },
    { NULL, 0, NULL, 0 }
  };

  oss1<<"reachability";
  while ((c = getopt_long(argc, argv, "hvqrpH:s:fl:g:c:x:y:z:b:", longopts, NULL)) != -1)
    {
      oss1 <<" -"<<(char)c;
      switch (c) {
      case 'h': help = true; break;
      case 'v': verbose = true; break;
      case 'q': quiet = true; break;
      case 'r': report = true; break;
      case 'p': parameters.paths = true; break;
      case 'H': parameters.htsize = atoi(optarg); break;
      case 'f': parameters.first_goal = true; break;
      case 'g': goal = optarg; break;
      case 5: goal_file = optarg; break;
      case 'c': coverage_predicates_file = optarg; break;
      case 's': search_alg_name = optarg; break;
      case 'x': parameters.x = atoi(optarg); break;
      case 'y': parameters.y = atoi(optarg); break;
      case 'z': parameters.z = atoi(optarg); break;
      case 1: parameters.special_file = optarg; break;
      case 2: time_limit = atoi(optarg); break;
      case 3: vm_limit = atoi(optarg); break;
      case 4:
	if (strcmp(optarg, "bfs")==0) parameters.order = BFS;
	if (strcmp(optarg, "dfs")==0) parameters.order = DFS;
	break;
      case 6: concise = true; break;
      case 7: bfs_level_output = optarg; break;
      default:
	cout << "Unknown option: "<<c<<endl;
	break;
      }
    }

  if (optind < argc) input_name = argv[optind]; 
  
}

void print_usage() {
  cout << "Usage: reach [options] input.dve"<<endl;
  cout << "\n The standard output of reachability contains information about the size of the state space, about error and deadlock states, and about the coverage of the model."<<endl;
  cout << endl;
  cout << "Warning: this is not completely stable version; report any strange behaviour to Radek"<<endl;
  cout << endl;
  cout << "General options:"<<endl;
  cout << "-h\n\t\thelp"<<endl;
  cout << "-v --verbose\n\tverbose (reports unreachable code, etc.)"<<endl;
  cout << "-q\n\tquiet (suppress standard output)"<<endl;
  cout << "-r\n\tprint report"<<endl;
  cout << "-p\n\toutput paths to interesting (deadlock, error, goal) states"<<endl;
  cout << "-f --first_goal\n\tstop after the first interesting state is found"<<endl;
  cout << "-g --goal ARG\n\texpression describing the goal state"<<endl;
  cout << "--goals ARG\n\tname of file which contains list of expressions describing goal states (one expression on each line)"<<endl;
  cout << "-c --cov_predicates ARG\n\tname of file which contains list of expressions which are used as predicates for measuring predicate coverage"<<endl;
  cout << "--time_limit ARG\n\ttime limit (in seconds); after the given time, the search is terminated"<<endl;
  cout << "--vm_limit ARG\n\tlimit on virtual memory size (in MB) after reaching the given memory consumption, the search is terminated"<<endl;
  cout << "-s --search ARG\n\tthe name of search strategy to be used, one of 'full', 'rw', 'alpha', 'bithash'"<<endl;
  cout << endl;
  cout << "Search specific options (these options work only with some search strategies, resp. have"<<endl;
  cout << " different meaning for each strategy):"<<endl;
  cout << endl;
  cout << "--order ARG\n\teither 'bfs' or 'dfs'"<<endl;
  cout << "--special ARG\n\tname of file with some strategy-specific information"<<endl;
  cout << "--bfs_level_output ARG\n\t arg=name of a file; information about number of states on BFS levels is outputed into this file"<<endl;
  cout << "-H --htsize ARG\n\tsize of the hash table"<<endl;
}

void read_goals() {
  vector<expression_t*> goals;
  if (goal != "") {
    dve_expression_t* expr = new dve_expression_t(goal, &System);
    goals.push_back(expr);
  }
  if (goal_file != "") {
    ifstream input(goal_file.c_str());
    string ex;
    while (getline(input, ex)) {
      dve_expression_t* expr = new dve_expression_t(ex, &System);
      goals.push_back(expr);
    }
  }
  observer->set_goals(goals);
}

void read_cov_predicates() {
  vector<expression_t*> predicates;
  if (coverage_predicates_file != "") {
    ifstream input(coverage_predicates_file.c_str());
    string ex;
    while (getline(input, ex)) {
      dve_expression_t* expr = new dve_expression_t(ex, &System);
      predicates.push_back(expr);
    }
    observer->set_predicates(predicates);
  }  
}

int main(int argc, char** argv) {


  read_options(argc, argv);
  if (input_name == "" || help) {
    print_usage();
    return 0;
  }

  try {
    System.read(input_name.c_str());
    observer = new reachability_observer_t(&System, time_limit, vm_limit);
    read_goals();
    read_cov_predicates();
    
    if (search_alg_name == "por") 
      search_alg = new por_search_t(&System, &parameters, observer);
    else if (search_alg_name == "rw") 
      search_alg = new rw_search_t(&System, &parameters, observer);      
    else if (search_alg_name == "alpha") 
      search_alg = new alpha_search_t(&System, &parameters, observer);      
    else if (search_alg_name == "bithash") 
      search_alg = new bithash_search_t(&System, &parameters, observer);      
//    else if (search_alg_name == "ril") 
//      search_alg = new ril_search_t(&System, &parameters, observer);      
    else {
      if (search_alg_name != "" && search_alg_name != "full") cerr << "Warning: "<<search_alg_name<< " not recognized as search algorithm. Using standard search."<<endl;
      search_alg_name = "full";
      if (bfs_level_output != "") search_alg = new full_search_t(&System, &parameters, observer, bfs_level_output);
      else search_alg = new full_search_t(&System, &parameters, observer);
    }
    

    ofstream output;
    output.open("tempout0");
    observer->set_output(&output);
    if (verbose) observer->set_verbose();

    if (report)
      {
	reporter.set_info("InitTime", timer.gettime());
	reporter.start_timer();
      }

    search_alg->do_search();

    output.close();
    if (report)
      {
	reporter.stop_timer();
	reporter.set_obligatory_keys(oss1.str(), input_name, "Safety", observer->get_states(), observer->get_states());
	reporter.set_info("Speed", (int) observer->get_states()/reporter.get_time());
	reporter.set_info("State size", System.get_space_sum());
	reporter.print(cout);
      }
    if (!quiet && observer->limit_reached())
      cout << "WARNING: limit on resources was reached; the search is not complete!"<<endl;
    if (!quiet) observer->print(parameters.paths);
    if (concise) cout << "Concise Answer:"<< observer->goal_found() <<endl;

  } catch (ERR_throw_t & err_type) {
    return 1;
  }  
  
  return 0;
}

#ifndef NDFS_HH
#define NDFS_HH

#include "sevine.h"
#include <deque>
#include <stack>
#include <vector>

/*
 *  Reference:
 *  Stefan Schwoon and Javier Esparza. A note on on-the-fly
 *  verification algorithms.  In Nicolas Halbwachs and Lenore Zuck,
 *  editors, Proceedings of the 11th International Conference on
 *  Tools and Algorithms for the Construction and Analysis of Systems
 *  (TACAS), volume 3440 of Lecture Notes in Computer Science, pages
 *  174--190, Edinburgh, UK, April 2005. Springer.
 */

namespace divine {

typedef enum {
    UNEXPANDED = 0,
    EXPANDED = 1,
    BLUE = 2,
    RED = 3,
} state_status_t;

typedef pair<state_ref_t,bool> stack_entry_t;

class ndfs_t {
public:
    ndfs_t (explicit_system_t& system);
    ndfs_t (explicit_system_t& system, explicit_storage_t& storage);
    virtual ~ndfs_t ();
    bool find_accepting_cycle (state_t &seed);
    bool find_accepting_cycle (state_ref_t &seed_ref);
    bool find_accepting_cycle (succ_container_t& seeds);
    deque<state_ref_t>& get_accepting_cycle() { return lasso; };
private:
    bool nested_dfs (state_ref_t& seed_ref);
    state_status_t get_state_status (state_ref_t& state_ref);
    void set_state_status (state_ref_t& state_ref, state_status_t status);
    void reconstruct_accepting_cycle();
    
    explicit_system_t& system;
    // XXX copy constructor and operator=().  Such a pain.  
    // Maybe assume ownership of visited instead?
    auto_ptr<explicit_storage_t> visited;
    bool free_visited;
    stack<stack_entry_t, vector<stack_entry_t> > path_stack;
    stack<stack_entry_t, vector<stack_entry_t> > cycle_stack;
    deque<state_ref_t> lasso;
};

   
ndfs_t::ndfs_t (explicit_system_t& system)
    : system (system)
    , visited (new explicit_storage_t())
    , free_visited (true)
{
    visited->set_appendix_size (sizeof (short));
    visited->init();
}

ndfs_t::ndfs_t (explicit_system_t& system, explicit_storage_t& visited)
    : system (system)
    , visited (&visited)
    , free_visited (false)
{}

ndfs_t::~ndfs_t() 
{
    if (!free_visited) {
        visited.release();
    }
}        
    
bool
ndfs_t::find_accepting_cycle (succ_container_t& states)
{
    for (succ_container_t::iterator i = states.begin(); i != states.end(); ++i) {
        if (find_accepting_cycle (*i)) {
            return true;
        }
        delete_state (*i);
    }
    return false;
}

bool
ndfs_t::find_accepting_cycle (state_t& seed_state) 
{
    state_ref_t seed_ref;
    visited->insert (seed_state, seed_ref);
    return find_accepting_cycle (seed_ref);
}

bool
ndfs_t::find_accepting_cycle (state_ref_t& seed_ref) 
{
    succ_container_t succs (system);

    path_stack.push (stack_entry_t (seed_ref, false));
    while (!path_stack.empty()) {
        stack_entry_t& stack_entry = path_stack.top();
        state_t state = visited->reconstruct (stack_entry.first);
        if (!stack_entry.second) {
            stack_entry.second = true;
            set_state_status (stack_entry.first, EXPANDED);
            system.get_succs (state, succs);
            
            for (succ_container_t::iterator i = succs.begin(); i != succs.end(); ++i) {
                state_ref_t state_ref;
                if (!visited->is_stored_if_not_insert (*i, state_ref)) {
                    set_state_status (state_ref, UNEXPANDED);
                }
                state_status_t status = get_state_status (state_ref);
                if (status == EXPANDED
                    && (system.is_accepting (state) || system.is_accepting (*i))) {
                    path_stack.push (stack_entry_t (state_ref, true));
                    reconstruct_accepting_cycle();
                    return true;
                } else if (status == UNEXPANDED) {
                    path_stack.push (stack_entry_t (state_ref, false));
                }
                delete_state (*i);
            }
        } else {
            // BLUE subgraph search complete
            state_ref_t state_ref = stack_entry.first;
            path_stack.pop();
            if (system.is_accepting (state) && nested_dfs (state_ref)) {
                reconstruct_accepting_cycle();
                return true;
            } else {
                set_state_status (state_ref, BLUE);
            }
        }
        delete_state (state);
    }
    return false;
}

bool
ndfs_t::nested_dfs (state_ref_t& state_ref)
{
    succ_container_t succs (system);
    cycle_stack.push (stack_entry_t (state_ref, false));
    while (!cycle_stack.empty()) {
        stack_entry_t& stack_entry = cycle_stack.top();
        if (!stack_entry.second) {
            stack_entry.second = true;
            state_t state = visited->reconstruct (stack_entry.first);
            system.get_succs (state, succs);
            
            for (succ_container_t::iterator i = succs.begin(); i != succs.end(); ++i) {
                state_ref_t succ_ref;
		//                bool stored = visited->is_stored (*i, succ_ref);
		//                assert (stored);
                state_status_t status = get_state_status (succ_ref);
                if (status == EXPANDED) {
                    cycle_stack.push (stack_entry_t (succ_ref, true));
                    return true;
                } else if (status == BLUE) {
                    set_state_status (succ_ref, RED);
                    cycle_stack.push (stack_entry_t (succ_ref, false));
                }
                delete_state (*i);
            }
            delete_state (state);
        } else {
            cycle_stack.pop();
        }
    }
    // RED subgraph search complete
    set_state_status (state_ref, RED);
    
    return false;
}

void
ndfs_t::reconstruct_accepting_cycle()
{
    while (!cycle_stack.empty()) {
        if (cycle_stack.top().second) {
            lasso.push_front (cycle_stack.top().first);
        }
        cycle_stack.pop();
    }
    while (!path_stack.empty()) {
        if (path_stack.top().second) {
            lasso.push_front (path_stack.top().first);
        }
        path_stack.pop();
    }
}
    
state_status_t
ndfs_t::get_state_status (state_ref_t& state_ref)
{
    short status;
    visited->get_app_by_ref (state_ref, status);
    return (state_status_t)status;
}

void
ndfs_t::set_state_status (state_ref_t& state_ref, state_status_t status)
{
    visited->set_app_by_ref (state_ref, (short)status);
}

}

#endif

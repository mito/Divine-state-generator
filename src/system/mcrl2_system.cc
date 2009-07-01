#include <iostream>
#include <fstream>
#include <sstream>
#include "common/deb.hh"
#include "system/mcrl2_system.hh"
#include <map>
#include <cstdlib>


#ifndef DOXYGEN_PROCESSING
using namespace divine;
using namespace std;
using namespace mcrl2;
#endif //DOXYGEN_PROCESSING

bool mcrl2_system_t::binstates = true;

//Constructor - yes it is not virtual, but it is needed and it completes
//the necessary interface
mcrl2_system_t::mcrl2_system_t(error_vector_t & evect)
    : system_t(evect), nstate(NULL), mcrl2_system(NULL) 
{
    mcrl2_system = new mcrl2::lps::specification();

    get_abilities().system_can_property_process = true; 
    get_abilities().explicit_system_can_system_transitions   = true;
}

mcrl2_system_t::~mcrl2_system_t()
{
    delete nstate;
    delete mcrl2_system; 
}

slong_int_t mcrl2_system_t::read(const char * const filename)
{
    // init parameters
    char *param;
    if ((param = getenv("DIVINE_MCRL2_STATES"))) {
	if (param == string("aterm")) {
	    binstates = false;
	} else if (param != string("binary")) {
	    cerr << "Warning: DIVINE_MCRL2_STATES: unknown parameter" << endl;
	}
    }

    // load the lps system form a file (.lps)
    mcrl2_system->load( filename );

    cerr << filename << " loaded..." << endl;

    // initialize the state space generator
    nstate = createNextState(*mcrl2_system, false /* use_free_vars */,
	    GS_STATE_VECTOR, GS_REWR_JITTYC);

    /* without the use_free_vars=false, we'd get states that prettyprint as
     * the same, but have different ATerm representation:
     * (gdb) call ATprintf("%t\n", 0x826e258)
     * STATE(@REWR@(56),DataVarId("dc1",SortId("Phil")),@REWR@(70),@REWR@(56),DataVarId("dc2",SortId("Phil")),@REWR@(71),@REWR@(56),DataVarId("dc5",SortId("Phil")),@REWR@(72),@REWR@(56),@REWR@(73),@REWR@(56),@REWR@(74),@REWR@(56),@REWR@(75))
     * (gdb) call ATprintf("%t\n", 0x826e3ac)
     * STATE(@REWR@(56),DataVarId("dc",SortId("Phil")),@REWR@(70),@REWR@(56),DataVarId("dc3",SortId("Phil")),@REWR@(71),@REWR@(56),DataVarId("dc5",SortId("Phil")),@REWR@(72),@REWR@(56),@REWR@(73),@REWR@(56),@REWR@(74),@REWR@(56),@REWR@(75))
     * (gdb) call ATprintf("%t\n", 0x826e588)
     * STATE(@REWR@(56),DataVarId("dc1",SortId("Phil")),@REWR@(70),@REWR@(56),DataVarId("dc3",SortId("Phil")),@REWR@(71),@REWR@(56),DataVarId("dc4",SortId("Phil")),@REWR@(72),@REWR@(56),@REWR@(73),@REWR@(56),@REWR@(74),@REWR@(56),@REWR@(75))
     * this may or may not be desirable.
     */

    return 0;
}

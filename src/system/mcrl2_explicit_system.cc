#include <exception>
#include <cassert>
#include <bitset>
#include <sstream>

#include "system/mcrl2_explicit_system.hh"
#include "system/mcrl2_system.hh"
#include "system/mcrl2_system_trans.hh"
#include "system/state.hh"
#include "common/bit_string.hh"
#include "common/error.hh"
#include "common/deb.hh"

#include <mcrl2/lps/nextstate.h>
#include <mcrl2/core/detail/struct.h>
#include <mcrl2/core/print.h>
#include <aterm2.h>

#ifndef DOXYGEN_PROCESSING
using namespace divine;
#endif
using std::ostream;
using std::vector;
using std::cout;

mcrl2_explicit_system_t::mcrl2_explicit_system_t(error_vector_t & evect):
   system_t(evect), explicit_system_t(evect), mcrl2_system_t(evect)
{
}

mcrl2_explicit_system_t::~mcrl2_explicit_system_t()
{
}

void mcrl2_explicit_system_t::print_state(state_t state,
                                          std::ostream & outs)
{
    char *buf;
    size_t bufsz;
    FILE *sstream;

    outs << "[";

    ATerm astate = 0;
    if (mcrl2_system_t::binstates)
	astate = ::ATreadFromSAFString(state.ptr, state.size);
    else
	astate = *(reinterpret_cast<ATerm*>(state.ptr));

    for (int i = 0; i < nstate->getStateLength(); i++) {
	if (i > 0)
	    outs << ",";

	ATermAppl a = nstate->getStateArgument(astate, i);
	if (mcrl2::core::detail::gsIsDataVarId(a))
	    outs << "_";
	else {
	    // TODO is this the only way?
	    sstream = open_memstream(&buf, &bufsz);
	    mcrl2::core::PrintPart_C(sstream, (ATerm) a, mcrl2::core::ppDefault);
	    fclose(sstream);
	    outs << buf;
	    free(buf);
	}
    }
    outs << "]";
}

state_t mcrl2_explicit_system_t::get_initial_state()
{
    state_t s;

    // instanciate an aterm to represent the initial state
    ATerm mcrlstate = nstate->getInitialState();

    if (mcrl2_system_t::binstates) {
	int len = 0;
	char *b = ::ATwriteToSAFString(mcrlstate, &len);
	s = new_state(b, len);
	free(b);
    } else
	s = new_state((char*) &mcrlstate, sizeof(ATerm));

    return s;
}

int mcrl2_explicit_system_t::get_succs(state_t state, succ_container_t * succs,
	enabled_trans_container_t * etc)
{
    state_t s;

    if (succs)
	succs->clear();
    if (etc)
	etc->clear();

    ATerm astate = NULL;
    if (mcrl2_system_t::binstates)
	astate = ::ATreadFromSAFString(state.ptr, state.size);
    else
	astate = *(reinterpret_cast<ATerm*>(state.ptr));

    // initiate mcrl2 state space generator
    NextStateGenerator *nsgen = nstate->getNextStates(astate);

    ATermAppl trans = NULL;
    ATerm at = NULL;

    // generate successors
    while ( nsgen->next(&trans,&at) )
    {
	if (mcrl2_system_t::binstates) {
	    int len = 0;
	    char *b = ::ATwriteToSAFString(at, &len);
	    s = new_state(b, len);
	    free(b);
	} else
	    s = new_state((char*) &at, sizeof(ATerm));

	if (succs)
	    succs->push_back(s);
	if (etc) {
	    etc->extend(1);
	    etc->back() = mcrl2_enabled_trans_t(trans, s);
	}
    }

    delete nsgen;

    int result = 0;
    if (((succs && succs->size()==0) || (etc && etc->size()==0)) && get_with_property()==false)
	result |= SUCC_DEADLOCK;

    return result;
}

bool mcrl2_explicit_system_t::get_enabled_trans_succ
   (const state_t state, const enabled_trans_t & enabled,
    state_t & new_state)
{
    new_state = dynamic_cast<const mcrl2_enabled_trans_t*>(&enabled)->get_succ();
    return false;
}

enabled_trans_t * mcrl2_explicit_system_t::new_enabled_trans() const
{
    return (new mcrl2_enabled_trans_t);
}

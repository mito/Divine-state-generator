#include "system/state.hh"
#include <string>
#include <cstring>
#include "system/mcrl2_system.hh"
#include "common/deb.hh"

using namespace divine;

state_t::state_t()
{
  ptr = 0; size = 0;
}

bool divine::operator< (const state_t& arg1, const state_t& arg2)
{
   return ((arg1.size==arg2.size) &&
           memcmp(arg1.ptr,arg2.ptr, arg1.size) < 0);
}

bool divine::operator> (const state_t& arg1, const state_t& arg2)
{
   return ((arg1.size==arg2.size) &&
           memcmp(arg1.ptr,arg2.ptr, arg1.size) > 0);
}

bool divine::operator!= (const state_t& arg1, const state_t& arg2)
{
   return ((arg1.size!=arg2.size) ||
           memcmp(arg1.ptr,arg2.ptr,arg1.size)!= 0);
}

bool divine::operator== (const state_t& arg1, const state_t& arg2)
{
   return ((arg1.size==arg2.size) &&
           memcmp(arg1.ptr,arg2.ptr,arg1.size)== 0);
}

state_t divine::duplicate_state(state_t state)
{
  state_t retval;
  retval.ptr = new char[state.size];
  memcpy (retval.ptr,state.ptr,state.size);

#ifdef HAVE_MCRL2
  /* mcrl2 hack */
  if (!mcrl2_system_t::binstates)
    ATprotect((ATerm*) retval.ptr);
#endif

  retval.size = state.size;
  return retval;
}

void divine::clear_state(state_t arg1)
{
  memset (arg1.ptr,0,arg1.size);
}

state_t divine::new_state(const std::size_t size)
{
  state_t retval;
  retval.size = size;
  retval.ptr = new char[size];
  memset (retval.ptr,0,size);

#ifdef HAVE_MCRL2
  /* mcrl2 hack */
  if (!mcrl2_system_t::binstates)
    ATprotect((ATerm*) retval.ptr);
#endif

  return retval;
}

state_t divine::new_state(char * const state_memory, const std::size_t size)
{
  state_t retval;
  retval.size = size;
  retval.ptr = new char[size];
  memcpy (retval.ptr, state_memory, size);

#ifdef HAVE_MCRL2
  /* mcrl2 hack */
  if (!mcrl2_system_t::binstates)
    ATprotect((ATerm*) retval.ptr);
#endif

  return retval;
}

void divine::realloc_state(state_t &state, size_t new_size)
{
#ifdef HAVE_MCRL2
  /* mcrl2 hack */
  if (!mcrl2_system_t::binstates)
    ATunprotect((ATerm*) state.ptr);
#endif

  if (state.ptr)
    delete [] state.ptr;
  state.size = new_size;
  state.ptr = new char[new_size];
  memset (state.ptr,0,new_size);

#ifdef HAVE_MCRL2
  /* mcrl2 hack */
  if (!mcrl2_system_t::binstates)
    ATprotect((ATerm*) state.ptr);
#endif
}


void divine::delete_state(state_t& state)
{
#ifdef HAVE_MCRL2
  /* mcrl2 hack */
  if (!mcrl2_system_t::binstates)
    ATunprotect((ATerm*) state.ptr);
#endif

  delete [] state.ptr;
  state.size = 0;
  state.ptr = 0;
}















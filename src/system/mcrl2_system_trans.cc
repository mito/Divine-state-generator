#include "system/mcrl2_system_trans.hh"
#include "system/mcrl2_explicit_system.hh"
#include <mcrl2/core/detail/struct.h>
#include <mcrl2/core/print.h>

#ifndef DOXYGEN_PROCESSING
using namespace divine;
#endif

void mcrl2_system_trans_t::copy_from(const system_trans_t & second) {
    const mcrl2_system_trans_t *s = dynamic_cast<const mcrl2_system_trans_t*>(&second);

    trans = s->trans;
    succ = s->succ;
}

system_trans_t & mcrl2_system_trans_t::operator=(const system_trans_t & second)
{
    copy_from(second);
    return (*this);
}

std::string mcrl2_system_trans_t::to_string() const
{
    std::ostringstream ostr;
    write(ostr);
    return ostr.str();
}

void mcrl2_system_trans_t::write(std::ostream & ostr) const
{
    char *buf;
    size_t bufsz;
    FILE *sstream;

    sstream = open_memstream(&buf, &bufsz);
    mcrl2::core::PrintPart_C(sstream, (ATerm) trans, mcrl2::core::ppDefault);
    fclose(sstream);
    ostr << buf;
    free(buf);
}

enabled_trans_t & mcrl2_enabled_trans_t::operator=(const enabled_trans_t & second)
{
    mcrl2_system_trans_t::copy_from(second);
    set_erroneous(second.get_erroneous());
    return (*this);
}

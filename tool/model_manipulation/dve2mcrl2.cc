// Convert DVE models to mCRL2
// Author: Michael Weber <michaelw@cs.utwente.nl>

// TODO:
// * Monitor process
// * Buffered Channels (there are none in DVE)
// * Coercion logic is not quite right, needs refactoring

#include <iostream>
#include <string>
#include <set>
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "sevine.h"

using namespace std;

// * DVE-related global variables
divine::dve_system_t DVE_system(divine::gerr);
divine::dve_symbol_table_t *dve_symtab;

namespace mcrl
{

static const string usage =
    "Usage: dve2mcrl2 [OPTIONS]...\n"
    "dve2mcrl2 translates DVE models to mCRL2 models.\n"
    "\nOptions:\n"
    "  -s, --show=VAR       make variable VAR observable in the mCRL2 model\n"
    "  -i, --input=FILE     read DVE model from FILE (default: stdin)\n"
    "  -o, --output=FILE    write mCRL2 model to FILE (default: stdout)\n"
    "\nVariable Designators:\n"
    "  P@                   current (control) state of process `P'\n"
    "  P.x                  local variable `x' of process `P'\n"
    "  x                    global variable `x'\n"
    "\nReport bugs to <http://anna.fi.muni.cz/divine/trac/>";

static struct option options[] = {
    { "help"  , no_argument,       NULL, 'h' },
    { "output", required_argument, NULL, 'o' },
    { "input" , required_argument, NULL, 'i' },
    { "show"  , required_argument, NULL, 's' },
    { 0, 0, 0, 0 },
};

// * Data structures representing the mCRL system
//
// The data structures here pretty much follow from the structure of
// mCRL system.  They are filled by the various translate_* functions,
// and finally printed by the print_* functions.
//
// For simplicity reasons, many DVE constructs are rendered directly
// as strings, because printing them out is the only goal.
struct sort_t {
    string name;
    string constructors;

    sort_t() {}
    sort_t(string name, string constructors)
        : name(name), constructors(constructors) {}
};

class var_t {
public:
    string name;
    string type;
    string initializer;

    var_t() {}
    var_t(const var_t& var)
        : name(var.name), type(var.type), initializer(var.initializer),
          vector_element_type_(var.vector_element_type_),
          needs_coercion_(var.needs_coercion_) {}
    var_t(const divine::dve_symbol_t& dve_var, const map<string,var_t>& env);
    var_t(string name, string type, string initializer)
        : name(name), type(type), initializer(initializer),
          vector_element_type_(""), needs_coercion_(false) {}
    var_t(string name, string type, string initializer, bool needs_coercion)
        : name(name), type(type), initializer(initializer),
          vector_element_type_(""), needs_coercion_(needs_coercion) {}
    
    bool is_vector() const { return vector_element_type_ != ""; }
    
    string get_effective_type() const {
        return is_vector() ? vector_element_type_ : type;
    }
    string maybe_coerce(const string s) const {
        if (needs_coercion_) {
            string effective_type = is_vector() ? vector_element_type_ : type;
            return "coerce_" + effective_type + "(" + s + ")";
        } else {
            return s;
        }
    }
    string maybe_coerce(ssize_t imm) const;

    string get_merge_operation(const string y, const string z) const {
        return "merge_" + type + "(" + name + ", " + y + ", " + z + ")";
    }
protected:
    string vector_element_type_; // XXX attributes of type, not variable
    bool needs_coercion_;        // XXX attributes of type, not variable
};

typedef enum {
    PREFIX,
    INFIX,
    POSTFIX,
} operator_notation_t;

struct operator_t {
    string name;
    string mcrl_name;
    operator_notation_t notation;
    string return_type;
    operator_t() {}
    operator_t(string name, string mcrl_name,
               string return_type, string typeX)
        : name(name), mcrl_name(mcrl_name), notation(PREFIX),
          return_type(return_type),
          varX(var_t("x", typeX, "")) {
        declare_unary_operator();
    }
    operator_t(string name, string mcrl_name,
               string return_type, string typeX, string typeY)
        : name(name), mcrl_name(mcrl_name), notation(PREFIX),
          return_type(return_type),
          varX(var_t("x", typeX, "")),
          varY(var_t("y", typeY, "")) {
        declare_binary_operator();
    }
    operator_t(string name, operator_notation_t n, string return_type)
        : name(name), mcrl_name(name), notation(n),
          return_type(return_type) {}
    operator_t(string name, string return_type)
        : name(name), mcrl_name(name), notation(PREFIX),
          return_type(return_type) {}
protected:
    var_t varX, varY;
    void declare_unary_operator();
    void declare_binary_operator();
};

struct array_t {
    string sort;
    size_t dimension;

    array_t() {}
    array_t(string sort, size_t dimension)
        : sort(sort), dimension(dimension) {}

    string get_sort_name() const {
        ostringstream s;
        s << "Array_" << sort << "_" << dimension;
        return s.str();
    }
    string get_sort_declaration() const {
        string decl = get_sort_name() + " = struct " + get_sort_name()
            + "(";
        {
            size_t i = 0;
            if (i < dimension) goto start;
            for (; i < dimension; ++i) {
                decl += ", ";
            start:
                decl += sort;
            }
            decl += ")";
            return decl;
        }
    }
};

struct eqn_block_t {
    vector<string> decls;
    vector<var_t> variables;
    vector<string> equations;
    vector<string> rules;
};

class action_t {
public:
    string name;
    string sort;
    action_t() {}
    action_t(const action_t& a)
        : name(a.name), sort(a.sort), coaction_name_(a.coaction_name_) {}
    action_t(string name, string sort)
        : name(name), sort(sort), coaction_name_("_" + name) {}
    action_t get_coaction() const {
        return action_t(coaction_name_, sort, name);
    }
    bool operator<(const action_t& other) const {
        return name < other.name;
    }
protected:
    string coaction_name_;
    action_t(string name, string sort, string coaction_name)
        : name(name), sort(sort), coaction_name_(coaction_name) {}
};

struct comm_t {
    action_t action, coaction, comm_action;
    comm_t() {}
    comm_t(action_t action);
    comm_t(action_t action, action_t comm_action);
    comm_t(action_t action, action_t coaction, action_t comm_action)
        : action(action), coaction(coaction), comm_action(comm_action) {}
};

struct process_t {
    string name;
    string init;
    map<string,var_t> variables;        // maps DVE vars to mCRL vars
    vector<string> transitions;
    var_t state_var;
    process_t() {}
    process_t(string name) : name(name) {}
    process_t(string name, string initial_state);
    const var_t& get_state_var() { return state_var; }
};

struct system_t {
    set<string> states;
    map<string,action_t> actions;
    vector<comm_t> comms;
    set<action_t> allowed_actions;
    map<string,sort_t> sorts;
    map<string,eqn_block_t> equations;
    map<string,array_t> arrays;
    map<string,var_t> variables;        // maps DVE vars to mCRL vars
    map<string,process_t> processes;
    set<pair<string,string> > visible_variables; // <process,DVE variable>

    void add_array(const array_t& array);
    bool is_visible_variable(string process, string variable) const {
        return visible_variables.find(pair<string,string>(process,variable))
            != visible_variables.end();
    }
};

// * Forward declarations
string translate_expr (const divine::dve_expression_t& expr,
                       const map<string,var_t>& env,
                       const var_t* storage = 0,
                       bool static_init = false,
                       const map<string,string>* effect_map = 0);

// * mCRL-related global variables
system_t mcrl;
map<int,operator_t> operators;

// * Utilities
string
print_int(int i)
{
    ostringstream s;
    s << i;
    return s.str();
}

string
concat_list (string list, string sep = ", ")
// Add SEP as separator if LIST is non-empty (in order to
// concatenate it to further non-empty lists.)
{
    if (list != "") {
        return list + sep;
    } else {
        return "";
    }
}

// converting DVE to mCRL types
string
get_type (const divine::dve_var_type_t type) {
    switch (type) {
    case divine::VAR_BYTE: return "UInt8";
    case divine::VAR_INT:  return "Int16";
    default: return "UNKNOWN";
    }
}

string
get_variables_list (const map<string,var_t>& vars, bool decl = false,
                    string prefix = "", string suffix = "", string sep = ", ")
{
    string s;
    {
        map<string,var_t>::const_iterator it = vars.begin();
        if (it != vars.end()) goto start;
        for (; it != vars.end(); ++it) {
            s += sep;
        start:
            s += prefix + it->second.name + suffix;
            if (decl) {
                s += ":" + it->second.type;
            }
        }
    }
    return s;
}

string
get_initializer_list (const map<string,var_t>& vars, string sep = ", ")
{
    string s;
    map<string,var_t>::const_iterator it = vars.begin();
    if (it != vars.end()) goto start;
    for (; it != vars.end(); ++it) {
        s += sep;
    start:
        s += it->second.initializer;
    }
    return s;
}

string
maybe_group (string s, string open = "(", string close = ")")
{
    if (s == "") {
        return s;
    }
    return open + s + close;
}

string
cast_index (string index_expr) 
{
    return "Int2Nat(" + index_expr + ")";
}

ostream&
print_operator (ostream& out, int dve_operator)
{
   map<int,operator_t>::const_iterator found = operators.find(dve_operator);
   if (found != operators.end()) {
       out << found->second.name;
   } else {
       out << "?OP_" << dve_operator << "?";
       cerr << "Unknown operator: " << dve_operator << endl;
   }
   return out;
}

const var_t*
find_variable (string name, const map<string,var_t>& env)
{
    map<string,var_t>::const_iterator found = env.find(name);
    if (found != env.end()) {
        return &found->second;
    } else {
        found = mcrl.variables.find(name);
        if (found != mcrl.variables.end()) {
            return &found->second;
        }
    }
    return 0;
}

string
variable_name(string process_ident, string variable_ident)
{
    return process_ident + "'" + variable_ident;
}

string
get_state_name_by_dve_id (size_t state_id)
{
    return string("State'") + dve_symtab->get_state(state_id)->get_name();
}

ostream&
print_expr (ostream& out, const divine::dve_expression_t& expr,
            const map<string,var_t>& env,
            const var_t* storage = 0,
            bool static_init = false,
            const map<string,string>* effect_map = 0)
{
    ostringstream sout;
    string expr_type;
    switch (expr.get_operator()) {
    case T_NAT:
        out << (storage ? storage->maybe_coerce(expr.get_value())
                : expr.to_string());
        return out;
    case T_ID:{
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable(expr.get_ident_gid());
        const var_t* var = find_variable(dve_var->get_name(), env);
        assert (var != 0);
        expr_type = var->get_effective_type();
        
        string ref;
        if (static_init) {
            ref = var->initializer;
        } else if (effect_map != 0) {
            ref = (*const_cast<map<string,string>*>(effect_map))[var->name];
        } else {
            ref = var->name;
        }

        if (var->is_vector()) {
            // vector without array subscript means vector[0]
            // (thank you, SPIN <http://spinroot.com/spin/Man/arrays.html>)
            sout << "get_" << var->type << "(" << ref << ", 0)";
        } else {
            sout << ref;
        }
        break;}
    case T_PARENTHESIS:
        out << "("; print_expr(out, *expr.left(), env, storage, static_init, effect_map) << ")";
        return out;
    case T_SQUARE_BRACKETS:{
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable(expr.get_ident_gid());
        const var_t* var = find_variable(dve_var->get_name(), env);
        assert (var != 0);
        assert (var->is_vector());
        expr_type = var->get_effective_type();
        
        string ref;
        if (static_init) {
            ref = var->initializer;
        } else if (effect_map != 0) {
            ref = (*const_cast<map<string,string>*>(effect_map))[var->name];
        } else {
            ref = var->name;
        }

        ostringstream iout;
        print_expr(iout, *expr.left(), env, 0, static_init, effect_map);
        sout << "get_" << var->type << "(" << ref << ", "
             << cast_index(iout.str()) << ")";
        break;}
    case T_DOT:{
        const divine::dve_symbol_t *dve_state = dve_symtab->get_state(expr.get_ident_gid());
        const divine::dve_symbol_t *dve_proc  = dve_symtab->get_process(dve_state->get_process_gid());
        const string& state_var = mcrl.processes[dve_proc->get_name()].get_state_var().name;
        sout << "coerce_BoolInt" + maybe_group(state_var + " == " + get_state_name_by_dve_id(expr.get_ident_gid()));
        break;}
    case T_FOREIGN_ID:
    case T_FOREIGN_SQUARE_BRACKETS:
        cerr << "Unhandled operator: ";
        print_operator(cerr, expr.get_operator()) << endl;
        print_operator(sout, expr.get_operator());
        break;
    case T_UNARY_MINUS:
    case T_TILDE:
    case T_BOOL_NOT:{
        print_operator(sout, expr.get_operator());
        sout << "(" ; print_expr(sout, *expr.right(), env, 0, static_init, effect_map); sout << ")";
        map<int,operator_t>::const_iterator found = operators.find(expr.get_operator());
        assert (found != operators.end());
        expr_type = found->second.return_type;
        break;}
    default:
        map<int,operator_t>::const_iterator found = operators.find(expr.get_operator());
        if (found != operators.end()) {
            expr_type = found->second.return_type;
            switch(found->second.notation) {
            case INFIX:
                sout << "(";
                print_expr(sout, *expr.left(), env, 0, static_init, effect_map);
                sout << " " << found->second.name << " ";
                print_expr(sout, *expr.right(), env, 0, static_init, effect_map);
                sout << ")";
                break;
            case PREFIX:
                sout << found->second.name << "(";
                print_expr(sout, *expr.left(), env, 0, static_init, effect_map);
                sout << ", ";
                print_expr(sout, *expr.right(), env, 0, static_init, effect_map);
                sout << ")";
                break;
            default:
                throw "Unknown operator notation";
            }
        } else {
            sout << "?OP_" << expr.get_operator() << "?";
            cerr << "Unknown operator: " << expr.get_operator() << endl;
        }
    }
        
    if (storage && storage->type != expr_type) {
        out << storage->maybe_coerce(sout.str());
    } else {
        out << sout.str();
    }
    return out;
}

void
emit_merge_operation (string sort)
{
    string merger = "merge_" + sort;
    eqn_block_t var_type_block;
    var_type_block.decls.push_back(merger + " : " + sort + "#" + sort
                                   + "#" + sort + " -> " + sort);
    var_t varX("x", sort, "");
    var_t varY("y", sort, "");
    var_type_block.variables.push_back(varX);
    var_type_block.variables.push_back(varY);
    var_type_block.equations.push_back(merger + "(" + varX.name + ", " + varX.name + ", " + varY.name + ") = " + varY.name);
    var_type_block.equations.push_back(merger + "(" + varX.name + ", " + varY.name + ", " + varX.name + ") = " + varY.name);
    mcrl.equations[sort] = var_type_block;
}

void
parse_visible_variables(const set<string>& vars)
{
    for(set<string>::const_iterator it = vars.begin();
        it != vars.end(); ++it) {
        size_t found;
        string proc, var;
        if ((found = it->find('.')) != string::npos) {
            proc = it->substr(0, found);
            var = variable_name(proc , it->substr(found+1));
        } else if ((found = it->find('@')) != string::npos) {
            proc = "MEM'";
            var = "state'" + it->substr(0, found);
        } else {                        // global (shared) variable
            proc = "MEM'";
            var = variable_name(proc, *it);
        }
        mcrl.visible_variables.insert(pair<string,string>(proc, var));
    }
}


// * Constructors (and functions called by constructors)
process_t::process_t (string name, string initial_state)
        : name(name), state_var(var_t("state'" + name, "State", initial_state)) {
        mcrl.variables[name] = state_var;
}

void
operator_t::declare_unary_operator()
{
    eqn_block_t op_block;
    var_t temp("RETVAL", return_type, "", return_type == "Int32"
               || return_type == "BoolInt"); // XXX
    op_block.variables.push_back(varX);
    op_block.decls.push_back(name + " : " + varX.type + " -> " + return_type);
    var_t tempX("X", "Bool", "", varX.type == "BoolInt");
    string rhs = mcrl_name + " " + tempX.maybe_coerce(varX.name);
    op_block.equations.push_back(name + "(" + varX.name + ")"
                                 + " = " + temp.maybe_coerce(rhs));
    mcrl.equations["Operator " + name] = op_block;
}

void
operator_t::declare_binary_operator()
{
    eqn_block_t op_block;
    var_t temp("RETVAL", return_type, "", return_type == "Int32"
               || return_type == "BoolInt"); // XXX
    op_block.variables.push_back(varX);
    op_block.variables.push_back(varY);
    op_block.decls.push_back(name + " : " + varX.type + " # " + varY.type + " -> " + return_type);
    var_t tempX("X", "Bool", "", varX.type == "BoolInt");
    var_t tempY("Y", "Bool", "", varY.type == "BoolInt");
    string rhs = tempX.maybe_coerce(varX.name)
        + " " + mcrl_name + " "
        + tempY.maybe_coerce(varY.name);

    if (mcrl_name == "div") {
        // XXX div(-2^31,-1) is probably not defined in signed m32 arithmetic
        op_block.equations.push_back(string("(") + varY.name + ">0) -> " + name + "(" + varX.name + ", " + varY.name + ")"
                                     + " = " + varX.name + " " + mcrl_name + " Int2Pos(" + varY.name + ")");
        op_block.equations.push_back(string("(") + varY.name + "<0) -> " + name + "(" + varX.name + ", " + varY.name + ")"
                                     + " = (-" + varX.name + ") " + mcrl_name + " Int2Pos(-" + varY.name + ")");
    } else {
        op_block.equations.push_back(name + "(" + varX.name + ", " + varY.name + ")"
                                     + " = " + temp.maybe_coerce(rhs));
    }
    mcrl.equations["Operator " + name] = op_block;
}

comm_t::comm_t(action_t action)
    : action(action),
      coaction(action.get_coaction()),
      comm_action(action_t("__" + action.name, action.sort))
{
    mcrl.actions[action.name] = action;
    mcrl.actions[coaction.name] = coaction;
    mcrl.actions[comm_action.name] = comm_action;
}

comm_t::comm_t(action_t action, action_t comm_action)
    : action(action),
      coaction(action.get_coaction()),
      comm_action(comm_action)
{
    mcrl.actions[action.name] = action;
    mcrl.actions[coaction.name] = coaction;
    mcrl.actions[comm_action.name] = comm_action;
}

var_t::var_t (const divine::dve_symbol_t& dve_var,
              const map<string,var_t>& env)
// Creates a variable.  Things are slightly complicated by the need to
// special-case variables of vector type.
{
    if (dve_var.get_process_gid() == divine::NO_ID) {
        name = variable_name("MEM'", dve_var.get_name());
    } else {
        const divine::dve_symbol_t *dve_proc  = dve_symtab->get_process(dve_var.get_process_gid());
        assert (dve_proc != 0);
        name = variable_name(dve_proc->get_name(), dve_var.get_name());
    }

    needs_coercion_ = true;
    if (dve_var.is_vector()) {
        array_t arr(get_type(dve_var.get_var_type()), dve_var.get_vector_size());
        mcrl.add_array(arr);
        type = arr.get_sort_name();
        vector_element_type_ = arr.sort;
        string init = type + "(";
        {
            size_t i = 0;
            if (i < dve_var.get_vector_size()) goto start;
            for (; i < dve_var.get_vector_size(); ++i) {
                init += ", ";
            start:
                const divine::dve_expression_t *expr =
                    i < dve_var.get_init_expr_count() ? dve_var.get_init_expr(i) : 0;
                var_t temp("TEMP", arr.sort, "", true);
                init += expr ? translate_expr(*expr, env, &temp, true) : "0";
            }
        }
        init += ")";
        initializer = init;
    } else {
        needs_coercion_ = true;
        const divine::dve_expression_t *expr = dve_var.get_init_expr();
        type = get_type(dve_var.get_var_type());
        initializer = expr ? translate_expr(*expr, env, this, true) : "0";       
        emit_merge_operation(type);
    }
}


string
var_t::maybe_coerce(ssize_t imm) const
{
    string type = get_effective_type();
    if (type == "UInt8" && imm >= 0 && imm < 256) {
        return print_int(imm);
    } else if (type == "Int16" && imm >= -32768 && imm < 32768) {
        return print_int(imm);
    } else if (type == "Int32" && imm >= INT_MIN && imm < INT_MAX) {
        return print_int(imm);
    } else if (needs_coercion_) {
        return "coerce_" + type + maybe_group(print_int(imm));
    }
    return print_int(imm);
}

void
system_t::add_array (const array_t& array)
// Adds type of array to mCRL spec.  Arrays (resp. vectors) are not
// built into the mCRL language, so for each element type used, we
// need to add some boilerplate code: getters, setters, merge
// operation (to handle DVE's synchronization semantics), etc..  Each
// array gets its own equation block (along with type and variable
// declarations used in the equations.)
{
    string array_sort = array.get_sort_name();
    arrays[array_sort] = array;
    eqn_block_t eqn_block;
    const string index_sort = "Nat";
    eqn_block.decls.push_back("get_" + array_sort + " : "
                              + array_sort + " # " + index_sort + " -> " + array.sort);
    eqn_block.decls.push_back("set_" + array_sort + " : "
                              + array_sort + " # " + index_sort + " # " + array.sort
                              + " -> " + array_sort);

    eqn_block.decls.push_back("merge_" + array_sort + " : "
                              + array_sort + " # " + array_sort + " # " + array_sort
                              + " -> " + array_sort);

    var_t varA("a", array_sort, "");
    var_t varB("b", array_sort, "");
    eqn_block.variables.push_back(varA);
    eqn_block.variables.push_back(varB);
    eqn_block.variables.push_back(var_t("i", index_sort, ""));
    eqn_block.variables.push_back(var_t("j", index_sort, ""));
    var_t varX("x", array.sort, "");
    eqn_block.variables.push_back(varX);
    eqn_block.variables.push_back(var_t("v", array.sort, ""));
    eqn_block.variables.push_back(var_t("w", array.sort, ""));

    string arg = array_sort + "(";
    string argY = arg;
    string argZ = arg;
    {
        size_t i = 0;
        if (i < array.dimension) goto start;
        for (; i < array.dimension; ++i) {
            arg  += ", ";
            argY += ", ";
            argZ += ", ";
        start:
            var_t varXi("x" + print_int(i), array.sort, "");
            var_t varYi("y" + print_int(i), array.sort, "");
            var_t varZi("z" + print_int(i), array.sort, "");
            arg  += varXi.name;
            argY += varYi.name;
            argZ += varZi.name;
            eqn_block.variables.push_back(varXi);
            eqn_block.variables.push_back(varYi);
            eqn_block.variables.push_back(varZi);
        }
    }
    arg += ")"; argY += ")"; argZ += ")";

    for (size_t j = 0; j < array.dimension; ++j) {
        string getter = "get_" + array_sort + "("
            + arg + ", " + print_int(j)
            + ") = " + "x" + print_int(j);
        eqn_block.equations.push_back(getter);
    }
    for (size_t j = 0; j < array.dimension; ++j) {
        string setter = "set_" + array_sort + "("
            + arg + ", " + print_int(j)
            + ", x) = " + array_sort + "(";
        size_t k = 0;
        goto start1;
        for (; k < array.dimension; ++k) {
            setter += ", ";
        start1:
            if (j == k) {
                setter += varX.name;
            } else {
                var_t var("x" + print_int(k), array.sort, "");
                setter += var.name;
            }
        }
        setter += ")";
        eqn_block.equations.push_back(setter);
    }

    string merger_rhs = "merge_" + array_sort + "("
        + arg + ", " + argY + ", " + argZ + ")";
    string args;
    for (size_t k = 0; k < array.dimension; ++k) {
        var_t varXi("x" + print_int(k), array.sort, "");
        var_t varYi("y" + print_int(k), array.sort, "");
        var_t varZi("z" + print_int(k), array.sort, "");
        args = concat_list(args, ", ")
            + varXi.get_merge_operation(varYi.name, varZi.name);
    }
    eqn_block.equations.push_back(merger_rhs + " = "
                                  + array_sort + maybe_group(args));

    // rules
    string merger1 = "merge_" + array_sort + "("
        + varA.name + ", " + varB.name + ", " + varA.name + ")";
    eqn_block.rules.push_back(merger1 + " = " + varB.name);
    string merger2 = "merge_" + array_sort + "("
        + varA.name + ", " + varA.name + ", " + varB.name + ")";
    eqn_block.rules.push_back(merger2 + " = " + varB.name);

    string setter = "set_" + array_sort;
    string getter = "get_" + array_sort;
    string setter_i_1 = setter + "(b,i,v)";
    string setter_j_1 = setter + "(" + setter_i_1 +",j,w)";

    string setter_j_2 = setter + "(b,j,w)";
    string setter_i_2 = setter + "(" + setter_j_2 +",i,v)";
    string lhs1 = setter_j_1 + " == " + setter_i_2;
    eqn_block.rules.push_back(lhs1 + " = " + "i != j || v == w");

    string getter_A_j = getter + "(" + setter_i_1 + ", j)";
    string getter_b_j = getter + "(b, j)";
    string eq2 = "w == " + getter_A_j
        + " = ((i == j && w == v) || (i != j && w == "
        + getter_b_j + "))";
    eqn_block.rules.push_back(eq2);

    string eq3 = getter_A_j + " = " + "if(i == j, v, " + getter_b_j + ")";
    eqn_block.rules.push_back(eq3);

    mcrl.equations[array_sort] = eqn_block;
    emit_merge_operation(array.sort);
}

string
gensym (string s = "G'")
{
    static int gensym_counter = 0;
    return "tmp'" + s + print_int(gensym_counter++);
}

void
add_effect (const divine::dve_expression_t& lvalue,
            const divine::dve_expression_t& right,
            map<string,var_t>& env,
            map<string,string>& effect_map,
            vector<var_t>& single_assigns)
// Adds side effect to the EFFECT_MAP of a transition.  EFFECT_MAP
// maps (mCRL) variable names to an expression which computes the
// updated value.
{
    switch (lvalue.get_operator()) {
    case T_ID:
    case T_FOREIGN_ID:{
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable(lvalue.get_ident_gid());
        const var_t* var = find_variable (dve_var->get_name(), env);
        assert (var != 0);
        if (var->is_vector()) {
            // vector lvalue without array subscript means vector[0]
            // (thank you, SPIN <http://spinroot.com/spin/Man/arrays.html>)
            var_t tmp(gensym(var->name), var->type, 
                      "set_" + var->type + "("
                      + effect_map[var->name] + ", 0, "
                      + translate_expr(right, env, var) + ")");
            effect_map[var->name] = tmp.name;
            single_assigns.push_back(tmp);
        } else {
            var_t tmp(gensym(var->name), var->type,
                      translate_expr(right, env, var, false, &effect_map));
            effect_map[var->name] = tmp.name;
            single_assigns.push_back(tmp);
            assert(translate_expr(lvalue, env) == var->name);
        }
        break;}
    case T_SQUARE_BRACKETS:
    case T_FOREIGN_SQUARE_BRACKETS:{
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable(lvalue.get_ident_gid());
        const var_t* var = find_variable (dve_var->get_name(), env);
        assert (var != 0);
        assert (var->is_vector());
        var_t tmp(gensym(var->name), var->type,
                  "set_" + var->type + "("
                  + effect_map[var->name] + ", "
                  + cast_index(translate_expr(*lvalue.left(), env)) + ", "
                  + translate_expr(right, env, var, false, &effect_map) + ")");
        effect_map[var->name] = tmp.name;
        single_assigns.push_back(tmp);
        break;}
    default:
        cerr << "Unexpected lvalue: " << lvalue.to_string() << endl;
    }
}

void
add_effect (const divine::dve_expression_t& lvalue,
            string value,
            map<string,var_t>& env,
            map<string,string>& effect_map,
            vector<var_t>& single_assigns)
// Adds side effect to the EFFECT_MAP of a transition.  EFFECT_MAP
// maps variable names to an expression which computes the updated
// value.
{
    switch (lvalue.get_operator()) {
    case T_ID:
    case T_FOREIGN_ID:{
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable(lvalue.get_ident_gid());
        const var_t* var = find_variable (dve_var->get_name(), env);
        assert(var != 0);
        if (var->is_vector()) {
            // vector lvalue without array subscript means vector[0]
            // (thank you, SPIN <http://spinroot.com/spin/Man/arrays.html>)
            var_t tmp(gensym(var->name), var->type, 
                      "set_" + var->type + "("
                      + effect_map[var->name] + ", 0, "
                      + var->maybe_coerce(value) + ")");
            effect_map[var->name] = tmp.name;
            single_assigns.push_back(tmp);
        } else {
            var_t tmp(gensym(var->name), var->type, var->maybe_coerce(value));
            effect_map[var->name] = tmp.name;
            single_assigns.push_back(tmp);
            assert(translate_expr(lvalue, env) == var->name);
        }
        break;}
    case T_SQUARE_BRACKETS:
    case T_FOREIGN_SQUARE_BRACKETS:{
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable(lvalue.get_ident_gid());
        const var_t* var = find_variable (dve_var->get_name(), env);
        assert (var != 0);
        assert (var->is_vector());
        var_t tmp(gensym(var->name), var->type,
                  "set_" + var->type + "("
                  + effect_map[var->name] + ", "
                  + cast_index(translate_expr(*lvalue.left(), env)) + ", "
                  + var->maybe_coerce(value) + ")");
        effect_map[var->name] = tmp.name;
        single_assigns.push_back(tmp);
        break;}
    default:
        cerr << "Unexpected lvalue: " << lvalue.to_string() << endl;
    }
}

// * Translation functions
//
// Translation functions pretty much follow from the data structures
// declared above.  First, all necessary information from the DVE
// system is acquired and stored in the data structures.  At the end,
// the data structures are printed as mCRL output.
string
translate_expr (const divine::dve_expression_t& expr,
                const map<string,var_t>& env,
                const var_t* storage,
                bool static_init,
                const map<string,string>* effect_map)
{
    ostringstream s;
    print_expr (s, expr, env, storage, static_init, effect_map);
    return s.str();
}

void
translate_transition (process_t& process, const divine::dve_transition_t& dve_trans)
// effects on global variables are translated by synchronization action
//   read_write_sync(V, V',V'', msg)
// where V is a vector of global variables, and msg is a message
// transmitted via channel (rendezvous) synchronization.
//
// 1) no synchronization
//    sum V._read_write_sync(V, V, V', 0)
//    
// 2) reader half of synchronization:
//    sum V1,V2,var._read_write_sync(V1, V2, V1', var)
//    
// 3) writer half of synchronization:
//    sum V1,V2._read_write_sync(V1, V1', V2, value)
//
// effect after 2)+3): updated global variables, var := value
{
    string guard;
    const var_t& state_var = process.get_state_var();
    guard += "(" + state_var.name + " == "
        + get_state_name_by_dve_id(dve_trans.get_state1_gid());
    if (dve_trans.get_guard()) {
        var_t guard_var("TEMP", "Bool", "", true);
        string dve_guard = translate_expr(*dve_trans.get_guard(), process.variables);
        if (dve_guard != "1") { // "true"
            guard += " && " + guard_var.maybe_coerce(dve_guard);
        }
    }
    guard += ")";

    map<string,string> effect_map;
    for (map<string, var_t>::iterator it = mcrl.variables.begin();
         it != mcrl.variables.end(); ++it) {
        effect_map[it->second.name] = it->second.name;
    }
    for (map<string, var_t>::iterator it = process.variables.begin();
         it != process.variables.end(); ++it) {
        effect_map[it->second.name] = it->second.name;
    }

    // state
    effect_map[state_var.name] = get_state_name_by_dve_id(dve_trans.get_state2_gid());

    vector<var_t> single_assigns;
    
    // sync
    string message = "m'";
    if (!dve_trans.is_without_sync()) {
        assert (dve_trans.get_sync_expr_list_size() <= 1); // XXX sync exprs > 1
        if (dve_trans.is_sync_ask()) {
            if (dve_trans.get_sync_expr_list_size() > 0) {
                add_effect(*dve_trans.get_sync_expr_list_item(0), message,
                           process.variables, effect_map, single_assigns);
            }
        } else if (dve_trans.is_sync_exclaim()) {
            message = "0";
            if (dve_trans.get_sync_expr_list_size() > 0) {
                message = translate_expr(*dve_trans.get_sync_expr_list_item(0),
                                         process.variables);
            }
        } else {
            unsigned int unknown_synchronization = 0;
            assert(unknown_synchronization);
        }
    }
    
    // effect
    for (size_t i = 0; i != dve_trans.get_effect_count(); i++) {
        const divine::dve_expression_t *expr = dve_trans.get_effect (i);
        assert (expr->get_operator() == T_ASSIGNMENT);
        add_effect(*expr->left(), *expr->right(), process.variables,
                   effect_map, single_assigns);
    }

    // emit effects as mCRL2
    string local_effect;
    {
        map<string, var_t>::iterator it = process.variables.begin();
        if (it != process.variables.end()) goto start;
        for (; it != process.variables.end(); ++it) {
            local_effect += ", ";
        start:
            local_effect += effect_map[it->second.name];
        }
    }

    string effect;
    {
        map<string, var_t>::iterator it = mcrl.variables.begin();
        if (it != mcrl.variables.end()) goto start1;
        for (; it != mcrl.variables.end(); ++it) {
            effect += ", ";
        start1:
            effect += effect_map[it->second.name];
        }
    }

    string tmp_var_decls;
    string pseudo_assignments;
    for (vector<var_t>::const_iterator it = single_assigns.begin();
         it != single_assigns.end(); ++it) {
        tmp_var_decls = concat_list(tmp_var_decls, ". ")
            + "sum " + it->name + ":" + it->type;
        pseudo_assignments = concat_list(pseudo_assignments, " && ")
            + maybe_group(it->name + " == " + it->initializer);
    }
    
    guard = concat_list(tmp_var_decls, ".\n    ")
        + maybe_group(concat_list(pseudo_assignments, " &&\n    ")
                      + guard);

    string transition = guard + " -> ";
    
    string global_var_list = get_variables_list (mcrl.variables, false);
    string global_var_list_ = get_variables_list (mcrl.variables, false, "", "'");
    if (dve_trans.is_without_sync()) {
        const action_t& sync_action = mcrl.actions["read_write_sync"];
        transition += " " + sync_action.get_coaction().name + "("
            + concat_list(global_var_list) + concat_list(global_var_list)
            + concat_list(effect) + "0" + ")."; // don't use message
    } else {
        action_t sync_action = mcrl.actions[dve_trans.get_sync_channel_name()];
        if (dve_trans.is_sync_ask())
            sync_action = sync_action.get_coaction();
        string channel = sync_action.name;
        if (dve_trans.is_sync_ask()) {
            transition += " " + channel
                + "(" + concat_list(global_var_list) + concat_list(global_var_list_)
                + concat_list(effect) + message + ").";
        } else if (dve_trans.is_sync_exclaim()) {
            transition += " " + channel
                + "(" + concat_list(global_var_list) + concat_list(effect)
                + concat_list(global_var_list_) + message + ").";
        } else {
            unsigned int unknown_synchronization = 0;
            assert(unknown_synchronization);
        }
    }
    transition += process.name;
    if (local_effect != "")
        transition += "(" + local_effect + ")";
    process.transitions.push_back(transition);
}

void
check_variable_visibility (process_t& process)
{
    string var_list = get_variables_list(process.variables);

    for (map<string,var_t>::iterator it = process.variables.begin();
         it != process.variables.end(); ++it) {
        const var_t& var = it->second;
        if (mcrl.is_visible_variable(process.name, var.name)) {
            action_t show_act("SHOW'"+process.name+"_"+it->first, var.type);
            mcrl.actions[show_act.name] = show_act;
            mcrl.allowed_actions.insert(show_act);
            process.transitions.push_back(show_act.name+maybe_group(var.name)
                                          + "." + process.name + maybe_group(var_list));
        }
    }
}

void
register_process (const divine::dve_process_t& dve_process)
{
    for (size_t i = 0; i < dve_process.get_state_count(); ++i) {
        size_t state = dve_process.get_state_gid(i);
        mcrl.states.insert(get_state_name_by_dve_id(state));
    }

    string initial_state =
        get_state_name_by_dve_id(dve_process.get_state_gid(dve_process.get_initial_state()));
    process_t process(dve_symtab->get_process (dve_process.get_gid())->get_name(),
                      initial_state);
    mcrl.processes[process.name] = process;
}

void
translate_process (const divine::dve_process_t& dve_process)
{
    process_t& process = mcrl.processes[dve_symtab->get_process (dve_process.get_gid())->get_name()];
    
    string var_sums  = get_variables_list (mcrl.variables, true, "sum ", "", ". ");
    string var_sums_ = get_variables_list (mcrl.variables, true, "sum ", "'", ". ");
    process.init = concat_list(var_sums, ". ")
        + concat_list(var_sums_, ". ")
        + "sum " + "m'" + ":Message. ";

    for (size_t i = 0; i < dve_process.get_variable_count(); ++i) {
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable (dve_process.get_variable_gid(i));
        var_t var(*dve_var, process.variables);
        process.variables[dve_var->get_name()] = var;
    }

    check_variable_visibility(process);    
    for (size_t i = 0; i < dve_process.get_trans_count(); ++i) {
        const divine::dve_transition_t* tr =
            dynamic_cast<const divine::dve_transition_t*>(dve_process.get_transition(i));
        translate_transition (process, *tr);
    }
}

void
register_global_variables()
{
    for (size_t i = 0; i < DVE_system.get_global_variable_count(); i++) {
        const divine::dve_symbol_t *dve_var = dve_symtab->get_variable (DVE_system.get_global_variable_gid(i));
        var_t var(*dve_var, map<string,var_t>());
        mcrl.variables[dve_var->get_name()] = var;
    }
}
    
void
translate_global_variables()
// Translates global variables.  Since mCRL does not support global
// (shared) variables, they are simulated by creating a process with
// local variables, which reacts on actions to retrieve and update
// their values.
{
    process_t mem("MEM'");
    mem.variables = mcrl.variables;
    string var_sums = get_variables_list(mcrl.variables, true, "sum ", "'", ". ");
    string var_list = get_variables_list(mcrl.variables);
    string new_var_list = get_variables_list(mcrl.variables, false, "", "'");
    string sync_var_sums = get_variables_list (mcrl.variables, true, "sum ", "''", ". ");
    string sync_var_list = get_variables_list (mcrl.variables, false, "", "''");
    mem.init = concat_list(var_sums, ". ");

    string combined_var_list;
    {
        map<string,var_t>::iterator it = mcrl.variables.begin();
        if (it != mcrl.variables.end()) goto start;
        for (; it != mcrl.variables.end(); ++it) {
            combined_var_list += ", ";
        start:
            combined_var_list += "\n      ";
            if (it->second.is_vector()) {
                combined_var_list += "merge_" + it->second.type + "("
                    + it->second.name
                    + ", " + it->second.name + string("'")
                    + ", " + it->second.name + string("''")
                    + ")";
            } else {
                combined_var_list +=
                    it->second.get_merge_operation(it->second.name + "'",
                                                   it->second.name + "''");
            }
        }
    }

    check_variable_visibility(mem);
    
    const sort_t& msg_sort = mcrl.sorts["Message"];
    var_t msg_var("m'", msg_sort.name, "");
    string msg_sum = "sum " + msg_var.name + ":" + msg_var.type;
    // read_write_sync not set up yet in mcrl.actions
    string transition = concat_list(sync_var_sums, ". ") + concat_list(msg_sum, ". ")
        + string("read_write_sync") + "("
        + concat_list(var_list) + concat_list(new_var_list)
        + concat_list(sync_var_list) + msg_var.name + ")."
        + mem.name + maybe_group(combined_var_list);

    mem.transitions.push_back(transition);

    mcrl.processes[mem.name] = mem;
}

void
register_actions()
// register stub actions, details are filled in later with
// `translate_actions'.
{
    action_t rws("read_write_sync", "FAKE_SORT");
    comm_t rws_comm(rws); // enough to declare it

    for (size_t i = 0; i < DVE_system.get_channel_count(); ++i) {
        string name = dve_symtab->get_channel(i)->get_name();
        action_t act(name, rws.sort);
        mcrl.actions[act.name] = act;
        mcrl.comms.push_back(comm_t(act, rws.get_coaction()));
    }
}

void
translate_actions()
{
    string sort;
    map<string,var_t>::iterator it = mcrl.variables.begin();
    if (it != mcrl.variables.end()) goto start;
    for (; it != mcrl.variables.end(); ++it) {
        sort += "#";
    start:
        sort += it->second.type;
    }

    const sort_t& msg_sort = mcrl.sorts["Message"];
    string rw_sync_type;
    if (sort != "") {
        rw_sync_type = sort + "#" + sort + "#" + sort + "#";
    }
    rw_sync_type += msg_sort.name;

    // provide _real_ action information (overwrites previously
    // registered stub actions)
    mcrl.comms.clear();
    action_t rws("read_write_sync", rw_sync_type);
    //mcrl.actions[rws.name] = rws;
    comm_t rws_comm(rws); // enough to declare it
    mcrl.allowed_actions.insert(rws_comm.comm_action);

    for (size_t i = 0; i < DVE_system.get_channel_count(); ++i) {
        string name = dve_symtab->get_channel(i)->get_name();
        action_t act(name, rws.sort);
        mcrl.actions[act.name] = act;
        mcrl.comms.push_back(comm_t(act, rws.get_coaction()));
    }
}

void
translate_base_sorts()
{
    sort_t byte_sort("UInt8", "Nat");
    sort_t int16_sort("Int16", "Int");
    sort_t int32_sort("Int32", "Int");
    sort_t boolint_sort("BoolInt", "Int32");
    sort_t msg_sort("Message", "Int");
    mcrl.sorts[byte_sort.name] = byte_sort;
    mcrl.sorts[int16_sort.name]  = int16_sort;
    mcrl.sorts[int32_sort.name]  = int32_sort;
    mcrl.sorts[boolint_sort.name] = boolint_sort;
    mcrl.sorts[msg_sort.name]  = msg_sort;

    eqn_block_t base_eqn_block;
    string byte_coerce = "coerce_"  + byte_sort.name;
    string int16_coerce  = "coerce_"  + int16_sort.name;
    string int32_coerce  = "coerce_"  + int32_sort.name;
    string int32_coerce1 = "_coerce_" + int32_sort.name;
    string bool_coerce = "coerce_Bool";
    string boolint32_coerce = "coerce_BoolInt";
    
    base_eqn_block.variables.push_back(var_t("b", "Bool", ""));
    base_eqn_block.decls.push_back(boolint32_coerce + " : Bool -> " + boolint_sort.name);
#if 1
    base_eqn_block.equations.push_back(boolint32_coerce + "(false) = 0");
    base_eqn_block.equations.push_back(boolint32_coerce + "(true)  = 1");
#else
    base_eqn_block.equations.push_back("!b -> " + boolint32_coerce + "(b) = 0");
    base_eqn_block.equations.push_back("b  -> " + boolint32_coerce + "(b) = 1");
#endif    
    base_eqn_block.equations.push_back("(" + boolint32_coerce + "(b) == 0) = !b");
    
    base_eqn_block.decls.push_back(bool_coerce + " : Int -> Bool");
    base_eqn_block.equations.push_back(bool_coerce + "(x) = x != 0");

    base_eqn_block.variables.push_back(var_t("x", "Int", ""));
    base_eqn_block.variables.push_back(var_t("y", "Int",  ""));
    
    base_eqn_block.decls.push_back("rem : Int # Int -> Int");
    base_eqn_block.equations.push_back("(x>=0 && y>0) -> rem(x,y) = Nat2Int(x mod Int2Pos(y))");
    base_eqn_block.equations.push_back("(x>=0 && y<0) -> rem(x,y) =  rem( x,-y)");
    base_eqn_block.equations.push_back("(x<0)         -> rem(x,y) = -rem(-x, y)");

    base_eqn_block.decls.push_back(byte_coerce + " : Int -> " + byte_sort.name);
    base_eqn_block.equations.push_back("(x >= 0 && x < 256) -> " + byte_coerce + "(x) = Int2Nat(x)");
//     base_eqn_block.equations.push_back("(x >= 0) -> " + byte_coerce + "(x) = x mod 256");
//     base_eqn_block.equations.push_back("(x < 0)  -> " + byte_coerce + "(x) = coerce_UInt8(256 - ((-x) mod 256))");

    base_eqn_block.decls.push_back(int16_coerce + " : Int -> " + int16_sort.name);
    base_eqn_block.equations.push_back("(x >= -32768 && x < 32768) -> " + int16_coerce + "(x) = x");
    
    base_eqn_block.decls.push_back(int32_coerce  + " : Int -> " + int32_sort.name);
    base_eqn_block.decls.push_back(int32_coerce1 + " : Int -> " + int32_sort.name);
    base_eqn_block.equations.push_back(int32_coerce + "(y) = "
                                       + int32_coerce1 + "(y mod 4294967296)");
    base_eqn_block.equations.push_back(string("(y >= 2147483648) -> ")
                                       + int32_coerce1 + "(y) = y - 4294967296");
    base_eqn_block.equations.push_back(string("(y < 2147483648)  -> ")
                                       + int32_coerce1 + "(y) = y");

    base_eqn_block.decls.push_back("m32_lognot : Int32 -> Int32");
    base_eqn_block.equations.push_back("(x>=0) -> m32_lognot(x) = coerce_Int32(UInt322Nat(not(Nat2UInt32(Int2Nat(x)))))");
    base_eqn_block.equations.push_back("(x<0)  -> m32_lognot(x) = coerce_Int32(UInt322Nat(not(Nat2UInt32(Int2Nat(4294967296+x)))))");

    base_eqn_block.decls.push_back("m32_logand : Int32 # Int32 -> Int32");
    base_eqn_block.equations.push_back("(x>=0 && y>=0) -> m32_logand(x,y) = coerce_Int32(UInt322Nat(and(Nat2UInt32(Int2Nat(x)),Nat2UInt32(Int2Nat(y)))))");

    base_eqn_block.decls.push_back("m32_logor : Int32 # Int32 -> Int32");
    base_eqn_block.equations.push_back("(x>=0 && y>=0) -> m32_logor(x,y) = coerce_Int32(UInt322Nat(or(Nat2UInt32(Int2Nat(x)),Nat2UInt32(Int2Nat(y)))))");

    base_eqn_block.decls.push_back("m32_lshift : Int32 # Int32 -> Int32");
    base_eqn_block.equations.push_back("(x>=0 && y>=0) -> m32_lshift(x,y) = coerce_Int32(UInt322Nat(shiftl(Nat2UInt32(Int2Nat(x)),Nat2UInt32(Int2Nat(y)))))");
    
    base_eqn_block.decls.push_back("m32_rshift : Int32 # Int32 -> Int32");
    base_eqn_block.equations.push_back("(x>=0 && y>=0) -> m32_rshift(x,y) = coerce_Int32(UInt322Nat(shiftr(Nat2UInt32(Int2Nat(x)),Nat2UInt32(Int2Nat(y)))))");

    base_eqn_block.variables.push_back(var_t("sx","State",""));
    base_eqn_block.variables.push_back(var_t("sy","State",""));
    base_eqn_block.decls.push_back("merge_State : State # State # State -> State");
    base_eqn_block.equations.push_back("merge_State(sx,sx,sy) = sy");
    base_eqn_block.equations.push_back("merge_State(sx,sy,sx) = sy");
    
    mcrl.equations["BASE SORT EQUATIONS"] = base_eqn_block;
}

void
translate_sorts()
{
    if (mcrl.states.size() > 0) {
        string decl = "struct ";
        set<string>::iterator it = mcrl.states.begin();
        goto start1;
        for (; it != mcrl.states.end(); ++it) {
            decl += " | ";
        start1:
            decl += *it;
        }
        sort_t state_sort(string("State"), decl);
        mcrl.sorts[state_sort.name] = state_sort;
    }
}

void
translate_system()
{
    translate_base_sorts();
    register_global_variables();
    register_actions();

    for (size_t i = 0; i < DVE_system.get_process_count(); ++i) {
        register_process (*dynamic_cast<divine::dve_process_t*>(DVE_system.get_process(i)));
    }
    for (size_t i = 0; i < DVE_system.get_process_count(); ++i) {
        translate_process (*dynamic_cast<divine::dve_process_t*>(DVE_system.get_process(i)));
    }

    process_t system("System'");
    string transition;

    map<string,process_t>::iterator it = mcrl.processes.begin();
    if (it != mcrl.processes.end()) goto start;
    for (; it != mcrl.processes.end(); ++it) {
        transition += " || ";
    start:
        const process_t& proc = it->second;
        if (proc.name != "MEM'") {
            transition += proc.name + maybe_group(get_initializer_list(proc.variables));
        } else {
            transition += "delta";      // XXX klugde
        }
    }
    system.transitions.push_back(transition);

    mcrl.processes[system.name] = system;
    
    translate_global_variables();
    translate_actions();
    translate_sorts();
}

void
setup_operators()
{
    operators[T_LT]    = operator_t("m32_lt", "<",  "BoolInt", "Int32", "Int32");
    operators[T_LEQ]   = operator_t("m32_le", "<=", "BoolInt", "Int32", "Int32");
    operators[T_EQ]    = operator_t("m32_eq", "==", "BoolInt", "Int32", "Int32");
    operators[T_NEQ]   = operator_t("m32_neq", "!=", "BoolInt", "Int32", "Int32");
    operators[T_GT]    = operator_t("m32_gt", ">",   "BoolInt", "Int32", "Int32");
    operators[T_GEQ]   = operator_t("m32_geq", ">=", "BoolInt", "Int32", "Int32");
    operators[T_PLUS]  = operator_t("m32_plus", "+",  "Int32", "Int32", "Int32");
    operators[T_MINUS] = operator_t("m32_minus", "-", "Int32", "Int32", "Int32");
    operators[T_MULT]  = operator_t("m32_times", "*", "Int32", "Int32", "Int32");
    operators[T_DIV]   = operator_t("m32_div", "div", "Int32", "Int32", "Int32");
    operators[T_MOD]   = operator_t("rem", "Int32"); // does not need coercing
    operators[T_AND]   = operator_t("m32_logand", "Int32");
    operators[T_OR]    = operator_t("m32_logor",  "Int32");
    operators[T_XOR]   = operator_t("?BIT-XOR?",  "Int32");
    operators[T_LSHIFT]= operator_t("m32_lshift", "Int32");
    operators[T_RSHIFT]= operator_t("m32_rshift", "Int32");
    operators[T_BOOL_AND]= operator_t("m32_and", "&&", "BoolInt", "BoolInt", "BoolInt");
    operators[T_BOOL_OR] = operator_t("m32_or", "||", "BoolInt", "BoolInt", "BoolInt");
    operators[T_IMPLY]   = operator_t("m32_implies", "=>", "BoolInt", "BoolInt", "BoolInt");
    operators[T_UNARY_MINUS] = operator_t("m32_uminus", "-", "Int32", "Int32");
    operators[T_TILDE]   = operator_t("m32_lognot", "Int32");
    operators[T_BOOL_NOT]= operator_t("m32_not", "!", "BoolInt", "BoolInt");
    operators[T_DOT]     = operator_t("?.?", "UNKNOWN_TYPE");
}

// * mCRL "pretty" printing functions
void
print_section_sort (ostream& out)
{
    out << "sort" << endl;
    for (map<string,sort_t>::iterator it = mcrl.sorts.begin();
         it != mcrl.sorts.end(); ++it) {
        const sort_t& sort = it->second;
        out << "  " << sort.name << " = " << sort.constructors << ";" << endl;
    }

    for (map<string,array_t>::const_iterator it = mcrl.arrays.begin();
         it != mcrl.arrays.end(); ++it) {
        out << "  " << it->second.get_sort_declaration() << ";" << endl;
    }
    out << endl;
}

void
print_process (process_t& process, ostream& out)
{
    out << "  " << process.name
        << maybe_group(get_variables_list(process.variables, true));
    out << " = " << process.init << endl;
    if (process.transitions.size() > 1) {
        out << "  ( " << endl;
    }

    vector<string>::iterator trans = process.transitions.begin();
    if (trans != process.transitions.end()) {
        goto start;
        for (;trans != process.transitions.end(); ++trans) {
            out << endl << "  + " << endl;
        start:
            out << "    " << *trans;
        }
    }
    if (process.transitions.size() > 1) {
        out << endl << "  )";
    }
    out << ";" << endl << endl;
}

void
print_section_eqn (ostream& out)
{
    for (map<string,eqn_block_t>::const_iterator eqn = mcrl.equations.begin();
         eqn != mcrl.equations.end(); ++eqn) {
        out << "% " << eqn->first << endl;
        if (eqn->second.decls.size() > 0) out << "map" << endl;
        for (vector<string>::const_iterator it = eqn->second.decls.begin();
             it != eqn->second.decls.end(); ++it) {
            out << "  " << *it << ";" << endl;
        }

        if (eqn->second.variables.size() > 0) out << "var" << endl;
        for (vector<var_t>::const_iterator it = eqn->second.variables.begin();
             it != eqn->second.variables.end(); ++it) {
            out << "  " << it->name << ":" << it->type << ";" << endl;
        }

        if (eqn->second.equations.size() > 0) out << "eqn" << endl;
        for (vector<string>::const_iterator it = eqn->second.equations.begin();
             it != eqn->second.equations.end(); ++it) {
            out << "  " << *it << ";" << endl;
        }
        for (vector<string>::const_iterator it = eqn->second.rules.begin();
             it != eqn->second.rules.end(); ++it) {
            out << "  " << *it << ";" << endl;
        }

        out << endl;
    }
}

void
print_section_act (ostream& out)
{
    out << "act" << endl;
    for (map<string,action_t>::iterator it = mcrl.actions.begin();
         it != mcrl.actions.end(); ++it) {
        out << "  " << it->second.name << " : " << it->second.sort << ";"
            << endl;
    }
    out << endl;
}

void
print_section_proc (ostream& out)
{
    out << "proc" << endl;
    for (map<string,process_t>::iterator it = mcrl.processes.begin();
         it != mcrl.processes.end(); ++it) {
        print_process(it->second, out);
    }
    out << endl;
}

void
print_comms (ostream& out,
             vector<comm_t>::iterator start, vector<comm_t>::iterator end)
{
    out << "{";
    if (start != end) goto start;
    for (; start != end; ++start) {
        out << ", ";
    start:
        out << start->action.name << "|" << start->coaction.name
            << "->" << start->comm_action.name;
    }
    out << "}";
}

void
print_section_init (ostream& out)
{
    const process_t& mem = mcrl.processes["MEM'"];
    string allowed_actions;
    for (set<action_t>::const_iterator it = mcrl.allowed_actions.begin();
         it != mcrl.allowed_actions.end(); ++it) {
        allowed_actions = concat_list(allowed_actions, ", ") + it->name;
    }
    
    out << "init" << endl
        << "  hide({__read_write_sync}," << endl
        << "  allow({" << allowed_actions << "}," << endl
        // extra comm section for 3-way handshake
        << "  comm({read_write_sync|_read_write_sync->__read_write_sync}," << endl
        << "    " << mem.name << maybe_group(get_initializer_list(mem.variables)) << " || " << endl
        << "  allow({_read_write_sync}," << endl;
    if (mcrl.comms.size() > 0) {
        out << "  comm(";
        print_comms(out, mcrl.comms.begin(), mcrl.comms.end());
        out << ", " << endl;
    }
    const process_t& system = mcrl.processes["System'"];
    out << "  " << system.name << "))))";
    if (mcrl.comms.size() > 0) out << ")";
    out << ";" << endl;
    out << endl;
}


}

// * main
void
die(string message,
    unsigned int exit_code = EXIT_FAILURE,
    ostream& out = cerr)
{
    out << message << endl;
    exit(exit_code);
}

int
main (int argc, char** argv) {
    set<string> vars;
    int option;
    string input_filename = "/dev/stdin";
    string output_filename;
    while ((option = getopt_long(argc, argv, "hs:i:o:",
                                 mcrl::options, NULL)) != -1) {
        switch (option) {
        case 's':
            vars.insert(optarg);
            break;
        case 'i':
            input_filename = optarg;
            break;
        case 'o':
            output_filename = optarg;
            break;
        case 'h':
            die(mcrl::usage, EXIT_SUCCESS, cout);
        case '?':
            exit(EXIT_FAILURE);
        default:
            abort();
        }
    }
    argc -= optind; argv += optind;
    if (argc > 0) die(mcrl::usage);

    if (DVE_system.read(input_filename.c_str()) != 0) {
        die("error while parsing file: " + input_filename, EXIT_FAILURE, cerr);
    };
    dve_symtab = DVE_system.get_symbol_table();
   
    mcrl::parse_visible_variables(vars);
    
    mcrl::setup_operators();
    mcrl::translate_system();

    ostream *out = &cout;
    if (output_filename != "" && output_filename != "-") {
        out = new ofstream(output_filename.c_str());
    }
    mcrl::print_section_sort(*out);
    mcrl::print_section_eqn(*out);
    mcrl::print_section_act(*out);
    mcrl::print_section_proc(*out);
    mcrl::print_section_init(*out);
    exit(EXIT_SUCCESS);
}

// Local Variables:
// compile-command: "g++ -g -Wall -I../../src dve2mcrl2.cc -o dve2mcrl2 -lsevine -L../../lib"
// End:

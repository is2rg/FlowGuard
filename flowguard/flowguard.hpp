/*
 * FlowGuard
 *
 * Copyright (C) 2019 Irene Diez-Franco
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FLOWGUARD_H
#define FLOWGUARD_H


#include "fg-serial.hpp"

#include "gcc-plugin.h" // register_callback
#include "plugin-version.h" // to get info about the plugin
#include "tree-pass.h" // defines pass_data, register_pass_info
#include "context.h" // for the global gcc g context

#include "stringpool.h"

#include "tree.h"
#include "basic-block.h"
#include "tree-ssa-alias.h"
#include "gimple-expr.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-pretty-print.h"

#include "tree-ssa-operands.h"
#include "tree-ssanames.h"
#include "tree-phinodes.h"
#include "gimple-ssa.h"
#include "ssa-iterators.h"
#include "tree-pretty-print.h"

#include "tree-dfa.h"
#include "alias.h"
#include "tree-ssa-alias.h"
#include "bitmap.h"

#include "diagnostic-core.h"

#include "rtl.h"

#include "tree-nested.h" //-> testing for build_addr
#include "tree-into-ssa.h" // testing need_ssa_update_p
#include "tree-ssa-operands.h"
#include "print-tree.h"

#include <cstdio>
#include <cstring>
//#include <sstream>
#include "fg-defuse.hpp"
#include "fg-alias.hpp"
#include "fg-instru.hpp"
// #include "fg-types.hpp"

// #include "debug.hpp"


#define fg_debug 1

using namespace std;


int plugin_is_GPL_compatible;

namespace flowguard
{
    /** Name of current fun **/
    const char *current_fun;
    /** Name of current src **/
    const char *current_src;
    /** len of curren src and fun **/
    int len_current_src = 0, len_current_fun = 0;
    /** Path where the dfg files are written **/
    string dfg_path = "";
    /** If dfg_file is enabled the dfg is instrumented to be loaded from files
        instead of the libinstru itself. If so, dfg_path needs exist.**/
    bool dfg_file = false;

    /* Maps file-name to function name to def-uses in that function.
       FGDefUse has a unordered_map<string, set<unsigned int> > */
    unordered_map<string, unordered_map<string,
                                        flowguard::FGDefUse*> > *g_def_uses;
    /* Maps set id to definition ids */
    unordered_map<int, set<int> > *g_def_sets;
    /* Maps source-file - function - varname (single string delimited by '+')
       to set id */
    unordered_map<string, int> *g_srcfunvar_setid;


     /* Cleans the map of def/uses */
    void clean_defs_uses_map();
    /* Cleans the map of definition sets */
    void clean_def_sets_map();
    /* Cleans the map of srcfunvar to set id */
    void clean_var_set_map();

    /** Given a tree operand get the c string of its name _vesion or
        name_version **/
    char* get_cstr_nameversion(tree *operand);
    /** A more complete version of cstr_nameversion, used in the types pass**/
    char* get_cstr_namever_strict(tree *operand);
    /** Checks if the given var is a string constant **/
    bool is_string_constant(tree *var);

    /** Dumps the data flow graph to a binary file **/
    void dump_dfg();


    const pass_data flowguard_ssa_clean_data = {
        GIMPLE_PASS,
        "flowguard_ssa_clean",
        OPTGROUP_NONE,
        TV_NONE,
        PROP_gimple_any | PROP_ssa,
        0, 0, 0, 0
    };

    class FlowGuardSSACleanPass : public gimple_opt_pass
    {
    public:
        FlowGuardSSACleanPass(gcc::context *ctxt)
            : gimple_opt_pass(flowguard_ssa_clean_data, ctxt)
            {
            }
        virtual bool gate(function *fun) override;
        virtual unsigned int execute(function *fun) override;
    };

    const pass_data flowguard_instru = {
        GIMPLE_PASS,
        "flowguard_instru",
        OPTGROUP_NONE,
        TV_NONE,
        PROP_gimple_any | PROP_ssa,
        0, 0, 0, 0
    };

    class FlowGuardInstruPass : public gimple_opt_pass
    {
        FGInstru *instru;
        FGDefUse *current_defuse;
        bool instrument_init = false;
    public:
        FlowGuardInstruPass(gcc::context *ctxt)
            : gimple_opt_pass(flowguard_instru, ctxt)
            {}
        virtual bool gate(function *fun) override;
        virtual unsigned int execute(function *fun) override;
        void set_current_defuse(FGDefUse *defuse) {
            this->current_defuse = defuse;}
        FGDefUse* get_current_defuse() { return this->current_defuse;}
    private:
        void set_def(gimple_stmt_iterator *gsi, tree *def);
        void check_def(gimple_stmt_iterator *gsi, tree *use);
        void set_phi(gimple_stmt_iterator *gsi, tree *rhs, string phi_name);
        void check_phi(gimple_stmt_iterator *gsi, tree *rhs);

        void inject_set_def(gimple_stmt_iterator *gsi, tree *def, string name);
        void inject_check_def(gimple_stmt_iterator *gsi, tree *use,
                              string name);
        void inject_set_phi_to_var(gimple_stmt_iterator *gsi, tree *rhs,
                                   string phi_name);
        void inject_check_phi(gimple_stmt_iterator *gsi, tree *rhs,
                              string phi_name);
        void inject_initialization(gimple_stmt_iterator *gsi);
    };
}



/**  Plugin end callback **/
void plugin_end_2(void *gcc_data, void *user_data);


/** Register callbacks and some initialization.
    Returns: 1 if error due to GCC version mismatch **/
int
plugin_init(struct plugin_name_args *plugin_info,
            struct plugin_gcc_version *version);

#endif /* FLOWGUARD_H */

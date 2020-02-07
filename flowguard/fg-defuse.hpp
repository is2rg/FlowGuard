#ifndef FG_DEFUSE_HPP
#define FG_DEFUSE_HPP
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <string>
#include <unordered_map>
#include <set>
#include <vector>

using namespace std;
#define fg_debug 1

namespace flowguard {
    class FGDefUse
    {
        string source_file, func_name;
        /* Map of vars to lines where they are used -> USEs at lines */
        unordered_map<string, set<int> > uses;
        /* Map of defs to lines where they are defined -> DEFs at lines
           Definitions occur in a single line, however at this phase some defs
           may occur or not (phi nodes), so we store all the possible defs. */
        unordered_map<string, set<int> > defs;
        /* For each key there is a list of variables that need to have the same
           uses as the key:
           a3 = phi <a2, a1>
           def(a3) is at def(a2) or def(a1)
           So we set the uses of a2 and a1 to be use(a3) */
        unordered_map<string, set<string> > phi_todo_add_uses;
        /* This maps holds the var names that should be defined at the given
           (key) bb. We do this because our instrumentation does not know where
           to put the phi statement def (cannot have a _real_ location since
           is a virtual op), so we take note of this definitions and add them
           in the beginning of the function to avoid 'dfi violation error' when
           the var is checked in a use. */
        unordered_map<int, set<string> > phi_todo_add_defs;
        /* Holds which vars need an additional definiition (of a phi var)
           since they are in the possible branches of a phi.
           Given:
           # auth_1 = PHI <auth_5(4), auth_11(6)>
           at auth_5 we need to also define auth_1 and the same
           thing with auth_11 */
        unordered_map<string, string> var_additional_def_phi;
        /* For each unique use of a struct-field (stf) variable the variable is
           held here. This set is later used to check if we need to add the
           definitions of those struct-field variables at the begining of the
           function. */
        set<string> maybe_stf_add_defs;
        /* When we have a dereference in the lhs, we count it as a use instead
           of a def, however, the var needs to be defined. In the alias part
           we check that stuff, but we might miss it. Thereby we add these
           vars to maybe_var_deref_add_def list which is checked after the
           alias. */
        set<string> maybe_var_dereflhs_add_def;
        /* Set of confirmed stf variables that need to be instrumented at the
           beginning of the function*/
        set<string> stf_add_defs;
        /* Set that contains phi ids of phi functions for which a set_phi call
         * has been inserted */
        set<int> processed_phis;
        /* Set of trees that represent the address of expressions for which a
         * set_def call has been inserted */
        set<void*> processed_exprs;
        /* Function parameters */
        set<string> func_parameters;
        set<string> func_parameters_remaining;
        /* Function parameters that need a set_def since they are later on
           used */
        vector<string> func_parameters_instrument;
        /* Store the global variables that need to be defined to prent dfi
           from failling */
        set<string> globals_to_def;
        // maps a phi 'result' with the possible ssa definitions that may
        // have so, given ptr_1 = phi(ptr_11, ptr_12);
        // if we have ptr_11 = &a, then one possible ssa is 'a'
        //            ptr_12 = &b, and the other possibility is 'b'
        unordered_map<string, set<string> > map_phi_possible_ssa_defs;
        // in this map we take note which phi result corresponds to each
        // phi argument
        unordered_map<string, set<string> > map_phi_arg_to_phi_result;
        // in this map we take note of each phi result and its arguments
        unordered_map<string, set<string> > map_phi_result_to_phi_arg;
        // each phi result has a per-function unique id
        unordered_map<string, int> map_phi_result_to_id;
        // last used phi result id, 0 if still unused
        int phi_result_id = 0;


        bool debug = false;
        bool debug_defuses = false;
        /* Track .MEM operatios or not */
        bool track_mem_ops = true;
        /* Checks whether a var is a memory operation */
        bool is_mem_op(string var);
        /** Checks if the given struct-field (stf) variable needs a definition
            at the begining of the function. This happens when there aren't
            definitions for the given stf or when the stf has definitions
            but the uses happen earlier. **/
        bool is_stf_def_needed(string stf);
        /** Internal method to generate stf names from vector**/
        string gen_stf_name(vector<char*> *stf_names, int version);

    public:
        FGDefUse(bool debug = false, bool track_mem_ops = true);
        FGDefUse(string source_file, string func_name, bool debug = false,
                 bool track_mem_ops = true);
        /** Adds the definition of var. **/
        void add_def(string var, double line);
        /** Adds a definition given a hash and a ssa version num. **/
        void add_def(unsigned long hash, int version, double line);
        /** Adds a definition given a ver name and a ssa version num.**/
        void add_def(const char *name, int version, double line);
        /** Adds the definition of var (_version) at line. **/
        void add_def(int version, double line);
        /** Add the definition of a var (struct) + field **/
        void add_def(const char *name, int version, const char *field,
                     double line);
        /** Adds the def of a complex struct, and the version of the leftmost
            (core) struct. **/
        void add_def(vector<char*> *stf_names, int version, double line);
        /** Adds the use of var at line. **/
        void add_use(string var, double line);
        /** Adds the use of var (hash_version) at line. **/
        void add_use(unsigned long hash, int version, double line);
        /** Adds the use of var (name_version) at line. **/
        void add_use(const char *name, int version, double line);
        /** Adds the use of var (_version) at line. **/
        void add_use(int version, double line);
        /** Adds the use of a var (struct ) + field **/
        void add_use(const char *name, int version, const char *field,
                     double line);
        /** Adds the use of a complex struct, and the version of the leftmost
            (core) struct. **/
        void add_use(vector<char*> *stf_names, int version, double line);

        /** Generates the uses for phi statement operands based on the
            phi_todo_add_uses map **/
        void generate_phi_operand_uses();
        /** Stores in the phi_todo_add_uses map, that the operands need to
            have the uses of 'result' **/
        void store_phi_todo_use(string result, vector<string> *operands);
        /**  Stores in the phi_todo_add_defs map, that the given bb needs
             the definition of the given var**/
        void store_phi_todo_def(int bb, string var);
        /** Adds a function parameter */
        void add_func_param(string &param);
        /* Checks if any func_param name is the comparison parameter, if so
           adds it to the to_instrument vector to instrument its def later */
        void check_function_argument_use(const char *varname, int version);
        /** Ads a parameter name to the must define list **/
        void add_func_param_to_define(string &param);


        void add_additional_def(string var, string additional_var);
        bool check_additional_def(string var);
        string get_additional_def_for_var(string var);

        /** Adds the struct-field variable to the set maybe_struct_add_defs
         **/
        void take_note_structfield(string stf);
        /** Checks the maybe_struct_add_defs set and populates the set of stf
            vars that need to be defined. Moreover, an additional definition
            is added (at 0) to those stf vars. **/
        void generate_needed_stf_defs();


        /* Takes note of the given global variable that needs to be defined
           at the beggining of the function to prevent dfi from failling.*/
        void add_global_to_def(char *var);
        void add_global_to_def(string &var);

        /** Checks if the given var has definitions (-1 is accounted as a
            def)**/
        bool has_definitions(string var);
        /** Checks if the given var has definitions (-1, is not accounted as
            a def) **/
        bool has_strict_definitions(string var);
        /** Returns a set with the uses of the given var **/
        set<int> *get_uses_of_var(string var);
        /** Returns a set with the definiitions of the given var **/
        set<int> *get_defs_of_var(string var);

        void set_debug(bool enable) { this->debug = enable; }
        bool is_debugging() { return this->debug; }
        void set_debug_defuses(bool enable) { this->debug_defuses = enable; }
        bool is_debug_defuses() { return this->debug_defuses; }
        void set_track_mem_ops(bool enable) { this->track_mem_ops = enable; }
        bool is_tracking_mem_ops() { return this->track_mem_ops; }
        string get_source_file() const { return this->source_file; }
        string get_func_name() const { return this->func_name; }
        unordered_map<string, set<int> > *get_defs() \
            { return &this->defs; }
        unordered_map<string, set<int> > *get_uses() \
            { return &this->uses; }
        unordered_map<int, set<string> > *get_phitodo_defs()\
            { return &phi_todo_add_defs; }
        set<string> *get_function_parameters() { return &func_parameters; }
        vector<string> *get_func_params_to_instru() \
            { return &this->func_parameters_instrument; }
        set<string> *get_maybe_var_dereflhs_add_def()
            { return &this->maybe_var_dereflhs_add_def; }
        set<string> *get_stf_add_defs() \
            { return &this->stf_add_defs; }
        set<string> *get_globals_add_defs() \
            { return &this->globals_to_def; }
        unordered_map<string, set<string> > *get_phi_possible_ssa_defs() \
            { return &this->map_phi_possible_ssa_defs;}
        unordered_map<string, set<string> > *get_phi_arg_to_phi_result() \
            { return &this->map_phi_arg_to_phi_result;}
        unordered_map<string, set<string> > *get_phi_result_to_phi_arg() \
            { return &this->map_phi_result_to_phi_arg;}
        unordered_map<string, int> *get_phi_result_to_id() \
            { return &this->map_phi_result_to_id;}
        set<int> *get_processed_phis() \
            { return &this->processed_phis; }
        set<void*> *get_processed_exprs() \
            { return &this->processed_exprs; }
        void print_map_phi_possible_ssa_defs();
        void print_map_phi_arg_to_phi_result();
        void print_map_phi_result_to_phi_arg();
        /* Checks if the given lhs is a phi argument in any of the recorded
           phis */
        bool is_lhs_phi_arg(string lhs);
        /* Checks if the given rhs is a phi result in any of the recorded
           phis */
        bool is_rhs_phi_result(string rhs);
        /* Generates an id for the given phi_result */
        int generate_phi_result_id(string phi_result);
    };

/** Given a var in the form name_vernum, returns name **/
    string clean_ssa_var_version(string var, int version);

} /* namespce flowguard */
#endif /** FGDEFUSE_HPP **/

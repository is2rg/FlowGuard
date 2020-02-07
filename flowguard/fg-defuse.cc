#include "fg-defuse.hpp"


flowguard::FGDefUse::FGDefUse(bool debug, bool track_mem_ops)
{
    this->debug = debug;
    this->track_mem_ops = track_mem_ops;
}

flowguard::FGDefUse::FGDefUse(string source_file, string func_name,
                              bool debug, bool track_mem_ops)
{
    this->debug = debug;
    this->track_mem_ops = track_mem_ops;
    this->source_file = source_file;
    this->func_name = func_name;
}

string clean_ssa_var_version(string var, int version)
{
    string tmp = "_" + to_string(version);
    size_t found = var.find_last_of(tmp);
    string ret;
    if (found != string::npos)
    {
        ret = var.substr(0, found);
    }
    return ret;
}

void flowguard::FGDefUse::add_def(string var, double line)
{
    if (this->track_mem_ops || !this->is_mem_op(var))
    {
        if (this->uses.count(var) == 0)
        {
            this->uses.emplace(make_pair(var, set<int>()));
        }
        if (this->defs.count(var) == 0)
        {
            this->defs.emplace(make_pair(var, set<int>()));
        }
        // We add -1 when a use needs an undefined definition
        if (this->defs[var].size() == 1 &&
            this->defs[var].find(-1) != this->defs[var].end())
        {
            this->defs[var].erase(-1);
        }
        this->defs[var].insert(line);
    }
}

void flowguard::FGDefUse::add_def(unsigned long hash, int version, double line)
{
    this->add_def(to_string(hash) + "_" + to_string(version), line);
}

void flowguard::FGDefUse::add_def(const char *name, int version, double line)
{
    string n(name);
    this->add_def(n + "_" + to_string(version), line);
}

void flowguard::FGDefUse::add_def(int version, double line)
{
    this->add_def("_" + to_string(version), line);
}

void
flowguard::FGDefUse::add_def(const char *name, int version, const char *field,
                             double line)
{
    string n(name);
    string f(field);
    string stf(n + "_" + to_string(version) + "#" + f);
    this->take_note_structfield(stf);
    this->add_def(stf, line);
}

void
flowguard::FGDefUse::add_def(vector<char*> *stf_names, int version, double line)
{
    string stf = this->gen_stf_name(stf_names, version);
    this->take_note_structfield(stf);
    this->add_def(stf, line);
}

void flowguard::FGDefUse::add_use(string var, double line)
{
    if (this->track_mem_ops || !this->is_mem_op(var))
    {
        if (this->uses.count(var) == 0)
        {
            this->add_def(var, -1);
        }
        this->uses[var].insert(line);
    }
}

void flowguard::FGDefUse::add_use(unsigned long hash, int version, double line)
{
    this->add_use(to_string(hash) + "_" + to_string(version), line);
}

void flowguard::FGDefUse::add_use(const char *name, int version, double line)
{
    string n(name);
    this->add_use(n + "_" + to_string(version), line);
}

void flowguard::FGDefUse::add_use(int version, double line)
{
    this->add_use("_" + to_string(version), line);
}

void
flowguard::FGDefUse::add_use(const char *name, int version, const char *field,
                             double line)
{
    string n(name);
    string f(field);
    string stf(n + "_" + to_string(version) + "#" + f);
    this->take_note_structfield(stf);
    this->add_use(stf, line);
}

void
flowguard::FGDefUse::add_use(vector<char*> *stf_names, int version,
                             double line)
{
    string stf = this->gen_stf_name(stf_names, version);
    this->take_note_structfield(stf);
    this->add_use(stf, line);
}

void flowguard::FGDefUse::generate_phi_operand_uses()
{
    // For each key, there is a list of variables that need to have the uses
    // of the key
    for (auto it = this->phi_todo_add_uses.begin();
         it != this->phi_todo_add_uses.end(); ++it)
    {
        if (this->uses.count(it->first))
        {
            for (auto op = this->phi_todo_add_uses[it->first].begin();
                 op != this->phi_todo_add_uses[it->first].end(); ++op)
            {
                if (this->uses.count(*op) == 0)
                {
                    this->uses.emplace(*op, set<int>());
                }
                else
                {
                    this->uses[*op].erase(-2);
                }
                for (auto use_of_key : this->uses[it->first])
                {
                    // if we have 'nested' phis we might insert a -2. Avoid it
                    if (use_of_key >= 0)
                        this->uses[*op].insert(use_of_key);
                }
            }
        }
    }
}

void
flowguard::FGDefUse::store_phi_todo_use(string result,
                                        vector<string> *operands)
{
    if (this->phi_todo_add_uses.count(result) == 0)
    {
        this->phi_todo_add_uses.emplace(result, set<string>());
    }
    for (auto operand : *operands)
    {
        this->phi_todo_add_uses[result].insert(operand);
    }
}

void
flowguard::FGDefUse::store_phi_todo_def(int bb, string var)
{
    if (this->phi_todo_add_defs.count(bb) == 0)
    {
        this->phi_todo_add_defs.emplace(bb, set<string> ());
    }
    this->phi_todo_add_defs[bb].insert(var);
}

void
flowguard::FGDefUse::add_func_param(string &param)
{
    this->func_parameters.insert(param);
    this->func_parameters_remaining.insert(param);
}

void
flowguard::FGDefUse::check_function_argument_use(const char *varname,
                                                 int version)
{
    if (this->func_parameters_remaining.size())
    {
        for (auto param : this->func_parameters_remaining)
        {
            if (strcmp(varname, param.c_str()) == 0)
            {
                string varstr(varname);
                this->func_parameters_remaining.erase(varstr);
                varstr += "_" + to_string(version);
                this->func_parameters_instrument.push_back(varstr);
                break;
            }
        }
    }
}

void flowguard::FGDefUse::take_note_structfield(string stf)
{
    this->maybe_stf_add_defs.insert(stf);
}

void flowguard::FGDefUse::generate_needed_stf_defs()
{
    for (auto stf : this->maybe_stf_add_defs)
    {
        if (this->is_stf_def_needed(stf))
        {
            this->stf_add_defs.insert(stf);
            this->add_def(stf, 0);
        }
    }
}

void flowguard::FGDefUse::add_global_to_def(char *var)
{
    string tmp(var);
    this->globals_to_def.insert(tmp);
}

void flowguard::FGDefUse::add_global_to_def(string &var)
{
    this->globals_to_def.insert(var);
}

bool flowguard::FGDefUse::has_definitions(string var)
{
    return this->defs.count(var) != 0;
}

bool flowguard::FGDefUse::has_strict_definitions(string var)
{
    if (this->defs.count(var) == 1)
        return (*this->defs[var].begin()) > -1;
    else
        return this->defs.count(var) != 0;
}

set<int>* flowguard::FGDefUse::get_uses_of_var(string var)
{
    return &this->uses[var];
}

set<int>* flowguard::FGDefUse::get_defs_of_var(string var)
{
    return &this->defs[var];
}


void
flowguard::FGDefUse::add_additional_def(string var, string additional_var)
{
    if (this->track_mem_ops || !this->is_mem_op(var))
    {
        this->var_additional_def_phi.emplace(var, additional_var);
    }
}

bool flowguard::FGDefUse::check_additional_def(string var)
{
    return this->var_additional_def_phi.count(var) != 0;
}

string flowguard::FGDefUse::get_additional_def_for_var(string var)
{
    return this->var_additional_def_phi[var];
}


bool flowguard::FGDefUse::is_mem_op(string var)
{
    size_t pos = var.find(".MEM_");
    return pos != string::npos;
}


bool flowguard::FGDefUse::is_stf_def_needed(string stf)
{
    if (this->has_strict_definitions(stf))
    {
        // check if the def line is less that all the uses
        for (auto use : this->uses[stf])
        {
            // (remember that if equal the definition takes priority)
            if (use < *this->defs[stf].begin())
                return true;
        }
        return false;
    }
    return true;
}

string flowguard::FGDefUse::gen_stf_name(vector<char*> *stf_names, int version)
{
    string stf = "";
    // We may have a ssa form like _4->a, if so we need to generate _4#a
    // rather than a_4
    if (stf_names->size() == 1)
    {
        stf = "_" + to_string(version) + "#" + (*stf_names->begin());
    }
    else
    {
        bool first = true;
        for (auto tmp : *stf_names)
        {
            string dummy(tmp);
            if (first)
            {
                first = false;
                stf += dummy + "_" + to_string(version);
            }
            else
            {
                stf += "#" + dummy;
            }
        }
    }
    return stf;
}

bool flowguard::FGDefUse::is_lhs_phi_arg(string lhs)
{
    return this->map_phi_arg_to_phi_result.count(lhs) != 0;
}

bool flowguard::FGDefUse::is_rhs_phi_result(string rhs)
{
    return this->map_phi_result_to_phi_arg.count(rhs) != 0;
}

int flowguard::FGDefUse::generate_phi_result_id(string phi_result)
{
    if (this->map_phi_result_to_id.count(phi_result) == 0)
    {
        this->phi_result_id++;
        return this->phi_result_id;
    }
    else
    {
        return this->map_phi_result_to_id[phi_result];
    }
}

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

#include "instrumentation.hpp"


static vector<string> split_v(const string &s, char delim)
{
    vector<string> elems;
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

static bool dfi_holds(set<int> &static_, set<int> &runtime)
{
    // check that everything on runtime it is on static_
    for (auto itr = runtime.begin(); itr != runtime.end(); ++itr)
    {
        bool found = false;
        for (auto its = static_.begin(); its != static_.end(); ++its)
        {
            if (*its > *itr)
            {
#if show_errors
                printf(" \e[33m[fg run] dfi check fail: static does not have"
                       " %d\e[0m\n", *itr);
#endif
                return false;
            }
            if (*its == *itr)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
#if show_errors
            printf(" \e[33m[fg run] dfi check fail: static does not have"
                   " %d\e[0m\n", *itr);
#endif
            return false;
        }
    }
    return true;
}

static void
update_func_vars(string &srcfunvar, unordered_map<string, set<string> > *map)
{
    size_t found = srcfunvar.rfind("+");
    if (found != string::npos)
    {
        string subs = srcfunvar.substr(0, found);
        if (map->count(subs) == 0)
            map->emplace(subs, set<string>());
        (*map)[subs].insert(srcfunvar);
    }
}

static void
_load_statics(vector<string> &setid_defs_lines,
              vector<string> &var_setid_lines)
{
    int first_size = setid_defs_lines.size();
    int last_set;
    bool read_id = true, read_set = false;
    for (auto line : setid_defs_lines)
    {
        if (read_id)
        {
            read_id = false;
            read_set = true;
            try
            {
                int ival = std::stoi(line);
                last_set = ival;
                g_static_setid_defs.emplace(ival, set<int>());
            }
            catch (const invalid_argument &ia)
            {
#if show_errors
                printf("\e[33mPanic at the disco - ids !!\e[0m\n");
                printf("\e[33mline: '%s'\e[0m\n", line.c_str());
#endif
            }
        }
        else if (read_set)
        {
            read_id = true;
            read_set = false;
            vector<string> values = split_v(line, ' ');
            for (auto val : values)
            {
                try
                {
                    int ival = std::stoi(val);
                    g_static_setid_defs[last_set].insert(ival);
                }
                catch (const invalid_argument &ia)
                {
#if show_errors
                        printf("\e[33mPanic at the disco - vals !\e[0m\n");
                        printf("\e[33mline: '%s'\e[0m\n", val.c_str());
#endif
                }
            }
        }
    }
    read_id = true;
    read_set = false;
    string last_var;
    for (int i = 0; i < var_setid_lines.size() - first_size; ++i)
    {
        string line = var_setid_lines[i];
        if (read_id)
        {
            read_id = false;
            read_set = true;
            last_var = line;
        }
        else if (read_set)
        {
            read_id = true;
            read_set = false;
            try
            {
                int ival = stoi(line);
                g_static_var_setid.emplace(make_pair(last_var, ival));
            }
            catch (const invalid_argument &ia)
            {
#if show_errors
                printf("\e[33mPanic at the disco - var-set !\e[0m\n");
                printf("\e[33mline: '%s'\e[0m\n", line.c_str());
#endif
            }
        }
    }
    printf(" [fg run] static dfg loaded\n");
    printf(" [fg run] static sets:\n");
    for (auto it = g_static_setid_defs.begin();
         it != g_static_setid_defs.end(); ++it)
    {
        printf("set id: %d,  values: ", it->first);
        for (auto val : it->second)
            printf("%d ", val);
        printf("\n");
    }
    printf(" [fg run] static src+fun+param - setid\n");
    for (auto it = g_static_var_setid.begin(); it != g_static_var_setid.end();
         it++)
    {
        printf("sfp-setid: %s - %d\n", it->first.c_str(), it->second);
    }
    printf(" [fg run] ~~~ end print statics ~~~\n");
    g_statics_loaded = true;
}

///                                 ///
/// Functions exported to flowguard ///
///                                 ///

void fg_load_statics()
{
    if (!g_statics_loaded)
    {
#if debug_fg
        printf(" [fg run] loading statics from lib");
#endif
        char *setid_defs_data = (char *)&_binary_fg_dfg_sets_fg_start;
        string s_sets(setid_defs_data);
        vector<string> lines = split_v(s_sets, '\n');
        
        int first_size = lines.size();
        int last_set;
        bool read_id = true, read_set = false;
        for (auto line : lines)
        {
            if (read_id)
            {
                read_id = false;
                read_set = true;
                try
                {
                    int ival = std::stoi(line);
                    last_set = ival;
                    g_static_setid_defs.emplace(ival, set<int>());
                }
                catch (const invalid_argument &ia)
                {
#if show_errors
                    printf("\e[33mPanic at the disco - ids !!\e[0m\n");
                    printf("\e[33mline: '%s'\e[0m\n", line.c_str());
#endif
                }
            }
            else if (read_set)
            {
                read_id = true;
                read_set = false;
                vector<string> values = split_v(line, ' ');
                for (auto val : values)
                {
                    try
                    {
                        int ival = std::stoi(val);
                        g_static_setid_defs[last_set].insert(ival);
                    }
                    catch (const invalid_argument &ia)
                    {
#if show_errors
                        printf("\e[33mPanic at the disco - vals !\e[0m\n");
                        printf("\e[33mline: '%s'\e[0m\n", val.c_str());
#endif
                    }
                }
            }
        }
        lines.clear();
        char *var_setid_data = (char *)&_binary_fg_dfg_vars_fg_start;
        string s_vars(var_setid_data);
        lines = split_v(s_vars, '\n');
        read_id = true;
        read_set = false;
        string last_var;
        for (int i = 0; i < lines.size() - first_size; ++i)
        {
            string line = lines[i];
            if (read_id)
            {
                read_id = false;
                read_set = true;
                last_var = line;
            }
            else if (read_set)
            {
                read_id = true;
                read_set = false;
                try
                {
                    int ival = stoi(line);
                    g_static_var_setid.emplace(make_pair(last_var, ival));
                }
                catch (const invalid_argument &ia)
                {
#if show_errors
                    printf("\e[33mPanic at the disco - var-set !\e[0m\n");
                    printf("\e[33mline: '%s'\e[0m\n", line.c_str());
#endif
                }
            }
        }
#if debug_fg
        printf(" [fg run] static dfg loaded\n");
        printf(" [fg run] static sets:\n");
        for (auto it = g_static_setid_defs.begin();
             it != g_static_setid_defs.end(); ++it)
        {
            printf("set id: %d,  values: ", it->first);
            for (auto val : it->second)
                printf("%d ", val);
            printf("\n");
        }
        printf(" [fg run] static src+fun+param - setid\n");
        for (auto it = g_static_var_setid.begin();
             it != g_static_var_setid.end(); it++)
        {
            printf("sfp-setid: %s - %d\n", it->first.c_str(), it->second);
        }
        printf(" [fg run] ~~~ end print statics ~~~\n");
#endif
    }
    g_statics_loaded = true;
}

void fg_load_statics(const char *path)
{
    if (!g_statics_loaded)
    {
#if debug_fg
        printf(" [fg run] loading statics from file");
#endif
        string setspath(path);
        setspath += "/fg_dfg_sets.fg";
        string varspath(path);
        varspath += "/fg_dfg_vars.fg";

        ifstream sin(setspath, ios::in);
        stringstream buffs;
        buffs << sin.rdbuf();
        vector<string> setdeflines = split_v(buffs.str(), '\n');
        
        ifstream vin(varspath, ios::in);
        stringstream buffv;
        buffv << vin.rdbuf();
        vector<string> varsetidlines = split_v(buffv.str(), '\n');
        _load_statics(setdeflines, varsetidlines);
    }
}

void fg_set_def_addr(void *ptr, int id)
{
    if (runtime_defs_table.count((long) ptr) == 0)
    {
        runtime_defs_table.emplace((long) ptr, set<int>());
    }
    /* TODO: */
    if (g_static_setid_defs.count(id) != 0)
        runtime_defs_table[(long)ptr].insert(id);
}

void fg_check_def_addr(void *ptr, int setid)
{
    if (runtime_defs_table.count((long) ptr) == 0)
    {
        printf(" \e[33m dfi consistency error: %p (%ld) not defined\e[0m\n",
               ptr, (long) ptr);
        raise(SIGABRT);
    }
    if (g_static_setid_defs.count(setid) == 0)
    {
        printf(" \e[33m dfi consistency error: set id %d not defined\e[0m\n",
               setid);
        raise(SIGABRT);
    }
    if (!dfi_holds(g_static_setid_defs[setid], runtime_defs_table[(long)ptr]))
    {
        printf(" \e[33m dfi consistency error: static dt and runtime dt do not"
               " match\e[0m\n");
        raise(SIGABRT);
    }
}

void fg_set_phi_to_var(int phi_id, int set_id, void *ptr)
{
    runtime_phi_code_address_table[phi_id] = (long) ptr;
    runtime_phi_id_to_addr_to_setid[phi_id][(long) ptr] = set_id;
}

void fg_check_phi(int phi_id, void *ptr)
{
    if (runtime_phi_code_address_table.count(phi_id) == 0)
    {
        printf(" \e[33m dfi consistency error: phi id not found\e[0m\n");
        raise(SIGABRT);
    }
    if (runtime_phi_code_address_table[phi_id] != (long) ptr)
    {
        printf(" \e[33m dfi consistency error: phi handling does not"
               " match\e[0m\n");
        raise(SIGABRT);
    }
    int set_id = runtime_phi_id_to_addr_to_setid[phi_id][(long) ptr];
    fg_check_def_addr(ptr, set_id);
}

void fg_check_def(const char *srcfunvar, int setid)
{
    bool err = false;
    if (g_static_setid_defs.count(setid) == 0)
    {
        err = true;
        printf(" \e[33m[fg run] error: set with id %d not defined\e[0m\n",
               setid);
    }
    if (err)
        raise(SIGABRT);

    string s_srcfunvar(srcfunvar);
    if (g_rdt.count(s_srcfunvar) == 0)
    {
        err = true;
        printf(" \e[33m[fg run] error: runtime var '%s' not defined\e[0m\n",
               srcfunvar);
    }
    if (err)
        raise(SIGABRT);
    else
    {
        if (!dfi_holds(g_static_setid_defs[setid], g_rdt[s_srcfunvar]))
        {
            printf(" \e[33m[fg run] data flow integrity error\n");
            printf("\tvar '%s' static set %d\e[0m\n", srcfunvar, setid);
            raise(SIGABRT);
        }
    }
}


void fg_clear_function_context(const char *srcfun)
{
    string tmp(srcfun);
    if (func_vars.count(tmp) != 0)
    {
        for (auto var : func_vars[tmp])
            g_rdt.erase(var);
        func_vars[tmp].clear();
    }
}


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

#include "fg-instru.hpp"


flowguard::FGInstru::FGInstru(unordered_map<string,
                              unordered_map<string, FGDefUse *> > *defuses,
                              unordered_map<int, set<int> > *def_sets,
                              unordered_map<string, int> *srcfunvar_setid,
                              bool debug)
{
    this->defuses = defuses;
    this->def_sets = def_sets;
    this->srcfunvar_setid = srcfunvar_setid;
    this->debug = debug;
    this->generate_definition_sets();
}

void flowguard::FGInstru::gen_srcfunvar_name(const char *src, int len_src,
                                             const char *func, int len_func,
                                             const char *var, int len_var,
                                             char *dest)
{
    strncpy(dest, src, len_src);
    dest[len_src] = '+';
    strncpy(dest + len_src + 1, func, len_func);
    dest[len_src + 1 + len_func] = '+';
    strncpy(dest + len_src + 1 + len_func + 1, var, len_var);
    dest[len_src + len_func + len_var + 2] = '\0';
}

void flowguard::FGInstru::generate_definition_sets()
{
    // iterates source files
    for (auto it = this->defuses->begin(); it != this->defuses->end(); ++it)
    {
        int len_src = it->first.length();
        // iterates function names inside source files
        for (auto itt = it->second.begin(); itt != it->second.end(); ++itt)
        {
            int len_func = itt->first.length();
            // iterates defition names (vars) in function-source
            for (auto ittt = itt->second->get_defs()->begin();
                 ittt != itt->second->get_defs()->end(); ++ittt)
            {
                // check if we have the set already defined
                bool defined = false;
                int len_var = ittt->first.length();
                char *dest = new char[len_src + len_func + len_var + 3];
                this->gen_srcfunvar_name(it->first.c_str(), len_src,
                                         itt->first.c_str(), len_func,
                                         ittt->first.c_str(), len_var,
                                         dest);
                string dest_s(dest);
                for (auto sit = this->def_sets->begin();
                     sit != this->def_sets->end(); ++sit)
                {
                    if (ittt->second == sit->second)
                    {
                        defined = true;
                        this->srcfunvar_setid->emplace(make_pair(dest_s,
                                                                 sit->first));
                        break;
                    }
                }
                if (!defined)
                {
                    this->def_sets->emplace(make_pair(set_id, ittt->second));
                    this->srcfunvar_setid->emplace(make_pair(dest_s, set_id));
                    ++set_id;
                }
                delete[] dest;
            }
        }
    }
}

int flowguard::FGInstru::get_setid_for_var(const char *var, const char* fun,
                                           const char *src)
{
    string src_s(src);
    string fun_s(fun);
    string var_s(var);
    string dest_s(src_s + "+" + fun_s + "+" + var_s);
    return this->srcfunvar_setid->count(dest_s) == 0 ? -1
        : (*this->srcfunvar_setid)[dest_s];
}

bool
flowguard::FGInstru::has_uses(const char *var, const char *fun,
                              const char *src)
{
    string s_var(var);
    string s_fun(fun);
    string s_src(src);
    string key(s_src + "+" + s_fun + "+" + s_var);
    set<int> *uses = (*this->defuses)[src][fun]->get_uses_of_var(s_var);
    if (uses->size() == 0 || uses->count(-2) != 0)
        return false;
    return true;
}


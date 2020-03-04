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

#ifndef FG_ALIAS_HPP
#define FG_ALIAS_HPP
#include <string>
#include <set>
#include <unordered_map>
#include <cassert>

using namespace std;


namespace flowguard {
    class FGAlias
    {
        string source_file, func_name;
        /** Set of aliased symbols that need to be added (set_def) at the
           beginning of the function **/
        set<string> aliased_sym_toadd;
        /** UUID to ssa name **/
        unordered_map<unsigned, string> uuid_to_ssaname;

    public:
        FGAlias();
        FGAlias(string source_file, string func_name);
        /** Adds an aliased symbol to the list of symbols to add at the
            beginning of the function **/
        void add_aliased_symbol(string &sym);
        /** Stores the uuid as the given ssa_name **/
        void collect_uuid_ssaname(unsigned uuid, char *ssa_name);
        /* General getters and setters */
        set<string>* get_aliased_sym_toadd()
            { return &this->aliased_sym_toadd; }
        string get_source_file() const { return this->source_file; }
        string get_func_name() const { return this->func_name; }
    };
}

#endif /* FG_ALIAS_HPP */

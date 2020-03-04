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

#ifndef FG_INSTRU_HPP
#define FG_INSTRU_HPP

#include "fg-defuse.hpp"
#include <cstring>

using namespace std;


namespace flowguard
{
    /* Generates the instrumentation of a function */
    class FGInstru
    {
        /* Map of def-use per source file function */
        unordered_map<string, unordered_map<string, FGDefUse *> > *defuses;
        /* Map that holds an identifier for each definition set */
        unordered_map<int, set<int> > *def_sets;
        /* Map that holds source file - function - varname - set id
           The key is the concatenation of file function varname with '+' in
           between (eg. some_file.c+main+var) */
        unordered_map<string, int> *srcfunvar_setid;
        /* current stmt's hash */
        int stmt_hash;
        /* Debug defsets */
        bool debug = false;
    public:
        /* Set counter */
        static int set_id;
        FGInstru(unordered_map<string,
                 unordered_map<string, FGDefUse *> > *defuses,
                 unordered_map<int, set<int> > *def_sets,
                 unordered_map<string, int> *srcfunvar_setid, bool debug);
        /* Organises the definitions into unique definition sets. */
        void generate_definition_sets();
        /* Returns the set for the given var, -1 if not found */
        int
        get_setid_for_var(const char *var, const char *fun, const char *src);
        /* Checks if the variable has uses, if it doesn't there is no need to
           instrument the definition */
        bool has_uses(const char *var, const char *fun,  const char *src);
        int get_stmt_hash() { return this->stmt_hash; }
        void set_stmt_hash(int stmt_hash) { this->stmt_hash = stmt_hash; }
    private:
        /* Constructs a varname in the form src+func+var int he given dest */
        void gen_srcfunvar_name(const char *src, int len_src, const char *func,
                                int len_func, const char *var, int len_var,
                                char *dest);
    };
} /* namespace flowguard */
#endif /* FG_INSTRU_HPP */

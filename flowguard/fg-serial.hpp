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

#ifndef FG_SERIAL_HPP
#define FG_SERIAL_HPP
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <set>
#include <string>
#include <sys/stat.h>

using namespace std;


namespace flowguard
{
    /* Writes the given map to a file */
    void write_dfg(unordered_map<string, int> *var_setid,
                   unordered_map<int, set<int> > *setid_defs,
                   string dfgpath = "");
    /* Overload << to dump unordered_map<int, set<int>> */
    ostream& operator<<(ostream& os, unordered_map<int, set<int> >& ob);
    /* Overload << to dump unordered_map<string, int> */
    ostream& operator<<(ostream&os, unordered_map<string, int>& ob);
    /* Given the path of the setid-defs file, this method parses the file to
       find the highest set id */
    int get_highest_set(string setid_defs_path);
}

#endif /* FG_SERIAL_HPP */

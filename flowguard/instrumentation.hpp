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

#ifndef INSTRUMENTATION_HPP
#define INSTRUMENTATION_HPP
#include <cstdio>
#include <cerrno>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <set>
#include <vector>
#include <signal.h>
#include <iomanip>


#define debug_fg 1
#define show_errors 1

using namespace std;


extern int _binary_fg_dfg_vars_fg_start;
extern int _binary_fg_dfg_sets_fg_start;
extern int _binary_fg_dfg_vars_fg_size;
extern int _binary_fg_dfg_sets_fg_size;


/* Table filled at runtime with each var definition. */
unordered_map<string, set<int> > g_rdt;
/* To cleanup the g_rdt faster after a function ends we have this map which
   references all the variables a given srcfun has. */
unordered_map<string, set<string> > func_vars;
/* Static srcfunvar-setid map */
unordered_map<string, int> g_static_var_setid;
/* Static setid- set defs map */
unordered_map<int, set<int> > g_static_setid_defs;
/* Flag to mark that we have loaded the static dfg */
bool g_statics_loaded = false;


unordered_map<long, set<int> > runtime_defs_table;
unordered_map<int, long> runtime_phi_code_address_table;
unordered_map<int, unordered_map<long, int> > runtime_phi_id_to_addr_to_setid;


static vector<string> split_v(const string &s, char delim);
static bool dfi_holds(set<int> &static_, set<int> &runtime);
static void
update_func_vars(string &srcfunvar, unordered_map<string, set<string> > *map);
static void
_load_statics(vector<string> &setid_defs_lines,
              vector<string> &var_setid_lines);

void fg_load_statics(void);
void fg_load_statics(const char *path);
void fg_set_def_addr(void *ptr, int id);
void fg_check_def_addr(void *ptr, int setid);
void fg_set_phi_to_var(int phi_id, int set_id, void *ptr);
void fg_check_phi(int phi_id, void *ptr);
#endif /* INSTRUMENTATION_HPP */

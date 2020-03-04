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

#include "fg-alias.hpp"

flowguard::FGAlias::FGAlias()
{
}

flowguard::FGAlias::FGAlias(string source_file, string func_name)
{
    this->source_file = source_file;
    this->func_name = func_name;
}

void flowguard::FGAlias::add_aliased_symbol(string &sym)
{
    this->aliased_sym_toadd.insert(sym);
}

void flowguard::FGAlias::collect_uuid_ssaname(unsigned uuid, char *ssa_name)
{
    assert(this->uuid_to_ssaname.count(uuid) == 0);
    string tmp(ssa_name);
    this->uuid_to_ssaname.emplace(make_pair(uuid, tmp));
}


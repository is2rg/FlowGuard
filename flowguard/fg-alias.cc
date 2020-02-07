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


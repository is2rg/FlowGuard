#include "fg-serial.hpp"

void
flowguard::write_dfg(unordered_map<string, int> *var_setid,
                     unordered_map<int, set<int> > *setid_defs, string dfgpath)
{
    string varsetfile("fg_dfg_vars.fg");
    string setiddefsfile("fg_dfg_sets.fg");
    if (dfgpath.length() > 0)
    {
        varsetfile = dfgpath + "/" + varsetfile;
        setiddefsfile = dfgpath + "/" + setiddefsfile;
    }
    struct stat statbuf;
    if (stat(varsetfile.c_str(), &statbuf) != 1)
    {
        // file exists
        ofstream varset(varsetfile, std::ios_base::app);
        varset << *var_setid;
        varset.close();
        ofstream setiddefs(setiddefsfile, std::ios_base::app);
        setiddefs << *setid_defs;
        setiddefs.close();
    }
    else
    {
        ofstream varset;
        varset.open(varsetfile);
        varset << *var_setid;
        varset.close();
        ofstream setiddefs;
        setiddefs.open(setiddefsfile);
        setiddefs << *setid_defs;
        setiddefs.close();
    }
}

ostream&
flowguard::operator<<(ostream& os, unordered_map<int, set<int>>& obj)
{
    string nl("\n");
    string space(" ");
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        os << to_string(it->first) << nl;
        for (auto n : it->second)
        {
            os << to_string(n);
            os << space;
        }
        os << nl;
    }
    return os;
}

ostream&
flowguard::operator<<(ostream&os, unordered_map<string, int>& obj)
{
    string nl("\n");
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        os << it->first << nl;
        os << to_string(it->second) << nl;
    }
    return os;
}

int flowguard::get_highest_set(string setid_defs_path)
{
    ifstream setsfile(setid_defs_path);
    string line;
    bool nowset = true, noterror = true;
    int highest_set = 0;
    while (std::getline(setsfile, line) && noterror)
    {
        if (nowset)
        {
            nowset = false;
            try
            {
                int setid = std::stoi(line);
                if (setid > highest_set)
                    highest_set = setid;
            }
            catch (const invalid_argument &ia)
            {
                // this will raise a compiler error
                highest_set = -1;
                noterror = false;
            }
        }
        else
        {
            nowset = true;
        }
    }
    setsfile.close();
    return highest_set;
}

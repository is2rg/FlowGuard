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

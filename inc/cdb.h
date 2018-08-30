#ifndef CDB_H
#define CDB_H

#include <string>
#include <vector>

using namespace std;

struct cdbKey
{
	cdbKey():isPrimary(false){}
	string name;
	string type;
	bool isPrimary;
};

struct cdbRecord
{
	string name;
	vector<string> PrimaryKey;
	vector<cdbKey> Keys;
};

class CCdb
{
public:
	CCdb(string file):filename(file)
	{}

	void scan();
	cdbKey *getCdbKeyWithName(string &name);

public:
	string filename;
	cdbRecord record;
};

#endif // CDB_H

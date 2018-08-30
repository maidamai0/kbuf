#include "cdb.h"
#include "common.h"

#include <fstream>


void CCdb::scan()
{
	ifstream input(filename);

	if(!input.is_open())
	{
		printf("open %s falied\n", filename.c_str());
	}

	string line;
	while(getline(input, line))
	{
		vector<string> vec;
		SplitWithSpace(line, vec);

		if(vec.size() < 2)
		{
			continue;
		}

		// head
		if(vec[0] == "create" && vec[1] == "table")
		{
			record.name = vec[2];
			continue;
		}

		// primary key
		if(vec[0] == "primary" && vec[1] == "key")
		{
			for(size_t i=2; i<vec.size(); ++i)
			{
				string name = vec[i];
				if(name == "hash")
				{
					break;
				}

				record.PrimaryKey.push_back(name);
				getCdbKeyWithName(name)->isPrimary = true;
			}
			continue;
		}

		// ordinary key
		cdbKey cKey;
		cKey.name = vec[0];
		cKey.type = vec[1];
		record.Keys .push_back(cKey);
	}
}

cdbKey* CCdb::getCdbKeyWithName(string &name)
{
	cdbKey *pKey = nullptr;
	for(auto & key : record.Keys)
	{
		if(key.name == name)
		{
			pKey = &key;
		}
	}

	if(!pKey)
	{
		printf("primary key \"%s\" undeclared in file:%s\n", name.c_str(), filename.c_str());
		abort();
	}

	return pKey;
}

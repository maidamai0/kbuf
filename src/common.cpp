#include "common.h"

#include <string>

using namespace std;


const static char lowstart = 'a' - 1;
const static char lowend = 'z' + 1;

const static char upstart = 'A' - 1;
const static char upend = 'Z' + 1;

const static char dif = 'A' - 'a';

void ToLowCase(string &str)
{
	for(auto & c : str)
	{
		c = tolower(c);
	}

	return;
}

void ToUpperCase(string &str)
{
	for(auto & c : str)
	{
		c = toupper(c);
	}

	return;
}

void ToLowCase(char &c)
{
	if(c > upstart && c < upend)
	{
		c -= dif;
	}

	return;
}

void ToUpperCase(char &c)
{
	if(c > lowstart && c < lowend)
	{
		c += dif;
	}

	return;
}

void NoExtention(string &fileName)
{
	size_t pos = fileName.find_last_of(".");
	fileName = fileName.substr(0, pos);

	ToLowCase(fileName);

	return;
}

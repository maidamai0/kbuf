#ifndef DATAWAPPEREGENERATOR_H
#define DATAWAPPEREGENERATOR_H

#include "protogenerator.h"
#include "sstream"

class DataWappereGenerator
{
public:
	DataWappereGenerator(const ProtoMessage &msg);
	void GenerateDataWapper();

private:

	void HeadGuard();
	void License();
	void Headers();
	void NameSpace();
	void FileStaticVar();

	void Schema();

	void ClassStart();
	void ClassEnd();

	void pub();
	void pri();

	void TypeDef();
	void create();
	void createWithData();
	void construct();
	void destruct();
	void Release();
	void InitSchema();

	void ToString();
	void ToStringByProto();
	void ToStringWriter();
	void ToStringWithSpecifiedField();
	void FromString();
	void FromStringNoCheck();
	void FromStringProto();
	void Serialize();
	void DeSerial();
	void getSet();

	void ObjectBegin();
	void set(string fun, string type);

	void CaseValue(const JsonKey &field, bool intBeStrinig = false);

	void Variable();


	void Write(string str="");
	void WriteWithNewLine(string str="");

	string TextToCpp(string src);


private:

	const ProtoMessage &m_msg;
	bool m_bGuardStart;
	ofstream m_dstFile;
	unsigned m_nIdent;
	char m_charArrTmp[1024];
	bool m_bHasEntryTime;
};

#endif // DATAWAPPEREGENERATOR_H

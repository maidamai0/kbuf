#ifndef PROTOGENERATOR_H
#define PROTOGENERATOR_H

#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "common.h"

using namespace std;
using namespace rapidjson;

struct JsonKey
{
	JsonKey():len(0), required(false),canBeStr(false)
	{}

	string name;
	string type;
	string fset;
	string fget;
	size_t len;
	bool required;
	bool canBeStr;			// ��integer�͵����ݣ��Ƿ������string
};

struct ProtoMessage
{
	ProtoMessage():isArrray(false){}

	string fileName;		// û�к�׺��,import,include��ʹ��
	string name;			// message�����ƣ���c++��������Ӧ
	string fieldName;		// ��Ϊ����message�е�һ���ֶ�ʱ���ֶε�����
	hash_t hash;			// fieldname�Ĺ�ϣֵ
	string fadder;			// ��Ϊ����message�е�һ���ֶΣ�add����/mutable
	string fsize;			// ��Ϊ����message�е�һ���ֶΣ�size����
	string fget;			// ��Ϊ����message�е�һ���ֶΣ�get����
	bool isArrray;			// ��Ϊ����message�е�һ���ֶ�ʱ���Ƿ�Ϊ����

	// �������ֶ�
	map<string , JsonKey> m_mapFields;

	// ���������ֶΣ�������������message
	// first:field name��second:message
	vector<ProtoMessage> m_VecSubMsg;

	string m_strSchema;
};

class protoGenerator
{
public:
	explicit protoGenerator(string srcFileName);

	bool GeneratorProto();
private:
	bool scan();
	bool GenerateSchema(string file, rapidjson::PrettyWriter<rapidjson::StringBuffer> &w);
	bool WriteSchemaValue(rapidjson::PrettyWriter<rapidjson::StringBuffer> &w,
						  const GenericMember<UTF8<char>, MemoryPoolAllocator<CrtAllocator>> &object);
	void write();
	bool isComplexType(string type);
	void log(string str);

public:
	ProtoMessage m_msg;

private:
	ifstream m_srcFile;
	ofstream m_dstFile;
	map<string, string> m_mapType;			// json���͵�protobuf���͵�ӳ��
	string m_strFileNameNoExt;
	ofstream m_log;
};

#endif // PROTOGENERATOR_H

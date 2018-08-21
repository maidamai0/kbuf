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

#include "spdlog/spdlog.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/logger.h"

#include "common.h"

using namespace std;
using namespace rapidjson;

struct JsonKey
{
	JsonKey():len(0), required(false),intCanBeStr(false),isTime(false)
	{}

	string name;
	string type;
	string fset;
	string fget;
	size_t len;
	bool required;
	bool intCanBeStr;			// string/int from json, int in protobuf, int in elasticsearch
	bool stringCanBeInt;		// string/int from json, string in protobuf, int in elasticsearch
	bool isTime;				// time
	bool isEntryTime;			// is EntryTime
};

struct ProtoKey
{
	 ProtoKey(string type, string name, bool array=false):
		 type(type),name(name),isArray(array)
	 {}

	string type;
	string name;
	bool isArray;
};

struct ProtoMessage
{
    ProtoMessage():isArrray(false), modifyTime(0){}

	string fileName;		// 没有后缀名,import,include是使用
	string name;			// title in json schema, proto message name is name_Proto, c++ class name is CName
	string fieldName;		// 作为其他message中的一个字段时，字段的名称
	hash_t hash;			// fieldname的哈希值
	string fadder;			// 作为其他message中的一个字段，add函数/mutable
	string fsize;			// 作为其他message中的一个字段，size函数
	string fget;			// 作为其他message中的一个字段，get函数
	bool isArrray;			// 作为其他message中的一个字段时，是否为数组
    time_t modifyTime;      // schema file modify time

	// 简单类型字段
	vector<JsonKey> m_vecFields;

	// 复杂类型字段，即类型是其他message
	// first:field name，second:message
	vector<ProtoMessage> m_VecSubMsg;

	vector<ProtoKey> m_VecProtoKey;		// for order in protobuf

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

public:
	ProtoMessage m_msg;

private:
	ifstream m_srcFile;
	ofstream m_dstFile;
	map<string, string> m_mapType;			// json类型到protobuf类型的映射
	string m_strFileNameNoExt;
};

#endif // PROTOGENERATOR_H

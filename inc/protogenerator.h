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

/*
 * direcotry tree
 * json
 *      schema          // schema files
 *      include         // c++ class wrap protobuf class header files
 *      protobuf        // protouf relate files, .proto, pb.h, pb.cpp
 *          src         // .pb.cpp
 *          proto       // .proto
 *          include     //  .pb.cpp
 *
 * current path is json/schema/
*/
const string protobufProtoPath  =   "../protobuf/proto";
const string protobufSrcPath    =   "../protobuf/src";
const string protoufIncludePath =   "../protobuf/include";
const string cppHeadPath        =   "../include";

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
	bool canBeStr;			// 对integer型的数据，是否可以是string
};

struct ProtoMessage
{
    ProtoMessage():isArrray(false), modifyTime(0){}

	string fileName;		// 没有后缀名,import,include是使用
	string name;			// message的名称，和c++的类名对应
	string fieldName;		// 作为其他message中的一个字段时，字段的名称
	hash_t hash;			// fieldname的哈希值
	string fadder;			// 作为其他message中的一个字段，add函数/mutable
	string fsize;			// 作为其他message中的一个字段，size函数
	string fget;			// 作为其他message中的一个字段，get函数
	bool isArrray;			// 作为其他message中的一个字段时，是否为数组
    time_t modifyTime;      // schema file modify time

	// 简单类型字段
	map<string , JsonKey> m_mapFields;

	// 复杂类型字段，即类型是其他message
	// first:field name，second:message
	vector<ProtoMessage> m_VecSubMsg;

	string m_strSchema;
};

class protoGenerator
{
public:
	explicit protoGenerator(string srcFileName);

	bool GeneratorProto();
private:
    bool Scan();
	bool GenerateSchema(string file, rapidjson::PrettyWriter<rapidjson::StringBuffer> &w);
	bool WriteSchemaValue(rapidjson::PrettyWriter<rapidjson::StringBuffer> &w,
						  const GenericMember<UTF8<char>, MemoryPoolAllocator<CrtAllocator>> &object);
    void Write();
    bool IsComplexType(string type);
    string GetTypeFromSchemaFile(string schemaFile);
    time_t GetFileModifyTime(string fileName);

public:
	ProtoMessage m_msg;

private:
	ifstream m_srcFile;
	ofstream m_dstFile;
	string m_strFileNameNoExt;
    time_t schemaModifyTime;
};

#endif // PROTOGENERATOR_H

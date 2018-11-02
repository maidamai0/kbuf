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

///
/// \brief scope of a key.
/// global	-> exist in json/protobuf/database, default value when not specified
/// db		-> database
/// js		-> json(protocol)
/// internal-> only exist in protobuf, used internal

const uint8_t KS_internal	= 0;
const uint8_t KS_db		= 1;
const uint8_t KS_js		= 1 << 1;
const uint8_t KS_global	= KS_db | KS_js;

struct JsonKey
{
	JsonKey():len(0), required(false),isTime(false),isEntryTime(false),isGeoPoint(false)
	  ,isNumberStr(false),isCreatTime(false),isExpiredTime(false)
	  ,scope(KS_global),isBoolean(false)
	{}

	string name;
	string type;
	string fset;
	string fget;
	size_t len;
	bool required;
	bool isTime;				// time
	bool isEntryTime;			// is EntryTime
	bool isGeoPoint;			// longitude or latitude
	string protoType;			// type in protobuf,can be string/int64 when type type==string && isNumber
	bool isNumberStr;			// string contans all digit
	bool isCreatTime;			// auto generated create time
	bool isExpiredTime;			// auto generated create time
	uint8_t scope;
	string dbType;				// type in database
	bool isBoolean;				// bool can be string or int;
};

// key-value in protobuf
// used to keep the same order in schema and protobuf
struct ProtoKey
{
	 ProtoKey(string type, string name, bool array=false):
		 type(type),name(name),isArray(array)
	 {}

	 ProtoKey():isArray(false)
	 {}

	string type;
	string name;
	bool isArray;
};

// geoPoint type in elasticSearch
struct CGeoPoint
{
	CGeoPoint(){}

	JsonKey lon;
	JsonKey lat;
	string name;
};

struct ProtoMessage
{
	ProtoMessage():isArrray(false), modifyTime(0), isNew(false),
	bHasEntryTime(false), bHasExpireDate(false), bAlone(true){}

	string fileName;		// filename with no postfix from schema file
	string name;			// title in json schema, proto message name is name_Proto, c++ class name is CName
	string fieldName;		// field name in another message
	hash_t hash;			// fieldname的哈希值
	string fadder;			// 作为其他message中的一个字段，add函数/mutable
	string fsize;			// 作为其他message中的一个字段，size函数
	string fget;			// 作为其他message中的一个字段，get函数
	bool isArrray;			// 作为其他message中的一个字段时，是否为数组
	time_t modifyTime;      // schema file modify time
	bool isNew;				// skip new file
	bool bHasEntryTime;
	bool bHasExpireDate;
	bool bAlone;			// whether this message can be saved alone

	// basic type
	vector<JsonKey> m_vecFields;

	// compound type
	// first:field name，second:message
	vector<ProtoMessage> m_VecSubMsg;

	// for order in protobuf
	vector<ProtoKey> m_VecProtoKey;

	CGeoPoint GeoPoint;

	// schema string, used to initialize schema of Rapidjson
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
	bool isLon(string str);
	bool isLat(string str);
	void write();
    bool isComplexType(string type);
	int Runcmd(const char * cmd);

	///
	/// \brief get scope of key
	/// \param can be "pub", "db"; "pub|db" means "pub" and "db"
	/// \return
	///
	unsigned char getScope(const string &scope);
	void getScopeTestRun(const string &scope);
	void getScopeTest(const string &scope, uint8_t actual);

public:
	ProtoMessage m_msg;

private:
	ifstream m_srcFile;
	ofstream m_dstFile;
	map<string, string> m_mapType;			// json类型到protobuf类型的映射
	string m_strFileNameNoExt;
};

#endif // PROTOGENERATOR_H

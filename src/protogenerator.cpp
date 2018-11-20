#include "protogenerator.h"

#include <ctime>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

using namespace rapidjson;

extern spdlog::logger *g_logger;

__attribute__((unused)) static const char* kTypeNames[] =
{
	"Null", "False", "True", "Object", "Array", "String", "Number"
};

// path for protobuf
static const string protoPath = "../protobuf/proto/";
static const string srcPath = "../protobuf/src/";
static const string incPath = "../protobuf/include/";

protoGenerator::protoGenerator(string src)
{
	size_t pos = src.find_last_of(".");
	m_strFileNameNoExt = src.substr(0, pos);
	m_msg.fileName = m_strFileNameNoExt;

	m_mapType["integer"] = "int64";
	m_mapType["number"]  = "double";
	m_mapType["boolean"] = "bool";
	m_mapType["string"] = "string";
}

bool protoGenerator::GeneratorProto()
{
	char cwd[1024] = {0};

	if(!scan())
	{
		return false;
	}

	string file(m_strFileNameNoExt + ".json");

	// log
	g_logger->trace("{} scan complete", file);


	// write schema
	rapidjson::StringBuffer buffer;
	PrettyWriter<StringBuffer> OutWriter(buffer);

	GenerateSchema(file, OutWriter);
	g_logger->trace("{} schema generated", file);

	m_msg.m_strSchema = buffer.GetString();

	// write protobuf source file
	write();

	// log
	g_logger->trace("{} write copmplete",
					protoPath + m_strFileNameNoExt + ".proto");

	char cmd[1024] = {0};

	// invoke protobuf compiler
	sprintf(cmd, "protoc --proto_path=%s --cpp_out=%s %s\n",
			protoPath.c_str(),
			srcPath.c_str(),
			(m_strFileNameNoExt + ".proto").c_str());
	if(Runcmd(cmd))
	{
		return false;
	}

	// modify postfix for cpp files
	sprintf(cmd, "mv %s.pb.cc %s.pb.cpp",
			(srcPath + m_strFileNameNoExt).c_str(),
			(srcPath + m_strFileNameNoExt).c_str());
	Runcmd(cmd);

	// move head files to incpath
	struct stat dirInfo;
	if(stat(incPath.c_str(), &dirInfo))
	{
		// dir not exist
		mkdir(incPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
//		sprintf(cmd, "");
//		Runcmd("mkdir -p ./include");
	}
	sprintf(cmd, "mv %s.pb.h %s",
			(srcPath + m_strFileNameNoExt).c_str(),
			incPath.c_str());
	Runcmd(cmd);

	getcwd(cwd, sizeof (cwd));
	g_logger->info("return to {}...", cwd);

	return true;

}

int protoGenerator::Runcmd(const char * cmd)
{
	g_logger->info(cmd);
	return system(cmd);
}

bool protoGenerator::CheckSchema(string &file, Document &schema)
{
	if(schema.HasParseError())
	{
		g_logger->error("invalid json file:{}", file);
		return false;
	}

	if(!schema.HasMember("title"))
	{
		g_logger->error("no title found in {}", file);
		return false;
	}
	m_msg.name = schema["title"].GetString();

	if(!schema.HasMember("type"))
	{
		g_logger->error("no type found in {}", file);
		return false;
	}

	if(schema.HasMember("alone"))
	{
		m_msg.bAlone = schema["alone"].GetInt();
	}

	if(!schema.HasMember("properties"))
	{
		g_logger->error("no properties found in {}", file);
		return false;
	}

	if(schema["properties"].GetType() != kObjectType)
	{
		g_logger->error("properties is not a object");
		return false;
	}

	return true;
}

void protoGenerator::GetOneOf(const Document::Member & object)
{
	JsonKey key;
	key.name = object.name.GetString();

	const Value & oneOf = object.value["oneOf"];

	string firstType = m_mapType[oneOf[0]["type"].GetString()];
	string secondtType = m_mapType[oneOf[1]["type"].GetString()];
	if(firstType == "double" || secondtType == "double")
	{
		// double
		key.isDoubleString = true;
		key.type = "double";
		key.dbType = "double";
		key.protoType = "double";
	}
	else if(firstType == "int64" || secondtType == "int64")
	{
		key.isBoolean = true;
		key.type = "string";
		key.dbType = "int64";
		key.isNumberStr = true;
		key.protoType = firstType;
	}

	AddField(key);
}

void protoGenerator::GetRef(const Document::Member & object)
{
	string name = object.name.GetString();

	protoGenerator pg(object.value["$ref"].GetString());

	if(!pg.GeneratorProto())
	{
		return;
	}

	pg.m_msg.fieldName = name;

	// TODO FIXME
	string fset("mutable_");
	fset.append(pg.m_msg.fieldName);
	ToLowCase(fset);
	pg.m_msg.fadder = fset;

	string fsize(pg.m_msg.fieldName);
	fsize.append("_size");
	ToLowCase(fsize);
	pg.m_msg.fsize = fsize;

	string get(pg.m_msg.fieldName);
	ToLowCase(get);
	pg.m_msg.fget = get;

	m_msg.m_VecSubMsg.push_back(pg.m_msg);

	ProtoKey proKey(pg.m_msg.name,name);
	m_msg.m_VecProtoKey.push_back(proKey);
}

void protoGenerator::GetAlias(const Document::Member & object)
{
	string name = object.name.GetString();

	protoGenerator pg(object.value["alias"].GetString());

	if(!pg.GeneratorProto())
	{
		return;
	}

	pg.m_msg.fieldName = name;

	// TODO FIXME
	string fset("mutable_");
	fset.append(pg.m_msg.fieldName);
	ToLowCase(fset);
	pg.m_msg.fadder = fset;

	string fsize(pg.m_msg.fieldName);
	fsize.append("_size");
	ToLowCase(fsize);
	pg.m_msg.fsize = fsize;

	string get(pg.m_msg.fieldName);
	ToLowCase(get);
	pg.m_msg.fget = get;

	m_msg.m_VecSubMsg.push_back(pg.m_msg);

	ProtoKey proKey(pg.m_msg.name,name);
	m_msg.m_VecProtoKey.push_back(proKey);
}

void protoGenerator::GetArray(const Document::Member & object)
{
	string name = object.name.GetString();

	assert(object.value.HasMember("type"));
	assert(object.value["type"] == "array");

	string fileName = object.value["items"]["$ref"].GetString();
	protoGenerator pg(fileName);

	// TODO merge
	if(!pg.GeneratorProto())
	{
		return;
	}

	pg.m_msg.isArrray = true;
	pg.m_msg.fieldName = name;

	ProtoKey proKey(pg.m_msg.name, name, true);
	m_msg.m_VecProtoKey.push_back(proKey);


	// Case will be case in protobuf, and case is a key word in c/c++
	// protobuf will append a '_', case_
	// need special treatment
	if(name == "Case")
	{
		name = "Case_";
	}

	string fadd("add_");
	fadd.append(name);
	ToLowCase(fadd);
	pg.m_msg.fadder = fadd;

	string fsize(name);
	fsize.append("_size");
	ToLowCase(fsize);
	pg.m_msg.fsize = fsize;

	string fget(name);
	ToLowCase(fget);
	pg.m_msg.fget = fget;

	m_msg.m_VecSubMsg.push_back(pg.m_msg);
}

void protoGenerator::GetSimple(const Document::Member & object)
{
	JsonKey jsKey;

	jsKey.name = object.name.GetString();
	jsKey.type = m_mapType[object.value["type"].GetString()];
	jsKey.dbType = jsKey.type;
	jsKey.protoType = jsKey.type;

	if(object.value.HasMember("format"))
	{
		if(string(object.value["format"].GetString()) == "number")
		{
			jsKey.dbType = "int64";
			jsKey.protoType = "int64";
		}
	}

	if(object.value.HasMember("dbtype"))
	{
		jsKey.dbType = m_mapType[object.value["dbtype"].GetString()];
	}

	if(object.value.HasMember("ptype"))
	{
		jsKey.protoType = m_mapType[object.value["ptype"].GetString()];
	}

	if(object.value.HasMember("annotation"))
	{
		jsKey.scope = getScope(object.value["annotation"].GetString());
	}

	if(object.value.HasMember("empty"))
	{
		jsKey.canBeEmpty = object.value["empty"].GetInt();
	}

	CheckTimeField(jsKey);

	AddField(jsKey);
}

void protoGenerator::CheckTimeField(JsonKey &jsKey)
{
	// XXXTime is date time
	string t = "Time";
	if(jsKey.name.length() > t.length())
	{
		if(jsKey.name.compare(jsKey.name.length()-t.length(), t.length(), t) == 0 ||
				jsKey.name == "TimeUpLimit" ||
				jsKey.name =="TimeLowLimit" ||
				jsKey.name == "ShotTimeEx")
		{
			jsKey.isTime = true;
			jsKey.protoType = "int64";
		}

		if(jsKey.name == "KDExpiredDate")
		{
			jsKey.isExpiredTime = true;
			m_msg.bHasExpireDate = true;
			jsKey.protoType = "int64";
		}

		if(jsKey.name == "EntryTime")
		{
			jsKey.isEntryTime = true;
			m_msg.bHasEntryTime = true;
		}

		if(jsKey.name == "CreatTime" && m_msg.name == "Disposition_Proto")
		{
			jsKey.isCreatTime = true;
		}
	}
}

bool protoGenerator::scan()
{
	// get schema file name, always end with ".json"
	string src = m_strFileNameNoExt;
	src.append(".json");
	g_logger->trace("scan {}", src.c_str());

	FileWrapper fw(src);

	if(!fw.fp)
	{
		g_logger->error("open {} failed:{}", fw.name, strerror(errno));
		return false;
	}

	char readBuffer[65536];
	FileReadStream is(fw.fp, readBuffer, sizeof(readBuffer));
	Document schema;
	schema.ParseStream(is);

	if(!CheckSchema(fw.name, schema))
	{
		return false;
	}

	Value pop = schema["properties"].GetObject();

	for(const auto &object : pop.GetObject())
	{

		if(object.value.HasMember("oneOf"))
		{
			GetOneOf(object);
		}
		else if(object.value.HasMember("$ref"))
		{
			GetRef(object);
		}
		else if(object.value.HasMember("alias"))
		{
			GetAlias(object);
		}
		else if(object.value.HasMember("items"))
		{
			GetArray(object);
		}
		else if(object.value.HasMember("type"))
		{
			GetSimple(object);
		}
	}

	return true;
}

void protoGenerator::write()
{
	struct stat stInfo;
	if(stat("../protobuf/proto", &stInfo))
	{
		// dir not exist
		Runcmd("mkdir -p ../protobuf/proto");
	}

	string dst("../protobuf/proto/");
	dst.append(m_strFileNameNoExt);
	dst.append(".proto");

	if(!stat(dst.c_str(), &stInfo))
	{
		// proto file exists
		time_t protoMtime = stInfo.st_mtim.tv_sec;
		string schema(m_strFileNameNoExt);
		schema.append(".json");

		stat(schema.c_str(), &stInfo);
		time_t schemaMtime = stInfo.st_mtim.tv_sec;

		if(protoMtime > schemaMtime)
		{
			// FIXME this not work, schema string has dependency
			// proto file is newer than schema file
			g_logger->info("{} is newer than {} skip...", dst, schema);
			m_msg.isNew = true;
			return;
		}
	}

	m_dstFile.open(dst.c_str());
	if(!m_dstFile.is_open())
	{
		g_logger->error("open %s failed", dst.c_str());
		return;
	}

	// proto version
	m_dstFile << "syntax = \"proto3\";" << endl;
	m_dstFile << endl;

	// java proto name
	//	m_dstFile << "option java_package = \"com.keda.protobuf\";" << endl;
	//	m_dstFile << endl;
	if(m_msg.name == "AnalysisNotify_Proto")
	{
//		m_dstFile << "option java_package = \"com.keda.protobuf\";" << endl;
		m_dstFile << "option java_outer_classname = \"AnalysisNotifyProtobuf\";" << endl;
		m_dstFile << endl;
	}

	m_dstFile << "option java_package = \"com.keda.viid.proto\";" << endl << endl;

	// import
	vector<string> imports;
	for(const auto &msg : m_msg.m_VecSubMsg)
	{
		string import(msg.fileName);

		// subimageinfo need special treatment for compatibality(bad)
		// use subimageinfo[{subimageinfo},{subimageinfo}] instead of
		// SubImageList:{subimageinfo[{subimageinfo},{subimageinfo}]}
		if(msg.fieldName == "SubImageList")
		{
			import = "subimageinfo";
		}
		else if(msg.fieldName == "FeatureList")
		{
			import = "feature";
		}

		// kbuf will resolve dependency
		// avoid duplicate imports
		if(find(imports.begin(), imports.end(), import) == imports.end())
		{
			imports.push_back(import);
		}
	}

	for(const string & import : imports)
	{
		m_dstFile << "import \"" << import << ".proto" << "\";" << endl;
	}
	m_dstFile << endl;

	// message name
	m_dstFile << "message " << m_msg.name << "{" << endl;

	unsigned i = 1;	// protobug need a number for every field
	for(const auto & key : m_msg.m_VecProtoKey)
	{
		string line;

		do{
			// subimageinfo
			if(key.name == "SubImageList")
			{
				line.append("repeated SubImageInfo_Proto SubImageList = ");
				line.append(to_string((i)));
				line.append(";");
				break;
			}

			// FeatureList
			if(key.name == "FeatureList")
			{
				line.append("repeated Feature_Proto FeatureList = ");
				line.append(to_string((i)));
				line.append(";");
				break;
			}

			if(key.isArray)
			{
				line.append("repeated ");
			}

			line.append(key.type);
			line.append("\t");
			line.append(key.name);
			line.append(" = ");
			line.append(to_string(i));
			line.append(";");

		}while(false);

		m_dstFile << "\t" << line << endl;
		++i;
	}

	m_dstFile << "}" << endl;

	g_logger->trace("{} generated", dst.c_str());

	m_dstFile.close();

	return;
}

bool protoGenerator::GenerateSchema(string file, rapidjson::PrettyWriter<StringBuffer> &w)
{
	FILE *fp = fopen(file.c_str(), "rb");

	if(!fp)
	{
		g_logger->error("open {} failed", file.c_str());
		return false;
	}

	char readBuffer[65536];
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	Document d;
	d.ParseStream(is);

	if(d.HasParseError())
	{
		g_logger->error("invalid json file:{}", file);
		return false;
	}

	assert(d.IsObject());

	w.StartObject();

	for(const auto &object : d.GetObject())
	{
		WriteSchemaValue(w, object);
	}

	w.EndObject();

	fclose(fp);

	return true;
}

bool protoGenerator::WriteSchemaValue(rapidjson::PrettyWriter<rapidjson::StringBuffer> &w,
									  const GenericMember<UTF8<char>, MemoryPoolAllocator<CrtAllocator>> &object)
{

	if(object.value.IsObject())
	{
		w.String(object.name.GetString());
		if(object.value.HasMember("$ref"))
		{
			string file = object.value["$ref"].GetString();
			GenerateSchema(file, w);
		}
		else
		{
			w.StartObject();

			for(const auto &subObject : object.value.GetObject())
			{
				WriteSchemaValue(w, subObject);
			}

			w.EndObject();
		}
	}
	else if(object.value.IsArray()) // "oneOf"[{},{}]
	{
		w.String(object.name.GetString());
		w.StartArray();

		for(const auto &value : object.value.GetArray())
		{
			if(value.IsString())
			{
				w.String(value.GetString());
			}
			else if(value.IsObject())
			{
				w.StartObject();

				for(const auto &subObject : value.GetObject())
				{
					WriteSchemaValue(w, subObject);
				}

				w.EndObject();
			}
		}

		w.EndArray();
	}
	else if(object.value.IsInt64())
	{
		if(object.name != "alone" &&
			object.name != "empty")
		{
			w.String(object.name.GetString());
			w.Int64(object.value.GetInt64());
		}
	}
	else if(object.value.IsNumber())
	{
		w.String(object.name.GetString());
		w.Double(object.value.GetDouble());
	}
	else if(object.value.IsString())
	{
		if(object.name != "description" &&
			object.name != "annotation" &&
			object.name != "ptype" &&
			object.name != "dbtype" &&
			object.name != "alias")
		{
			w.String(object.name.GetString());
			w.String(object.value.GetString());
		}
	}
	else
	{
		// should not be here!
		g_logger->error("Field %s has Wrong type:%d\n",
			   object.name.GetString(),
			   object.value.GetType());
	}

	return true;
}

bool protoGenerator::isComplexType(string type)
{
	return (type == "object" || type == "array");
}

uint8_t protoGenerator::getScope(const string &scope)
{
	string temp;
	vector<string> VecScopes;
	uint8_t ret = KS_internal;

	// split scopes
	for(const auto & c : scope)
	{
		if(c!=' ' && c!='\t')
		{
			if(c == '|')
			{
				VecScopes.push_back(temp);
				temp.clear();
			}
			else
			{
				temp.push_back(c);
			}
		}

		if(!temp.empty())
		{
			VecScopes.push_back(temp);
		}
	}


	// calculate value
	for(const auto & v : VecScopes)
	{
		if(v == "js")
		{
			ret |= KS_js;
		}
		else if(v == "db")
		{
			ret |= KS_db;
		}
		else if(v == "internal")
		{
			ret = KS_internal;
			break;
		}
		else if(v == "global")
		{
			ret = KS_global;
			break;
		}
	}

	return ret;
}

void protoGenerator::getScopeTestRun(const string &scope)
{
	getScopeTest("js", KS_js);
	getScopeTest("db", KS_db);
	getScopeTest("db|js", KS_global);
}

void protoGenerator::getScopeTest(const string &scope, uint8_t expected)
{
	uint8_t actual = getScope(scope);

	if(actual != expected)
	{
		printf("FAILED:exprcted:%d, get:%d, original string %s:\n",
			   expected, actual, scope.c_str());
	}
	else
	{
		printf("PASSED");
	}
}

void protoGenerator::AddField(JsonKey &key)
{
	if(key.name  == "Direction" && m_msg.name == "GPSData_Proto")
	{
		// gpsdata/direction special handle
		key.name = "Orientation";
	}

	// setter
	key.fset = "set_";
	key.fset.append(key.name);
	ToLowCase(key.fset);

	// getter
	key.fget = key.name;
	ToLowCase(key.fget);

	if(isLon(key.name))
	{
		key.isGeoPoint = true;
		m_msg.GeoPoint.lon = key;

	}else if(isLat(key.name))
	{
		key.isGeoPoint = true;
		m_msg.GeoPoint.lat = key;

		if(key.name.find("ShotPlace") != string::npos)
		{
			m_msg.GeoPoint.name = "ShotPlaceLocation";
		}
		else
		{
			m_msg.GeoPoint.name = "Location";
		}
	}
	m_msg.m_vecFields.push_back(key);

	// protobug key
	ProtoKey proKey(key.protoType, key.name);

	// these two name has been wrong
	// but we wont't want to change protobuf
	if(proKey.name == "DisappearTime" && m_msg.name == "MotorVehicle_Proto")
	{
		proKey.name = "DisAppearTime";
	}

	if(proKey.name == "FaceDisappearTime" && m_msg.name == "Face_Proto")
	{
		proKey.name = "FaceDisAppearTime";
	}
	m_msg.m_VecProtoKey.push_back(proKey);
}

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

bool protoGenerator::scan()
{
	string src = m_strFileNameNoExt;
	src.append(".json");

	g_logger->trace("scan {}", src.c_str());

	FILE *fp = fopen(src.c_str(), "rb");

	if(!fp)
	{
		g_logger->error("open {} failed:{}", src.c_str(), strerror(errno));
		return false;
	}

	char readBuffer[65536];
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	Document d;
	d.ParseStream(is);

	if(d.HasParseError())
	{
		g_logger->error("invalid json file:{}", src);
		return false;
	}

    if(!d.HasMember("title"))
    {
        g_logger->error("no title found in {}", src.c_str());
        return false;
    }
	m_msg.name = d["title"].GetString();

    if(!d.HasMember("type"))
    {
        g_logger->error("no type found in {}", src.c_str());
        return false;
    }

    if(!d.HasMember("properties"))
    {
        g_logger->error("no properties found in {}", src.c_str());
        return false;
    }

    if(d["properties"].GetType() != kObjectType)
    {
        g_logger->error("properties is not a object");
        return false;
    }

	Value pop = d["properties"].GetObject();

	for(const auto &object : pop.GetObject())
	{
		string name = object.name.GetString();
		string type;
		string protType;
		string dbType;
		bool isNumberStr = false;
		bool isTime = false;
		bool isEntryTime = false;
		bool isCreateTime = false;
		bool isExpiredTime = false;
		uint8_t scope = KS_global; // default value when not specified

		if(object.value.HasMember("oneOf"))
		{
			// type must be int64
			type = "string";
			dbType = "int64";
			isNumberStr = true;

			// 这种字段，标准里的定义都是string，但是考虑到性能，在es里存的是int
			// 最开始的想法是，这种字段在protobuf中的字段全使用int64，但是在制作schema的时候
			// 只做了部分字段，生成proto文件交给其他部门使用了。为了不给其他部门造成额外的工作量
			// 以后这种字段在protobuf中的类型，以第一个为准，而不是全部使用int64
			// 不出意外，以后这种情况应该都是string
			const Value & multiType = object.value["oneOf"];

			// 在protobuf中使用第一个类型
			const Value &firstType = multiType[0];
			assert(firstType.HasMember("type"));
			protType = m_mapType[firstType["type"].GetString()];
		}
		else if(object.value.HasMember("$ref"))
		{
			// 非数组字段的引用
			string fileName = object.value["$ref"].GetString();

			protoGenerator pg(fileName);

			if(!pg.GeneratorProto())
			{
				return false;
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
			type = pg.m_msg.name;

			ProtoKey proKey(type,name);
			m_msg.m_VecProtoKey.push_back(proKey);

			continue;
		}
		else
		{
			assert(object.value.HasMember("type"));
			type = object.value["type"].GetString();

			if(type == "array")
			{
				// 数组字段引用
				assert(object.value.HasMember("items"));
				assert(object.value["items"].HasMember("$ref"));
				string fileName = object.value["items"]["$ref"].GetString();

				protoGenerator pg(fileName);

				if(!pg.GeneratorProto())
				{
					return false;
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

				type = pg.m_msg.name;

				continue;
			}
			else
			{
				type = m_mapType[object.value["type"].GetString()];

				if(object.value.HasMember("format"))
				{
					if(string(object.value["format"].GetString()) == "number")
					{
						dbType = "int64";
						protType = "int64";
					}
				}

				if(object.value.HasMember("ptype"))
				{
					protType = m_mapType[object.value["ptype"].GetString()];
				}

				if(object.value.HasMember("dbtype"))
				{
					dbType = m_mapType[object.value["dbtype"].GetString()];
				}

				if(object.value.HasMember("annotation"))
				{
					scope = getScope(object.value["annotation"].GetString());
				}

				// XXXTime is date time
				string t = "Time";
				if(name.length() > t.length())
				{
					if(name.compare(name.length()-t.length(), t.length(), t) == 0 ||
							name == "TimeUpLimit" ||
							name =="TimeLowLimit" ||
							name == "ShotTimeEx")
					{						
						isTime = true;
						protType = "int64";
					}

					if(name == "KDExpiredDate")
					{
						isExpiredTime = true;
						m_msg.bHasExpireDate = true;
						protType = "int64";
					}

					if(name == "EntryTime")
					{
						isEntryTime = true;
						m_msg.bHasEntryTime = true;
					}

					if(name == "CreatTime" && m_msg.name == "Disposition_Proto")
					{
						isCreateTime = true;
					}
				}
			}

			if(type.empty())
			{
				g_logger->warn("{}:{} has no type", src, name);
				return false;
			}
		}

		JsonKey key;

		if(name  == "Direction" && m_msg.name == "GPSData_Proto")
		{
			// gpsdata/direction special handle
			name = "Orientation";
		}

		key.name = name;

		key.type = type;
		if(protType.empty())
		{
			key.protoType = key.type;
		}
		else
		{
			key.protoType = protType;
		}

		if(dbType.empty())
		{
			key.dbType = key.protoType;
		}
		else
		{
			key.dbType = dbType;
		}

		key.isNumberStr = isNumberStr;
		key.isTime = isTime;
		key.isEntryTime = isEntryTime;
		key.isCreatTime = isCreateTime;
		key.isExpiredTime = isExpiredTime;
		key.scope = scope;

		unsigned len = 0;
		if(key.type == "string" && !key.isTime)
		{
			char msg[1024] = {};
			sprintf(msg, "field:%s", key.name.c_str());

			if(!(object.value.HasMember("maxLength")))
			{
				g_logger->info("{}:{} missed maxLength", src, key.name);
			}
			else
			{
				len = object.value["maxLength"].GetUint();
			}
		}

		key.fset = "set_";
		key.fset.append(key.name);
		ToLowCase(key.fset);

		key.fget = key.name;
		ToLowCase(key.fget);

		key.len = len;

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

		ProtoKey proKey(key.protoType, name);

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

	fclose(fp);

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
		m_dstFile << "option java_package = \"com.keda.protobuf\";" << endl;
		m_dstFile << "option java_outer_classname = \"AnalysisNotifyProtobuf\";" << endl;
		m_dstFile << endl;
	}

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
		w.String(object.name.GetString());
		w.Int64(object.value.GetInt64());
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
			object.name != "dbtype")
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

bool protoGenerator::isLon(string str)
{
	return (str == "Longitude" || str == "ShotPlaceLongitude");
}

bool protoGenerator::isLat(string str)
{
	return (str == "Latitude" || str == "ShotPlaceLatitude");
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

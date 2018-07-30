#include "protogenerator.h"

#include <ctime>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>

using namespace rapidjson;

extern spdlog::logger *g_logger;

__attribute__((unused)) static const char* kTypeNames[] =
{
	"Null", "False", "True", "Object", "Array", "String", "Number"
};

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

	m_log.open("log.txt");
	if(!m_log.is_open())
	{
        g_logger->critical("open log failed\n");
	}

	if(!scan())
	{
		return false;
	}
    g_logger->info("scan complete\n");

	rapidjson::StringBuffer buffer;
	PrettyWriter<StringBuffer> OutWriter(buffer);

	string file(m_strFileNameNoExt);
	file.append(".json");
	GenerateSchema(file, OutWriter);
	printf("schema generate\n");

	m_msg.m_strSchema = buffer.GetString();

	write();
	printf("write copmplete\n");

	char cmd[1024] = {0};

	string dst(m_strFileNameNoExt);
	dst.append(".proto");

	chdir("../protobuf/proto");
	getcwd(cwd, sizeof (cwd));
	cout << "current path:" << cwd << endl;

	sprintf(cmd, "protoc --cpp_out=../ $1 %s\n",dst.c_str());
	if(system(cmd))
	{
		return false;
	}

	chdir("../");
	getcwd(cwd, sizeof (cwd));
	cout << "current path:" << cwd << endl;
	// current path: json/protobuf

	struct stat dirInfo;
	if(stat("./src", &dirInfo))
	{
		// dir not exist
		system("mkdir -p ./src");
	}

	sprintf(cmd, "mv %s.pb.cc ./src/%s.pb.cpp",
			m_strFileNameNoExt.c_str(),
			m_strFileNameNoExt.c_str());
	system(cmd);

	if(stat("./include", &dirInfo))
	{
		// dir not exist
		system("mkdir -p ./include");
	}
	sprintf(cmd, "mv %s.pb.h ./include/%s.pb.h",
			m_strFileNameNoExt.c_str(),
			m_strFileNameNoExt.c_str());
	system(cmd);

	chdir("../schema");
	getcwd(cwd, sizeof (cwd));
	cout << "current path:" << cwd << endl;
	// current path: json/schema

	return true;

}

bool protoGenerator::scan()
{
	string src = m_strFileNameNoExt;
	src.append(".json");

    g_logger->info("scan {}", src.c_str());

	FILE *fp = fopen(src.c_str(), "rb");

	if(!fp)
	{
        g_logger->error("open {} failed", src.c_str());
		return false;
	}

	char readBuffer[65536];
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	Document d;
	d.ParseStream(is);

	if(d.HasParseError())
	{
		printf("invalid json file\n");
		return false;
	}

	assert(d.HasMember("title"));
	m_msg.name = d["title"].GetString();

	assert(d.HasMember("type"));
	assert(d.HasMember("properties"));

	assert(d["properties"].GetType() == kObjectType);

	Value pop = d["properties"].GetObject();

	for(const auto &object : pop.GetObject())
	{
		string name = object.name.GetString();
		string type;
		bool canBeString = false;

		string s(src);
		s.append(name);
		log(s);

		if(object.value.HasMember("oneOf"))
		{
			type = "int64";
			canBeString = true;
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

				string fadd("add_");
				fadd.append(pg.m_msg.fieldName);
				ToLowCase(fadd);
				pg.m_msg.fadder = fadd;

				string fsize(pg.m_msg.fieldName);
				fsize.append("_size");
				ToLowCase(fsize);
				pg.m_msg.fsize = fsize;

				string fget(pg.m_msg.fieldName);
				ToLowCase(fget);
				pg.m_msg.fget = fget;

				m_msg.m_VecSubMsg.push_back(pg.m_msg);

				type = pg.m_msg.name;
				continue;
			}
			else
			{
				type = m_mapType[object.value["type"].GetString()];
			}

			if(type.empty())
			{
				string s(src);
				s.append(" : ");
				s.append(name);
				s.append("has no type");
				log(s);
				printf("%s\n", s.c_str());
				return false;
			}
		}

		JsonKey key;
		key.name = name;
		key.type = type;
		key.canBeStr = canBeString;

		unsigned len = 0;
		if(key.type == "string")
		{
			char msg[1024] = {};
			sprintf(msg, "field:%s", key.name.c_str());

			if(!(object.value.HasMember("maxlength")))
			{
				string s(src);
				s.append(" : ");
				s.append(key.name);
				s.append("missd maxlength");

				log(s);
				printf("%s\n", s.c_str());
			}
			else
			{
				len = object.value["maxlength"].GetUint();
			}
		}

		key.fset = "set_";
		key.fset.append(key.name);
		ToLowCase(key.fset);

		key.fget = key.name;
		ToLowCase(key.fget);

		key.len = len;

		m_msg.m_mapFields[key.name] = key;
	}

	fclose(fp);

	return true;
}

/// \note 因为默认值的关系，所有字段都使用repeated
void protoGenerator::write()
{
	struct stat dirInfo;
	if(stat("../protobuf/proto", &dirInfo))
	{
		// dir not exist
		system("mkdir -p ../protobuf/proto");
	}

	string dst("../protobuf/proto/");
	dst.append(m_strFileNameNoExt);
	dst.append(".proto");

	m_dstFile.open(dst.c_str());
	if(!m_dstFile.is_open())
	{
		printf("open %s failed\n", dst.c_str());
		return;
	}

	// proto version
	m_dstFile << "syntax = \"proto3\";" << endl;
	m_dstFile << endl;


	// import
	for(const auto &msg : m_msg.m_VecSubMsg)
	{
		string import(msg.fileName);
		m_dstFile << "import \"" << import << ".proto" << "\";" << endl;
	}
	m_dstFile << endl;

	// message name
	m_dstFile << "message " << m_msg.name << "{" << endl;

	// 简单类型的字段
	unsigned i = 1;
	for(const auto &kv : m_msg.m_mapFields)
	{

		auto field = kv.second;
		string line;

		line.append(field.type);
		line.append("\t");
		line.append(field.name);
		line.append(" = ");
		line.append(to_string(i));
		line.append(";");

		m_dstFile << "\t" << line << endl;

		++i;
	}

	// 复杂类型的字段
	for(const auto &msg : m_msg.m_VecSubMsg)
	{
		string line;

		if(msg.isArrray)
		{
			line.append("repeated ");
		}

		line.append(msg.name);
		line.append("\t");
		line.append(msg.fieldName);
		line.append(" = ");
		line.append(to_string(i));
		line.append(";");

		m_dstFile << "\t" << line << endl;

		++i;
	}

	m_dstFile << "}" << endl;

	printf("%s generated\n", dst.c_str());

	return;
}

bool protoGenerator::GenerateSchema(string file, rapidjson::PrettyWriter<StringBuffer> &w)
{
	FILE *fp = fopen(file.c_str(), "rb");

	if(!fp)
	{
		printf("open %s failed\n", file.c_str());
		return false;
	}

	char readBuffer[65536];
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	Document d;
	d.ParseStream(is);

	if(d.HasParseError())
	{
		printf("invalid json file\n");
		return false;
	}

	assert(d.IsObject());

	w.StartObject();

	for(const auto &object : d.GetObject())
	{
		WriteSchemaValue(w, object);
	}

	w.EndObject();

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
	else if(object.value.IsArray())
	{
		w.String(object.name.GetString());
		w.StartArray();

		for(const auto &value : object.value.GetArray())
		{
			if(value.IsString())
			{
				w.String(value.GetString());
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
		w.String(object.name.GetString());
		w.String(object.value.GetString());
	}
	else
	{
		// should not be here!
		printf("Field %s has Wrong type:%d\n",
			   object.name.GetString(),
			   object.value.GetType());
	}

	return true;
}

bool protoGenerator::isComplexType(string type)
{
	return (type == "object" || type == "array");
}

void protoGenerator::log(string str)
{
	m_log << time(nullptr) << " : " << str << endl;
}

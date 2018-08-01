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

	if(!scan())
	{
		return false;
	}

	string file(m_strFileNameNoExt);
	file.append(".json");

	g_logger->trace("{} scan complete", file);

	rapidjson::StringBuffer buffer;
	PrettyWriter<StringBuffer> OutWriter(buffer);

	GenerateSchema(file, OutWriter);
	g_logger->trace("{} schema generated", file);

	m_msg.m_strSchema = buffer.GetString();

	write();
	string dst("../protobuf/proto/");
	dst.append(m_strFileNameNoExt);
	dst.append(".proto");
	g_logger->trace("{} write copmplete", dst);

	char cmd[1024] = {0};

	dst = m_strFileNameNoExt;
	dst.append(".proto");

	chdir("../protobuf/proto");
	getcwd(cwd, sizeof (cwd));
	g_logger->info("go {} to compile ptoto files...", cwd);

	sprintf(cmd, "protoc --cpp_out=../ $1 %s\n",dst.c_str());
	if(system(cmd))
	{
		return false;
	}

	chdir("../");
	getcwd(cwd, sizeof (cwd));
	g_logger->info("go {} to move files...", cwd);
	// go to directory: json/protobuf

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
	g_logger->info("return to {}...", cwd);

	return true;

}

bool protoGenerator::scan()
{
	string src = m_strFileNameNoExt;
	src.append(".json");

    g_logger->trace("scan {}", src.c_str());

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
		g_logger->error("invalid json file");
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
		bool canBeString = false;

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
				g_logger->warn("{}:{} has no type", src, name);
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
                g_logger->warn("{}:{} missed maxLength", src, key.name);
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

void protoGenerator::write()
{
    struct stat stInfo;
    if(stat("../protobuf/proto", &stInfo))
	{
		// dir not exist
		system("mkdir -p ../protobuf/proto");
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
            // proto file is newer than schema file
			g_logger->info("{} is newer than {} skip...", dst, schema);
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

    g_logger->trace("{} generated", dst.c_str());

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

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

/// \brief json type <-> protobuf type
static map<string, string> g_typeMap =
{
    {"integer", "int64"},
    {"number", "double"},
    {"boolean", "bool"},
    {"string", "string"}
};

protoGenerator::protoGenerator(string src):schemaModifyTime(0)
{
	size_t pos = src.find_last_of(".");
	m_strFileNameNoExt = src.substr(0, pos);
	m_msg.fileName = m_strFileNameNoExt;
}

bool protoGenerator::GeneratorProto()
{
     schemaModifyTime = GetFileModifyTime(src);

    // scan json schema files
    if(!Scan())
	{
        g_logger->trace("error occured when scan {}, exit", file);
		return false;
	}
    g_logger->trace("{} scan complete", file);

    // convert json schema text to cpp strings
    // Recursion
	string file(m_strFileNameNoExt);
	file.append(".json");

	rapidjson::StringBuffer buffer;
	PrettyWriter<StringBuffer> OutWriter(buffer);

    if(!GenerateSchema(file, OutWriter))
    {
        g_logger->error("{} schema negerate failed, exit");
    }
    g_logger->trace("{} schema generated", file);
	m_msg.m_strSchema = buffer.GetString();

    // generate proto files
    Write();
    g_logger->trace("{} write copmplete", dst);

    // call 'proc' to generate *.pb.h, *.pb.cc
    string dst = m_strFileNameNoExt;
	dst.append(".proto");
    SystemCmd("protoc --proto_path=%s --cpp_out=. %s\n",g_strProtobufProtoPath,dst.c_str());

    // move *.pb.cc to ../protobuf/src/*.pb.cpp
	struct stat dirInfo;
    if(stat(g_strProtobufSrcPath.c_str(), &dirInfo))
	{
        SystemCmd("mkdir -p %s", g_strProtobufSrcPath.c_str());
	}
    SystemCmd("mv %s.pb.cc %s/%s.pb.cpp",
              m_strFileNameNoExt.c_str(),
              g_strProtobufSrcPath.c_str(),
              m_strFileNameNoExt.c_str());

    // move *.pb.h to ../protobuf/include/*.pb.h
    if(stat(g_strProtoufIncludePath.c_str(), &dirInfo))
	{
		// dir not exist
        SystemCmd("mkdir -p %s", g_strProtoufIncludePath);
	}
    SystemCmd("mv %s.pb.h %s/%s.pb.h",
              m_strFileNameNoExt.c_str(),
              g_strProtoufIncludePath.c_str(),
              m_strFileNameNoExt.c_str());

	return true;
}

bool protoGenerator::Scan()
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
                type = g_typeMap[object.value["type"].GetString()];
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

void protoGenerator::Write()
{
    struct stat stInfo;
    if(stat(g_strProtobufProtoPath.c_str(), &stInfo))
	{
		// dir not exist
        SystemCmd("mkfir -p %s", g_strProtobufProtoPath.c_str());
	}

    string dst(g_strProtobufProtoPath);
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

bool protoGenerator::IsComplexType(string type)
{
	return (type == "object" || type == "array");
}

string protoGenerator::GetTypeFromSchemaFile(string schemaFile)
{
    string type;
    FILE *fp = fopen(schemaFile.c_str(), "rb");

    do
    {
        if(!fp)
        {
            g_logger->error("open {} failed", src.c_str());
            break;
        }

        char readBuffer[65536];
        FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        Document d;
        d.ParseStream(is);

        if(d.HasParseError())
        {
            g_logger->error("invalid json file:{}", schemaFile);
            break;
        }

        if(!d.HasMember("title"))
        {
            g_logger->error("no title found in {}", src.c_str());
            break;
        }
        type = d["title"].GetString();

    }while(false);

    fclose(fp);
    return type;
}

time_t protoGenerator::GetFileModifyTime(string fileName)
{
    struct stat stInfo;
    if(!stat(fileName.c_str(), &stInfo) != 0)
    {
        g_logger->trace("{} does not exist", fileName);
        return 0;
    }

    return stInfo.st_mtim.tv_sec;
}

void protoGenerator::SystemCmd(const char *fmt, ...)
{
    char cmd[1024] = {0};
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(cmd, fmt, argptr);
    va_end(argptr);

    if(cmd[strlen(cmd) - 1] != '\n')
    {
        cmd[strlen(cmd)] = '\n';
    }

    printf("excute command: %s",cmd);
    system(cmd);
}

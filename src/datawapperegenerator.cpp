#include "datawapperegenerator.h"
#include "rapidjson/document.h"

using namespace rapidjson;
extern spdlog::logger *g_logger;

DataWappereGenerator::DataWappereGenerator(const ProtoMessage &msg):
	m_msg(msg),
	m_bGuardStart(true),
	m_nIdent(0),
	m_bHasEntryTime(false)
{
	memset(m_charArrTmp, 0, 1024);
}

void DataWappereGenerator::GenerateDataWapper()
{
    if(m_msg.name.empty())
    {
        g_logger->info("kb: Nothing to be done");
        return;
    }

	// 不是存列表类型，并且没有定义EntryTime字段
	if(m_msg.m_mapFields.find("EntryTime") != m_msg.m_mapFields.end())
	{
		m_bHasEntryTime = true;
	}

	for(const auto & msg : m_msg.m_VecSubMsg)
	{
		DataWappereGenerator dw(msg);
		dw.GenerateDataWapper();
	}

	struct stat dirInfo;
	if(stat("./include", &dirInfo))
	{
		// dir not exist
		system("mkdir -p ./include");
	}

	string fileName("./include/");
	fileName.append(m_msg.fileName);

	fileName.append(".h");

	m_dstFile.open(fileName.c_str());

	if(!m_dstFile.is_open())
	{
		g_logger->error("open %s failed\n", fileName.c_str());
		return;
	}

	HeadGuard();
	WriteWithNewLine();

	License();
	WriteWithNewLine();

	Headers();
	WriteWithNewLine();

	Schema();
	WriteWithNewLine();

//	NameSpace();
//	WriteWithNewLine();

	FileStaticVar();
	WriteWithNewLine();

	ClassStart();

	pub();

	TypeDef();
	WriteWithNewLine();

	construct();
	WriteWithNewLine();

	create();
	WriteWithNewLine();

	createWithData();
	WriteWithNewLine();

	Init();
	WriteWithNewLine();

	destruct();
	WriteWithNewLine();

	Release();
	WriteWithNewLine();

	ToString();
	WriteWithNewLine();

	ToStringWriter();
	WriteWithNewLine();

	ToStringByProto();
	WriteWithNewLine();

	ToStringWithSpecifiedField();
	WriteWithNewLine();

	FromString();
	WriteWithNewLine();

	FromStringNoCheck();
	WriteWithNewLine();

	FromStringProto();
	WriteWithNewLine();

	Serialize();
	WriteWithNewLine();

	DeSerial();
	WriteWithNewLine();

	ObjectBegin();
	WriteWithNewLine();

	set("bool SetString(hash_t hKey, const char* value, size_t length)", "string");
	WriteWithNewLine();

	set("bool SetNull(hash_t hKey)", "null");
	WriteWithNewLine();

	set("bool SetBool(hash_t hKey, bool value)", "bool");
	WriteWithNewLine();

	set("bool SetInt(hash_t hKey, int64_t value)", "int64");
	WriteWithNewLine();

	set("bool SetDouble(hash_t hKey, double value)", "double");
	WriteWithNewLine();

	getSet();
	WriteWithNewLine();

	pri();
	InitSchema();

	pri();
	Variable();

	ClassEnd();
	WriteWithNewLine();

	HeadGuard();

	g_logger->info("{} generated", fileName.c_str());

	return;
}

void DataWappereGenerator::HeadGuard()
{
	string dst(m_msg.fileName);
	dst.append("_H");
	ToUpperCase(dst);

	if(m_bGuardStart)
	{
		string tmp("#ifndef ");
		tmp.append(dst);
		WriteWithNewLine(tmp);

		tmp = "#define ";
		tmp.append(dst);
		WriteWithNewLine(tmp);

		m_bGuardStart = false;
	}
	else
	{
		Write("#endif");
	}

	return;
}

void DataWappereGenerator::License()
{
	string name(m_msg.fileName);
	name.append(".json");

	sprintf(m_charArrTmp, "// Generated by the kbuf.  DO NOT EDIT!\n"
						  "// source: %s", name.c_str());

	WriteWithNewLine(m_charArrTmp);

	return;
}

void DataWappereGenerator::Headers()
{

	sprintf(m_charArrTmp, "#include \"%s.pb.h\"", m_msg.fileName.c_str());
	WriteWithNewLine(m_charArrTmp);

	WriteWithNewLine("#include <sstream>");
	WriteWithNewLine("#include \"google/protobuf/util/json_util.h\"");
	WriteWithNewLine("#include \"viidbasedata.h\"");
	WriteWithNewLine("#include \"viidjsonreadhandler.h\"");

	for(const auto & submsg : m_msg.m_VecSubMsg)
	{
		string hd = submsg.fileName;
		string usrHd("#include \"");
		usrHd.append(hd);
		usrHd.append(".h\"");
		WriteWithNewLine(usrHd);
	}

	return;
}

void DataWappereGenerator::Schema()
{
	// message has schema
	if(m_msg.fileName.find("msg") == string::npos)
	{
		return;
	}

	char tmp[1024*1024] = {0};
	string schema = TextToCpp(m_msg.m_strSchema);

	sprintf(tmp,"static const char* g_str%sSchema =\n%s;",
			m_msg.name.c_str(),
			schema.c_str()
			);

	WriteWithNewLine(tmp);
}

void DataWappereGenerator::NameSpace()
{
	WriteWithNewLine("using namespace google::protobuf::util;");
	WriteWithNewLine("using namespace rapidjson;");

	return;
}

void DataWappereGenerator::FileStaticVar()
{
	sprintf(m_charArrTmp, "static Document *g_%sDoc = nullptr;\n"
						  "static SchemaDocument *g_%sSmDoc = nullptr;",
			m_msg.name.c_str(),
			m_msg.name.c_str());

	WriteWithNewLine(m_charArrTmp);
}

void DataWappereGenerator::ClassStart()
{
	string name(m_msg.name);
	ToUpperCase(name[0]);

	sprintf(m_charArrTmp, "class C%s : public CViidBaseData\n{", name.c_str());
	WriteWithNewLine(m_charArrTmp);
	++m_nIdent;
}

void DataWappereGenerator::ClassEnd()
{
	--m_nIdent;
	Write("};");

	return;
}

void DataWappereGenerator::TypeDef()
{
	string name(m_msg.name);
	ToUpperCase(name[0]);

	sprintf(m_charArrTmp, "typedef shared_ptr<C%s> sharedPtr;", name.c_str());
	WriteWithNewLine(m_charArrTmp);
}

void DataWappereGenerator::create()
{
	string name(m_msg.name);
	ToUpperCase(name[0]);
	sprintf(m_charArrTmp, "static sharedPtr Create%s()\n{",name.c_str());
	WriteWithNewLine(m_charArrTmp);
	++m_nIdent;

	sprintf(m_charArrTmp, "sharedPtr sp(new C%s);\n"
						  "sp->m_data = new %s;\n"
						  "sp->m_bOutData = false;\n",
			name.c_str(), m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);

	WriteWithNewLine("sp->InitSchema();\nreturn sp\n;");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::createWithData()
{
	string name(m_msg.name);
	ToUpperCase(name[0]);
	sprintf(m_charArrTmp, "static sharedPtr Create%sWithData(%s *data)\n{",name.c_str(), m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);
	++m_nIdent;

	sprintf(m_charArrTmp, "sharedPtr sp(new C%s);\n"
						  "sp->m_data = data;\n"
						  "sp->m_bOutData = true;\n",
			name.c_str());
	WriteWithNewLine(m_charArrTmp);

	// in case of msg has no entryTime
	if(m_bHasEntryTime)
	{
		WriteWithNewLine("sp->m_data->set_entrytime(UnixTimeToPrettyTime(time(nullptr)));");
	}

	WriteWithNewLine("sp->InitSchema();\nreturn sp;\n");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::construct()
{
	string name(m_msg.name);
	ToUpperCase(name[0]);

	// constructor witout parameter
	sprintf(m_charArrTmp, "C%s()\n", name.c_str());
	Write(m_charArrTmp);

	WriteWithNewLine();

	WriteWithNewLine("{\n");
	++m_nIdent;

	WriteWithNewLine("Init();");

	--m_nIdent;
	WriteWithNewLine("}\n");

	// constructor witout parameter of 'm_data'
	sprintf(m_charArrTmp, "C%s(%s *data)\n{",name.c_str(), m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);
	++m_nIdent;

	WriteWithNewLine("m_data = data;\n"
					 "m_bOutData = false;\n");

	WriteWithNewLine("InitSchema();");

	--m_nIdent;
	WriteWithNewLine("}\n");
}

void DataWappereGenerator::destruct()
{
	string name(m_msg.name);
	ToUpperCase(name[0]);

	sprintf(m_charArrTmp, "~C%s()", name.c_str());
	WriteWithNewLine(m_charArrTmp);
	WriteWithNewLine("{");

	++m_nIdent;

	WriteWithNewLine("Release();");

	--m_nIdent;

	WriteWithNewLine("}");

}

void DataWappereGenerator::InitSchema()
{
	WriteWithNewLine("void InitSchema()\n{");
	++m_nIdent;

	if(m_msg.fileName.find("msg") == string::npos)
	{
		WriteWithNewLine("return;");
	}
	else
	{
		sprintf(m_charArrTmp, "if(!g_%sDoc)\n"
							  "{\n"
							  "\tg_%sDoc = new Document;\n"
							  "\tg_%sDoc->Parse(g_str%sSchema);\n"
							  "\tg_%sSmDoc = new SchemaDocument(*g_%sDoc);\n"
							  "}\n",
				m_msg.name.c_str(),
				m_msg.name.c_str(),
				m_msg.name.c_str(),
				m_msg.name.c_str(),
				m_msg.name.c_str(),
				m_msg.name.c_str());

		WriteWithNewLine(m_charArrTmp);
	}

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::Init()
{
	WriteWithNewLine("void Init()\n{");
	++m_nIdent;

	sprintf(m_charArrTmp, "m_data = new %s;\n"
						  "m_bOutData = false;\n",
			m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);

	WriteWithNewLine("InitSchema();");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::Release()
{
	WriteWithNewLine("void Release()\n{");
	++m_nIdent;

	WriteWithNewLine("if(m_data && !m_bOutData)\n"
					 "{\n"
					 "\tdelete m_data;\n"
					 "\tm_data = nullptr;\n"
					 "}");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::ToString()
{
	WriteWithNewLine("string &ToString(string &str, bool read = false)\n"
					 "{");
	++m_nIdent;

	WriteWithNewLine("rapidjson::StringBuffer buffer;\n"
					 "rapidjson::Writer<StringBuffer> writer(buffer);\n");

	WriteWithNewLine("ToStringWriter(writer, read);");


	WriteWithNewLine("str = buffer.GetString();\nreturn str;");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::ToStringWriter()
{
	WriteWithNewLine("void ToStringWriter(rapidjson::Writer<StringBuffer> &writer, bool read = false)\n"
					 "{");
	++m_nIdent;

	WriteWithNewLine("writer.StartObject();");
	WriteWithNewLine();

	// 简单类型，简单类型不是array
	if(!m_msg.m_mapFields.empty())
	{
		for(const auto & kv : m_msg.m_mapFields)
		{
			auto key = kv.second;

			sprintf(m_charArrTmp, "writer.String(\"%s\");", key.name.c_str());
			WriteWithNewLine(m_charArrTmp);

			if(key.type == "string")
			{
				sprintf(m_charArrTmp, "writer.String(m_data->%s().c_str());", key.fget.c_str());

				string t = "Time";
				bool isTime = (key.name.compare(key.name.length()-t.length(), t.length(), t) == 0);
				bool isEntryTime = (key.name == "EntryTime");

				if(isEntryTime)
				{
					// 保持一致
					sprintf(m_charArrTmp, "if(read)\n"
										  "{\n"
										  "\tstring s = %s();\n"
										  "\twriter.String(PrettyTimeToRaw(s).c_str());\n"
										  "}\n"
										  "else\n"
										  "{\n"
										  "\twriter.String(getEntryTime().c_str());\n"
										  "}",
							"getEntryTime");
				}
				else if(isTime)
				{
					sprintf(m_charArrTmp,"if(read)\n"
										 "{\n"
										 "\tstring s = m_data->%s();\n"
										 "\twriter.String(PrettyTimeToRaw(s).c_str());\n"
										 "}\n"
										 "else\n"
										 "{\n"
										 "\twriter.String(m_data->%s().c_str());\n"
										 "}",
							key.fget.c_str(),
							key.fget.c_str());
				}
				else
				{
					sprintf(m_charArrTmp, "writer.String(m_data->%s().c_str());", key.fget.c_str());
				}
			}
			else if(key.type == "int64")
			{
				sprintf(m_charArrTmp, "writer.Int64(m_data->%s());", key.fget.c_str());
			}
			else if(key.type == "double")
			{
				sprintf(m_charArrTmp, "writer.Double(m_data->%s());", key.fget.c_str());
			}
			else if(key.type == "bool")
			{
				sprintf(m_charArrTmp, "writer.Bool(m_data->%s());", key.fget.c_str());
			}
			else
			{
				g_logger->error("unkonwn type : %s\n", key.type.c_str());
			}

			WriteWithNewLine(m_charArrTmp);
			WriteWithNewLine();
		}
	}

	// 复合类型
	if(!m_msg.m_VecSubMsg.empty())
	{
		for(const auto &msg : m_msg.m_VecSubMsg)
		{
			WriteWithNewLine("{");
			++m_nIdent;

			if(msg.isArrray)
			{
				sprintf(m_charArrTmp,
						"int cnt = m_data->%s();\n"
						"writer.String(\"%s\");\n"
						"writer.StartArray();\n"
						"for (int i=0; i<cnt; ++i)\n"
						"{\n"
						"\tauto RawP = m_data->%s(i);\n"
						"\tauto sp = C%s::Create%sWithData(&RawP);\n"
						"\tsp->setEntryTime(getEntryTime());\n"
						"\tsp->setExpiredTime(getExpiredTime());\n"
						"\tsp->ToStringWriter(writer, read);\n"
						"}\n"
						"writer.EndArray();",
						msg.fsize.c_str(),
						msg.fieldName.c_str(),
						msg.fget.c_str(),
						msg.name.c_str(),
						msg.name.c_str());
			}
			else
			{
				sprintf(m_charArrTmp,
						"if(m_data->has_%s())\n{\n"
						"\tauto p = m_data->%s();\n"
						"\tauto sp = C%s::Create%sWithData(&p);\n"
						"\tsp->setEntryTime(getEntryTime());\n"
						"\tsp->setExpiredTime(getExpiredTime());\n"
						"\twriter.String(\"%s\");\n"
						"\tsp->ToStringWriter(writer, read);\n}\n",
						msg.fget.c_str(),
						msg.fget.c_str(),
						msg.name.c_str(),
						msg.name.c_str(),
						msg.fieldName.c_str());
			}

			WriteWithNewLine(m_charArrTmp);
			--m_nIdent;
			WriteWithNewLine("}");

			WriteWithNewLine();
		}
	}

	WriteWithNewLine("if(!read)\n{");
	++m_nIdent;

	// entryTime
	WriteWithNewLine("writer.String(\"EntryTime\");\n"
					 "writer.String(getEntryTime().c_str());");

	// expiredTime
	WriteWithNewLine("writer.String(\"KDExpiredDate\");\n"
					 "writer.String(getExpiredTime().c_str());");

	--m_nIdent;
	WriteWithNewLine("}");



	WriteWithNewLine();

	WriteWithNewLine("writer.EndObject();");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::ToStringByProto()
{
	WriteWithNewLine("string& ToStringByProto(string &str) const\n"
					 "{");

	++m_nIdent;

	WriteWithNewLine("google::protobuf::util::JsonPrintOptions op;\n"
					 "op.add_whitespace = false;\n"
					 "\n"
					 "google::protobuf::util::Status st = MessageToJsonString(*m_data, &str, op);\n"
					 "if(!st.ok())\n"
					 "{\n"
					 "\tstr.clear();\n"
					 "}\n"
					 "\n"
					 "return str;");

	--m_nIdent;

	WriteWithNewLine("}");

	return;
}

void DataWappereGenerator::ToStringWithSpecifiedField()
{
	WriteWithNewLine("string &ToStringWithSpecifiedField(string &str, const set<string> fields)\n"
					 "{");
	++m_nIdent;


	WriteWithNewLine("rapidjson::StringBuffer buffer;\n"
					 "rapidjson::Writer<StringBuffer> writer(buffer);\n");
	WriteWithNewLine();

	WriteWithNewLine("writer.StartObject();");
	WriteWithNewLine();

	WriteWithNewLine("__attribute__((unused)) auto end = fields.end();");

	// 应该不会指定复制类型
	if(!m_msg.m_mapFields.empty())
	{
		for(const auto & kv : m_msg.m_mapFields)
		{
			auto key = kv.second;

			sprintf(m_charArrTmp, "if(fields.find(\"%s\") != end)\n"
								  "{",
								  kv.first.c_str());
			WriteWithNewLine(m_charArrTmp);
			++m_nIdent;

			sprintf(m_charArrTmp, "writer.String(\"%s\");", key.name.c_str());
			WriteWithNewLine(m_charArrTmp);

			if(key.type == "string")
			{
				sprintf(m_charArrTmp, "writer.String(m_data->%s().c_str());", key.fget.c_str());

				string t = "Time";
				bool isTime = (key.name.compare(key.name.length()-t.length(), t.length(), t) == 0);
				bool isEntryTime = (key.name == "EntryTime");

				if(isEntryTime)
				{
					// 保持一致
					sprintf(m_charArrTmp, "string s = %s();\n"
										  "writer.String(PrettyTimeToRaw(s).c_str());",
							"getEntryTime");
				}
				else if(isTime)
				{
					sprintf(m_charArrTmp, "{\n\tstring s = m_data->%s();\n"
										  "\twriter.String(PrettyTimeToRaw(s).c_str());\n}",
							key.fget.c_str());
				}
				else
				{
					sprintf(m_charArrTmp, "writer.String(m_data->%s().c_str());", key.fget.c_str());
				}
			}
			else if(key.type == "int64")
			{
				sprintf(m_charArrTmp, "writer.Int64(m_data->%s());", key.fget.c_str());
			}
			else if(key.type == "double")
			{
				sprintf(m_charArrTmp, "writer.Double(m_data->%s());", key.fget.c_str());
			}
			else if(key.type == "bool")
			{
				sprintf(m_charArrTmp, "writer.Bool(m_data->%s());", key.fget.c_str());
			}
			else
			{
				g_logger->error("unkonwn type : %s\n", key.type.c_str());
			}

			WriteWithNewLine(m_charArrTmp);

			--m_nIdent;
			WriteWithNewLine("}");
			WriteWithNewLine();
		}
	}

	// 复合类型
	if(!m_msg.m_VecSubMsg.empty())
	{
		for(const auto &msg : m_msg.m_VecSubMsg)
		{
			WriteWithNewLine("{");
			++m_nIdent;

			if(msg.isArrray)
			{
				sprintf(m_charArrTmp,
						"int cnt = m_data->%s();\n"
						"writer.String(\"%s\");\n"
						"writer.StartArray();\n"
						"for (int i=0; i<cnt; ++i)\n"
						"{\n"
						"\tauto RawP = m_data->%s(i);\n"
						"\tauto sp = C%s::Create%sWithData(&RawP);\n"
						"\tsp->setEntryTime(getEntryTime());\n"
						"\tsp->setExpiredTime(getExpiredTime());\n"
						"\tsp->ToStringWriter(writer, true);\n"
						"}\n"
						"writer.EndArray();",
						msg.fsize.c_str(),
						msg.fieldName.c_str(),
						msg.fget.c_str(),
						msg.name.c_str(),
						msg.name.c_str());
			}
			else
			{
				sprintf(m_charArrTmp,
						"auto p = m_data->%s();\n"
						"auto sp = C%s::Create%sWithData(&p);\n"
						"sp->setEntryTime(getEntryTime());\n"
						"sp->setExpiredTime(getExpiredTime());\n"
						"writer.String(\"%s\");\n"
						"sp->ToStringWriter(writer, true);\n",
						msg.fget.c_str(),
						msg.name.c_str(),
						msg.name.c_str(),
						msg.fieldName.c_str());
			}

			WriteWithNewLine(m_charArrTmp);
			--m_nIdent;
			WriteWithNewLine("}");

			WriteWithNewLine();
		}
	}

	WriteWithNewLine("writer.EndObject();");

	WriteWithNewLine("str = buffer.GetString();\nreturn str;");
	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::Serialize()
{
	WriteWithNewLine("bool Serialize(CSerialBuf &buf) const\n"
					 "{");

	++m_nIdent;

	WriteWithNewLine("buf.SetLen(m_data->ByteSize());");

	WriteWithNewLine("return m_data->SerializeToArray(buf.GetBuf(), buf.GetLen());");

	--m_nIdent;

	WriteWithNewLine("}");

	return;
}

void DataWappereGenerator::DeSerial()
{
	WriteWithNewLine("bool DeSerialize(const void * data, int size)\n"
					 "{");

	++m_nIdent;

	WriteWithNewLine("return m_data->ParseFromArray(data, size);");

	--m_nIdent;

	WriteWithNewLine("}");

	return;
}

void DataWappereGenerator::FromString()
{
	WriteWithNewLine("bool FromString(const string &str)\n"
					 "{");
	++m_nIdent;

	sprintf(m_charArrTmp,
			"auto sp = C%s::Create%sWithData(this->m_data);\n"
			"viidJsonReadHandler handler(sp);\n",
			m_msg.name.c_str(),
			m_msg.name.c_str());

	WriteWithNewLine(m_charArrTmp);

	WriteWithNewLine();

	sprintf(m_charArrTmp, "Reader rd;\n"
						  "StringStream ss(str.c_str());\n"
						  "GenericSchemaValidator<SchemaDocument, viidJsonReadHandler> validator(*(g_%sSmDoc), handler);\n"
						  "ParseResult ok = rd.Parse<kParseValidateEncodingFlag>(ss, validator);\n"
						  "if(!ok)\n"
						  "{\n"
						  "\tstring err(GetParseError_En(ok.Code()));\n"
						  "\tSetErrStr(err);\n"
						  "\tif(!validator.IsValid())\n"
						  "\t{\n"
						  "\t\tStringBuffer sb;\n"
						  "\t\tWriter<StringBuffer> w(sb);\n"
						  "\t\tvalidator.GetError().Accept(w);\n"
						  "\t\tSetErrStr(sb.GetString());\n"
						  "\t}\n"
						  "\treturn false;\n"
						  "}",
			m_msg.name.c_str());

	WriteWithNewLine(m_charArrTmp);

	// EntryTime
	if(m_bHasEntryTime)
	{
		WriteWithNewLine("m_data->set_entrytime(RawTimeToPretty(time(nullptr)));");
	}

	WriteWithNewLine("return true;");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::FromStringNoCheck()
{
	WriteWithNewLine("bool FromStringNoCheck(const string &str)\n"
					 "{");
	++m_nIdent;

	sprintf(m_charArrTmp, "auto sp = C%s::Create%sWithData(this->m_data);\n"
						  "viidJsonReadHandler handler(sp);\n"
						  "Reader rd;\n"
						  "StringStream ss(str.c_str());\n"
						  "if(!rd.Parse<kParseValidateEncodingFlag>(ss, handler))\n"
						  "{\n"
						  "\treturn  false;\n"
						  "}\n"
						  "return true;",
			m_msg.name.c_str(),
			m_msg.name.c_str());

	WriteWithNewLine(m_charArrTmp);

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::FromStringProto()
{
	WriteWithNewLine("bool FromStringProto(const string &str)\n"
					 "{");
	++m_nIdent;

	WriteWithNewLine("google::protobuf::util::JsonParseOptions op;\n"
					 "op.ignore_unknown_fields = true;\n"
					 "JsonStringToMessage(str, this->m_data, op);\n"
					 "return true;");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::getSet()
{
	// data
	sprintf(m_charArrTmp, "%s *GetData()\n"
						  "{\n"
						  "\treturn  m_data;\n"
						  "}",
			m_msg.name.c_str());

	WriteWithNewLine(m_charArrTmp);

	WriteWithNewLine();

	/*
	// 简单类型
	if(!m_msg.m_mapFields.empty())
	{
		for(const auto kv : m_msg.m_mapFields)
		{
			auto field = kv.second;
			string name = field.name;
			string type = field.type;
			if(type == "int64")
			{
				type = "int64_t";
			}

			ToUpperCase(name[0]);

			sprintf(m_charArrTmp, "void Set%s(const %s src)\n"
								  "{\n"
								  "\tm_data->%s(src);\n"
								  "}",
					name.c_str(),
					type.c_str(),
					field.fset.c_str());

			WriteWithNewLine(m_charArrTmp);

			sprintf(m_charArrTmp, "%s Get%s() const\n"
								  "{\n"
								  "\t return m_data->%s();\n"
								  "}",
					type.c_str(),
					name.c_str(),
					field.fget.c_str());

			WriteWithNewLine(m_charArrTmp);
			WriteWithNewLine();
		}
	}

	// 复杂类型，size， get， set
	if(!m_msg.m_VecSubMsg.empty())
	{
		for(const auto msg : m_msg.m_VecSubMsg)
		{
			sprintf(m_charArrTmp, "%s *Add%s()\n"
								  "{\n"
								  "\treturn  m_data->%s();\n"
								  "}",
					msg.name.c_str(),
					msg.name.c_str(),
					msg.fadder.c_str());

			WriteWithNewLine(m_charArrTmp);

			if(msg.isArrray)
			{
				string getSize(msg.fget);
				getSize.append("_size");

				sprintf(m_charArrTmp, "int Get%sObjectSize()\n"
									  "{\n"
									  "\treturn m_data->%s();\n"
									  "}",
						msg.name.c_str(),
						getSize.c_str());
				WriteWithNewLine(m_charArrTmp);

				sprintf(m_charArrTmp, "%s Get%s(int index)\n"
									  "{\n"
									  "\treturn m_data->%s(index);\n"
									  "}",
						msg.name.c_str(),
						msg.name.c_str(),
						msg.fget.c_str());
				WriteWithNewLine(m_charArrTmp);
			}
			else
			{
				sprintf(m_charArrTmp, "const %s& Get%s()\n"
									  "{\n"
									  "\treturn m_data->%s();\n"
									  "}",
						msg.name.c_str(),
						msg.name.c_str(),
						msg.fget.c_str());
				WriteWithNewLine(m_charArrTmp);
			}
			WriteWithNewLine();
		}
	}
*/

	// EntryTime特殊处理
	if(m_bHasEntryTime)
	{
		// overload EntryTime set/get
		WriteWithNewLine("void setEntryTime(const string src) {m_data->set_entrytime(src);}\n"
						 "string getEntryTime() const {return m_data->entrytime();}");
	}
}

void DataWappereGenerator::ObjectBegin()
{
	WriteWithNewLine("bool ObjectBegin(hash_t hKey, SP_Data &newPtr)\n{");
	++m_nIdent;

	if(!m_msg.m_VecSubMsg.empty())
	{
		WriteWithNewLine("switch (hKey)\n"
						 "{");

		for(const auto & msg : m_msg.m_VecSubMsg)
		{

			string className(msg.name);
			ToUpperCase(className[0]);

			string lowCaseMessageName(msg.name);
			ToLowCase(lowCaseMessageName);

			sprintf(m_charArrTmp,
					"case \"%s\"_hash:// %lu\n"
					"\t{\n"
					"\t\tauto p = m_data->%s();\n"
					"\t\tnewPtr = C%s::Create%sWithData(p);\n"
					"\t\tbreak;\n"
					"\t}",
					msg.fieldName.c_str(),
					get_str_hash(msg.fieldName.c_str()),
					msg.fadder.c_str(),
					msg.name.c_str(),
					msg.name.c_str());

			WriteWithNewLine(m_charArrTmp);
		}

		WriteWithNewLine("default:\n"
						 "\t{\n"
						 "\t\t// should not be here\n"
						 "\t\treturn false;\n"
						 "\t}");

		WriteWithNewLine("}");

		WriteWithNewLine("return true;");
	}
	else
	{
		WriteWithNewLine("// should not be here");
		WriteWithNewLine("return false;");
	}

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::set(string fun, string type)
{
	fun.append("\n{");
	WriteWithNewLine(fun);
	++m_nIdent;

	bool hasType = false;
	for(const auto &kv : m_msg.m_mapFields)
	{
		if(kv.second.type == type)
		{
			hasType = true;
			break;
		}
	}

	if(hasType)
	{
		WriteWithNewLine("switch (hKey) \n{");

		for(const auto &kv : m_msg.m_mapFields)
		{
			auto field = kv.second;

			if(field.type == type)
			{
				CaseValue(field);
			}

			// double类型的数据，有时可能会以int形式给出，因此在int里也写一份
			if(type == "int64")
			{
				if(field.type == "double")
				{
					CaseValue(field);
				}
			}

			// 有些字段是int，但是有可能会以string的形式给出
			if(type == "string")
			{
				if((field.type == "int64") && field.canBeStr)
				{
					CaseValue(field, true);
				}
			}
		}

		WriteWithNewLine("default:\n"
						 "\t{\n"
						 "\t\t// should not be here\n"
						 "\t\t// TODO log err\n"
						 "\t}");

		WriteWithNewLine("}");
	}

	WriteWithNewLine("return true;");

	--m_nIdent;
	WriteWithNewLine("}");
}

void DataWappereGenerator::CaseValue(const JsonKey &field, bool intBeStrinig)
{
	sprintf(m_charArrTmp, "case \"%s\"_hash: // %lu",field.name.c_str(), get_str_hash(field.name.c_str()));
	WriteWithNewLine(m_charArrTmp);

	++m_nIdent;
	WriteWithNewLine("{");
	++m_nIdent;

	if(field.type == "string")
	{
		string t = "Time";
		bool isTime = (field.name.compare(field.name.length()-t.length(), t.length(), t) == 0);
		bool isEntryTime = (field.name == "EntryTime");

		if(isEntryTime)
		{
			// EntryTime should set by viid
			// TODO break;
			// sprintf(m_charArrTmp, "m_data->%s(RawTimeToPretty(time(nullptr)));", field.fset.c_str());
			sprintf(m_charArrTmp, "%s", "break;");
		}
		else if(isTime)
		{
			// xxTime, string as YYYYMMDDHHMMSS
			sprintf(m_charArrTmp, "{\n\tstring s(value);\n"
								  "\tm_data->%s(RawTimeToPretty(s));\n}",
					field.fset.c_str());
		}
		else
		{
			sprintf(m_charArrTmp, "m_data->%s(value);", field.fset.c_str());
		}
	}
	else if(intBeStrinig)	// 目前只有int型的，会有这种情况
	{
		// "3" -> 3
		sprintf(m_charArrTmp, "m_data->%s(stringToInt(value));",field.fset.c_str());
	}
	else
	{
		sprintf(m_charArrTmp, "m_data->%s(value);", field.fset.c_str());
	}

	WriteWithNewLine(m_charArrTmp);

	WriteWithNewLine("break;");

	--m_nIdent;
	WriteWithNewLine("}");
	--m_nIdent;
}

void DataWappereGenerator::pub()
{
	m_nIdent = 0;
	WriteWithNewLine("public:");
	++m_nIdent;
}

void DataWappereGenerator::pri()
{
	m_nIdent = 0;
	WriteWithNewLine("private:");
	++m_nIdent;
}

void DataWappereGenerator::Variable()
{
	string tmp(m_msg.name);
	tmp.append(" *m_data;");
	WriteWithNewLine(tmp);

	WriteWithNewLine("string m_strSchema;");
	WriteWithNewLine("bool m_bOutData;");

	return;
}

void DataWappereGenerator::Write( string str)
{
	stringstream ss(str);
	string line;
	while(getline(ss, line))
	{
		for(unsigned i=0; i<m_nIdent; ++i)
		{
			m_dstFile << "\t";
		}

		m_dstFile << line;
	}

	return;
}

void DataWappereGenerator::WriteWithNewLine(string str)
{
	if(str.empty())
	{
		m_dstFile << endl;
		return;
	}

	stringstream ss(str);
	string line;
	while(getline(ss, line))
	{
		for(unsigned i=0; i<m_nIdent; ++i)
		{
			m_dstFile << "\t";
		}

		m_dstFile << line;
		m_dstFile << endl;
	}

	return;
}

string DataWappereGenerator::TextToCpp(string src)
{
	stringstream ss(src);
	string line;
	string out;
	while (getline(ss, line))
	{
		string outline("\"");

		for(auto const &c : line)
		{
			if(c == '\"')
			{
				outline.append("\\\"");
			}
			else if(c== '\r' || c == '\n')
			{
				outline.append("\\n");
			}
			else
			{
				outline.push_back(c);
			}
		}
		outline.append("\"\n");

		out.append(outline);
	}

	return out;
}

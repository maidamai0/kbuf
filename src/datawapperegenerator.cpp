#include "datawapperegenerator.h"
#include "rapidjson/document.h"

using namespace rapidjson;
extern spdlog::logger *g_logger;

DataWappereGenerator::DataWappereGenerator(const ProtoMessage &msg, string sqlfile):
	m_msg(msg),
	m_bGuardStart(true),
	m_nIdent(0),
	m_bIsMsg(false),
	m_bIsObject(false),
	m_sqlFileName(sqlfile)
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

	if(m_msg.isNew)
	{
		g_logger->info("kb: already compiled");
		return;
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
		g_logger->error("open {} failed\n", fileName.c_str());
		return;
	}

	// message has schema
	if(m_msg.fileName.find("msg") != string::npos ||
			m_msg.fileName == "videosliceinfo" ||
			m_msg.fileName == "imageinfo" ||
			m_msg.fileName == "imagelist" ||
			m_msg.fileName == "videoslicelist" ||
			m_msg.fileName == "subscribecancel" ||
			m_msg.fileName == "dispositioncancel")
	{
		m_bIsMsg = true;
	}

	if(m_msg.fileName.find("msg") == string::npos &&
			m_msg.fileName.find("list") == string::npos)
	{
		m_bIsObject = true;
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

	destruct();
	WriteWithNewLine();

	Release();
	WriteWithNewLine();

	// experimental
	if(!m_sqlFileName.empty())
	{
		g_logger->info("sql:{}", m_sqlFileName);
		ToCdb();
		WriteWithNewLine();
	}

//	if(m_bIsObject)
//	{
//		ToCdb();
//		WriteWithNewLine();
//	}

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

//	FromStringNoCheck();
//	WriteWithNewLine();

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

	if(m_bIsMsg)
	{
		pri();
		InitSchema();
	}

	pri();
	Variable();

	ClassEnd();
	WriteWithNewLine();

	HeadGuard();

	g_logger->info("{} generated", fileName.c_str());

	m_dstFile.close();

	return;
}

void DataWappereGenerator::GenerateHeader()
{
	if(m_msg.name.empty())
	{
		g_logger->info("kb: Nothing to be done");
		return;
	}

	if(m_msg.isNew)
	{
		g_logger->info("kb: already compiled");
		return;
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
		g_logger->error("open {} failed\n", fileName.c_str());
		return;
	}

	HeadGuard();
	WriteWithNewLine();

	License();
	WriteWithNewLine();

	Headers();
	WriteWithNewLine();

	ClassStart();

	pub();

	TypeDef();
	WriteWithNewLine();

	string name(m_msg.name);
	ToUpperCase(name[0]);

	// constructor witout parameter
	sprintf(m_charArrTmp, "C%s();", name.c_str());
	Write(m_charArrTmp);
	WriteWithNewLine();

	// constructor with parameter of 'm_data'
	sprintf(m_charArrTmp, "C%s(%s *data);",name.c_str(), m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);

	// create
	sprintf(m_charArrTmp, "static sharedPtr Create%s();",name.c_str());
	WriteWithNewLine(m_charArrTmp);
	WriteWithNewLine();

	// create with data
	sprintf(m_charArrTmp, "static sharedPtr Create%sWithData(%s *data);",
			name.c_str(), m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);

	// destruct
	sprintf(m_charArrTmp, "~C%s();", name.c_str());
	WriteWithNewLine(m_charArrTmp);
	WriteWithNewLine();

	// release
	WriteWithNewLine("void Release();");
	WriteWithNewLine();

	// Tocdb
	// experimental
	sprintf(m_charArrTmp, "static void ProtoToCdb(cdb::RowMutation::Builder *row_mutation_build, const %s *prot);",
			m_msg.name.c_str());

	// toString
	WriteWithNewLine("string &ToString(string &str, bool read = false);");
	WriteWithNewLine();

	// toStringWriter
	WriteWithNewLine("string& ToStringByProto(string &str) const;");
	WriteWithNewLine();

	WriteWithNewLine("string &ToStringWithSpecifiedField(string &str, const set<string> fields);");
	WriteWithNewLine();

	WriteWithNewLine("bool FromString(const string &str, bool validate = true);");
	WriteWithNewLine();

	WriteWithNewLine("bool FromStringProto(const string &str);");
	WriteWithNewLine();

	WriteWithNewLine("bool Serialize(CSerialBuf &buf) const;");
	WriteWithNewLine();

	WriteWithNewLine("bool DeSerialize(const void * data, int size);");
	WriteWithNewLine();

	WriteWithNewLine("bool ObjectBegin(hash_t hKey, CViidBaseData *&newPtr);");
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

	sprintf(m_charArrTmp, "%s *GetData();",m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);
	WriteWithNewLine();

	pri();
	Variable();

	ClassEnd();
	WriteWithNewLine();

	HeadGuard();

	g_logger->info("{} generated", fileName.c_str());

	m_dstFile.close();

	return;
}

void DataWappereGenerator::GenerateCpp()
{
	if(m_msg.name.empty())
	{
		g_logger->info("kb: Nothing to be done");
		return;
	}

	if(m_msg.isNew)
	{
		g_logger->info("kb: already compiled");
		return;
	}

	for(const auto & msg : m_msg.m_VecSubMsg)
	{
		DataWappereGenerator dw(msg);
		dw.GenerateCpp();
	}

	struct stat dirInfo;
	if(stat("./src", &dirInfo))
	{
		// dir not exist
		system("mkdir -p ./src");
	}

	string fileName("./src/");
	fileName.append(m_msg.fileName);
	fileName.append(".cpp");

	m_dstFile.open(fileName.c_str());

	if(!m_dstFile.is_open())
	{
		g_logger->error("open {} failed\n", fileName.c_str());
		return;
	}

	// message has schema
	if(m_msg.fileName.find("msg") != string::npos ||
			m_msg.fileName == "videosliceinfo" ||
			m_msg.fileName == "imageinfo" ||
			m_msg.fileName == "imagelist" ||
			m_msg.fileName == "videoslicelist" ||
			m_msg.fileName == "subscribecancel" ||
			m_msg.fileName == "dispositioncancel")
	{
		m_bIsMsg = true;
	}

	if(m_msg.fileName.find("msg") == string::npos &&
			m_msg.fileName.find("list") == string::npos)
	{
		m_bIsObject = true;
	}

	sprintf(m_charArrTmp, "#include \"%s.h\"", m_msg.fileName.c_str());
	WriteWithNewLine(m_charArrTmp);
	WriteWithNewLine();

	FileStaticVar();
	WriteWithNewLine();

	construct();
	WriteWithNewLine();

	create();
	WriteWithNewLine();

	createWithData();
	WriteWithNewLine();

	destruct();
	WriteWithNewLine();

	Release();
	WriteWithNewLine();

	// experimental
	if(!m_sqlFileName.empty())
	{
		g_logger->info("sql:{}", m_sqlFileName);
		ToCdb();
		WriteWithNewLine();
	}

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

	if(m_bIsMsg)
	{
		pri();
		InitSchema();
	}

	g_logger->info("{} generated", fileName.c_str());

	m_dstFile.close();

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

	if(m_bIsMsg)
	{
		WriteWithNewLine("#include <mutex>");
	}

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
	if(!m_bIsMsg)
	{
		return;
	}

	char tmp[1024*1024*2] = {0};
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
	if(!m_bIsMsg)
	{
		return;
	}

	sprintf(m_charArrTmp, "static Document *g_%sDoc = nullptr;\n"
						  "static SchemaDocument *g_%sSmDoc = nullptr;\n"
						  "static mutex g_%sMutex;",
			m_msg.name.c_str(),
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

	sprintf(m_charArrTmp, "sharedPtr sp(new C%s);\n",
			name.c_str());
	WriteWithNewLine(m_charArrTmp);
	WriteWithNewLine("return sp;\n");

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

	sprintf(m_charArrTmp, "sharedPtr sp(new C%s(data));\n",name.c_str());
	WriteWithNewLine(m_charArrTmp);

	// in case of msg has no entryTime
	if(m_msg.bHasEntryTime)
	{
		WriteWithNewLine("sp->m_data->set_entrytime(time(nullptr));");
	}

	WriteWithNewLine("return sp;\n");
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

	sprintf(m_charArrTmp, "m_data = new %s;\n"
						  "m_bOutData = false;\n",
			m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);
	if(m_bIsMsg)
	{
		WriteWithNewLine("InitSchema();");
	}

	if(m_msg.name == "ImageListObject")
	{
		WriteWithNewLine("SetType(CT_IM_NO_OUT);");
	}
	else if(m_msg.name == "VideoSliceListObject")
	{
		WriteWithNewLine("SetType(CT_VS_NO_OUT);");
	}
	else if(m_msg.name == "ImageListMsg")
	{
		WriteWithNewLine("SetType(CT_INV);");
	}
	else if(m_msg.name == "VideoSliceListMsg")
	{
		WriteWithNewLine("SetType(CT_INV);");
	}

	--m_nIdent;
	WriteWithNewLine("}\n");

	// constructor with parameter of 'm_data'
	sprintf(m_charArrTmp, "C%s(%s *data)\n{",name.c_str(), m_msg.name.c_str());
	WriteWithNewLine(m_charArrTmp);
	++m_nIdent;

	WriteWithNewLine("m_data = data;\n"
					 "m_bOutData = true;\n");

	if(m_bIsMsg)
	{
		WriteWithNewLine("InitSchema();");
	}

	if(m_msg.name == "ImageListObject")
	{
		WriteWithNewLine("SetType(CT_IM_NO_OUT);");
	}
	else if(m_msg.name == "VideoSliceListObject")
	{
		WriteWithNewLine("SetType(CT_VS_NO_OUT);");
	}
	else if(m_msg.name == "ImageListMsg")
	{
		WriteWithNewLine("SetType(CT_INV);");
	}
	else if(m_msg.name == "VideoSliceListMsg")
	{
		WriteWithNewLine("SetType(CT_INV);");
	}

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

	if(!m_bIsMsg)
	{
		WriteWithNewLine("return;");
	}
	else
	{
		sprintf(m_charArrTmp,
							"if(g_%sSmDoc)\n"
							"{\n"
							"\treturn;\n"
							"}\n"
							"g_%sMutex.lock();\n"
							"if(!g_%sSmDoc)\n{\n"
							"\tg_%sDoc = new Document;\n"
							"\tg_%sDoc->Parse(g_str%sSchema);\n"
							"\tg_%sSmDoc = new SchemaDocument(*g_%sDoc);\n"
							"}\n"
							"g_%sMutex.unlock();\n",
				m_msg.name.c_str(),
				m_msg.name.c_str(),
				m_msg.name.c_str(),
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

void DataWappereGenerator::ToCdb()
{
	CCdb sql(m_sqlFileName);
	sql.scan();

	WriteWithNewLine("// experimental");
	sprintf(m_charArrTmp, "static void ProtoToCdb(cdb::RowMutation::Builder *row_mutation_build, const %s *prot)\n{",
			m_msg.name.c_str());

	WriteWithNewLine(m_charArrTmp);
	++m_nIdent;

	const JsonKey *pJsonKey = nullptr;
	if(!m_msg.m_vecFields.empty())
	{
		for(const auto & cdbkey : sql.record.Keys)
		{
			if(cdbkey.isPrimary)
			{
				continue;
			}

			pJsonKey = GetJsonKeyWithName(cdbkey.name);
			if(!pJsonKey)
			{
				g_logger->warn("'{}' in [{}] not found in [{}.proto]", cdbkey.name, m_sqlFileName, m_msg.fileName);
				continue;
			}

			if(cdbkey.type == "string")
			{
				if(pJsonKey->protoType == "string")
				{
					sprintf(m_charArrTmp, "row_mutation_build->SetString(\"%s\", prot->%s());",cdbkey.name.c_str(), pJsonKey->fget.c_str());
				}
				else
				{
					// int
					sprintf(m_charArrTmp, "row_mutation_build->SetString(\"%s\", IntTOString(prot->%s()));",cdbkey.name.c_str(), pJsonKey->fget.c_str());
				}
			}
			else if(cdbkey.type == "integer" || cdbkey.type == "long")
			{
				if(pJsonKey->protoType == "int64")
				{
					sprintf(m_charArrTmp, "row_mutation_build->SetInt64(\"%s\", prot->%s());",cdbkey.name.c_str(), pJsonKey->fget.c_str());
				}
				else
				{
					// string
					sprintf(m_charArrTmp, "row_mutation_build->SetInt64(\"%s\", stringToInt(prot->%s()));",cdbkey.name.c_str(), pJsonKey->fget.c_str());
				}

			}
			else if(cdbkey.type == "double")
			{
				sprintf(m_charArrTmp, "row_mutation_build->SetDouble(\"%s\", prot->%s());",cdbkey.name.c_str(), pJsonKey->fget.c_str());
			}
			else if(cdbkey.type == "bool")
			{
				sprintf(m_charArrTmp, "row_mutation_build->SetInt64(\"%s\", prot->%s());",cdbkey.name.c_str(), pJsonKey->fget.c_str());
			}
			else if(cdbkey.type == "timestamp" && pJsonKey->protoType == "int64")
			{
				sprintf(m_charArrTmp, "row_mutation_build->SetTimestamp(\"%s\", UnixTimeToTimeVal(prot->%s()));",cdbkey.name.c_str(), pJsonKey->fget.c_str());
			}
			else
			{
				g_logger->error("unkonwn type,cdb: {} in {} at {}, proto: {} in {}\n",
								cdbkey.type, cdbkey.name, m_sqlFileName, pJsonKey->protoType, pJsonKey->name);
				continue;
			}

			WriteWithNewLine(m_charArrTmp);
		}
	}

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

	// basic key-value
	if(!m_msg.m_vecFields.empty())
	{

		for(const auto & key : m_msg.m_vecFields)
		{
			// this two field not need any more,but we do not want to change proto
			if(m_msg.name == "WifiUsersData_Proto")
			{
				if(key.name == "CollectorName" ||
						key.name == "CollectorIDType")
				{
					continue;
				}
			}

			// subimageinfo special handle
			if(m_msg.name == "SubImageInfo_Proto")
			{
				if(	key.name == "DeviceType" ||
					key.name == "DeviceSNNo" ||
					key.name == "ImeiSn" ||
					key.name == "APSId" ||
					key.name  == "SelfDefData" ||
					key.name == "DataSize")
				{
					continue;
				}
			}


			if(key.name == "LocalPath")
			{
				continue;
			}

			if(key.name == "MountPath")
			{
				WriteWithNewLine("if(!read)\n"
								 "{\n"
								 "\twriter.String(\"MountPath\");\n"
								 "\twriter.String(m_data->mountpath().c_str());\n"
								 "}");
				continue;
			}

			// GeoPoint
			if(key.isGeoPoint)
			{
				continue;
			}

			if(m_msg.name == "DispositionNotificationListMsg")
			{
				if(key.name == "ReceiveAddr")
				{
					continue;
				}
			}

			// data in image will be move to subimage
			if(m_msg.name == "SubImageInfo_Proto")
			{
				if(key.name == "Data")
				{
					WriteWithNewLine("if(read && m_data->data().length())\n"
									 "{\n"
									 "\twriter.String(\"Data\");\n"
									 "\twriter.String(m_data->data().c_str());\n"
									 "}");
					continue;

				}
			}

			// uploading in videoSliceInfo
			if(m_msg.name == "VideoSliceInfo_Proto")
			{
				if(key.name == "Uploading")
				{
					WriteWithNewLine("if(!read)\n"
									 "{\n"
									 "\twriter.String(\"Uploading\");\n"
									 "\twriter.Int64(m_data->uploading());\n"
									 "}");
					continue;

				}
			}

			// kd extend field in motovehicle
			if(m_msg.name == "MotorVehicle_Proto")
			{
				if(key.name == "KDDispositionType" ||
						key.name == "KDCollectorName" ||
						key.name == "KDNote")
				{
					WriteWithNewLine("if(!read)");
					WriteWithNewLine("{");
					++m_nIdent;

					// key
					sprintf(m_charArrTmp, "writer.String(\"%s\");", key.name.c_str());
					WriteWithNewLine(m_charArrTmp);

					// value
					if(key.name == "KDCollectTime")
					{
						sprintf(m_charArrTmp, "writer.String(UnixTimeToPrettyTime(m_data->%s()).c_str());",key.fget.c_str());
					}
					else
					{
						sprintf(m_charArrTmp, "writer.String(m_data->%s().c_str());",key.fget.c_str());
					}
					WriteWithNewLine(m_charArrTmp);

					--m_nIdent;
					WriteWithNewLine("}");
					continue;
				}
			}

			// imageinfo in notification
			if(m_msg.name == "ImageInfo_Proto" && key.name != "ImageID" &&
				key.name != "CollectorName" && key.name != "CollectorOrg" &&
				key.name != "CollectorIDType" && key.name != "CollectorID" &&
				key.name != "EntryClerk" && key.name != "EntryClerkOrg" &&
				key.name != "EntryClerkIDType" && key.name != "EntryClerkID")
			{
				WriteWithNewLine("if(!read)");
				WriteWithNewLine("{\n");
				++m_nIdent;

				// key
				sprintf(m_charArrTmp, "writer.String(\"%s\");", key.name.c_str());
				WriteWithNewLine(m_charArrTmp);

				// value
				if(key.type == "string")
				{
					if(key.isEntryTime)
					{
						// entrytime
						sprintf(m_charArrTmp, "writer.String(%s(getEntryTime()).c_str());\n",
								"UnixTimeToPrettyTime");
					}
					else if(key.isExpiredTime)
					{
						// Expiredtime
						sprintf(m_charArrTmp, "writer.String(%s(getExpiredTime()).c_str());\n",
								"UnixTimeToPrettyTime");
					}
					else if(key.isTime)
					{
						sprintf(m_charArrTmp,"writer.String(UnixTimeToPrettyTime(m_data->%s()).c_str());\n",
								key.fget.c_str());
					}
					else if(key.protoType == "string" && key.isNumberStr)
					{
						sprintf(m_charArrTmp, "writer.Int64(stringToInt(m_data->%s()));\n",
								key.fget.c_str());
					}
					else if(key.protoType == "int64" && key.isNumberStr)
					{
						sprintf(m_charArrTmp, "writer.Int64(m_data->%s());\n",
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
					g_logger->error("unkonwn type : {}\n", key.type.c_str());
				}

				WriteWithNewLine(m_charArrTmp);
				WriteWithNewLine();

				--m_nIdent;
				WriteWithNewLine("}\n");

				continue;
			}

			// key
			if(key.name == "Orientation" && m_msg.name == "GPSData_Proto")
			{
				WriteWithNewLine("if(read)\n"
								 "{\n"
								 "\twriter.String(\"Direction\");\n"
								 "}\n"
								 "else\n"
								 "{\n"
								 "\twriter.String(\"Orientation\");\n"
								 "}");
			}
			else
			{
				sprintf(m_charArrTmp, "writer.String(\"%s\");", key.name.c_str());
				WriteWithNewLine(m_charArrTmp);
			}

			// value
			if(key.type == "string")
			{
				if(key.isEntryTime)
				{
					// entrytime
					sprintf(m_charArrTmp, "if(read)\n"
										  "{\n"
										  "\twriter.String(%s(getEntryTime()).c_str());\n"
										  "}\n"
										  "else\n"
										  "{\n"
										  "\twriter.String(UnixTimeToPrettyTime(getEntryTime()).c_str());\n"
										  "}",
							"UnixTimeToRawTime");
				}
				else if(key.isExpiredTime)
				{
					// Expiredtime
					sprintf(m_charArrTmp, "if(read)\n"
										  "{\n"
										  "\twriter.String(%s(getExpiredTime()).c_str());\n"
										  "}\n"
										  "else\n"
										  "{\n"
										  "\twriter.String(UnixTimeToPrettyTime(getExpiredTime()).c_str());\n"
										  "}",
							"UnixTimeToRawTime");
				}
				else if(key.isTime)
				{
					sprintf(m_charArrTmp,"if(read)\n"
										 "{\n"
										 "\twriter.String(UnixTimeToRawTime(m_data->%s()).c_str());\n"
										 "}\n"
										 "else\n"
										 "{\n"
										 "\twriter.String(UnixTimeToPrettyTime(m_data->%s()).c_str());\n"
										 "}",
							key.fget.c_str(),
							key.fget.c_str());
				}
				else if(key.protoType == "string" && key.isNumberStr)
				{
					sprintf(m_charArrTmp, "if(read)\n"
										  "{\n"
										  "\twriter.String(m_data->%s().c_str());\n"
										  "}\n"
										  "else\n"
										  "{\n"
										  "\twriter.Int64(stringToInt(m_data->%s()));\n"
										  "}",
							key.fget.c_str(),
							key.fget.c_str());
				}
				else if(key.protoType == "int64" && key.isNumberStr)
				{
					sprintf(m_charArrTmp, "if(read)\n"
										  "{\n"
										  "\twriter.String(IntTOString(m_data->%s()).c_str());\n"
										  "}\n"
										  "else\n"
										  "{\n"
										  "\twriter.Int64(m_data->%s());\n"
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
				g_logger->error("unkonwn type : {}\n", key.type.c_str());
			}

			WriteWithNewLine(m_charArrTmp);
			WriteWithNewLine();
		}
	}

	// GeoPoint
	if(!m_msg.GeoPoint.lat.name.empty())
	{
		sprintf(m_charArrTmp,
				"if(!read)\n"
				"{\n"
				"\twriter.String(\"%s\");\n"
				"\twriter.StartObject();\n"
				"\twriter.String(\"lon\");\n"
				"\twriter.Double(m_data->%s());\n"
				"\twriter.String(\"lat\");\n"
				"\twriter.Double(m_data->%s());\n"
				"\twriter.EndObject();\n"
				"}\n"
				"else\n"
				"{\n"
				"\twriter.String(\"%s\");\n"
				"\twriter.Double(m_data->%s());\n"
				"\t\n"
				"\twriter.String(\"%s\");\n"
				"\twriter.Double(m_data->%s());\n"
				"}",
				m_msg.GeoPoint.name.c_str(),
				m_msg.GeoPoint.lon.fget.c_str(),
				m_msg.GeoPoint.lat.fget.c_str(),
				m_msg.GeoPoint.lon.name.c_str(),
				m_msg.GeoPoint.lon.fget.c_str(),
				m_msg.GeoPoint.lat.name.c_str(),
				m_msg.GeoPoint.lat.fget.c_str());
		WriteWithNewLine(m_charArrTmp);
	}

	// compound type
	if(!m_msg.m_VecSubMsg.empty())
	{
		for(const auto &msg : m_msg.m_VecSubMsg)
		{

			// special handle for subimagelist
			if(msg.fieldName == "SubImageList")
			{
				WriteWithNewLine("if(m_data->subimagelist_size())\n"
								 "{\n"
								 "\n"
								 "\tif(read)\n"
								 "\t{\n"
								 "\t\twriter.String(\"SubImageList\");\n"
								 "\t\twriter.StartObject();\n"
								 "\t\twriter.String(\"SubImageInfoObject\");\n"
								 "\t\twriter.StartArray();\n"
								 "\t\tfor(auto & RawP : *(m_data->mutable_subimagelist()))\n"
								 "\t\t{\n"
								 "\t\t\tauto sp = CSubImageInfo_Proto::CreateSubImageInfo_ProtoWithData(&RawP);\n"
								 "\t\t\tsp->setEntryTime(getEntryTime());\n"
								 "\t\t\tsp->setExpiredTime(getExpiredTime());\n"
								 "\t\t\tsp->ToStringWriter(writer, read);\n"
								 "\t\t}\n"
								 "\t\twriter.EndArray();\n"
								 "\t\twriter.EndObject();\n"
								 "\t}\n"
								 "\telse\n"
								 "\t{\n"
								 "\t\twriter.String(\"SubImageList\");\n"
								 "\t\twriter.StartArray();\n"
								 "\t\tfor(auto & RawP : *(m_data->mutable_subimagelist()))\n"
								 "\t\t{\n"
								 "\t\t\tauto sp = CSubImageInfo_Proto::CreateSubImageInfo_ProtoWithData(&RawP);\n"
								 "\t\t\tsp->setEntryTime(getEntryTime());\n"
								 "\t\t\tsp->setExpiredTime(getExpiredTime());\n"
								 "\t\t\tsp->ToStringWriter(writer, read);\n"
								 "\t\t}\n"
								 "\t\twriter.EndArray();\n"
								 "\t}\n"
								 "}");

				WriteWithNewLine();
				continue;
			}

			if(msg.fieldName == "FeatureList")
			{
				WriteWithNewLine("if(m_data->featurelist_size())\n"
								 "{\n"
								 "\n"
								 "\tif(read)\n"
								 "\t{\n"
								 "\t\twriter.String(\"FeatureList\");\n"
								 "\t\twriter.StartObject();\n"
								 "\t\twriter.String(\"FeatureObject\");\n"
								 "\t\twriter.StartArray();\n"
								 "\t\tfor(auto & RawP : *(m_data->mutable_featurelist()))\n"
								 "\t\t{\n"
								 "\t\t\tauto sp = CFeature_Proto::CreateFeature_ProtoWithData(&RawP);\n"
								 "\t\t\tsp->setEntryTime(getEntryTime());\n"
								 "\t\t\tsp->setExpiredTime(getExpiredTime());\n"
								 "\t\t\tsp->ToStringWriter(writer, read);\n"
								 "\t\t}\n"
								 "\t\twriter.EndArray();\n"
								 "\t\twriter.EndObject();\n"
								 "\t}\n"
								 "\telse\n"
								 "\t{\n"
								 "\t\twriter.String(\"FeatureList\");\n"
								 "\t\twriter.StartArray();\n"
								 "\t\tfor(auto & RawP : *(m_data->mutable_featurelist()))\n"
								 "\t\t{\n"
								 "\t\t\tauto sp = CFeature_Proto::CreateFeature_ProtoWithData(&RawP);\n"
								 "\t\t\tsp->setEntryTime(getEntryTime());\n"
								 "\t\t\tsp->setExpiredTime(getExpiredTime());\n"
								 "\t\t\tsp->ToStringWriter(writer, read);\n"
								 "\t\t}\n"
								 "\t\twriter.EndArray();\n"
								 "\t}\n"
								 "}");

				WriteWithNewLine();
				continue;
			}

			// gpsdata save alone
			if(msg.fieldName == "GPSData")
			{
				if(msg.isArrray)
				{
					sprintf(m_charArrTmp,
							"if(m_data->%s() && read)\n"
							"{\n"
							"\n"
							"\twriter.String(\"%s\");\n"
							"\twriter.StartArray();\n"
							"\tfor(auto & RawP : *(m_data->mutable_%s()))\n"
							"\t{\n"
							"\t\tauto sp = C%s::Create%sWithData(&RawP);\n"
							"\t\tsp->setEntryTime(getEntryTime());\n"
							"\t\tsp->setExpiredTime(getExpiredTime());\n"
							"\t\tsp->ToStringWriter(writer, read);\n"
							"\t}\n"
							"\twriter.EndArray();\n"
							"}",
							msg.fsize.c_str(),
							msg.fieldName.c_str(),
							msg.fget.c_str(),
							msg.name.c_str(),
							msg.name.c_str());
				}
				else
				{
					sprintf(m_charArrTmp,
							"if(m_data->has_%s() && read)\n{\n"
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

				WriteWithNewLine();
				continue;
			}

			if(msg.isArrray)
			{
				sprintf(m_charArrTmp,
						"if(m_data->%s())\n"
						"{\n"
						"\n"
						"\twriter.String(\"%s\");\n"
						"\twriter.StartArray();\n"
						"\tfor(auto & RawP : *(m_data->mutable_%s()))\n"
						"\t{\n"
						"\t\tauto sp = C%s::Create%sWithData(&RawP);\n"
						"\t\tsp->setEntryTime(getEntryTime());\n"
						"\t\tsp->ToStringWriter(writer, read);\n"
						"\t}\n"
						"\twriter.EndArray();\n"
						"}",
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
						"\twriter.String(\"%s\");\n"
						"\tsp->ToStringWriter(writer, read);\n}\n",
						msg.fget.c_str(),
						msg.fget.c_str(),
						msg.name.c_str(),
						msg.name.c_str(),
						msg.fieldName.c_str());
			}

			WriteWithNewLine(m_charArrTmp);

			WriteWithNewLine();
		}
	}

	if(m_msg.name != "SubImageInfo_Proto" &&
			m_msg.name != "Feature_Proto" &&
			(!m_msg.bHasEntryTime || !m_msg.bHasExpireDate))
	{
		WriteWithNewLine("if(!read)\n{");
		++m_nIdent;

		// entryTime
		if(!m_msg.bHasEntryTime)
		{
			WriteWithNewLine("writer.String(\"EntryTime\");\n"
							 "writer.String(UnixTimeToPrettyTime(getEntryTime()).c_str());");
		}

		// expiredTime
		if(!m_msg.bHasExpireDate)
		{
			WriteWithNewLine("writer.String(\"KDExpiredDate\");\n"
							 "writer.String(UnixTimeToPrettyTime(getExpiredTime()).c_str());");
		}

		--m_nIdent;
		WriteWithNewLine("}");
	}

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

	// basic string
	if(!m_msg.m_vecFields.empty())
	{
		for(const auto & key : m_msg.m_vecFields)
		{
			sprintf(m_charArrTmp, "if(fields.find(\"%s\") != end)\n"
								  "{",
								  key.name.c_str());
			WriteWithNewLine(m_charArrTmp);
			++m_nIdent;

			if(key.name == "Orientation" && m_msg.name == "GPSData_Proto")
			{
				sprintf(m_charArrTmp, "writer.String(\"%s\");", "Direction");
			}
			else
			{
				sprintf(m_charArrTmp, "writer.String(\"%s\");", key.name.c_str());
			}
			WriteWithNewLine(m_charArrTmp);

			if(key.type == "int64")
			{
				sprintf(m_charArrTmp, "writer.Int64(m_data->%s());", key.fget.c_str());
			}
			else if(key.type == "string")
			{
				if(key.isEntryTime)
				{
					sprintf(m_charArrTmp, "writer.String(%s(getEntryTime()).c_str());\n",
							"UnixTimeToRawTime");
				}
				else if(key.isExpiredTime)
				{
					// Expiredtime
					sprintf(m_charArrTmp, "writer.String(%s(getExpiredTime()).c_str());\n",
							"UnixTimeToRawTime");
				}
				else if(key.isTime)
				{
					sprintf(m_charArrTmp,"writer.String(UnixTimeToRawTime(m_data->%s()).c_str());\n",
							key.fget.c_str());
				}
				else if(key.protoType == "int64" && key.isNumberStr)
				{
					sprintf(m_charArrTmp, "writer.String(IntTOString(m_data->%s()).c_str());", key.fget.c_str());
				}
				else
				{
					sprintf(m_charArrTmp, "writer.String(m_data->%s().c_str());", key.fget.c_str());
				}
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

	// compound types
	if(!m_msg.m_VecSubMsg.empty())
	{
		for(const auto &msg : m_msg.m_VecSubMsg)
		{
			if(msg.fieldName == "SubImageList")
			{
				// TODO complete this
				continue;
			}

			if(msg.fieldName == "FeatureList")
			{
				// TODO complete this
				continue;
			}

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
	WriteWithNewLine("bool FromString(const string &str, bool validate = true)\n"
					 "{");
	++m_nIdent;

	if(m_bIsMsg)
	{
		WriteWithNewLine("viidJsonReadHandler handler(this);\n"
						 "Reader rd;\n"
						 "StringStream ss(str.c_str());\n");

		WriteWithNewLine();

		sprintf(m_charArrTmp,
				"if(likely(validate))\n"
				"{\n"
				"\tGenericSchemaValidator<SchemaDocument, viidJsonReadHandler> validator(*(g_%sSmDoc), handler);\n"
				"\tParseResult ok = rd.Parse<kParseValidateEncodingFlag>(ss, validator);\n"
				"\tif(!validator.IsValid())\n"
				"\t{\n"
				"\t\tStringBuffer sb;\n"
				"\t\tWriter<StringBuffer> w(sb);\n"
				"\t\tvalidator.GetError().Accept(w);\n"
				"\t\tSetErrStr(sb.GetString());\n"
				"\t\treturn false;\n"
				"\t}\n"
				"\telse if(!ok)\n"
				"\t{\n"
				"\t\tstring err(GetParseError_En(ok.Code()));\n"
				"\t\tSetErrStr(err);\n"
				"\t\treturn false;\n"
				"\t}\n"
				"}\n"
				"else\n"
				"{\n"
				"\tParseResult ok = rd.Parse(ss, handler);\n"
				"\tif(!ok)\n"
				"\t{\n"
				"\t\tstring err(GetParseError_En(ok.Code()));\n"
				"\t\tSetErrStr(err);\n"
				"\t\treturn false;\n"
				"\t}\n"
				"}\n",
				m_msg.name.c_str());

		WriteWithNewLine(m_charArrTmp);

		// EntryTime
		if(m_msg.bHasEntryTime)
		{
			WriteWithNewLine("m_data->set_entrytime(time(nullptr));");
		}
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

	// overload get/set of entrytime if needed
	if(m_msg.bHasEntryTime)
	{
		// overload EntryTime set/get
		WriteWithNewLine("void setEntryTime(const time_t src) {m_data->set_entrytime(src);}\n"
						 "time_t getEntryTime() const {return m_data->entrytime();}");
	}

	// overload get/set of entrytime if needed
	if(m_msg.bHasExpireDate)
	{
		WriteWithNewLine();

		// overload EntryTime set/get
		WriteWithNewLine("void setExpiredTime(const time_t src) {m_data->set_kdexpireddate(src);}\n"
						 "time_t getExpiredTime() const {return m_data->kdexpireddate();}\n"
						 "void UpdateExpiredTime(time_t s){m_data->set_kdexpireddate(getEntryTime() + s);}");
	}
}

void DataWappereGenerator::ObjectBegin()
{
	WriteWithNewLine("bool ObjectBegin(hash_t hKey, CViidBaseData *&newPtr)\n{");
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

			// featureList, subImageList special handle
			if(msg.fieldName == "FeatureList")
			{
				sprintf(m_charArrTmp,
						"case \"FeatureObject\"_hash:// %lu\n"
						"\t{\n"
						"\t\tauto p = m_data->add_featurelist();\n"
						"\t\tnewPtr = new CFeature_Proto(p);\n"
						"\t\tbreak;\n"
						"\t}",
						get_str_hash("FeatureObject"));

			}
			else if(msg.fieldName == "SubImageList")
			{
				sprintf(m_charArrTmp,
						"case \"SubImageInfoObject\"_hash:// %lu\n"
						"\t{\n"
						"\t\tauto p = m_data->add_subimagelist();\n"
						"\t\tnewPtr = new CSubImageInfo_Proto(p);\n"
						"\t\tbreak;\n"
						"\t}",
						get_str_hash("SubImageInfoObject"));

			}
			else
			{
				if(m_msg.name == "ImageListMsg" && msg.fieldName == "ImageList")
				{
					sprintf(m_charArrTmp,
							"case \"%s\"_hash:// %lu\n"
							"\t{\n"
							"\t\tSetType(CT_IM_NO_OBJECT);\n"
							"\t\tauto p = m_data->%s();\n"
							"\t\tnewPtr = new C%s(p);\n"
							"\t\tbreak;\n"
							"\t}",
							msg.fieldName.c_str(),
							get_str_hash(msg.fieldName.c_str()),
							msg.fadder.c_str(),
							msg.name.c_str());
				}
				else if(m_msg.name == "ImageListMsg" && msg.fieldName == "ImageListObject")
				{
					sprintf(m_charArrTmp,
							"case \"%s\"_hash:// %lu\n"
							"\t{\n"
							"\t\tSetType(CT_STD);\n"
							"\t\tauto p = m_data->%s();\n"
							"\t\tnewPtr = new C%s(p);\n"
							"\t\tbreak;\n"
							"\t}",
							msg.fieldName.c_str(),
							get_str_hash(msg.fieldName.c_str()),
							msg.fadder.c_str(),
							msg.name.c_str());
				}
				else if(m_msg.name == "VideoSliceListMsg" && msg.fieldName == "VideoSliceList")
				{
					sprintf(m_charArrTmp,
							"case \"%s\"_hash:// %lu\n"
							"\t{\n"
							"\t\tSetType(CT_VS_NO_OBJECT);\n"
							"\t\tauto p = m_data->%s();\n"
							"\t\tnewPtr = new C%s(p);\n"
							"\t\tbreak;\n"
							"\t}",
							msg.fieldName.c_str(),
							get_str_hash(msg.fieldName.c_str()),
							msg.fadder.c_str(),
							msg.name.c_str());
				}
				else if(m_msg.name == "VideoSliceListMsg" && msg.fieldName == "VideoSliceListObject")
				{
					sprintf(m_charArrTmp,
							"case \"%s\"_hash:// %lu\n"
							"\t{\n"
							"\t\tSetType(CT_STD);\n"
							"\t\tauto p = m_data->%s();\n"
							"\t\tnewPtr = new C%s(p);\n"
							"\t\tbreak;\n"
							"\t}",
							msg.fieldName.c_str(),
							get_str_hash(msg.fieldName.c_str()),
							msg.fadder.c_str(),
							msg.name.c_str());
				}
				else if(m_msg.name == "DispositionListObject" && msg.fieldName == "DispositionObject")
				{
					sprintf(m_charArrTmp,
							"case \"%s\"_hash:// %lu\n"
							"\t{\n"
							"\t\tauto p = m_data->%s();\n"
							"\t\tnewPtr = new C%s(p);\n"
							"\t\tp->set_creattime(time(nullptr));\n"
							"\t\tbreak;\n"
							"\t}",
							msg.fieldName.c_str(),
							get_str_hash(msg.fieldName.c_str()),
							msg.fadder.c_str(),
							msg.name.c_str());
				}
				else
				{
					sprintf(m_charArrTmp,
							"case \"%s\"_hash:// %lu\n"
							"\t{\n"
							"\t\tauto p = m_data->%s();\n"
							"\t\tnewPtr = new C%s(p);\n"
							"\t\tbreak;\n"
							"\t}",
							msg.fieldName.c_str(),
							get_str_hash(msg.fieldName.c_str()),
							msg.fadder.c_str(),
							msg.name.c_str());
				}
			}

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

	// check existence if type
	bool hasType = false;
	for(const auto &key : m_msg.m_vecFields)
	{
		if(key.type == type)
		{
			hasType = true;
			break;
		}

		// compatible
		if(type == "int64")
		{
			if(key.type == "double" ||
			  (key.isNumberStr && key.protoType == "int64"))
			{
				hasType = true;
				break;
			}
		}
	}

	// case
	if(hasType)
	{
		WriteWithNewLine("switch (hKey) \n{");

		for(const auto &field : m_msg.m_vecFields)
		{
			// not needed anymore,but we do not want to change proto
			if(m_msg.name == "WifiUsersData_Proto")
			{
				if(field.name == "CollectorName" ||
					field.name == "CollectorIDType")
				{
					continue;
				}
			}

			if(field.type == type)
			{
				CaseValue(field, type);
			}

			// construct function: SetInt(hash_t Hkey, int64_t value)
			// double and number string can be given as an int
			if(type == "int64")
			{
				// should contains double
				if(field.type == "double")
				{
					CaseValue(field, type);
				}

				// should contains nummeric string
				if(field.isNumberStr)
				{
					CaseValue(field, type);
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

void DataWappereGenerator::CaseValue(const JsonKey &field, string type)
{	
	if(field.name == "Orientation" && m_msg.name == "GPSData_Proto")
	{
		sprintf(m_charArrTmp, "case \"%s\"_hash: // %lu","Direction", get_str_hash("Direction"));
	}
	else
	{
		sprintf(m_charArrTmp, "case \"%s\"_hash: // %lu",field.name.c_str(), get_str_hash(field.name.c_str()));
	}
	WriteWithNewLine(m_charArrTmp);

	++m_nIdent;
	WriteWithNewLine("{");
	++m_nIdent;

	if(field.isEntryTime || field.isCreatTime)
	{
		// EntryTime should set by viid
		sprintf(m_charArrTmp, "%s", "// viid generate");
	}
	else if(field.isTime || field.isExpiredTime)
	{
		sprintf(m_charArrTmp, "m_data->%s(RawTimeToUnixTime(value));", field.fset.c_str());
	}
	else if(field.isNumberStr && field.protoType == "int64" && type == "string")
	{
		// "3" -> 3
		sprintf(m_charArrTmp, "m_data->%s(stringToInt(value));",field.fset.c_str());
	}
	else if(field.isNumberStr && field.protoType == "int64" && type == "int64")	// for compatible
	{
		// 3 -> 3, where key(3) should be string ,but we will accept too if it is an int
		// that's numer string can be given as string("3") or int(3)
		sprintf(m_charArrTmp, "m_data->%s(value);",field.fset.c_str());
	}
	else if(field.isNumberStr && field.protoType == "string" && type == "int64")	// for compatible
	{
		// 3 -> "3", where key(3) should be string ,but we will accept too if it is an int
		// that's numer string can be given as string("3") or int(3)
		sprintf(m_charArrTmp, "m_data->%s(IntTOString(value));",field.fset.c_str());
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

const JsonKey * DataWappereGenerator::GetJsonKeyWithName(string name)
{
	const JsonKey *pKey = nullptr;
	for(const JsonKey & key : m_msg.m_vecFields)
	{

		ToLowCase(name);

		if(name == key.fget)
		{
			pKey = &key;
			break;
		}
	}

	return pKey;
}

bool DataWappereGenerator::stringStartWith(string src, string st)
{
	if(src.length() < st.length())
	{
		return false;
	}

	return st == src.substr(0, st.length());
}

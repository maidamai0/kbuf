/*
  视图库数据类生成器
  功能如下为输入jsonschema，生成一个数据类，该数据类可以解析符合schema规范的json，支持序列化，反序列化，
  也可以生成json

  数据类封装了对应的protobuf，并使用rapidJson进行json相关操作

  支持对json进行校验，内容包括
	1.	string 内容全部是数字的化，该字段可以使用数字或全为数字的string
	2.	数据编码校验，数据编码如果不能以utf8进行解析，则返回400
	3.	schema 支持的数据类型是string number integer
	4.	当schema中有EntryTime字段时，认为该字段表示入库时间，其实这样不是很合理，
		应该改成，无论协议/schema中有什么字段，都跟入库时间没关系
	5.	凡是以Time(大小写敏感)结尾的字段，都认为其在es中的字段时Date

	// TODO
 */

#include <initializer_list>

#include "datawapperegenerator.h"

spdlog::logger *g_logger = nullptr;

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		cout << "usage:\n"
				"./kbuf filename";
		return 1;
	}

    spdlog::sink_ptr consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::warn);
    consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    // shared_ptr<spdlog::sinks::sink> fileSink = std::make_shared<spdlog::sinks::basic_file_sink>(logname, true);
    // auto fileSink = new spdlog::sinks::basic_file_sink_mt( "compilelog.txt", true);
    spdlog::sink_ptr fileSink( new spdlog::sinks::basic_file_sink_mt( "compilelog.txt", true));
    fileSink->set_level(spdlog::level::trace);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

    g_logger = new spdlog::logger("multi_sink", {consoleSink, fileSink});
    g_logger->set_level(spdlog::level::trace);

    g_logger->info("initialized ok");


	string name = argv[1];
	protoGenerator pg(name);

	if(!pg.GeneratorProto())
	{
		return 1;
	}

	char cwd[1024] = {0};
	getcwd(cwd, sizeof (cwd));
	cout << "current path:" << cwd << endl; // json/schema

	chdir("../");	// json
	getcwd(cwd, sizeof (cwd));
	cout << "current path:" << cwd << endl;

	DataWappereGenerator dw(pg.m_msg);
	dw.GenerateDataWapper();

	return 0;
}

/*
  瑙嗗浘搴撴暟鎹被鐢熸垚鍣
  鍔熻兘濡備笅涓鸿緭鍏sonschema锛岀敓鎴愪竴涓暟鎹被锛岃鏁版嵁绫诲彲浠ヨВ鏋愮鍚坰chema瑙勮寖鐨刯son锛屾敮鎸佸簭鍒楀寲锛屽弽搴忓垪鍖栵紝
  涔熷彲浠ョ敓鎴恓son

  鏁版嵁绫诲皝瑁呬簡瀵瑰簲鐨刾rotobuf锛屽苟浣跨敤rapidJson杩涜json鐩稿叧鎿嶄綔

  鏀寔瀵筳son杩涜鏍￠獙锛屽唴瀹瑰寘鎷
	1.	string 鍐呭鍏ㄩ儴鏄暟瀛楃殑鍖栵紝璇ュ瓧娈靛彲浠ヤ娇鐢ㄦ暟瀛楁垨鍏ㄤ负鏁板瓧鐨剆tring
	2.	鏁版嵁缂栫爜鏍￠獙锛屾暟鎹紪鐮佸鏋滀笉鑳戒互utf8杩涜瑙ｆ瀽锛屽垯杩斿洖400
	3.	schema 鏀寔鐨勬暟鎹被鍨嬫槸string number integer
	4.	褰搒chema涓湁EntryTime瀛楁鏃讹紝璁や负璇ュ瓧娈佃〃绀哄叆搴撴椂闂达紝鍏跺疄杩欐牱涓嶆槸寰堝悎鐞嗭紝
		搴旇鏀规垚锛屾棤璁哄崗璁schema涓湁浠€涔堝瓧娈碉紝閮借窡鍏ュ簱鏃堕棿娌″叧绯
	5.	鍑℃槸浠ime(澶у皬鍐欐晱鎰缁撳熬鐨勫瓧娈碉紝閮借涓哄叾鍦╡s涓殑瀛楁鏃禗ate

	// TODO
 */

#include <initializer_list>
#include <map>
#include "datawapperegenerator.h"

spdlog::logger *g_logger = nullptr;

map<string, vector<string> > g_UnPublish;
map<string, vector<string> > g_NotUsed;

int compile(string schemaFileName, string sqlfileName = "")
{
	protoGenerator pg(schemaFileName);

	if(!pg.GeneratorProto())
	{
		return 1;
	}

	char cwd[1024] = {0};
	chdir("../");
	getcwd(cwd, sizeof (cwd));
	g_logger->info("go {} to generate head files...", cwd);    // json

	DataWappereGenerator dw(pg.m_msg, sqlfileName);
	dw.GenerateDataWapper();

	chdir("./schema");
	getcwd(cwd, sizeof (cwd));
	g_logger->info("return to {} for next file...", cwd);    // json/schema

	return 0;
}

void updateMap(string line, map<string, vector<string> > & map)
{
	string key;
	string value;
	bool bIsValue = false;
	for(const auto c : line)
	{
		if(c==' ' || c=='\t' || c=='\r')
		{
			continue;
		}

		if(c == '.')
		{
			bIsValue = true;
			continue;
		}

		if(bIsValue)
		{
			value.push_back(c);
		}
		else
		{
			key.push_back(c);
		}
	}

	map[key].push_back(value);
}


int main(int argc, char *argv[])
{
	// initialize LOG
	spdlog::sink_ptr consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	consoleSink->set_level(spdlog::level::warn);
	consoleSink->set_pattern("[%^%l%$] %v");

	spdlog::sink_ptr fileSink( new spdlog::sinks::basic_file_sink_mt( "compilelog.txt", true));
	fileSink->set_level(spdlog::level::trace);
	fileSink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

	g_logger = new spdlog::logger("multi_sink", {consoleSink, fileSink});
	g_logger->set_level(spdlog::level::trace);

	ifstream makeLists;
	if(argc < 2)
	{
		struct stat fileStat;
		if(stat("KMakeLists.txt", &fileStat))
		{
			g_logger->error("NO KMakeLists.txt found, Provide a 'KMakeLists.txt' which contains files to be read, or specify a filename as a argment to ./kb to read");
			return 1;
		}
		else
		{
			makeLists.open("KMakeLists.txt");
		}
	}

	if(makeLists.is_open())
	{
		string file;
		while (getline(makeLists, file))
		{
			vector<string> vec;
			SplitWithSpace(file, vec);

			if(vec.size() == 2)
			{
				g_logger->info("Compile {}...", file);
				compile(vec[0], vec[1]);
				g_logger->info("Compile {} complete\n", file);
			}
			else
			{
				// size = 1
				g_logger->info("Compile {}...", file);
				compile(file);
				g_logger->info("Compile {} complete\n", file);
			}
		}
	}
	else
	{
		compile(argv[1]);
	}

	return 0;
}

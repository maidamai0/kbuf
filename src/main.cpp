/*
  ��ͼ��������������
  ��������Ϊ����jsonschema������һ�������࣬����������Խ�������schema�淶��json��֧�����л��������л���
  Ҳ��������json

  �������װ�˶�Ӧ��protobuf����ʹ��rapidJson����json��ز���

  ֧�ֶ�json����У�飬���ݰ���
	1.	string ����ȫ�������ֵĻ������ֶο���ʹ�����ֻ�ȫΪ���ֵ�string
	2.	���ݱ���У�飬���ݱ������������utf8���н������򷵻�400
	3.	schema ֧�ֵ�����������string number integer
	4.	��schema����EntryTime�ֶ�ʱ����Ϊ���ֶα�ʾ���ʱ�䣬��ʵ�������Ǻܺ���
		Ӧ�øĳɣ�����Э��/schema����ʲô�ֶΣ��������ʱ��û��ϵ
	5.	������Time(��Сд����)��β���ֶΣ�����Ϊ����es�е��ֶ�ʱDate

	// TODO
	1. oneof
 */
#include "datawapperegenerator.h"


int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		cout << "usage:\n"
				"./kbuf filename";
		return 1;
	}

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

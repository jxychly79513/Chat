#include "..\request.h"
#include "stdafx.h"
#include "Server.h"

int main()
{
	cout << "Chat����ϵͳ�����1.0" << endl;
	auto server = Server::Create(9999);
	if (server == nullptr)
	{
		cout << "����ʧ��" << endl;
		return 1;
	}
	return server->StartServer();
}
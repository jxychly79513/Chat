#include "stdafx.h"
#include "Client.h"

using namespace std;

int main()
{
	cout << "Chat����ϵͳ1.0" << endl;
	cout << "�������û���ע��:" << endl;

	string userName;
	cin >> userName;

	auto client = Client::Create(userName, "heyoh.wicp.net:14034");
	if (client == nullptr)
	{
		cout << "����ʧ��" << endl;
	}
	client->StartClient();
	client->release();

	system("pause");
	return 0;

}
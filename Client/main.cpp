#include "stdafx.h"
#include "Client.h"

using namespace std;

int main()
{
	cout << "Chat����ϵͳ1.0" << endl;
	cout << "�������û���ע��:" << endl;

	string userName;
	cin >> userName;

	auto client = Client::Create(userName, "127.0.0.1:9999");
	client->StartClient();
	client->release();

	system("pause");
	return 0;

}
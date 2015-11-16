#include "Client.h"

#define PUTS_LOCK m_puts.lock()
#define PUTS_UNLOCK m_puts.unlock()

#define endl "\n"<<">"

Client::Client()
	: m_isChatRequset(false)
{}

Client * Client::Create(const string &userName, const string &addr)
{
	WORD sockVer = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVer, &wsaData))
	{
		cout << "WSA��ʼ��ʧ��" << endl;
		return nullptr;
	}

	Client *client = new Client();

	client->m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client->m_socket == INVALID_SOCKET)
	{
		cout << "�׽��ֳ�ʼ��ʧ��" << endl;
	}

	int pos = addr.find(":");
	string domain = addr.substr(0, pos);
	int port = atoi(addr.substr(pos + 1).c_str());

	hostent * host = gethostbyname(domain.c_str());
	in_addr ip;
	memcpy(&ip.S_un.S_addr, host->h_addr_list[0], host->h_length);

	sockaddr_in serAddr;
	serAddr.sin_addr.S_un.S_addr = ip.S_un.S_addr;
	serAddr.sin_port = htons(port);
	serAddr.sin_family = AF_INET;

	if (connect(client->m_socket, (LPSOCKADDR)(&serAddr), sizeof(serAddr)) == SOCKET_ERROR)
	{
		cout << "����ʧ��" << endl;
		delete client;
		return nullptr;
	}
	cout << "���ӳɹ������ڳ���ע���û���" << userName << endl;
	//============ע��============================
	REQUEST request;
	request.data.registered.flag = RT_REGISTERED;
	strcpy(request.data.registered.userName, userName.c_str());

	send(client->m_socket, request.ruler, sizeof(request.data.registered), 0);

	thread getResponse(&Client::GetResponse, client);		//�½��̣߳����ϼ������Է���������Ϣ
	getResponse.detach();

	return client;
}

int Client::GetResponse()
{
	while (true)
	{
		char dataBuffer[BUFFER_SIZE];
		int ret = recv(m_socket, dataBuffer, BUFFER_SIZE, 0);
		if (ret > 0)
		{
			RESPONSE data;
			for (int i = 0; i < ret; i++)
			{
				data.ruler[i] = dataBuffer[i];
			}
			int flag = *(int*)&dataBuffer[0];

			if (flag == RE_ERROR)//===============================������Ϣ=======================================
			{
				PUTS_LOCK; cout << data.data.error.errorMsg << endl; PUTS_UNLOCK;
			}
			else if (flag == RE_CHAT_REQUEST)//======================��������=======================================
			{
				PUTS_LOCK; cout << "һλ����" << data.data.chatRequest.userName << "�ĺ�������������죬ͬ��(y)���Ǿܾ�(n)��"; PUTS_UNLOCK;
				
				m_field.lock();
				m_isChatRequset = true;
				m_rtUserName = data.data.chatRequest.userName;
				m_field.unlock();
			}
			else if (flag == RE_AGREE_RETURN)//===========================��������Ļ�ִ=================================
			{
				PUTS_LOCK;
				if (data.data.agreeReturn.isAgree == 1)
				{
					cout << "�Է���ͬ���������ͨ���ѽ���" << endl;
				}
				else
				{
					cout << "�Է��ܾ����������" << endl;
				}
				PUTS_UNLOCK;
			}
			else if (flag == RE_EXIT_CHAT_RETURN)//===========================�˳�����Ļ�ִ==============================
			{
				PUTS_LOCK; cout << "�Է���ֹ�˵�ǰͨ��" << endl; PUTS_UNLOCK;
			}
			else if (flag == RE_RECEIVE)//====================================������Ϣ====================================
			{
				PUTS_LOCK; cout << data.data.receive.message << endl; PUTS_UNLOCK;
			}
			else if (flag == RE_SEND_LIST)//==================================�յ��û��б�===============================
			{
				PUTS_LOCK; cout << data.data.sendList.list << endl; PUTS_UNLOCK;
			}
		}
	}
	return 0;
}

void Client::StartClient()
{
	while (true)
	{
		string _cmd;
		getline(cin, _cmd);
		stringstream cmd(_cmd);

		string cmdHead;
		cmd >> cmdHead;
		//=========================��ʼ��������=================================================
		if (m_isChatRequset)
		{
			while (true)
			{
				if (cmdHead == "y")
				{
					PUTS_LOCK; cout << "���ڿ�����" << m_rtUserName << "��ͨ�����⽫��ֹ��ǰ��ͨ��" << endl; PUTS_UNLOCK;

					REQUEST request;
					request.data.agree.flag = RT_AGREE;
					request.data.agree.isAgree = 1;
					strcpy(request.data.agree.userName, m_rtUserName.c_str());

					send(m_socket, request.ruler, sizeof(request.data.agree), 0);
					break;
				}
				else if (cmdHead == "n")
				{
					PUTS_LOCK; cout << "�Ѿܾ�" << endl; PUTS_UNLOCK;

					REQUEST request;
					request.data.agree.flag = RT_AGREE;
					request.data.agree.isAgree = 0;
					strcpy(request.data.agree.userName, m_rtUserName.c_str());

					send(m_socket, request.ruler, sizeof(request.data.agree), 0);
					break;
				}
			}
			m_isChatRequset = false;
		}
		else 
		{
			if (cmdHead == "chat")
			{
				string parameter;
				cmd >> parameter;
				if (parameter == "with")
				{
					string userName;
					cmd >> userName;

					REQUEST request;
					request.data.requestChat.flag = RT_REQUEST_CHAT;
					strcpy(request.data.requestChat.userName, userName.c_str());

					send(m_socket, request.ruler, sizeof(request.data.requestChat), 0);
				}
				else if (parameter == "exit")
				{
					REQUEST request;
					request.data.exitChat.flag = RT_EXIT_CHAT;

					send(m_socket, request.ruler, sizeof(request.data.exitChat), 0);
				}
				else
				{
					cout << "�������" << endl;
				}
			}
			else if (cmdHead == "m")
			{
				string message;
				cmd >> message;

				REQUEST request;
				request.data.send.flag = RT_SEND;
				strcpy(request.data.send.message, message.c_str());

				send(m_socket, request.ruler, sizeof(request.data.send), 0);
			}
			else if (cmdHead == "list")
			{
				REQUEST request;
				request.data.getList.flag = RT_GET_LIST;
				
				send(m_socket, request.ruler, sizeof(request.data.getList), 0);
			}
			else if (cmdHead == "exit")
			{
				REQUEST request;
				request.data.exit.flag = RT_EXIT;

				send(m_socket, request.ruler, sizeof(request.data.exit), 0);
				break;
			}
			else if (cmdHead == "help")
			{
				cout << "===================����====================\n";
				cout << "list ��ȡ�����û��б�\n";
				cout << "chat with <�û���> ��ָ���û�����\n";
				cout << "chat exit �˳���ǰ�Ự\n";
				cout << "m <��Ϣ����> ������Ϣ\n";
				cout << "exit �˳�������ע�⣬�������exit�˳�����\n";
				cout << "help �鿴����\n" << endl;
			}
			else if (cmdHead == "");
			else
			{
				cout << "�������" << endl;
			}
		}
	}
}

void Client::release()
{
	closesocket(m_socket);
	WSACleanup();
	delete this;
}

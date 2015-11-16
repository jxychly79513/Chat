#include "Server.h"

Server::Server()
{
}

Server::~Server()
{
	for (auto pClient : m_clientList)
	{
		closesocket(pClient->GetSocket());
		delete pClient;
	}
}

Client * Server::FindClientByName(const string & name)
{
	for (auto pClient : m_clientList)
	{
		if (pClient->GetName() == name)
		{
			return pClient;
		}
	}
	return nullptr;
}

Server * Server::Create(u_short port)
{
	WORD sockVer = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVer, &wsaData))
	{
		cout << "WSA��ʼ��ʧ��" << endl;
		return nullptr;
	}

	Server *server = new Server();

	server->m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server->m_socket == INVALID_SOCKET)
	{
		cout << "�׽��ֳ�ʼ��ʧ��" << endl;
		delete server;
		return nullptr;
	}

	server->m_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server->m_addr.sin_family = AF_INET;
	server->m_addr.sin_port = htons(port);
	if (::bind(server->m_socket, (LPSOCKADDR)&server->m_addr, sizeof(server->m_addr)) == SOCKET_ERROR)
	{
		cout << "�󶨶˿�ʧ��" << endl;
		delete server;
		return nullptr;
	}

	return server;
}

void Server::release()
{
	closesocket(m_socket);
	WSACleanup();
	delete this;
}

int Server::Listen()
{
	if (listen(m_socket, 5) == SOCKET_ERROR)
	{
		cout << "����ʧ��" << endl;
		return 1;
	}
	cout << "������������" << endl;

	while (true)
	{
		SOCKET clientSocket;
		sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);

		clientSocket = accept(m_socket, (LPSOCKADDR)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			cout << "����ʧ��" << endl;
			return 2;
		}
		Client *pClient = new Client(clientSocket, clientAddr);

		m_listMt.lock();
		m_clientList.push_back(pClient);
		m_listMt.unlock();

		cout << "���յ�����" << inet_ntoa(clientAddr.sin_addr) << "������" << endl;

		thread clientThread(&Server::ClientThread, this, pClient);
		clientThread.detach();
	}

	return 0;
}

int Server::ClientThread(Client *pClient)
{
	while (true)
	{
		char dataBuff[BUFFER_SIZE];
		int ret = recv(pClient->GetSocket(), dataBuff, BUFFER_SIZE, 0);
		if (ret > 0)
		{
			REQUEST data;
			for (int i = 0; i < ret; i++)
			{
				data.ruler[i] = dataBuff[i];
			}

			int flag = *(int*)&dataBuff[0];
			if (flag == RT_REQUEST_CHAT)//=====================��������========================================================
			{
				Client *client = FindClientByName(data.data.requestChat.userName);

				RESPONSE response;
				if (client)
				{					
					response.data.chatRequest.flag = RE_CHAT_REQUEST;
					strcpy(response.data.chatRequest.userName, pClient->GetName().c_str());
					send(client->GetSocket(), response.ruler, sizeof(response.data.chatRequest), 0);
				}
				else
				{
					response.data.error.flag = RE_ERROR;
					strcpy(response.data.error.errorMsg, "no such user");
					send(pClient->GetSocket(), response.ruler, sizeof(response.data.error), 0);
				}
			}
			else if (flag == RT_REGISTERED)//=====================ע��==============================================================
			{
				string userName = data.data.registered.userName;
				
				if (FindClientByName(userName) != nullptr)	//���������
				{
					RESPONSE response;
					response.data.error.flag = RE_ERROR;
					strcpy(response.data.error.errorMsg, "duplication of name");
					send(pClient->GetSocket(), response.ruler, sizeof(response.data.error), 0);
				}
				else
				{	
					pClient->SetName(userName);
				}			

			}
			else if (flag == RT_AGREE)//=====================��������Ļ�ִ========================================================
			{
				Client *client = FindClientByName(data.data.agree.userName);
				if (!client)	continue;

				RESPONSE response;
			
				if (data.data.agree.isAgree == 1)
				{
					response.data.agreeReturn.flag = RE_AGREE_RETURN;
					response.data.agreeReturn.isAgree = 1;
					send(client->GetSocket(), response.ruler, sizeof(response.data.agreeReturn), 0);

					pClient->SetConnenter(client);
					client->SetConnenter(pClient);			
				}
				else if (data.data.agree.isAgree == 0)
				{
					response.data.agreeReturn.flag = RE_AGREE_RETURN;
					response.data.agreeReturn.isAgree = 0;
					send(client->GetSocket(), response.ruler, sizeof(response.data.agreeReturn), 0);
				}
				else
				{
					response.data.error.flag = ERROR;
					strcpy(response.data.error.errorMsg, "'isAgree'is wrong");
					send(pClient->GetSocket(), response.ruler, sizeof(response.data.error), 0);
				}
			}
			else if (flag == RT_EXIT_CHAT)//========================�˳�����=====================================================
			{
				RESPONSE response;
				if (pClient->GetConnenter() == nullptr)
				{
					response.data.error.flag = RE_ERROR;
					strcpy(response.data.error.errorMsg, "no connenter");
					send(pClient->GetSocket(), response.ruler, sizeof(response.data.error), 0);
				}
				else
				{
					response.data.exitChatReturn.flag = RE_EXIT_CHAT_RETURN;
					send(pClient->GetConnenter()->GetSocket(), response.ruler, sizeof(response.data.exitChatReturn), 0);

					pClient->GetConnenter()->SetConnenter(nullptr);
					pClient->SetConnenter(nullptr);
				}
			}
			else if (flag == RT_SEND)//==============================������Ϣ===========================================
			{
				RESPONSE response;
				if (pClient->GetConnenter() == nullptr)
				{
					response.data.error.flag = RE_ERROR;
					strcpy(response.data.error.errorMsg, "no connenter");
					send(pClient->GetSocket(), response.ruler, sizeof(response.data.error), 0);
				}
				else
				{
					response.data.receive.flag = RE_RECEIVE;
					strcpy(response.data.receive.message, data.data.send.message);
					send(pClient->GetConnenter()->GetSocket(), response.ruler, sizeof(response.data.receive), 0);
				}
			}
			else if (flag == RT_GET_LIST)//===========================��ȡ�����û��б�=====================================
			{
				RESPONSE response;
				
				string list = "";
				for (auto client : m_clientList)
				{
					list += client->GetName();
					list += "\n";
				}
				if (list.length() < 124)
				{
					response.data.sendList.flag = RE_SEND_LIST;
					strcpy(response.data.sendList.list, list.c_str());
					send(pClient->GetSocket(), response.ruler, sizeof(response.data.sendList), 0);
				}
				else
				{
					response.data.error.flag = RE_ERROR;
					strcpy(response.data.error.errorMsg, "faild");
					send(pClient->GetSocket(), response.ruler, sizeof(response.data.error), 0);
				}
			}
			else if (flag == RT_EXIT)//==========================�˳�============================================
			{
				if (pClient->GetConnenter() != nullptr)
				{
					RESPONSE response;
					response.data.exitChatReturn.flag = RE_EXIT_CHAT_RETURN;
					send(pClient->GetConnenter()->GetSocket(), response.ruler, sizeof(response.data.exitChatReturn), 0);

					pClient->GetConnenter()->SetConnenter(nullptr);
				}
				for (size_t i = 0; i < m_clientList.size(); i++)
				{
					if (m_clientList[i] == pClient)
					{
						m_listMt.lock();
						m_clientList.erase(m_clientList.begin() + i);
						m_listMt.unlock();
						break;
					}
				}

				cout << "��ַΪ" << inet_ntoa(pClient->GetAddr().sin_addr) << "���û����˳�" << endl;
				closesocket(pClient->GetSocket());
				delete pClient;
				break;
			}
		}
	}

	return 0;
}

int Server::StartServer()
{
	thread listen(&Server::Listen, this);
	listen.detach();

	while (true)
	{
		string cmd;
		cin >> cmd;
	
		if (cmd == "exit")
		{
			if (m_clientList.size() != 0)
			{
				cout << "����δ�Ͽ��Ŀͻ���" << endl;
			}
			else
			{
				break;
			}	
		}
		else if (cmd == "cexit")
		{
			cout << "�Ƿ�ǿ���˳����������⽫ʹ��δ�Ͽ��Ŀͻ��˵���(y/n)";
			string isexit;
			cin >> isexit;
			if (isexit == "y")
			{
				break;
			}
		}
		else if (cmd == "num")
		{
			cout << "Ŀǰ��" << m_clientList.size() << "������" << endl;
		}
		else
		{
			cout << "�������" << endl;
		}
	}
	return 0;
}


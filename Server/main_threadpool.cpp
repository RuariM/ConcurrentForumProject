#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include "TCPServer.h"
//#include "ReceivedSocketData.h"
#include "TCPClient.h"
#include "RequestParser.h"
#include "MemoryManager.h"
#include "ThreadPool.hpp"


#define DEFAULT_PORT 12345
#define POOLSIZE 15

MemoryManager* memoryManager; 

bool terminateServer = false;

void serverThreadFunction(TCPServer& server, ReceivedSocketData data);
std::string messageParser(ReceivedSocketData receivedData);

int main()
{
	TCPServer server(DEFAULT_PORT); //server

	ReceivedSocketData receivedData; //data from client var

	ThreadPool pool(POOLSIZE); 
	 
	std::vector<std::thread> serverThreads;

	memoryManager = new MemoryManager();

	std::cout << "Starting server. Send \"exit\" (without quotes) to terminate." << std::endl;

	while (!terminateServer) //handles data 
	{
		receivedData = server.accept(); //waits while no exit command for client data

		std::cout << "Client connected on socket: " << receivedData.ClientSocket << "\n";
		if (!terminateServer) //checks if server should still be active
		{
			pool.enqueue(serverThreadFunction, std::ref(server), receivedData);
		}
	}

	std::cout << "Server terminated." << std::endl;

	delete memoryManager;

	return 0;
}
std::string messageParser(ReceivedSocketData receivedData)
{

	PostRequest post = PostRequest::parse(receivedData.request);
	if (post.valid)
		return memoryManager->postMsg(post.getMessage(), post.getTopicId());

	ReadRequest read = ReadRequest::parse(receivedData.request);
	if (read.valid)
		return memoryManager->readMsgs(read.getTopicId(), read.getPostId());

	CountRequest count = CountRequest::parse(receivedData.request);
	if (count.valid)
		return memoryManager->countMsg(count.getTopicId());

	ListRequest list = ListRequest::parse(receivedData.request);
	if (list.valid)
		return memoryManager->listTopics();

	ExitRequest exitReq = ExitRequest::parse(receivedData.request); 
	if (exitReq.valid)
		return "TERMINATING"; //server exit

	return "Unknown request: " + receivedData.request;
}
void serverThreadFunction(TCPServer& server, ReceivedSocketData data) //thread function that handles msg recived from client
{
	unsigned int socketIndex = data.ClientSocket;

	do {
		server.receiveData(data, 0);
		
		if (data.request != "") //request process case
		{
			data.reply = messageParser(data); 		//call relavent parser method

			server.sendReply(data);
		}
	} while (data.request != "EXIT" && !terminateServer); //once exit sent loop stops

	if (!terminateServer && data.request == "EXIT")
	{
		terminateServer = true;

		TCPClient tempClient(std::string("127.0.0.1"), DEFAULT_PORT);
		tempClient.OpenConnection();
		tempClient.CloseConnection();
	}

	server.closeClientSocket(data);
}
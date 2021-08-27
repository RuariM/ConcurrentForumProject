#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "TCPClient.h"
#include <shared_mutex>
#include <unordered_map>
#include <iostream>

class MessageGenerator
{
public:
	MessageGenerator();
	~MessageGenerator();
	std::string getPostMsg();
	std::string getReadMsg();
	std::unordered_map<std::string, std::vector<std::string>> getSentPosts();
private:
	//std::vector<Post> previousTopics;
	std::unordered_map<std::string, std::vector<std::string>> previousPosts;
	std::vector<std::string> quickLookup;
	std::mutex topicLock;
	//bool savedMsgs;
	 //to test if server corupts messages
};


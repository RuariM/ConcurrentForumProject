#pragma once
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <shared_mutex>
#include <iostream>
#include <mutex>

struct messages {
	std::vector<std::string> msg;
	std::mutex msgLock;
};

class MemoryManager
{
public:
	MemoryManager();
	~MemoryManager();
	std::string postMsg(std::string msg, std::string topic); //places in hashmap
	std::string readMsgs(std::string topic, int msgId); //retrives from hashmap
	std::string listTopics(); //returns a list of all topic ids
	std::string countMsg(std::string topic); //returns number of msgs for a topic

private:
	std::unordered_map<std::string, std::vector<std::string>> messageMap; 
	std::shared_mutex mapLock;
};
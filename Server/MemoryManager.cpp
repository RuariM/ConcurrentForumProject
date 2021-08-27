#include "MemoryManager.h"
#include <shared_mutex>
#include <map>
#include <vector>

MemoryManager::MemoryManager() {

}

MemoryManager::~MemoryManager() {

}

std::string MemoryManager::postMsg(std::string msg, std::string topic) //places in hashmap
{
	mapLock.lock();
	messageMap[topic].push_back(msg);
	std::string postId = std::to_string(messageMap[topic].size());
	mapLock.unlock();
	return postId;
}
std::string MemoryManager::readMsgs(std::string topic, int msgId) //retrives from hashmap
{
	std::string topicMsg;
	mapLock.lock_shared();
	if (messageMap.find(topic) != messageMap.end() && messageMap[topic].size() >= msgId && msgId < messageMap[topic].size()) //topic does exist and msgId is equal or less than amount of msgs
	{
		
			topicMsg = messageMap[topic].at(msgId);
	}
	else
		topicMsg = "";//doesnt exist
	mapLock.unlock_shared();

	return topicMsg;
}

std::string MemoryManager::listTopics() //returns a list of all topic ids
{
	std::string topics;
	mapLock.lock_shared();
	if (!messageMap.empty())
	{
		for (auto it = messageMap.begin(); it != messageMap.end(); ++it)
		{
			topics += it->first + "#";
		}
	}
	mapLock.unlock_shared();
	return topics;
}
std::string MemoryManager::countMsg(std::string topic)
{
	std::string count;
	mapLock.lock_shared();
	if (messageMap.find(topic) != messageMap.end()) //topic exists
	{
		count = std::to_string(messageMap[topic].size());
	}
	else //no topics
	{
		count = std::to_string(0);
	}
	mapLock.unlock_shared();
	return count;
}

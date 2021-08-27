#include "MessageGenerator.h"

MessageGenerator::MessageGenerator()
{

}
MessageGenerator::~MessageGenerator()
{
}

std::string MessageGenerator::getPostMsg()
{
	srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::string request = "";
	std::vector<std::string> msgs = { "abcdefghijklmnopqrstuvwxyz", "history", "a", "xyz", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghij",
										"the sky was blue.", "abcdefghijklmno@pqrstu#vwxyz", "0123456789" };

	request.append("POST");
	std::string topicStr = "";
	int topicLength = rand() % 160; //does value > 140 work? 
	int choice = rand() % 2;

	std::string msg = msgs.at(rand() % msgs.size());

	topicLock.lock();
	if (choice == 0) //create new topic
	{
		for (int i = 0; i < topicLength; i++) //random length between 0 - 150 
		{
			int asciiVal = 0;
			int charChoice = rand() % 3;
			if (charChoice == 0) //0-9 48+9 //65+26 A-Z //a-z 97 + 26
				asciiVal = rand() % 26 + 65;

			if (charChoice == 1)
				asciiVal = rand() % 26 + 97;

			if (charChoice == 2)
				asciiVal = rand() % 9 + 48;

			topicStr += char(asciiVal);
		}

		topicStr = "@" + topicStr;
		//save topic for use in read and later posts
		previousPosts[topicStr].push_back(msg);
		quickLookup.push_back(topicStr);
		//topicLock.unlock();
	}
	else if (choice == 1 && !previousPosts.empty()) //use previously posted topic
	{
		int index = rand() % previousPosts.size();
		auto it = previousPosts.begin();
		std::advance(it, index);
		std::string topic = it->first;
		previousPosts[topic].push_back(msg);
		topicStr += topic;
	}

	topicLock.unlock();

	request.append(topicStr);
	request.append("#");
	request.append(msg);


	return request;

}
std::string MessageGenerator::getReadMsg()
{
	srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());

	std::string request = "";
	int index = 0;
	request.append("READ");

	if (!previousPosts.empty())
	{
		topicLock.lock();
		index = rand() % previousPosts.size();
		/*auto it = sentPosts.begin();
		std::advance(it, index);
		std::string topic = it->first;*/
		std::string topic = quickLookup[index];
		request.append(topic);

		request.append("#");
		//request.append(std::to_string(rand() % it->second.size()));
		request.append(std::to_string(rand() % previousPosts[topic].size()));
		topicLock.unlock();
	}
	else
	{
		request.append("@");
		request.append("#");
		request.append(std::to_string(index));
	}


	return request;
}

std::unordered_map<std::string, std::vector<std::string>> MessageGenerator::getSentPosts()
{
	return previousPosts;
}
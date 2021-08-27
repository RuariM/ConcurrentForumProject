
//spin up threads
//generate topics and messages
//send msg to client
//store read messages

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <queue>
#include "TCPClient.h"
#include <string>
#include <ctime>
#include <random>
#include "MessageGenerator.h"
#include <mutex>
#include <numeric>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>

#define DEFAULT_PORT 12345
#define TRUNCATE 140

struct timeanalysis {
	std::string postOrRead;
	int thread;
	std::vector<int> requestsASecond;
	std::chrono::duration<double> time;
};


bool sendExit = false;
int runTime = 0;
std::vector<std::string> readmsgs;

bool debug = false;

MessageGenerator* messageGenerator;

std::mutex timesLock;
std::vector<timeanalysis> times[2]; //index 0 posts. index 1 read 

void posterThreadFunction(char** argv, int tId); //could TCPClient be global
void readerThreadFunction(char** argv, int tId);
void exitFunction(char** argv);
void listAndCount(char** argv);
void checkMsgs(std::vector<std::pair<std::string, std::string>>);

int main(int argc, char** argv)
{
	// Validate the parameters
	if (argc != 6) {
		printf("usage: %s server-name|IP-address\n", argv[0]);
		return 1;
	}

	int numPostThreads = std::stoi(argv[2]);
	int numReadThreads = std::stoi(argv[3]);

	runTime = std::stoi(argv[4]);

	debug = std::stoi(argv[5]);

	messageGenerator = new MessageGenerator();

	TCPClient client(argv[1], DEFAULT_PORT); //do i need this?

	std::vector<std::thread> posterThreads;
	std::vector<std::thread> readerThreads;


	for (int i = 0; i < numPostThreads; i++)
		posterThreads.emplace_back(posterThreadFunction, argv, i);

	//wait for 0.05 of a seconds to post some messages
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	for (int i = 0; i < numReadThreads; i++)
		readerThreads.emplace_back(readerThreadFunction, argv, i);

	//wait for however many seconds of runtime

	for (auto& th : posterThreads) //joins threads after theyre finished running
		th.join();
	for (auto& th : readerThreads)
		th.join();

	if (debug == true)
		listAndCount(argv);

	exitFunction(argv);

	int totalPostRequests = 0;
	int totalReadRequests = 0;

	//timings per thread per second
	for (int i = 0; i < 2; ++i)
	{
		for (auto j = times[i].begin(); j < times[i].end(); j++)
		{
			std::cout << j->postOrRead << j->thread << "\n";
			if (j->requestsASecond.size() == runTime)
			{
				for (int k = 0; k < j->requestsASecond.size(); k++)
				{
					std::cout << "Second " << k << ": " << j->requestsASecond.at(k) << "\n";
				}
			}
			if (i == 0)
			{
				int threadRequests = accumulate(j->requestsASecond.begin(), j->requestsASecond.end(), 0.0);
				std::cout << "Average: " << threadRequests / j->requestsASecond.size() << "\n";
				totalPostRequests += threadRequests;
			}
			if (i == 1)
			{
				int threadRequests = accumulate(j->requestsASecond.begin(), j->requestsASecond.end(), 0.0);
				std::cout << "Average: " << threadRequests / j->requestsASecond.size() << "\n";
				totalReadRequests += threadRequests;
			}
			std::cout << "Runtime: " << j->time.count() << "\n\n";
		}
	}



	std::cout << "press any key to exit...";

	delete messageGenerator;

	return 0;
}
void listAndCount(char** argv)
{
	TCPClient listcountClient(argv[1], DEFAULT_PORT);
	listcountClient.OpenConnection();

	std::vector<std::string> topicsVec;
	std::string topics = listcountClient.send("LIST");

	if (topics != "")
	{
		//could use count in here to get topic index might be slow
		std::stringstream ss(topics);

		while (ss.good())
		{
			std::string str;
			std::getline(ss, str, '#');
			topicsVec.push_back(str);
		}
		topicsVec.pop_back(); //remove extra last element of blankspace
	}

	std::unordered_map<std::string, std::vector<std::string>> previousPosts = messageGenerator->getSentPosts();

	//slow with large amount of topics
	std::vector<std::string> notFound;
	std::vector<std::string> found;
	std::vector<std::string> dontMatch;

	int counter = 0;

	if (!previousPosts.empty())
	{
		for (auto i = previousPosts.begin(); i != previousPosts.end(); i++)
		{
			if (std::find(topicsVec.begin(), topicsVec.end(), i->first.substr(0, TRUNCATE)) != topicsVec.end())
			{
				found.push_back(i->first);

				std::string request = "COUNT" + i->first.substr(0, TRUNCATE);
				std::string reply = listcountClient.send(request);
				if (std::stoi(reply) == i->second.size())
				{

					counter++;

				}
				else
					dontMatch.push_back(i->first);
			}
			else
				notFound.push_back(i->first);

		}
		if (counter == previousPosts.size())
		{
			std::cout << "LIST/COUNT match with client records" << "\n";
		}
	}
	listcountClient.CloseConnection();
}
void exitFunction(char** argv)
{
	TCPClient exitClient(argv[1], DEFAULT_PORT);
	exitClient.OpenConnection();

	std::string reply = exitClient.send("EXIT");
	std::cout << reply;
	std::cout << "\n";
	exitClient.CloseConnection();
}
void posterThreadFunction(char** argv, int tId)
{
	std::chrono::high_resolution_clock::time_point threadstart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point secondstart = std::chrono::high_resolution_clock::now();

	int requestsCount = 0;
	std::vector<int> sRequests;
	timeanalysis posterTimes;
	posterTimes.thread = tId;
	posterTimes.postOrRead = "POST THREAD: ";

	TCPClient postClient(argv[1], DEFAULT_PORT);
	std::string request = "";
	std::string reply = "";

	postClient.OpenConnection();

	std::chrono::high_resolution_clock::time_point clientTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> loopCheck;

	do
	{
		if (debug && requestsCount < 500 || !debug)
		{
			request = messageGenerator->getPostMsg();
			reply = postClient.send(request);

			if (request != "")
			{
				requestsCount++;
			}
		}

		std::chrono::high_resolution_clock::time_point secondcheck = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> checktime = std::chrono::duration_cast<std::chrono::duration<double>>(secondcheck - secondstart);

		if (checktime.count() >= 1.00) //check time eslapsed since threadstart or last time check > 1 sec
		{
			posterTimes.requestsASecond.push_back(requestsCount);
			requestsCount = 0; //reset to zero to count next second num requests.
			secondstart = std::chrono::high_resolution_clock::now(); //reset the clock to count a new second
		}

		std::chrono::high_resolution_clock::time_point loopTime = std::chrono::high_resolution_clock::now();
		loopCheck = std::chrono::duration_cast<std::chrono::duration<double>>(loopTime - threadstart);
	} // loop slightly longer to grab the last second
	while (loopCheck.count() - 0.10 <= runTime);

	postClient.CloseConnection();

	//timing and requests calculations
	std::chrono::high_resolution_clock::time_point threadend = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> run_time = std::chrono::duration_cast<std::chrono::duration<double>>(threadend - threadstart);

	posterTimes.time = run_time;
	timesLock.lock();
	times[0].push_back(posterTimes);
	timesLock.unlock();
}
void readerThreadFunction(char** argv, int tId)
{
	std::chrono::high_resolution_clock::time_point threadstart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point secondstart = std::chrono::high_resolution_clock::now();

	int requestsCount = 0;
	std::vector<int> sRequests;

	std::vector<std::pair<std::string, std::string>> replymsgs;

	timeanalysis readerTimes;
	readerTimes.thread = tId;
	readerTimes.postOrRead = "READ THREAD: ";

	TCPClient readerClient(argv[1], DEFAULT_PORT);
	std::string request = "";
	std::string reply = "";
	readerClient.OpenConnection();

	std::chrono::duration<double> loopCheck;
	do
	{
		if (debug && requestsCount < 500 || !debug)
		{

			request = messageGenerator->getReadMsg();
			reply = readerClient.send(request);

			if (request != "")
			{
				requestsCount++;

			}
			if (reply != "")
				replymsgs.push_back(std::make_pair(request, reply));
		}
		std::chrono::high_resolution_clock::time_point secondcheck = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> checktime = std::chrono::duration_cast<std::chrono::duration<double>>(secondcheck - secondstart);

		if (checktime.count() >= 1.00) //check time eslapsed since threadstart or last time check > 1 sec
		{
			readerTimes.requestsASecond.push_back(requestsCount);
			requestsCount = 0; //reset to zero to count next second num requests.
			secondstart = std::chrono::high_resolution_clock::now(); //reset the clock to count a new second
		}

		std::chrono::high_resolution_clock::time_point loopTime = std::chrono::high_resolution_clock::now();
		loopCheck = std::chrono::duration_cast<std::chrono::duration<double>>(loopTime - threadstart);

	} //loop slightly longer to grab the last second
	while (loopCheck.count() - 0.10 <= runTime);

	readerClient.CloseConnection();


	//timing and requests calculations
	std::chrono::high_resolution_clock::time_point threadend = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> run_time = std::chrono::duration_cast<std::chrono::duration<double>>(threadend - threadstart);
	readerTimes.time = run_time;

	timesLock.lock();
	times[1].push_back(readerTimes);
	timesLock.unlock();

	if (debug == true && !replymsgs.empty())//check if READ responses match local record
		checkMsgs(replymsgs);
}

void checkMsgs(std::vector<std::pair<std::string, std::string>> readMsgs)
{
	std::vector<std::string> notFound;
	std::vector<std::string> found;

	std::unordered_map<std::string, std::vector<std::string>> previousPosts = messageGenerator->getSentPosts();

	for (auto i = readMsgs.begin(); i != readMsgs.end(); i++)
	{
		std::regex readRegex("^READ(@[^@#]*)#([0-9]+)$");
		std::smatch readMatch;
		if (std::regex_match(i->first, readMatch, readRegex, std::regex_constants::match_default))
		{
			//std::string id = 
			std::string topicId = readMatch[1];
			int postId = std::stoi(readMatch[2]);
			if (previousPosts[topicId].at(postId).substr(0, TRUNCATE) == i->second)
				found.push_back(topicId + " " + std::to_string(postId) + "#" + i->second);
			else
				notFound.push_back(topicId + " " + std::to_string(postId) + "#" + i->second);
		}
	}
	if (found.size() == readMsgs.size())
	{
		std::cout << "READ requests match client records" << "\n";
	}
}


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

void posterThreadFunction(char** argv, int tId, std::queue<std::string> postInput); //could TCPClient be global
void readerThreadFunction(char** argv, int tId, std::queue<std::string> readInput);
void exitFunction(char** argv);

int main(int argc, char** argv)
{
	if (argc != 5) {
		printf("usage: %s server-name|IP-address\n", argv[0]);
		return 1;
	}

	int numPostThreads = std::stoi(argv[2]);
	int numReadThreads = std::stoi(argv[3]);

	runTime = std::stoi(argv[4]);

	messageGenerator = new MessageGenerator();	

	//TCPClient client(argv[1], DEFAULT_PORT); 

	std::queue<std::string> postQueue;
	std::queue<std::string> readQueue;

	for (int i = 0; i < (runTime * 100); i++)
	{
		postQueue.push(messageGenerator->getPostMsg());
	}
	for (int i = 0; i < (runTime * 100); i++)
	{
		readQueue.push(messageGenerator->getReadMsg());
	}
	std::vector<std::thread> posterThreads;
	std::vector<std::thread> readerThreads;


	for (int i = 0; i < numPostThreads; i++)
		posterThreads.emplace_back(posterThreadFunction, argv, i, postQueue);

	//wait for 0.05 of a seconds to post some messages
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	for (int i = 0; i < numReadThreads; i++)
		readerThreads.emplace_back(readerThreadFunction, argv, i, readQueue);

	//wait for however many seconds of runtime

	for (auto& th : posterThreads) //joins threads after theyre finished running
		th.join();
	for (auto& th : readerThreads)
		th.join();


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
			}
			std::cout << "Runtime: " << j->time.count() << "\n\n";
		}
	}

	std::string fileName = std::to_string(numPostThreads) + "Post_" + std::to_string(numReadThreads) + "Read_" + std::to_string(runTime) + "_Second " + ".csv";

	std::ofstream timesStream;
	std::ifstream infile(fileName);

	if (!infile.good())
	{
		timesStream.open(fileName);
		timesStream << "avgPostSecond,totalPoster,avgReadSecond,totalReader,totalRequests,avgPerThread,avgPerThreadSec,runTime,postThreads,readThreads" << "\n";
	}
	else
		timesStream.open(fileName, std::ios_base::app);

	if (numPostThreads > 0)
	{
		timesStream << totalPostRequests / (numPostThreads * runTime) << ","; //avg poster a second
		timesStream << totalPostRequests << ","; //number of poster requests
	}
	else //no times
	{
		timesStream << "0,";
		timesStream << "0,";
	}

	if (numReadThreads > 0)
	{
		timesStream << totalReadRequests / (numReadThreads * runTime) << ","; //avg reader a second
		timesStream << totalReadRequests << ",";
	}
	else //no times
	{
		timesStream << "0,";
		timesStream << "0,";
	}

	
	 //number of reader requests
	timesStream << totalPostRequests + totalReadRequests << ","; //total requests
	timesStream << (totalPostRequests + totalReadRequests) / (numPostThreads + numReadThreads) << ","; //avgPerThread
	timesStream << (totalPostRequests + totalReadRequests) / (numPostThreads + numReadThreads) / runTime << ",";  //avgPerThreadSec
	timesStream << runTime << ","; //runtTime
	timesStream << numPostThreads << ",";
	timesStream << numReadThreads << ",";
	timesStream << "\n";

	timesStream.close();

	std::cout << "press any key to exit...";

	delete messageGenerator;

	return 0;
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
void posterThreadFunction(char** argv, int tId, std::queue<std::string> postInput)
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
		
		request = postInput.front();
		reply = postClient.send(request);

		if (request != "")
		{
			requestsCount++;
		}
		if (reply != "")
		{
			postInput.push(postInput.front());
			postInput.pop();
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
void readerThreadFunction(char** argv, int tId, std::queue<std::string> readInput)
{
	std::chrono::high_resolution_clock::time_point threadstart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point secondstart = std::chrono::high_resolution_clock::now();


	int requestsCount = 0;
	std::vector<int> sRequests;

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
		request = readInput.front();
		reply = readerClient.send(request);

		if (request != "")
		{
			requestsCount++;

		}
		if (reply != "")
		{
			readInput.push(readInput.front());
			readInput.pop();
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

}


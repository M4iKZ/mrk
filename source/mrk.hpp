#pragma once

#include <mutex>

#include "common.hpp"
#include "net.hpp"

sockFuncions sock;
statistics statis;

std::mutex _mutex;

std::atomic<bool> isRunning;

std::vector<std::thread> threads;

void threadMain(uint64_t, std::unique_ptr<threadData>&);

int socketConnect(std::unique_ptr<threadData>&);
int socketReconnect(std::unique_ptr<threadData>&, std::unique_ptr<connection>&);
void socketCheck(std::unique_ptr<threadData>&, std::unique_ptr<connection>&);
void socketWrite(std::unique_ptr<threadData>&, std::unique_ptr<connection>&);
void socketRead(std::unique_ptr<threadData>&, std::unique_ptr<connection>&);

void socketErrorConnect(uint32_t&);

void setResults(std::unique_ptr<threadData>&, std::unique_ptr<connection>&);

std::string makeRequest(const config, bool = false);

void printStats(std::string, std::unique_ptr<stats>&, std::string(*normalize)(long double, int));
void printUnits(long double, std::string(*normalize)(long double, int), int, int = 2);

bool parseArgs(config*, std::string&, std::string&, int, char**);
bool parseArg(std::string, char&, std::string&);

std::string version();
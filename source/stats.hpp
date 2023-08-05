#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <chrono>
#include <cmath>

struct errorsData
{
    uint32_t connect;
    uint32_t read;
    uint32_t write;
    uint32_t status;
    uint32_t timeout;
};

struct stats 
{
    uint64_t limit;
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> data;
    std::atomic<uint64_t> count;
    std::atomic<uint64_t> min;
    std::atomic<uint64_t> max;
};

void statsInit(std::unique_ptr<stats>&, uint64_t);

std::chrono::high_resolution_clock::time_point timeNow(int = 0);
bool hasTimePassed(const std::chrono::high_resolution_clock::time_point&, int);
long long getTime_s(const std::chrono::high_resolution_clock::time_point&);
long long getTime_us(const std::chrono::high_resolution_clock::time_point&);

int stats_record(std::unique_ptr<stats>&, uint64_t);
void stats_correct(std::unique_ptr<stats>&, int64_t);
long double stats_mean(std::unique_ptr<stats>&);
long double stats_stdev(std::unique_ptr<stats>&, long double);
long double stats_within_stdev(std::unique_ptr<stats>&, long double, long double, uint64_t);

#include "stats.hpp"

void statsInit(std::unique_ptr<stats>& statis, uint64_t max)
{
    uint64_t limit = max + 1;
    
    statis->limit = limit;
    statis->count = 0;
    statis->min = UINT64_MAX;
    statis->max = 0;
    
    // Initialize data vector with atomic values
    for (uint64_t i = 0; i < limit; ++i) 
        statis->data.emplace_back(std::make_unique<std::atomic<uint64_t>>());    
}

std::chrono::high_resolution_clock::time_point timeNow(int add)
{
    if (add)
        return std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(add);
    else
        return std::chrono::high_resolution_clock::now();
}

bool hasTimePassed(const std::chrono::high_resolution_clock::time_point& startTime, int duration) 
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    return (currentTime - startTime) >= std::chrono::milliseconds(duration);
}

long long getTime_s(const std::chrono::high_resolution_clock::time_point& startTime)
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - startTime).count();
}

long long getTime_us(const std::chrono::high_resolution_clock::time_point& startTime) 
{    
    return std::chrono::duration_cast<std::chrono::microseconds>(timeNow() - startTime).count();
}

int stats_record(std::unique_ptr<stats>& statis, uint64_t n) 
{
    if (n >= statis->limit) return 0;

    statis->data[n]->fetch_add(1, std::memory_order_relaxed);
    statis->count.fetch_add(1, std::memory_order_relaxed);
    uint64_t min = statis->min.load(std::memory_order_relaxed);
    uint64_t max = statis->max.load(std::memory_order_relaxed);
    
    while (n < min) 
        statis->min.compare_exchange_weak(min, n, std::memory_order_relaxed);
    
    while (n > max) 
        statis->max.compare_exchange_weak(max, n, std::memory_order_relaxed);
    
    return 1;
}

void stats_correct(std::unique_ptr<stats>& statis, int64_t expected) 
{
    for (uint64_t n = static_cast<uint64_t>(expected) * 2; n <= static_cast<uint64_t>(statis->max); n++) 
    {
        uint64_t count = statis->data[n]->load();
        int64_t m = static_cast<int64_t>(n) - expected;

        while (count && m > expected) 
        {
            statis->data[static_cast<size_t>(m)]->fetch_add(count);
            statis->count.fetch_add(count);
            m -= expected;
        }
    }
}

long double stats_mean(std::unique_ptr<stats>& statis)
{
    if (statis->count == 0) return 0.0;

    uint64_t sum = 0;
    for (uint64_t i = statis->min; i <= statis->max; i++) 
        sum += *statis->data[i] * i;
    
    return sum / (long double)statis->count;
}

long double stats_stdev(std::unique_ptr<stats>& statis, long double mean)
{
    long double sum = 0.0;
    if (statis->count < 2) return 0.0;
    
    for (uint64_t i = statis->min; i <= statis->max; i++) 
    {
        if (statis->data[i]) 
            sum += statis->data[i]->load() * powl(i - mean, 2);
    }

    return sqrtl(sum / (statis->count - 1));
}

long double stats_within_stdev(std::unique_ptr<stats>& statis, long double mean, long double stdev, uint64_t n)
{
    long double upper = mean + (stdev * n);
    long double lower = mean - (stdev * n);
    uint64_t sum = 0;

    for (uint64_t i = statis->min; i <= statis->max; i++)
    {
        if (i >= lower && i <= upper)
            sum += statis->data[i]->load();
    }
        
    if (statis->count.load() == 0) 
        return 0.0;
    
    return (sum / (long double)statis->count.load()) * 100;
}
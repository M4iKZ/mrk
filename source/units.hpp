#pragma once

#include <iostream>
#include <cstdint>
#include <cstddef>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

struct units
{
    int scale;
    std::string base;
    std::vector<std::string> values;
};

std::string formatBinary(long double);
std::string formatMetric(long double, int = 2);
std::string formatTime_us(long double, int = 2);
std::string formatTime_s(long double);

int scanMetric(std::string, uint64_t&);
int scanTime(std::string, uint64_t&);

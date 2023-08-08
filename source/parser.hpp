#pragma once

#include <iostream>
#include <vector>
#include <string>

enum class ParseState 
{
    START,
    HTTP,
    STATUS_CODE,
    FINISH
};

enum class ParseURLState
{
    SCHEMA,
    HOST,
    PORT,
    URI,
    DONE
};

struct ParsedURL 
{
    std::string schema;
    std::string host;
    std::string port;
    std::string uri;
};

ParsedURL parseURL(const std::string&);
int extractStatusCode(const std::vector<char>&);
bool getContentLength(const std::vector<char>&, size_t&);
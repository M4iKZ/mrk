#pragma once

#include "common.hpp"

enum status
{
    OK,
    ERR,
    RETRY
};

struct sockFuncions 
{    
    status(*connect)(std::unique_ptr<connection>&, const std::string&);
    size_t(*readable)(std::unique_ptr<connection>&);
    status(*write)(std::unique_ptr<connection>&, size_t&);
    status(*read)(std::unique_ptr<connection>&, size_t&);
    void(*close)(const socket_t&);
};

status sockConnect(std::unique_ptr<connection>&, const std::string&);
size_t sockReadable(std::unique_ptr<connection>&);
status sockWrite(std::unique_ptr<connection>&, size_t&);
status sockRead(std::unique_ptr<connection>&, size_t&);
void sockClose(const socket_t&);
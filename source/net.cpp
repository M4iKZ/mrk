
#include "net.hpp"

status sockConnect(std::unique_ptr<connection>& conn, const std::string& host) 
{
    return OK;
}

size_t sockReadable(std::unique_ptr<connection>& conn)
{
#ifdef _WIN32
    // TO BE ADDED?
    return OK;
#else
    int n, rc;
    rc = ioctl(conn->fd, FIONREAD, &n);
    return rc == -1 ? 0 : n;
#endif
}

status sockWrite(std::unique_ptr<connection>& conn, size_t& n)
{       
    ssize_t r = send(conn->fd, conn->data.data(), conn->data.size(), 0);    
    if (r <= 0)
    {
#ifdef _WIN32 
        if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
        if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif	   
            return RETRY;
        
        return ERR;        
    }
        
    n = r;

    return OK;
}

status sockRead(std::unique_ptr<connection>& conn, size_t& n)
{
    std::vector<char> chunk;
    while(true)
    {
        chunk.resize(RECVBUF);
        ssize_t r = recv(conn->fd, chunk.data(), chunk.size(), 0);
        if (r > 0)
        {
            conn->data.insert(conn->data.end(), chunk.begin(), chunk.begin() + r);
            chunk.clear();

            n += r;
        }
        else
        {
            if (conn->data.size() > 0)
                break;
#ifdef _WIN32 
            if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
            if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif	   
                return RETRY;

            return ERR;
        }
    }

    return OK;
}

void sockClose(const socket_t& fd)
{
#ifdef _WIN32		
    shutdown(fd, SD_BOTH);
    closesocket(fd);
    WSACleanup();
#else
    close(fd);
#endif	
}
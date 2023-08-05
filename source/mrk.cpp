
#include "mrk.hpp"

void usage() 
{
    printf("Usage: mrk <options> <url>                            \n"
        "  Options:                                            \n"
        "    -c, --connections <N>  Connections to keep open   \n"
        "    -d, --duration    <T>  Duration of test           \n"
        "    -t, --threads     <N>  Number of threads to use   \n"
        "                                                      \n"
        "    -v, --version          Print version details      \n"
        "                                                      \n"
        "  Numeric arguments may include a SI unit (1k, 1M, 1G)\n"
        "  Time arguments may include a time unit (2s, 2m, 2h)\n");
}

int main(int argc, char** argv)
{
    config cfg;
    std::string url, headers;
    
    if (!parseArgs(&cfg, url, headers, argc, argv)) 
    {
        usage();
        return 0;
    }
        
    cfg.url = parseURL(url);
    if (cfg.url.schema == "https")
    {
        // TO DO
        return 0;
    }
    else
    {
        sock = { sockConnect, sockReadable, sockWrite, sockRead, sockClose };
    }

    statsInit(statis.latency, cfg.timeout * 1000);
    statsInit(statis.requests, MAX_THREAD_RATE_S);

    threads.resize(cfg.threads);

    std::vector<std::unique_ptr<threadData>> threadsData;
    threadsData.resize(cfg.threads);
    
    isRunning.store(true);

    for (uint64_t i = 0; i < cfg.threads; ++i) 
    {
        std::unique_ptr<threadData> data = std::make_unique<threadData>();
        data->cfg = cfg;
        data->connections = cfg.connections / cfg.threads;
        
        threadsData.at(i) = std::move(data);
        
        threads.emplace_back(std::thread(&threadMain, i + 1, std::ref(threadsData.at(i))));
    }

    std::string time = formatTime_s(cfg.duration);
    std::cout << "Running mrk for " << time << " @ " << url << std::endl;
    std::cout << "  " << cfg.threads << " threads and " << cfg.connections << " connections" << std::endl;

    auto start = timeNow();
    uint64_t complete = 0;
    uint64_t bytes = 0;
    uint64_t sent = 0;
    errorsData errors = {};

    std::this_thread::sleep_for(std::chrono::seconds(cfg.duration));
    
    isRunning.store(false);
        
    for (auto& t : threadsData)
    {
        complete += t->complete;
        bytes += t->bytes;
        sent += t->sent;

        errors.connect += t->errors.connect;
        errors.read += t->errors.read;
        errors.write += t->errors.write;
        errors.timeout += t->errors.timeout;
        errors.status += t->errors.status;
    }
    
    auto runtime_us = getTime_us(start);
    auto runtime_s = getTime_s(start);
    auto req_per_s = complete / runtime_s;
    auto bytes_per_s = bytes / runtime_s;

    if (complete / cfg.connections > 0) 
    {
        int64_t interval = runtime_us / (complete / cfg.connections);
        stats_correct(statis.latency, interval);
    }

    printf("  Thread Stats%6s%11s%8s%12s\n", "Avg", "Stdev", "Max", "+/- Stdev");

    printStats("Latency", statis.latency, formatTime_us);
    printStats("Req/Sec", statis.requests, formatMetric);
    
    std::string runtime_msg = formatTime_us(runtime_us, 0);

    printf("  %d requests in %s, %sB sent, %sB read\n", (int)complete, runtime_msg.c_str(), formatBinary(sent).c_str(), formatBinary(bytes).c_str());
    if (errors.connect || errors.read || errors.write || errors.timeout) 
    {
        printf("  Socket errors: connect %d, read %d, write %d, timeout %d\n",
                errors.connect, errors.read, errors.write, errors.timeout);
    }

    if (errors.status) 
        printf("  Non-2xx or 3xx responses: %d\n", errors.status);
    
    printf("Requests/sec: %9.2lld\n", static_cast<long long>(req_per_s));
    printf("Transfer/sec: %10sB\n", formatBinary(bytes_per_s).c_str());
    
    for(auto& t : threads)    
        if(t.joinable())        
            t.join();
    
    return 91;
}

void threadMain(uint64_t id, std::unique_ptr<threadData>& thread)
{
    FD_ZERO(&thread->fds);

    for (uint64_t i = 0; i < thread->connections; ++i)
        socketConnect(thread);

    fd_set write_fds, read_fds;
    FD_ZERO(&write_fds);
    FD_ZERO(&read_fds);

    struct timeval timeout;
    timeout.tv_sec = 1; 
    timeout.tv_usec = 0;

    thread->start = timeNow(RECORD_INTERVAL_MS);

    while (isRunning.load())
    {
        write_fds = thread->fds;    
        read_fds = thread->fds;    
    
        int ready_fds = select(thread->max + 1, &read_fds, &write_fds, NULL, &timeout);
        if (ready_fds == -1)             
            break;
        
        for (int i = 0; i < thread->fd.size(); ++i)
        {            
            if (FD_ISSET(thread->fd[i], &write_fds) || FD_ISSET(thread->fd[i], &read_fds))
            {
                std::unique_ptr<connection>& conn = std::ref(thread->conns[thread->fd[i]]);                
                if (conn->phase == CONNECT)
                    socketCheck(thread, conn);
                else if (conn->phase == WRITE)
                    socketWrite(thread, conn);
                else if (conn->phase == READ)
                    socketRead(thread, conn);
            }
        }
        
        if (hasTimePassed(thread->start, RECORD_INTERVAL_MS))
        {
            uint64_t elapsed_ms = getTime_us(thread->start) / 1000;
            uint64_t requests = (thread->requests / (double)elapsed_ms) * 1000;

            stats_record(statis.requests, requests);

            thread->requests = 0;
            thread->start = timeNow(RECORD_INTERVAL_MS);
        }        
    }
}

int socketConnect(std::unique_ptr<threadData>& thread) 
{
#ifdef _WIN32
    WSADATA wsaData;

    int wsaResult = WSAStartup(MAKEWORD(2, 0), &wsaData);
    if (wsaResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", wsaResult);
        return 0;
    }
#endif
        
    int fd, flags;
    if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Cannot create socket\n");
        return 0;
    }
    
#ifdef _WIN32    
    u_long mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode) != 0)
#else
    flags = fcntl(fd, F_GETFL, 0);
    if (!fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
#endif
    {
        printf("Problems with not-blocking\n");
        sock.close(fd);
        return 0;
    }

    addrinfo hints{}, * result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(thread->cfg.url.host.c_str(), thread->cfg.url.port.c_str(), &hints, &result) != 0)
    {
        printf("getaddrinfo failed\n");
        sock.close(fd);
        return 0;
    }
    
    sockaddr_in serveraddr{};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr.s_addr;
    serveraddr.sin_port = htons(std::stoi(thread->cfg.url.port));
    if (connect(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
    {        
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) 
#else
        if (errno != EINPROGRESS)
#endif
        {
            socketErrorConnect(thread->errors.connect);
            sock.close(fd);
            
            return -1;
        }
    }

    flags = 1;
#ifdef _WIN32
    char enable = flags ? 1 : 0;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
#else
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
#endif
        
    std::unique_ptr<connection> conn = std::make_unique<connection>();
    conn->request = makeRequest(thread->cfg);
    conn->fd = fd;
    
    FD_SET(fd, &thread->fds);

    thread->fd.push_back(fd);
    thread->conns.insert({ fd, std::move(conn) });

    if (fd > thread->max)
        thread->max = fd;
        
    return fd;
}

int socketReconnect(std::unique_ptr<threadData>& thread, std::unique_ptr<connection>& conn)
{   
    thread->fd.erase(std::remove(thread->fd.begin(), thread->fd.end(), conn->fd), thread->fd.end());
    FD_CLR(conn->fd, &thread->fds);
    
    thread->conns.erase(conn->fd);
    
    conn.reset();

    return socketConnect(thread);
}

void socketCheck(std::unique_ptr<threadData>& thread, std::unique_ptr<connection>& conn)
{
    switch (sock.connect(conn, thread->cfg.url.host))
    {
    case OK:    
        break;
    case ERR:
        socketErrorConnect(thread->errors.connect);
        socketReconnect(thread, conn);
        return;        
    case RETRY: 
        return;
        break;
    }

    conn->phase = WRITE;

    socketWrite(thread, conn);
}

void socketWrite(std::unique_ptr<threadData>& thread, std::unique_ptr<connection>& conn)
{
    if (!conn->written)
    {
        conn->data.clear();
        conn->data.insert(conn->data.begin(), conn->request.begin(), conn->request.end());   
    
        conn->start = timeNow();
        conn->pending = thread->cfg.pipeline;
    }

    size_t n;
    switch (sock.write(conn, n)) 
    {
    case OK:    
        break;
    case ERR: 
        thread->errors.write++;
        socketReconnect(thread, conn);
        return;
        break;
    case RETRY: 
        return;
    }
    
    conn->written += conn->data.size();
    thread->sent += conn->written;
    conn->data.clear();

    conn->phase = READ;

    socketRead(thread, conn);
}

void socketRead(std::unique_ptr<threadData>& thread, std::unique_ptr<connection>& conn)
{    
    size_t n;
    switch (sock.read(conn, n)) 
    {
    case OK:    
        break;
    case ERR: 
        thread->errors.read++;
        socketReconnect(thread, conn);
        return;
    case RETRY: 
        return;            
    }
        
    setResults(thread, conn);

    thread->bytes += conn->data.size();
    conn->written = 0;

    conn->phase = CONNECT;
}

void socketErrorConnect(uint32_t& connect)
{
    connect++;
}

void setResults(std::unique_ptr<threadData>& thread, std::unique_ptr<connection>& conn)
{   
    int status = extractStatusCode(conn->data);
    if (status < 0)
    {
        thread->errors.status++;
    }
    else
    {
        thread->complete++;
        thread->requests++;

        if (status > 399)
            thread->errors.status++;

        if (!stats_record(statis.latency, getTime_us(conn->start)))
        {
            thread->errors.timeout++;
        }
    }

    conn->written = 0;
    conn->phase = WRITE;
}

std::string makeRequest(const config cfg)
{
    std::string request; 
    request += "GET " + cfg.url.uri + " HTTP/1.1\r\n";
    request += "Host: " + cfg.url.host;

    if (!cfg.url.port.empty())
        request += ":" + cfg.url.port;
    
    request += "\r\n";
    request += "User-Agent: mrk a HTTP benchmarking tool\r\n";
    request += "Connection: keep-alive\r\n";
    request += "\r\n";

    return request;
}

void printStats(std::string name, std::unique_ptr<stats>& stats, std::string(*normalize)(long double, int))
{
    uint64_t max = stats->max;
    long double mean = stats_mean(stats);
    long double stdev = stats_stdev(stats, mean);

    printf("    %-10s", name.c_str());
    printUnits(mean, normalize, 8);
    printUnits(stdev, normalize, 10);
    printUnits(max, normalize, 9);
    printf("%8.2Lf%%\n", stats_within_stdev(stats, mean, stdev, 1));
}

void printUnits(long double n, std::string(*normalize)(long double, int), int width, int p)
{
    std::string msg = normalize(n, p);
    int len = msg.size(), pad = 2;

    if (isalpha(msg[len - 1])) pad--;
    if (isalpha(msg[len - 2])) pad--;
    width -= pad;
    
    printf("%*.*s%.*s", width, width, msg.c_str(), pad, "  ");
}

bool parseArgs(config* cfg, std::string& url, std::string& headers, int argc, char** argv)
{
    int opt = 1;
    for (; opt < argc; ++opt)
    {
        char c;
        std::string arg;
        if(!parseArg(argv[opt], c, arg)) break;

        switch (c) 
        {
        case 't':
            if (scanMetric(arg, cfg->threads)) return false;
            break;
        case 'c':
            if (scanMetric(arg, cfg->connections)) return false;
            break;
        case 'd':
            if (scanTime(arg, cfg->duration)) return false;
            break;
        case 'v':
            printf("mrk %s\n", version().c_str());
            printf("Created by M4iKZ, http://m4i.kz - Based on wrk\n");
            return false;
            break;
        case 'h':
        case '?':
        default:
            return false;
            break;
        }
    }

    if (opt == argc || !cfg->threads || !cfg->duration) return false;

    if (!cfg->connections || cfg->connections < cfg->threads) 
    {
        std::cout << "number of connections must be >= threads" << std::endl;
        return false;
    }

    url = argv[opt];

    return true;
}

bool parseArg(std::string arg, char& c, std::string& out)
{
    if (arg[0] != '-' || arg.size() < 2)
        return false;

    if (arg[1] == 't')
        c = 't';
    else if (arg[1] == 'c')
        c = 'c';
    else if (arg[1] == 'd')
        c = 'd';
    else if (arg[1] == 'v')
        c = 'v';
    else if (arg[1] == 'h')
        c = 'h';
    else if (arg[1] == '?')
        c = '?';
    else    
        return false;

    out = arg.substr(2);
    
    return true;
}

std::string version()
{
    std::string versionString = VERSION;

    // Check if GCC compiler
#ifdef __GNUC__
    versionString += " Compiler: GCC ";
    versionString += std::to_string(__GNUC__);
    versionString += ".";
    versionString += std::to_string(__GNUC_MINOR__);
#endif

    // Check if Clang compiler
#ifdef __clang__
    versionString += " Compiler: Clang ";
    versionString += std::to_string(__clang_major__);
    versionString += ".";
    versionString += std::to_string(__clang_minor__);
#endif

    // Check if MSVC compiler
#ifdef _MSC_VER
    versionString += " Compiler: MSVC ";
    versionString += std::to_string(_MSC_VER);
#endif

    return versionString;
}
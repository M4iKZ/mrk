# mrk - a C++17 HTTP benchmarking tool

  mrk is a modern HTTP benchmarking tool written in C++17 
  it is capable of generating a significant amount of load when executed on a single multi-core CPU. 
  
  the tool is compatible with both **Windows** and **Linux** operating systems

  mrk is based on [wrk](https://github.com/wg/wrk), although it does not currently achieve the same level of performance.

## Todo
- [ ] support OpenSSL 3.0 to enable secure HTTPS connections
- [ ] improve overall performance
- [ ] Address a bug that occurs during the testing of a server with limited file descriptors (fds)
- [ ] add more command line options
- [ ] support lua scripts

## Basic Usage

```
  mrk -t10 -c500 -d60s http://localhost:8888
```

  This runs a benchmark for 60 seconds, using 10 threads, 
  and keeping 600 HTTP connections open.

  Output test [Orpy](https://github.com/M4iKZ/Orpy) on Linux:

```
  Running mrk for 1m @ http://localhost:8888
    10 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.57ms    4.16ms  81.18ms   89.49%
    Req/Sec    73.87k     8.96k  107.00k    70.59%
    22157258 requests in 1m, 2.19GB sent, 10.13GB read
  Requests/sec:    369287
  Transfer/sec:    172.92MB
```

## How to build

On Linux:

```
  mkdir build & cd build
  cmake ..
  make  
```

On Windows create a **build** folder and open a command line in it:

```
  cmake ..
```

Open .sln file created with Visual Studio

## Command Line Options
```
  -c, --connections: total number of HTTP connections to keep open with
                     each thread handling N = connections/threads

  -d, --duration:    duration of the test, e.g. 2s, 2m, 2h

  -t, --threads:     total number of threads to use
```
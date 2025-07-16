
#include"Epoller/Epoller.h"
#include<time.h>
#include<sys/time.h>
#include<iostream>
int main() {
    
    timespec ts;
    clock_gettime(0, &ts);
    timeval tv{0,0};
    gettimeofday(&tv, nullptr);
    time_t tSec = ts.tv_sec;
    struct tm *sysTime = localtime(&ts.tv_sec);
    struct tm t = *sysTime;

    std::cout<< t.tm_year+1900 << " " << t.tm_mon + 1 << " " << t.tm_mday << " " << t.tm_hour << " " << t.tm_min << " " << t.tm_sec << " " << tv.tv_sec << " " << ts.tv_sec << " " << ts.tv_nsec << std::endl;

    time_t timer = time(nullptr);
    sysTime = localtime(&timer);
    t = *sysTime;

    std::cout<< t.tm_year+1900 << " " << t.tm_mon + 1 << " " << t.tm_mday << " " << t.tm_hour << " " << t.tm_min << " " << t.tm_sec << " " << timer << std::endl;
    return 0;
}

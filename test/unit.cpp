#include<unistd.h>
#include<iostream>
using std::cout;
using std::endl;

////////////////////////////////////////////////////// ThreadPool /////////////////////////////////////////////////////
#include"../core/ThreadPool/ThreadPool.h"
void threadPool_test(){
    ThreadPool pool(5);
    for(int i = 0; i < 40; ++i){
        pool.AddTask([=]{ cout<< std::this_thread::get_id()<< " "<< i << " " << endl;});
    }
    sleep(4);
}
////////////////////////////////////////////////////// ThreadPool /////////////////////////////////////////////////////


////////////////////////////////////////////////////// Buffer /////////////////////////////////////////////////////
#include"../core/Buffer/Buffer.h"
void buffer_test(){
    Buffer buf(16);
    
    int err = 0;
    buf.Read(STDIN_FILENO, &err);
    if(err != 0){
        perror("read");
    }
    buf.Write(STDOUT_FILENO, &err);
    if(err != 0){
        perror("read");
    }
}
////////////////////////////////////////////////////// Buffer /////////////////////////////////////////////////////

////////////////////////////////////////////////////// Log /////////////////////////////////////////////////////
#include"../core/Log/Log.h"
void log_test_1(){
    Log::GetInstance()->Init(ERROR);
    Log::GetInstance()->SetMaxLines(4);
    LOG_DEBUG("hehehe%d,  %s", 1, "dsadas");
    LOG_INFO("ggggg%d,  %s", 1, "dsadas");
    LOG_DEBUG("sssss%d,  %s", 1, "dsadas");
    LOG_INFO("INFOfff %f, %d, %d", 3.1, 111, 222);
    LOG_WARN("WARNNNN %s %s", "jjj", "ccc");
    LOG_ERR("ERRRRR %d %d", 22, 5555);
    // sleep(2);

}

void log_test_2() {
    int cnt = 0;
    LogLevel level = DEBUG;
    Log::GetInstance()->Init(level, "./log1", ".log", 0);
    for(LogLevel level: {ERROR, WARN, INFO , DEBUG}) {
        Log::GetInstance()->SetLevel(level);
        for(int j = 0; j < 10000; j++ ){
            for(LogLevel l: {DEBUG, INFO, WARN, ERROR}) {
                Log::GetInstance()->Write(l,"%s 111111111 %d ============= ", "Test", cnt++);
            }
        }
    }
    cnt = 0;
    Log::GetInstance()->Init(level, "./log2", ".log", 5000);
    for(LogLevel level: {DEBUG, INFO, WARN, ERROR}) {
        Log::GetInstance()->SetLevel(level);
        for(int j = 0; j < 10000; j++ ){
            for(LogLevel l: {DEBUG, INFO, WARN, ERROR}) {
                Log::GetInstance()->Write(l,"%s 222222222 %d ============= ", "Test", cnt++);
            }
        }
    }
}

void ThreadLogTask(LogLevel i, int cnt) {
    for(int j = 0; j < 10000; j++ ){
        Log::GetInstance()->Write(i,"PID:[%04d]======= %05d ========= ", gettid(), cnt++);
    }
}

#include"../core/ThreadPool/ThreadPool.h"
void log_thread_test() {
    Log::GetInstance()->Init(DEBUG, "./log3", ".log", 5000);
    ThreadPool threadpool(6);
    for(int i = 0; i < 18; i++) {
        LogLevel l;
        switch (i%4)
        {
        case 0:
            l = DEBUG;
            break;
        case 1:
            l = INFO;
            break;
        case 2:
            l = WARN;
            break;
        case 3:
            l = ERROR;
            break;
        default:
            l = DEBUG;
            break;
        }
        threadpool.AddTask(std::bind(ThreadLogTask, l, i * 10000));
    }
    getchar();
}
////////////////////////////////////////////////////// Log /////////////////////////////////////////////////////

int main(){
    // threadPool_test();

    // buffer_test();

    // log_test_1();
    log_test_2();
    log_thread_test();
    return 0;
}
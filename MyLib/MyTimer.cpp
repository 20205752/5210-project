#include "MyTimer.h"

#include <time.h>
#include <stddef.h>
#include <chrono>

#ifdef _WIN32
// ---- Windows 平台 ----
#include <Windows.h>
#else
// ---- Linux / Unix 平台 ----
#include <sys/time.h>
#endif

using namespace std;

/*
 *  Return the current time in double format (seconds).
 */
double getCurrentTime() {
#ifdef _WIN32
    // Windows 版本：使用 chrono 获取微秒时间
    long long time = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now().time_since_epoch()).count();
    return time / 1000000.0;
#else
    // Linux 版本：使用 gettimeofday()
    struct timeval tim;
    gettimeofday(&tim, NULL);
    return tim.tv_sec + tim.tv_usec / 1000000.0;
#endif
}

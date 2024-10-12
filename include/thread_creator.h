#ifndef THREAD_CREATOR_H
#define THREAD_CREATOR_H

#ifdef PTHREAD_FOUND
#include <pthread.h>
typedef void *(*thread_func) (void *);
typedef pthread_t thread;
#define THREAD_FUNCTION(name) void *name (void *arg)
#else
#include <windows.h>
typedef DWORD WINAPI (*thread_func) (void *);
typedef HANDLE thread;
#define THREAD_FUNCTION(name) DWORD WINAPI name (void *arg)
#endif // PTHREAD_FOUND

thread thread_create (thread_func func, void *arg);

void thread_stop (thread t);

void thread_join (thread t);

#endif // THREAD_CREATOR_H

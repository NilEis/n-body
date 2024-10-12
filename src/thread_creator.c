#include "thread_creator.h"

thread thread_create (thread_func func, void *arg)
{
#ifdef PTHREAD_FOUND
    pthread_t t;
    pthread_create (&t, NULL, func, arg);
#else
    HANDLE t = CreateThread (NULL, 0, func, arg, 0, NULL);
#endif
}

void thread_stop (thread t)
{
#ifdef PTHREAD_FOUND
    pthread_cancel (t);
#else
    TerminateThread (t, 0);
#endif
}

void thread_join (thread t)
{
#ifdef PTHREAD_FOUND
    pthread_join (t, NULL);
#else
    WaitForSingleObject (t, INFINITE);
#endif
}
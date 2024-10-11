#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_ERROR "ERROR: "
#define LOG_CONTINUE ""
#define LOG_INFO "INFO:  "

#define LOG(a, str, ...) printf (a str, ##__VA_ARGS__)

#endif // LOG_H

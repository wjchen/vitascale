#ifndef _LOG_H_
#define _LOG_H_

#define LOG_BUFFER_SIZE  256
// #define VSCALE_DEBUG

int vsnprintf(char *s, size_t n, const char *format, va_list arg);
int snprintf(char * s, size_t n, const char * format, ...);


void vs_log_print(const char *titleid, const char *function, const char *format, ...);
#ifdef VSCALE_DEBUG
#define DBG(titleid, ...) vs_log_print(titleid, __FUNCTION__, __VA_ARGS__)
#else
#define DBG(titleid, ...) 
#endif
#endif
#define LOG(titleid, ...) vs_log_print(titleid, __FUNCTION__, __VA_ARGS__)

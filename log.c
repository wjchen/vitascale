#include <vitasdk.h>
#include <taihen.h>

#include "utils.h"
#include "log.h"
#include "config.h"

#define MAX_LOGFILE_SIZE 1024*1024

static void vs_log_empty() 
{
    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0) {
        sceIoClose(fd);
    }
}

void vs_log_print(const char *titleid, const char *function, const char *format, ...)
{
    int nwrite = 0;
    if (vs_get_filesize(LOG_PATH) > MAX_LOGFILE_SIZE) {
        vs_log_empty();
    }
    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd < 0) {
        return;
    }
    char buffer[LOG_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "[%s] %s:", titleid, function);
    nwrite = sceIoWrite(fd, buffer, strlen(buffer));
    if (nwrite != strlen(buffer)) {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    sceIoWrite(fd, buffer, strlen(buffer));
    sceIoClose(fd);
    va_end(args);
}

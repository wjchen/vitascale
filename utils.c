#include <vitasdk.h>
#include <taihen.h>

#include "utils.h"
int atoi(const char *nptr);
double atof(const char *nptr);

int vs_strlen(const char *path)
{
    if (path) {
        int len = 0;
        while (*path != 0) {
            path++;
            len++;
        }
        return len;
    }
    return 0;
}

int vs_get_filesize(const char *path)
{
    int ret;
    SceIoStat stat;
    if (vs_strlen(path) <= 0) return -1;
    ret = sceIoGetstat(path, &stat);
    if (ret >= 0) {
        return stat.st_size;
    }
    return -1;
}

int vs_atoi(const char *value)
{
    if (vs_strlen(value) <= 0) {
        return 0;
    }
    return atoi(value);
}

double vs_atof(const char *value)
{
    if (vs_strlen(value) <= 0) {
        return 0.0f;
    }
    return atof(value);
}

int vs_str_move(char *dst, char *src)
{
    int i;
    int n = vs_strlen(src);
    if (n == 0 || vs_strlen(dst) == 0) {
        return 0;
    }
    if (src == dst) {
        return n;
    }

    for (i = 0; i< n; i++) {
        dst[i] = src[i];
    }
    dst[n] = 0;
    return n;
}
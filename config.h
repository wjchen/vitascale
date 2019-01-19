#ifndef __CONFIG_H
#define __CONFIG_H

#define MAIN_DIR    "ux0:/data/vitascale"
#define CONFIG_PATH "ux0:/data/vitascale/config.txt"
#define LOG_PATH    "ux0:/data/vitascale/log.txt"

#define MAX_CFGFILE_SIZE  1024*1024
#define MAX_LOGFILE_SIZE  1024*1024
#define LINE_BUF_SIZE 256

#define TITLEID_SIZE      16

typedef struct {
    int   sx;
    int   sy;
    int   width;
    int   height;
    float scale;
    int   read_status;
} vitascale_cfg_t;

int vs_config_load(const char *titleid, vitascale_cfg_t *cfg);

#endif

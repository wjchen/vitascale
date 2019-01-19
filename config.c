#include <vitasdk.h>
#include <taihen.h>
#include "utils.h"
#include "config.h"

static char line_buf[LINE_BUF_SIZE+1] = {0};

#define CONFIG_SECTION_GLOBAL 1
#define CONFIG_SECTION_GAME   2


int snprintf(char *str, size_t size, const char *format, ...);

static  char curr_tileid[16];
static  int curr_section = CONFIG_SECTION_GLOBAL;

static int search_char_idx(const char *line, int ch)
{
    int i;
    for (i=0;;i++) {
        if (line[i] == 0) break;
        if (line[i] == ch) return i;
    }
    return -1;
}


static char * line_trim(char *line)
{
    int i;
    int comment_idx = search_char_idx(line, '#');
    if (comment_idx >= 0) {
        line[comment_idx] = 0;
    }
    comment_idx = search_char_idx(line, ';');
    if (comment_idx >= 0) {
        line[comment_idx] = 0;
    }
    int n = vs_strlen(line);
    char *real_line = NULL;
    if (n <= 0) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        int ch = line[i];
        if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t') {
            // skip
        } else {
            real_line = &line[i];
            break;
        }
    }
    if (i == n) { // all space, skip
        return NULL;
    }
    for (i = n-1; i >= 0; i--) {
        int ch = line[i];
        if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t') {
            line[i] = 0;
        } else {
            break;
        }
    }

    return real_line;
}

static int config_proc_line(const char *titleid, vitascale_cfg_t* cfg, char *line)
{
    char *real_line = line_trim(line);
    int line_len = vs_strlen(real_line);
    int title_len = vs_strlen(titleid);

    if (real_line == NULL || line_len <= 2) {
        return 0;
    }

    if (title_len+2 == line_len && 
        real_line[0] == '[' && real_line[line_len-1] == ']') {
        curr_section = CONFIG_SECTION_GAME;
        real_line[line_len-1] = 0;
        snprintf(curr_tileid, 16, "%s", &real_line[1]);
        return 0;
    }
    if (curr_section == CONFIG_SECTION_GLOBAL) {
    } else if (curr_section == CONFIG_SECTION_GAME) {
        if (strcmp(curr_tileid, titleid) == 0) {
            int idx = search_char_idx(real_line, '=');
            if (idx < 0) {
                return 0;
            }
            real_line[idx] = 0;
            char *key = line_trim(real_line);
            char *value = line_trim(real_line+idx+1);
            if (strcmp(key, "x") == 0) {
                cfg->sx = vs_atoi(value);
                cfg->read_status |= 1;
            } else if (strcmp(key, "y") == 0) {
                cfg->sy = vs_atoi(value);
                cfg->read_status |= 2;
            } else if (strcmp(key, "width") == 0) {
                cfg->width = vs_atoi(value);
                cfg->read_status |= 4;
            } else if (strcmp(key, "height") == 0) {
                cfg->height = vs_atoi(value);
                cfg->read_status |= 8;
            } else if (strcmp(key, "scale") == 0) {
                cfg->scale = vs_atof(value);
                cfg->read_status |= 16;
            } 
            if (cfg->read_status == 31) {
                return 1;
            }
        }
    }
    return 0;
}

int vs_config_load(const char *titleid, vitascale_cfg_t *cfg)
{
    int ret = 0;
    int line_num = 0;
    int pos = 0;
    int line_skip = 0;
    int nread = 0;
    int file_size = vs_get_filesize(CONFIG_PATH);
    SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        return -1;
    }
    cfg->sx = 0;
    cfg->sy = 0;
    cfg->width = 0;
    cfg->height = 0;
    cfg->scale = 0.0f;
    cfg->read_status = 0;
    curr_section = CONFIG_SECTION_GLOBAL;
    curr_tileid[0] = 0;
    line_buf[0] = 0;

    while (1) {
        int line_status = -1;
        int n = sceIoRead(fd, line_buf+pos, LINE_BUF_SIZE-pos);
        if (n <= 0) {
            if (pos != 0) {
                line_num++;
                line_status = config_proc_line(titleid, cfg, line_buf);
                if (line_status == 1) {
                    ret = 0;
                    break;
                }
                //printf("remain: %04d: %s\n", line_num, line_buf);
            }
            if (cfg->read_status != 0) {
                ret = -4; // some config missing
            } else {
                ret = -2;
            }
            break;
        }
        nread += n;
        if (nread >= MAX_CFGFILE_SIZE+LINE_BUF_SIZE) {
            ret = -3;
            break;
        }
        line_buf[pos + n] = 0;
        int end_idx = search_char_idx(line_buf, '\n');
        if (end_idx < 0) {
            if (n == LINE_BUF_SIZE-pos) { // line large
                line_skip = 1;
                pos = 0;
            } else { 
                if (line_skip) {
                    line_skip = 0;
                    pos = 0;
                } else if (nread == file_size) { // last line
                    //printf("read_last: %04d: %s\n", line_num, line_buf);
                    line_status = config_proc_line(titleid, cfg, line_buf);
                    line_num++;
                    if (line_status == 1) {
                        sceIoClose(fd);
                        return 0;
                    }
                } else { // partial read
                    pos = pos + n;
                }
            }
        } else {
            char *line_data = line_buf;
            if (line_skip) {
                line_skip = 0;
                line_data = &line_buf[end_idx+1];
            }
            while (1) {
                end_idx = search_char_idx(line_data, '\n');
                if (end_idx < 0) {
                    //printf("str_move\n");
                    pos = vs_str_move(line_buf, line_data);
                    break;
                }
                line_data[end_idx] = 0;
                line_num++;
                //printf("readline: %04d: %s\n", line_num, line_data);
                line_status = config_proc_line(titleid, cfg, line_data);
                if (line_status == 1) {
                    sceIoClose(fd);
                    return 0;
                }
                line_data = line_data+end_idx+1;
            }
        }
    }
    sceIoClose(fd);
    return ret;
}

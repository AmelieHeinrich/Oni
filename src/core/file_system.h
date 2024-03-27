/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 13:45:12
 */

#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

#include "core.h"

typedef struct file_t {
    char *path;
    void *handle;
} file_t;

enum file_open_mode {
    FileOpen_Read = BIT(1),
    FileOpen_Write = BIT(2),
    FileOpen_Create = BIT(3),
    FileOpen_Overwrite = BIT(4)
};

void file_open(file_t *file, const char *path, int mode);
void file_close(file_t *file);

void file_write_utf8(file_t *file, const char* buffer, uint64_t len);

#endif

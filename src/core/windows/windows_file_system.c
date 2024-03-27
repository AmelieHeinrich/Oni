/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 17:28:12
 */

#include <core/file_system.h>
#include <stdio.h>

#include <Windows.h>

void file_open(file_t *file, const char *path, int mode)
{
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = 0;

    if (mode & FileOpen_Read) {
        dwDesiredAccess |= GENERIC_READ;
        dwShareMode |= FILE_SHARE_READ;
    }
    if (mode & FileOpen_Write) {
        dwDesiredAccess |= GENERIC_WRITE;
        dwShareMode |= FILE_SHARE_WRITE;
    }
    if (mode & FileOpen_Create) {
        dwCreateDisposition |= CREATE_ALWAYS;
    }
    if (mode & FileOpen_Overwrite) {
        dwCreateDisposition |= TRUNCATE_EXISTING;
    }

    HANDLE handle = CreateFileA(path, dwDesiredAccess, dwShareMode, NULL, dwCreateDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!handle) {
        fprintf(stderr, "[FILE ERROR] Failed to open file at path %s\n", path);
        return;
    }

    file->handle = handle;
    file->path = strdup(path);
}

void file_close(file_t *file)
{
    CloseHandle(file->handle);
    free(file->path);
}

void file_write_utf8(file_t *file, const char* buffer, uint64_t len)
{
    DWORD dwSizeWritten = 0;
    if (!WriteFile(file->handle, buffer, len, &dwSizeWritten, NULL)) {
        fprintf(stderr, "[FILE ERROR] Failed to write file at path %s\n", file->path);
    }
    if (dwSizeWritten != len) {
        fprintf(stderr, "[FILE WARN] Only wrote %d/%d bytes at file %s\n", dwSizeWritten, len, file->path);
    }
}

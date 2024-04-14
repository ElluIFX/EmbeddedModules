// 用 lfs(littlefs) 模拟标准C库的文件操作

#ifndef LFS_STDLIB_H
#define LFS_STDLIB_H

#include "lfs.h"
#include "lwprintf.h"
#include "modules.h"
#include "uni_io.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STDLIB_LFS
#define STDLIB_LFS lfs
#endif

#ifndef EOF
#define EOF (-1)
#endif

extern lfs_t STDLIB_LFS;

#undef FILE
#undef ssize_t

#define FILE lfs_file_t
#define ssize_t lfs_ssize_t
#define fpos_t lfs_off_t

#undef stdin
#undef stdout
#undef stderr
#define stdin ((void*)0xBEEF)
#define stdout ((void*)0xDEAD)
#define stderr stdout

static inline int __cmode_to_lfs(const char* mode) {
    int flag = 0;
    if (strchr(mode, 'r')) {
        flag |= LFS_O_RDONLY;
    }
    if (strchr(mode, 'w')) {
        flag |= LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
    }
    if (strchr(mode, 'a')) {
        flag |= LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND;
    }
    if (strchr(mode, '+')) {
        flag &= ~(LFS_O_RDONLY | LFS_O_WRONLY);
        flag |= LFS_O_RDWR;
    }
    return flag;
}

static inline FILE* __fopen(const char* path, const char* mode) {
    FILE* file = m_alloc(sizeof(FILE));
    if (file) {
        int res = lfs_file_open(&STDLIB_LFS, file, path, __cmode_to_lfs(mode));
        if (res < 0) {
            m_free(file);
            file = NULL;
        }
    }
    return file;
}

static inline int __fclose(FILE* file) {
    int ret = lfs_file_close(&STDLIB_LFS, file);
    m_free(file);
    return ret;
}

static inline ssize_t __fread(void* ptr, size_t size, size_t nmemb,
                              FILE* file) {
    return lfs_file_read(&STDLIB_LFS, file, ptr, size * nmemb);
}

static inline ssize_t __fwrite(const void* ptr, size_t size, size_t nmemb,
                               FILE* file) {
    return lfs_file_write(&STDLIB_LFS, file, ptr, size * nmemb);
}

static inline int __fseek(FILE* file, long offset, int whence) {
    return lfs_file_seek(&STDLIB_LFS, file, offset, whence);
}

static inline long __ftell(FILE* file) {
    return lfs_file_tell(&STDLIB_LFS, file);
}

static inline int __feof(FILE* file) {
    return lfs_file_size(&STDLIB_LFS, file) == lfs_file_tell(&STDLIB_LFS, file);
}

static inline void __rewind(FILE* file) {
    lfs_file_rewind(&STDLIB_LFS, file);
}

static inline int __remove(const char* path) {
    return lfs_remove(&STDLIB_LFS, path);
}

static inline int __rename(const char* oldpath, const char* newpath) {
    return lfs_rename(&STDLIB_LFS, oldpath, newpath);
}

static inline int __ferror(FILE* file) {
    return 0;
}

static inline int __fflush(FILE* file) {
    return lfs_file_sync(&STDLIB_LFS, file);
}

static inline int __fgetc(FILE* file) {
    unsigned char c;
    if (__fread(&c, 1, 1, file) == 1) {
        return c;
    } else {
        return EOF;
    }
}

static inline char* __fgets(char* s, int size, FILE* file) {
    char* ptr = s;
    while (size > 1) {
        int c = __fgetc(file);
        if (c == EOF) {
            if (ptr == s) {
                return NULL;
            } else {
                break;
            }
        }
        *ptr++ = c;
        if (c == '\n') {
            break;
        }
        size--;
    }
    *ptr = '\0';
    return s;
}

static inline int __fputc(int c, FILE* file) {
    unsigned char ch = c;
    if (__fwrite(&ch, 1, 1, file) == 1) {
        return c;
    } else {
        return EOF;
    }
}

static inline int __fputs(const char* s, FILE* file) {
    int len = strlen(s);
    if (__fwrite(s, 1, len, file) == len) {
        return len;
    } else {
        return EOF;
    }
}

static int __fprintf_lwfunc(int ch, lwprintf_t* lwobj) {
    FILE* file = (FILE*)(lwobj->arg);
    if (file == stdout || file == stderr) {
        putchar(ch);
    } else if (file == stdin) {
        return EOF;
    }
    return __fputc(ch, file);
}

static inline int __fprintf(FILE* file, const char* format, ...) {
    va_list args;
    va_start(args, format);
    lwprintf_t lwp_pub;
    lwp_pub.out_fn = __fprintf_lwfunc;
    lwp_pub.arg = file;
    int ret = lwprintf_vprintf_ex(&lwp_pub, format, args);
    va_end(args);
    return ret;
}

static inline size_t __fsize(FILE* file) {
    return lfs_file_size(&STDLIB_LFS, file);
}

static inline int __fgetpos(FILE* file, fpos_t* pos) {
    *pos = lfs_file_tell(&STDLIB_LFS, file);
    return 0;
}

static inline int __fsetpos(FILE* file, const fpos_t* pos) {
    return lfs_file_seek(&STDLIB_LFS, file, *pos, LFS_SEEK_SET);
}

static inline int __vfprintf(FILE* file, const char* format, va_list args) {
    lwprintf_t lwp_pub;
    lwp_pub.out_fn = __fprintf_lwfunc;
    lwp_pub.arg = file;
    return lwprintf_vprintf_ex(&lwp_pub, format, args);
}

static inline int __vfscanf(FILE* file, const char* format, va_list args) {
    return EOF;
}

#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fseek
#undef ftell
#undef feof
#undef rewind
#undef remove
#undef rename
#undef ferror
#undef fflush
#undef fgetc
#undef fgets
#undef fputc
#undef fputs
#undef fprintf
#undef fsize
#undef fgetpos
#undef fsetpos
#undef vfprintf
#undef vfscanf
#define fopen(path, mode) __fopen(path, mode)
#define fclose(file) __fclose(file)
#define fread(ptr, size, nmemb, file) __fread(ptr, size, nmemb, file)
#define fwrite(ptr, size, nmemb, file) __fwrite(ptr, size, nmemb, file)
#define fseek(file, offset, whence) __fseek(file, offset, whence)
#define ftell(file) __ftell(file)
#define feof(file) __feof(file)
#define rewind(file) __rewind(file)
#define remove(path) __remove(path)
#define rename(oldpath, newpath) __rename(oldpath, newpath)
#define ferror(file) __ferror(file)
#define fflush(file) __fflush(file)
#define fgetc(file) __fgetc(file)
#define fgets(s, size, file) __fgets(s, size, file)
#define fputc(c, file) __fputc(c, file)
#define fputs(s, file) __fputs(s, file)
#define fprintf(file, format, ...) __fprintf(file, format, __VA_ARGS__)
#define fsize(file) __fsize(file)
#define fgetpos(file, pos) __fgetpos(file, pos)
#define fsetpos(file, pos) __fsetpos(file, pos)
#define vfprintf(file, format, args) __vfprintf(file, format, args)
#define vfscanf(file, format, args) __vfscanf(file, format, args)

#ifdef __cplusplus
}
#endif

#endif  // LFS_STDLIB_H

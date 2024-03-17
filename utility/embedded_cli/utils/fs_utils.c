/**
 * @file fs_utils.c
 * @brief 文件系统命令行工具
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0.0
 * @date 2024-03-11
 *
 * THINK DIFFERENTLY
 */

#include "fs_utils.h"

#include "lfs.h"
#include "sds.h"

// Private Defines --------------------------

#define FS_PRINTLN(...) PRINTLN(__VA_ARGS__)
#define FS_PRINT(...) PRINT(__VA_ARGS__)

#if FS_ENABLE_DEBUG
#define FS_DEBUG(...) LOG_DEBUG(__VA_ARGS__)
#else
#define FS_DEBUG(...)
#endif

#if FS_ENABLE_YMODEM
#include "ymodem.h"
#endif

#define FILE_COLOR T_WHITE
#define DIR_COLOR T_CYAN
#define ERROR_COLOR T_RED

// Private Typedefs -------------------------

// Private Macros ---------------------------

// Private Variables ------------------------

static EmbeddedCli *cli;
static lfs_t *lfs;
static lfs_file_t file;
static lfs_dir_t dir;
static struct lfs_info info;
static sds path;
static sds inv;

// Public Variables -------------------------

// Private Functions ------------------------

static const char *get_error(int errno) {
  switch (errno) {
    case LFS_ERR_IO:
      return "io error";
    case LFS_ERR_CORRUPT:
      return "file system corrupt";
    case LFS_ERR_NOENT:
      return "no such file or directory";
    case LFS_ERR_EXIST:
      return "file already exists";
    case LFS_ERR_NOTDIR:
      return "target is not a directory";
    case LFS_ERR_ISDIR:
      return "target is a directory";
    case LFS_ERR_NOTEMPTY:
      return "directory not empty";
    case LFS_ERR_BADF:
      return "bad file descriptor";
    case LFS_ERR_FBIG:
      return "file too large";
    case LFS_ERR_INVAL:
      return "invalid argument";
    case LFS_ERR_NOSPC:
      return "no space left on device";
    case LFS_ERR_NOMEM:
      return "no memory available";
    case LFS_ERR_NOATTR:
      return "no attribute";
    case LFS_ERR_NAMETOOLONG:
      return "name too long";
    default:
      return "unknown error";
  }
}

#define FS_ERROR(fmt, ...) \
  PRINTLN(T_FMT(ERROR_COLOR) "error: " fmt T_RST, ##__VA_ARGS__)
#define FS_ERRORNO(errno) \
  PRINTLN(T_FMT(ERROR_COLOR) "error: %s" T_RST, get_error(errno))
#define FS_ERRORNOP(errno, path) \
  PRINTLN(T_FMT(ERROR_COLOR) "error: %s (for %s)" T_RST, get_error(errno), path)

static sds to_absolute_path(sds path_in) {
  if (!path_in) return NULL;
  int len;
  sds temp = NULL;
  sds new_path = NULL;
  FS_DEBUG("path in: %s", path_in);
  if (path_in[0] == '/') {
    new_path = sdsnew(path_in);  // absolute path
  } else {
    new_path = sdsdup(path);  // relative path
    if (new_path[sdslen(new_path) - 1] != '/') new_path = sdscat(new_path, "/");
    new_path = sdscat(new_path, path_in);
  }
  FS_DEBUG("new path: %s", new_path);
process_point:
  // example: /xxx/yyy/../zzz
  len = sdslen(new_path);
  int last_slash = 0;
  int last_last_slash = -1;
  for (int i = 1; i < len - 1; i++) {
    if (new_path[i] == '/') {
      last_last_slash = last_slash;
      last_slash = i;
      continue;
    }
    if (new_path[i - 1] == '/' && new_path[i] == '.' &&
        new_path[i + 1] == '.') {
      if (last_last_slash == -1) {  // no parent dir
        FS_ERROR("no parent dir for root");
        goto error;
      }
      if (last_last_slash == 0) {
        temp = sdsnew("/");
      } else {
        temp = sdsnewlen(new_path, last_last_slash);  // /xxx/yyy
      }
      if (new_path[i + 2] == '/' && new_path[i + 3] != '\0') {
        if (last_last_slash > 0) temp = sdscat(temp, "/");  // /xxx/yyy/
        temp = sdscat(temp, new_path + i + 3);              // /xxx/yyy/zzz
      }
      sdsfree(new_path);
      new_path = temp;
      goto process_point;
    }
    if (new_path[i - 1] == '/' && new_path[i] == '.' &&
        new_path[i + 1] == '/') {                  // /xxx/./yyy just remove .
      sds temp = sdsnewlen(new_path, last_slash);  // /xxx
      if (new_path[i + 1] == '/' && new_path[i + 2] != '\0') {
        temp = sdscat(temp, "/");               // /xxx/
        temp = sdscat(temp, new_path + i + 2);  // /xxx/yyy
      }
      sdsfree(new_path);
      new_path = temp;
      goto process_point;
    }
  }
  if (new_path[sdslen(new_path) - 1] == '.') sdsrange(new_path, 0, -2);
  if (new_path[sdslen(new_path) - 1] == '/') sdsrange(new_path, 0, -2);
  return new_path;
error:
  sdsfree(new_path);
  return NULL;
}

static void ls_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  sds path_in = NULL;
  sds new_path = NULL;
  int errno;
  if (argc) {
    path_in = sdsnew(embeddedCliGetToken(args, 1));
    new_path = to_absolute_path(path_in);
    sdsfree(path_in);
    if (!new_path) return;
    if ((errno = lfs_dir_open(lfs, &dir, new_path)) != LFS_ERR_OK) {
      FS_ERRORNO(errno);
      sdsfree(new_path);
      return;
    }
    sdsfree(new_path);
  } else {
    if ((errno = lfs_dir_open(lfs, &dir, path)) != LFS_ERR_OK) {
      FS_ERRORNO(errno);
      return;
    }
  }

  lfs_dir_rewind(lfs, &dir);
  FS_PRINTLN("Type   Size       Name");
  FS_PRINTLN("----   --------   ----");
  while (lfs_dir_read(lfs, &dir, &info) > 0) {
    if (strcmp(path, "/") == 0 && strcmp(info.name, "..") == 0)
      continue;  // skip .. in root
    if (info.type == LFS_TYPE_REG) {
      FS_PRINTLN(T_FMT(FILE_COLOR) "file   %-8d   %s" T_RST, info.size,
                 info.name);
    } else if (info.type == LFS_TYPE_DIR) {
      FS_PRINTLN(T_FMT(DIR_COLOR) "dir    -          %s" T_RST, info.name);
    }
  }

  lfs_dir_close(lfs, &dir);
}

static void cd_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  int len;
  int errno;
  sds path_in = NULL;
  sds new_path = NULL;
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 1) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  path_in = sdsnew(embeddedCliGetToken(args, 1));

  new_path = to_absolute_path(path_in);
  sdsfree(path_in);

  if (!new_path) return;

  if ((errno = lfs_dir_open(lfs, &dir, new_path)) != LFS_ERR_OK) {
    FS_ERRORNO(errno);
    sdsfree(new_path);
    return;
  } else {
    lfs_dir_close(lfs, &dir);
  }

  // update path
  sdsfree(path);
  path = new_path;
  // update invitation
  sdsfree(inv);
  inv = sdscat(sdsdup(path), " > ");
  embeddedCliSetInvitation(cli, inv);
  return;
}

static void mkdir_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  int errno;
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 1) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  sds path_in = sdsnew(embeddedCliGetToken(args, 1));
  sds new_path = to_absolute_path(path_in);
  sdsfree(path_in);
  if (!new_path) return;
  FS_DEBUG("mkdir %s", new_path);
  if ((errno = lfs_mkdir(lfs, new_path)) != LFS_ERR_OK) {
    FS_ERRORNO(errno);
  }
  sdsfree(new_path);
}

static int rm_dir_rec(sds dirname) {
  sds temp;
  lfs_dir_t dirnow;
  struct lfs_info infonow;
  int errno;
  int cnt = 0;
  if ((errno = lfs_dir_open(lfs, &dirnow, dirname)) != LFS_ERR_OK) {
    FS_ERRORNO(errno);
    return 0;
  }
  lfs_dir_rewind(lfs, &dirnow);
  while (lfs_dir_read(lfs, &dirnow, &infonow) > 0) {
    if (strcmp(infonow.name, ".") == 0 || strcmp(infonow.name, "..") == 0)
      continue;
    temp = sdsdup(dirname);
    if (temp[sdslen(temp) - 1] != '/') temp = sdscat(temp, "/");
    temp = sdscat(temp, infonow.name);
    if (infonow.type == LFS_TYPE_REG) {
      if (!(errno = lfs_remove(lfs, temp))) {
        cnt++;
        FS_PRINTLN("rm %s", temp);
      } else {
        FS_ERRORNOP(errno, temp);
      }
    } else if (infonow.type == LFS_TYPE_DIR) {
      cnt += rm_dir_rec(temp);
    }
    sdsfree(temp);
  }
  lfs_dir_close(lfs, &dirnow);
  if (strcmp(dirname, "/") == 0) return cnt;
  if (!(errno = lfs_remove(lfs, dirname))) {
    cnt++;
    FS_PRINTLN("rm %s", dirname);
  } else {
    FS_ERRORNOP(errno, dirname);
  }
  return cnt;
}

static void rm_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  sds path_in = NULL;
  sds new_path = NULL;
  int errno;
  if (argc < 1) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  } else if (embeddedCliCheckToken(args, "-r", 1)) {
    if (argc < 2) {
      embeddedCliPrintCurrentHelp(cli);
      return;
    }
    path_in = sdsnew(embeddedCliGetToken(args, 2));
    new_path = to_absolute_path(path_in);
    sdsfree(path_in);
    if (!new_path) return;
    int cnt = rm_dir_rec(new_path);
    sdsfree(new_path);
    FS_PRINTLN("removed %d files/dirs", cnt);
    return;
  }
  path_in = sdsnew(embeddedCliGetToken(args, 1));
  new_path = to_absolute_path(path_in);
  sdsfree(path_in);
  if (!new_path) return;
  FS_DEBUG("rm %s", new_path);
  if ((errno = lfs_remove(lfs, new_path)) != LFS_ERR_OK) {
    FS_ERRORNO(errno);
  }
  sdsfree(new_path);
}

static void tree_rec(sds dirname, int depth) {
  sds temp;
  lfs_dir_t dirnow;
  int errno;
  struct lfs_info infonow;
  if ((errno = lfs_dir_open(lfs, &dirnow, dirname)) != LFS_ERR_OK) {
    FS_ERRORNOP(errno, dirname);
    return;
  }
  lfs_dir_rewind(lfs, &dirnow);
  while ((errno = lfs_dir_read(lfs, &dirnow, &infonow)) > 0) {
    if (strcmp(infonow.name, ".") == 0 || strcmp(infonow.name, "..") == 0)
      continue;
    for (int i = 0; i < depth; i++) {
      FS_PRINT("|  ");
    }
    if (infonow.type == LFS_TYPE_REG) {
      FS_PRINTLN("|--" T_FMT(FILE_COLOR) "%s" T_RST, infonow.name);
    } else if (infonow.type == LFS_TYPE_DIR) {
      FS_PRINTLN("|--" T_FMT(DIR_COLOR) "%s" T_RST, infonow.name);
      temp = sdscat(sdsdup(dirname), "/");
      temp = sdscat(temp, infonow.name);
      tree_rec(temp, depth + 1);
      sdsfree(temp);
    }
  }
  if (errno != LFS_ERR_OK) {
    FS_ERRORNO(errno);
  }
  lfs_dir_close(lfs, &dirnow);
}

static void tree_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  sds path_in = NULL;
  sds new_path = NULL;
  if (!argc) {
    path_in = sdsnew(path);
  } else {
    path_in = sdsnew(embeddedCliGetToken(args, 1));
  }
  new_path = to_absolute_path(path_in);
  sdsfree(path_in);
  if (!new_path) return;
  FS_PRINTLN(T_FMT(DIR_COLOR) "%s" T_RST, new_path);
  tree_rec(new_path, 0);
  sdsfree(new_path);
}

static void touch_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 1) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  sds path_in = sdsnew(embeddedCliGetToken(args, 1));
  sds new_path = to_absolute_path(path_in);
  int errno;
  sdsfree(path_in);
  if (!new_path) return;
  FS_DEBUG("touch %s", new_path);
  if ((errno = lfs_file_open(lfs, &file, new_path,
                             LFS_O_CREAT | LFS_O_WRONLY | LFS_O_EXCL)) !=
      LFS_ERR_OK) {
    FS_ERRORNO(errno);
  } else {
    lfs_file_close(lfs, &file);
  }
  sdsfree(new_path);
}

static void pwd_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  FS_PRINTLN("%s", path);
}

static int cp(sds src_path_in, sds dst_path_in, bool append) {
  sds src_path = to_absolute_path(src_path_in);
  sds dst_path = to_absolute_path(dst_path_in);
  lfs_file_t src_file;
  lfs_file_t dst_file;
  sdsfree(src_path_in);
  sdsfree(dst_path_in);
  int errno;
  if (!src_path || !dst_path) {
    sdsfree(src_path);
    sdsfree(dst_path);
    return 0;
  }
  FS_DEBUG("cp %s %s", src_path, dst_path);
  if ((errno = lfs_file_open(lfs, &src_file, src_path, LFS_O_RDONLY)) !=
      LFS_ERR_OK) {
    FS_ERRORNOP(errno, src_path);
    sdsfree(src_path);
    sdsfree(dst_path);
    return 0;
  }
  if ((errno = lfs_file_open(
           lfs, &dst_file, dst_path,
           append ? (LFS_O_CREAT | LFS_O_WRONLY | LFS_O_APPEND)
                  : (LFS_O_CREAT | LFS_O_WRONLY | LFS_O_TRUNC))) !=
      LFS_ERR_OK) {
    FS_ERRORNOP(errno, dst_path);
    sdsfree(src_path);
    sdsfree(dst_path);
    lfs_file_close(lfs, &src_file);
    return 0;
  }
  char rdbuf[128];
  int written = 0;
  while (1) {
    int n = lfs_file_read(lfs, &src_file, rdbuf, sizeof(rdbuf));
    if (n < 0) {
      FS_ERRORNOP(n, src_path);
      break;
    }
    if (n == 0) break;
    if ((n = lfs_file_write(lfs, &dst_file, rdbuf, n)) < 0) {
      FS_ERRORNOP(n, dst_path);
      break;
    }
    written += n;
  }
  sdsfree(src_path);
  sdsfree(dst_path);
  lfs_file_close(lfs, &src_file);
  lfs_file_close(lfs, &dst_file);
  return written;
}

static void cat_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 1) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  } else {
    int sel = embeddedCliCheckToken(args, ">", 2) +
              embeddedCliCheckToken(args, ">>", 2) * 2;
    if (sel) {
      if (argc < 3) {
        embeddedCliPrintCurrentHelp(cli);
        return;
      }
      sds src_path_in = sdsnew(embeddedCliGetToken(args, 1));
      sds dst_path_in = sdsnew(embeddedCliGetToken(args, 3));
      cp(src_path_in, dst_path_in, sel == 2);
      sdsfree(src_path_in);
      sdsfree(dst_path_in);
      return;
    }
  }
  sds path_in = sdsnew(embeddedCliGetToken(args, 1));
  sds new_path = to_absolute_path(path_in);
  int errno;
  sdsfree(path_in);
  if (!new_path) return;
  FS_DEBUG("cat %s", new_path);
  if ((errno = lfs_file_open(lfs, &file, new_path, LFS_O_RDONLY)) !=
      LFS_ERR_OK) {
    FS_ERRORNO(errno);
    sdsfree(new_path);
    return;
  }
  FS_PRINTLN("FILE: %s", new_path);
  sdsfree(new_path);
  const int max_width = 80;
  char linebuf[max_width + 1];
  int line_idx = 0;
  char rdbuf[max_width * 2];
  int line_no = 1;
  bool header_printed = false;
  while (1) {
    int n = lfs_file_read(lfs, &file, rdbuf, sizeof(rdbuf));
    if (n < 0) {
      FS_ERRORNO(n);
      break;
    }
    if (n == 0) break;
    for (int i = 0; i < n; i++) {
      if (rdbuf[i] == '\n') {
        linebuf[line_idx] = '\0';
        if (!header_printed) {
          FS_PRINT(T_FMT(T_GREEN) "%04d|" T_RST, line_no++);
          header_printed = true;
        }
        FS_PRINTLN("%s", linebuf);
        header_printed = false;
        line_idx = 0;
      } else if (rdbuf[i] == '\r') {
        continue;
      } else if (rdbuf[i] >= 32 && rdbuf[i] <= 126) {
        linebuf[line_idx++] = rdbuf[i];
      }
      if (line_idx >= max_width) {
        linebuf[line_idx] = '\0';
        if (!header_printed) {
          FS_PRINT(T_FMT(T_GREEN) "%04d|" T_RST, line_no++);
          FS_PRINTLN("%s", linebuf);
          FS_PRINT(T_FMT(T_GREEN) "   | " T_RST);
          header_printed = true;
        }
        line_idx = 0;
      }
    }
  }
  if (!header_printed) {
    FS_PRINT(T_FMT(T_GREEN) "%04d|" T_RST, line_no++);
    header_printed = true;
  }
  if (line_idx) {
    linebuf[line_idx] = '\0';
    FS_PRINTLN("%s", linebuf);
  } else {
    FS_PRINTLN("");
  }
  lfs_file_close(lfs, &file);
}

static void hexdump_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  int width = 8;
  if (argc < 1) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  } else if (embeddedCliCheckToken(args, "-w", 1)) {
    if (argc < 3) {
      embeddedCliPrintCurrentHelp(cli);
      return;
    }
    width = atoi(embeddedCliGetToken(args, 2));
    width = width > 0 ? width : 16;
    width = width < 128 ? width : 128;
  }
  sds path_in = sdsnew(embeddedCliGetToken(args, -1));
  sds new_path = to_absolute_path(path_in);
  int errno;
  sdsfree(path_in);
  if (!new_path) return;
  FS_DEBUG("hexdump %s", new_path);
  if ((errno = lfs_file_open(lfs, &file, new_path, LFS_O_RDONLY)) !=
      LFS_ERR_OK) {
    FS_ERRORNO(errno);
    sdsfree(new_path);
    return;
  }
  FS_PRINTLN("FILE: %s", new_path);
  sdsfree(new_path);
  char *rdbuf = (char *)m_alloc(width);
  if (!rdbuf) {
    FS_ERROR("failed to allocate buffer");
    lfs_file_close(lfs, &file);
    return;
  }
  int pos = 0;
  while (1) {
    int n = lfs_file_read(lfs, &file, rdbuf, width);
    if (n < 0) {
      FS_ERRORNO(n);
      break;
    }
    if (n == 0) break;
    FS_PRINT(T_FMT(T_GREEN) "0x%04X " T_RST, pos);
    for (int i = 0; i < n; i++) {  // print hex
      FS_PRINT("%02X ", (unsigned char)rdbuf[i]);
    }
    for (int i = n; i < width; i++) {  // padding
      FS_PRINT("   ");
    }
    FS_PRINT(T_FMT(T_GREEN) "| " T_RST);
    for (int i = 0; i < n; i++) {  // print ascii
      if (rdbuf[i] >= 32 && rdbuf[i] <= 126) {
        FS_PRINT("%c", rdbuf[i]);
      } else {
        FS_PRINT(T_FMT(T_BLUE) "." T_RST);
      }
    }
    FS_PRINTLN("");
    pos += n;
  }
  lfs_file_close(lfs, &file);
  m_free(rdbuf);
}

static char *echo_buf = NULL;
static int echo_size = 0;
static int echo_written = 0;

static char echo_handler(EmbeddedCli *cli, char data) {
  int errno;
  if (data == 4) {  // ctrl+d
    if (echo_size) {
      if ((errno = lfs_file_write(lfs, &file, echo_buf, echo_size)) < 0) {
        FS_ERRORNO(errno);
      } else {
        echo_written += echo_size;
      }
    }
    lfs_file_close(lfs, &file);
    m_free(echo_buf);
    echo_buf = NULL;
    echo_size = 0;
    PRINTLN("");
    PRINTLN("Written %d bytes", echo_written);
    embeddedCliResetRawHandler(cli);
    return 0;
  }
  if (data == '\r') data = '\n';  // silly windows
  echo_buf[echo_size++] = data;
  if (echo_size == 32) {
    if ((errno = lfs_file_write(lfs, &file, echo_buf, echo_size)) < 0) {
      FS_ERRORNO(errno);
    } else {
      echo_written += echo_size;
    }
    echo_size = 0;
  }
  return data;
}

static void echo_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 1) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  bool append = false;
  if (embeddedCliCheckToken(args, "-a", 1)) {
    if (argc < 2) {
      embeddedCliPrintCurrentHelp(cli);
      return;
    }
    append = true;
  }
  sds path_in = sdsnew(embeddedCliGetToken(args, append ? 2 : 1));
  sds new_path = to_absolute_path(path_in);
  int errno;
  sdsfree(path_in);
  if (!new_path) return;
  FS_DEBUG("echo %s", new_path);
  if ((errno = lfs_file_open(
           lfs, &file, new_path,
           append ? (LFS_O_CREAT | LFS_O_WRONLY | LFS_O_APPEND)
                  : (LFS_O_CREAT | LFS_O_WRONLY | LFS_O_TRUNC))) !=
      LFS_ERR_OK) {
    FS_ERRORNO(errno);
    sdsfree(new_path);
    return;
  }
  sdsfree(new_path);
  echo_buf = (char *)m_alloc(36);
  echo_size = 0;
  if (!echo_buf) {
    FS_ERROR("failed to create buffer");
    lfs_file_close(lfs, &file);
    return;
  }
  FS_PRINTLN("Input content, end with Ctrl+D:");
  echo_written = 0;
  embeddedCliSetRawHandler(cli, echo_handler);
}

static void cp_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 2) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  sds src_path_in = sdsnew(embeddedCliGetToken(args, 1));
  sds dst_path_in = sdsnew(embeddedCliGetToken(args, 2));
  int written = cp(src_path_in, dst_path_in, false);
  sdsfree(src_path_in);
  sdsfree(dst_path_in);
  FS_PRINTLN("Copied %d bytes", written);
}

static sds get_basename(sds path) {
  int len = sdslen(path);
  int last_slash = -1;
  for (int i = len - 1; i >= 0; i--) {
    if (path[i] == '/') {
      last_slash = i;
      break;
    }
  }
  if (last_slash == -1) return sdsdup(path);
  return sdsnewlen(path + last_slash + 1, len - last_slash - 1);
}

static void mv_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  int errno;
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 2) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  sds src_path_in = sdsnew(embeddedCliGetToken(args, 1));
  sds dst_path_in = sdsnew(embeddedCliGetToken(args, 2));
  sds src_path = to_absolute_path(src_path_in);
  sds dst_path = to_absolute_path(dst_path_in);
  if (!src_path || !dst_path) {
    goto free;
  }
  if (strncmp(src_path, dst_path, sdslen(src_path)) == 0) {
    FS_ERROR("destination contains source");
    goto free;
  }
  FS_DEBUG("mv %s %s", src_path, dst_path);
  if ((errno = lfs_rename(lfs, src_path, dst_path)) != LFS_ERR_OK) {
    if (errno == LFS_ERR_ISDIR) {
      sds src_basename = get_basename(src_path);
      if (dst_path[sdslen(dst_path) - 1] != '/') {
        dst_path = sdscat(dst_path, "/");
      }
      dst_path = sdscat(dst_path, src_basename);
      sdsfree(src_basename);
      if ((errno = lfs_rename(lfs, src_path, dst_path)) != LFS_ERR_OK) {
        FS_ERRORNO(errno);
      }
    } else {
      FS_ERRORNO(errno);
    }
  }
free:
  sdsfree(src_path);
  sdsfree(dst_path);
  sdsfree(src_path_in);
  sdsfree(dst_path_in);
  return;
}

static void format_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  int argc = embeddedCliGetTokenCount(args);
  int errno;
  if (argc < 1 || !embeddedCliCheckToken(args, "--confirm", 1)) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  const struct lfs_config *cfg = lfs->cfg;
  lfs_unmount(lfs);
  if ((errno = lfs_format(lfs, cfg)) != LFS_ERR_OK) {
    FS_ERRORNO(errno);
    return;
  }
  if ((errno = lfs_mount(lfs, cfg)) != LFS_ERR_OK) {
    FS_ERRORNO(errno);
    return;
  }
  FS_PRINTLN("File system formatted");
}

#if FS_ENABLE_YMODEM
int y_transmit_ch(y_uint8_t ch) {
  FS_PRINT("%c", ch);
  return 0;
}

sds y_file_name = NULL;

int y_receive_nanme_size_callback(void **ptr, char *file_name,
                                  y_uint32_t file_size) {
  if (!y_file_name) {
    sds path_in = sdsnew(file_name);
    y_file_name = to_absolute_path(path_in);
    sdsfree(path_in);
    if (!y_file_name) return -1;
    if (lfs_file_open(lfs, &file, y_file_name,
                      LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0) {
      return -1;
    }
  }
  return 0;
}

int y_receive_file_data_callback(void **ptr, char *file_data,
                                 y_uint32_t w_size) {
  if (lfs_file_write(lfs, &file, file_data, w_size) < 0) {
    return -1;
  }
  return 0;
}

int y_receive_file_callback(void **ptr) {
  if (y_file_name) lfs_file_close(lfs, &file);
  return 0;
}

static void ymodem_sub_handler(EmbeddedCli *cli, const char *data, size_t len) {
  ymodem_receive_buffer((y_uint8_t *)data, len);
}

static void rb_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (argc < 1) {
    y_file_name = NULL;
  } else {
    sds path_in = sdsnew(embeddedCliGetToken(args, 1));
    if (y_file_name) sdsfree(y_file_name);
    y_file_name = to_absolute_path(path_in);
    sdsfree(path_in);
    if (!y_file_name) return;
    int errno;
    if ((errno = lfs_file_open(lfs, &file, y_file_name,
                               LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC)) !=
        LFS_ERR_OK) {
      FS_ERRORNO(errno);
      sdsfree(y_file_name);
      y_file_name = NULL;
      return;
    }
    FS_PRINTLN("Specify file: %s", y_file_name);
  }
  FS_PRINTLN("YModem receive started, hit Ctrl+C to cancel");
  embeddedCliSetRawBufferHandler(cli, ymodem_sub_handler);
  int cnt = ymodem_receive();
  embeddedCliResetRawBufferHandler(cli);
  FS_PRINTLN("");
  if (cnt == 0 || (cnt & (1 << 15))) {
    FS_ERROR("YModem receive failed or canceled");
  } else {
    FS_PRINTLN("YModem receive finished, file saved to %s", y_file_name);
  }
  sdsfree(y_file_name);
  y_file_name = NULL;
}

#endif  // FS_ENABLE_YMODEM

// Public Functions -------------------------

void FSUtils_AddCmdToCli(EmbeddedCli *reg_cli, lfs_t *reg_lfs) {
  lfs = reg_lfs;
  cli = reg_cli;
  path = sdsnew("/");
  inv = sdscat(sdsdup(path), " > ");
  embeddedCliSetInvitation(cli, inv);
  // register commands
  static CliCommandBinding cd_cmd = {
      .name = "cd",
      .usage = "cd <dir>",
      .help = "Change working directory",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = cd_cmd_func,
  };
  embeddedCliAddBinding(cli, cd_cmd);
  static CliCommandBinding ls_cmd = {
      .name = "ls",
      .usage = "ls [dir]",
      .help = "List files and directories",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = ls_cmd_func,
  };
  embeddedCliAddBinding(cli, ls_cmd);
  static CliCommandBinding mkdir_cmd = {
      .name = "mkdir",
      .usage = "mkdir <dir>",
      .help = "Create a directory",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = mkdir_cmd_func,
  };
  embeddedCliAddBinding(cli, mkdir_cmd);
  static CliCommandBinding rm_cmd = {
      .name = "rm",
      .usage = "rm [-r recursive] <file|dir>",
      .help = "Remove a file or directory",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = rm_cmd_func,
  };
  embeddedCliAddBinding(cli, rm_cmd);
  static CliCommandBinding tree_cmd = {
      .name = "tree",
      .usage = "tree [dir]",
      .help = "List files and directories in tree view",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = tree_cmd_func,
  };
  embeddedCliAddBinding(cli, tree_cmd);
  static CliCommandBinding touch_cmd = {
      .name = "touch",
      .usage = "touch <file>",
      .help = "Create a empty file",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = touch_cmd_func,
  };
  embeddedCliAddBinding(cli, touch_cmd);
  static CliCommandBinding pwd_cmd = {
      .name = "pwd",
      .usage = "pwd",
      .help = "Print working directory",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = pwd_cmd_func,
  };
  embeddedCliAddBinding(cli, pwd_cmd);
  static CliCommandBinding cat_cmd = {
      .name = "cat",
      .usage = "cat <file> [> | >>] [file2]",
      .help = "Print file content or redirect to another file",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = cat_cmd_func,
  };
  embeddedCliAddBinding(cli, cat_cmd);
  static CliCommandBinding hexdump_cmd = {
      .name = "hexdump",
      .usage = "hexdump [-w width] <file>",
      .help = "Print file content in hex",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = hexdump_cmd_func,
  };
  embeddedCliAddBinding(cli, hexdump_cmd);
  static CliCommandBinding echo_cmd = {
      .name = "echo",
      .usage =
          "echo [-a append | -iN insert at line N | -dN delete line N] "
          "<file>",
      .help = "Write content to file or do limited editing",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = echo_cmd_func,
  };
  embeddedCliAddBinding(cli, echo_cmd);
  static CliCommandBinding cp_cmd = {
      .name = "cp",
      .usage = "cp <src> <dst>",
      .help = "Copy file",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = cp_cmd_func,
  };
  embeddedCliAddBinding(cli, cp_cmd);
  static CliCommandBinding mv_cmd = {
      .name = "mv",
      .usage = "mv <src> <dst>",
      .help = "Move file",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = mv_cmd_func,
  };
  embeddedCliAddBinding(cli, mv_cmd);
  static CliCommandBinding format_cmd = {
      .name = "format",
      .usage = "format [--confirm]",
      .help = "Format file system, confirm is required",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = format_cmd_func,
  };
  embeddedCliAddBinding(cli, format_cmd);
#if FS_ENABLE_YMODEM
  static CliCommandBinding rb_cmd = {
      .name = "rb",
      .usage = "rb <force_file_path>",
      .help = "Receive binary file via YModem",
      .context = NULL,
      .autoTokenizeArgs = 1,
      .func = rb_cmd_func,
  };
  embeddedCliAddBinding(cli, rb_cmd);
#endif
}

// Source Code End --------------------------

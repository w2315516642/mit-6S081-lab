#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char *
fmtname(char *path)
{
  static char buf[DIRSIZ + 1];
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if (strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = '\0';
  return buf;
}

void
fork_exec(char *argv[])
{
  int pid = fork();
  if (pid < 0) {
    printf("fork error.\n");
    exit(1);
  }
  if (pid == 0) {
    exec(argv[0], argv);
    printf("command error: %s\n", argv[0]);
    exit(1);
  } else {
    wait((int *)0);
  }
}

void
find(char *path, const char *t_name, int argc, char *argv[])
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  int use_exec = argc > 0;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
  case T_DEVICE:
  case T_FILE:
    char *name = fmtname(path);
    if (strcmp(name, t_name) == 0) {
      if (use_exec) {
        argv[argc] = path;
        fork_exec(argv);
      } else {
        printf("%s\n", path);
      }
    }
    break;
  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      printf("find: path is too long");
      close(fd);
      return;
    }
    // 匹配目录本身
    {
      char *name = fmtname(path);
      if (strcmp(name, t_name) == 0) {
        if (use_exec) {
          argv[argc] = path;
          fork_exec(argv);
        } else {
          printf("%s\n", path);
        }
      }
    }
    // 匹配其它对象
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if (stat(buf, &st) < 0) {
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      char *name = fmtname(buf);
      // if (strcmp(name, t_name) == 0) {
      //   printf("%s\n", buf); // 输出路径
      // }
      if (st.type == T_DIR && strcmp(name, ".") != 0 &&
          strcmp(name, "..") != 0) {
        find(buf, t_name, argc, argv);
      } else if (strcmp(name, t_name) == 0) {
        if (use_exec) {
          argv[argc] = buf;
          fork_exec(argv);
        } else {
          printf("%s\n", buf);
        }
      }
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("number of args error.\n");
    exit(1);
  } else if (argc == 3) {
    find(argv[1], argv[2], 0, 0);
  } else if (argc == 4) {
    printf("number of args error.\n");
    exit(1);
  } else if (strcmp(argv[3], "-exec") != 0) {
    printf("args error, get %s.\n", argv[3]);
    exit(1);
  } else {
    char *path = argv[1];
    char *t_name = argv[2];
    char *cmd[argc - 4 + 2];
    for (int i = 0; i < argc - 4; i++) {
      cmd[i] = argv[i + 4];
    }
    cmd[argc - 4 + 1] = 0; // 空指针

    find(path, t_name, argc - 4, cmd);
  }
  exit(0);
}

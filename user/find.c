#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

char *exec_argv[MAXARG];
int exec_argc = 0;

char *
fmtname(char *path)
{
  static char buf[DIRSIZ + 1];
  char *p;

  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}

void
run_exec(char *file)
{
  if (fork() == 0) {
    exec_argv[exec_argc] = file;
    exec_argv[exec_argc + 1] = 0;
    exec(exec_argv[0], exec_argv);
    fprintf(2, "find: exec %s failed\n", exec_argv[0]);
    exit(1);
  } else {
    wait(0);
  }
}

//------------regex matchher copied from grep.c --------------------
int matchhere(char *, char *);
int matchstar(int, char *, char *);

int
match(char *re, char *text)
{
  if (re[0] == '^')
    return matchhere(re + 1, text);
  do { // must look at empty string
    if (matchhere(re, text))
      return 1;
  } while (*text++ != '\0');
  return 0;
}

// matchhere: search for re at beginning of text
int
matchhere(char *re, char *text)
{
  if (re[0] == '\0')
    return 1;
  if (re[1] == '*')
    return matchstar(re[0], re + 2, text);
  if (re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if (*text != '\0' && (re[0] == '.' || re[0] == *text))
    return matchhere(re + 1, text + 1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int
matchstar(int c, char *re, char *text)
{
  do { // a * matches zero or more instances
    if (matchhere(re, text))
      return 1;
  } while (*text != '\0' && (*text++ == c || c == '.'));
  return 0;
}

// -------------------REGX MATCHER  ENDS----------------------------

void
find(char *path, char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type == T_FILE) {
    if (match(target, fmtname(path))) {
      if (exec_argc > 0)
        run_exec(path);
      else
        printf("%s\n", path);
    }
    close(fd);
    return;
  }

  if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      fprintf(2, "find: path too long\n");
      close(fd);
      return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;

      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if (stat(buf, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", buf);
        continue;
      }

      if (match(target, fmtname(buf))){
        if (exec_argc > 0)
          run_exec(buf);
        else
          printf("%s\n", buf);
      }

      if (st.type == T_DIR)
        find(buf, target);
    }
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  static char anchored[128];

  if (argc < 3) {
    fprintf(2, "Usage: find dir name [-exec cmd]\n");
    exit(1);
  }

  int i = 3;
  if (argc > 3 && strcmp(argv[3], "-exec") == 0) {
    for (i = 4; i < argc; i++)
      exec_argv[exec_argc++] = argv[i];
  }
  // build an anchored copy of the pattern once
  char *p = anchored;
  int len = strlen(argv[2]);
  if (argv[2][0] != '^')
    *p++ = '^';
  memmove(p, argv[2], len);
  p += len;
  if (len == 0 || argv[2][len - 1] != '$')
    *p++ = '$';
  *p = '\0';	

  find(argv[1], anchored);
  exit(0);
}


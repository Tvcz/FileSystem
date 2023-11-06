// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "storage.h"
#include "inode.h"
#include "directory.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

// inode bitmap 4096 bytes (1 block)
#define INODE_BITMAP_START 0
// block bitmap 4096 bytes (1 block)
#define BLOCK_BITMAP_START 4096
// inodes 4096 bytes (1 block)
#define INODE_START 8192
// blocks 524288 bytes (128 blocks)
#define BLOCK_START 12288

// implementation for: man 2 access
// Checks if a file exists.
int nufs_access(const char *path, int mask)
{
  int inum = tree_lookup(path);
  // TODO: Mask? (can be read/written/executed?)
  printf("access(%s, %04o) -> %d\n", path, mask, inum);
  return inum != -1 ? 0 : -1;
}

// Gets an object's attributes (type, permissions, size, etc).
// Implementation for: man 2 stat
// This is a crucial function.
int nufs_getattr(const char *path, struct stat *st)
{
  int rv = storage_stat(path, st);
  printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode,
         st->st_size);
  return rv == 0 ? 0 : -ENOENT;
}

// concatenates two strings
char *concat(char *s1, char *s2)
{
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi)
{
  struct stat st;
  int rv;

  int inum_dir = tree_lookup(path);
  inode_t *inode_dir = get_inode(inum_dir);
  slist_t *dir_entries = directory_list_path(path);
  slist_t *cur = dir_entries;
  slist_t *entry_paths = NULL;
  s_print(dir_entries);

  char *full_path; 
  if (strcmp(path, "/") != 0)
  {
    full_path = concat((char * ) path, "/");
  }
  else
  {
    full_path = (char *)malloc(8);
    strcpy(full_path, "/");
  }

  while (cur != NULL)
  {
    entry_paths = s_cons(concat(full_path, cur->data), entry_paths);
    cur = cur->next;
  }
  s_print(entry_paths);

  slist_t *cur_name = dir_entries;
  slist_t *cur_path = entry_paths;
  while (cur_name != NULL && cur_path != NULL)
  {
    rv = nufs_getattr(cur_path->data, &st);
    if (rv != 0)
    {
      return rv;
    }
    filler(buf, cur_name->data, &st, 0);
    cur_name = cur_name->next;
    cur_path = cur_path->next;
  }

  s_free(dir_entries);
  s_free(entry_paths);
  free(full_path);
  printf("readdir(%s) -> %d\n", path, rv);
  return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
int nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
  int inum = find_or_create(path, mode);
  int rv = inum != -1 ? 0 : -1;
  fprintf(stderr, "mknod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int nufs_mkdir(const char *path, mode_t mode)
{
  int rv = nufs_mknod(path, DIRECTORY_MODE, 0);

  int inum = tree_lookup(path);
  inode_t *dir = get_inode(inum);
  ((dirhead_t *)(blocks_get_block(get_inode(inum)->block)))->num_entries = 0;
  directory_put(dir, ".", inum);

  char **sp = split_path(path);
  int inum_parent = tree_lookup(sp[0]);
  inode_t *dir_parent = get_inode(inum_parent);
  directory_put(dir, "..", inum_parent);
  free(sp[0]);
  free(sp[1]);
  free(sp);

  printf("mkdir(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_unlink(const char *path)
{
  int rv = storage_unlink(path);
  printf("unlink(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_link(const char *from, const char *to)
{
  int rv = storage_link(from, to);
  printf("link(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_rmdir(const char *path)
{
  int rv = storage_unlink(path);
  printf("rmdir(%s) -> %d\n", path, rv);
  return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int nufs_rename(const char *from, const char *to)
{
  int rv = storage_rename(from, to);
  printf("rename(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_chmod(const char *path, mode_t mode)
{
  int rv = 0;
  printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

int nufs_truncate(const char *path, off_t size)
{
  int rv = storage_truncate(path, size);
  printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
  return rv;
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible.
int nufs_open(const char *path, struct fuse_file_info *fi)
{
  int inum = tree_lookup(path);
  int rv = inum != -1 ? 0 : -1;
  printf("open(%s) -> %d\n", path, rv);
  return rv;
}

// Actually read data
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
  int rv = storage_read(path, buf, size, offset);
  fprintf(stderr, "read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Actually write data
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi)
{
  int rv = storage_write(path, buf, size, offset);
  fprintf(stderr, "write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Update the timestamps on a file or directory.
int nufs_utimens(const char *path, const struct timespec ts[2])
{
  int rv = 0;
  fprintf(stderr, "utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n", path, ts[0].tv_sec,
          ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
  return rv;
}

// Extended operations
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data)
{
  int rv = 0;
  printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
  return rv;
}

void nufs_init_ops(struct fuse_operations *ops)
{
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->write = nufs_write;
  ops->utimens = nufs_utimens;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[])
{
  assert(argc > 2 && argc < 6);
  printf("Mounted %s as data file\n", argv[--argc]);
  storage_init(argv[argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}

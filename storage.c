#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bitmap.h"
#include "inode.h"
#include "storage.h"
#include "directory.h"

void storage_init(const char *image_path)
{
  blocks_init(image_path);
  void *bbm = get_blocks_bitmap();
  if (bitmap_get(bbm, 1) == 0)
  {
    int inode_block = alloc_block();
    assert(inode_block == INODE_BLOCK);
  }
  if (bitmap_get(bbm, 2) == 0)
  {
    root_init();
  }
}

// split the given path into a directory path and a filenmae
char **split_path(const char *path)
{
  char **result = malloc(sizeof(char *) * 2);
  char *dir = malloc(sizeof(char) * strlen(path));
  char *file = malloc(sizeof(char) * strlen(path));
  int i = 0;
  int j = 0;
  int k = 0;
  while (path[i] != '\0')
  {
    if (path[i] == '/')
    {
      j = i;
    }
    i++;
  }
  while (path[k] != '\0')
  {
    if (k < j)
    {
      dir[k] = path[k];
    }
    else if (k > j)
    {
      file[k - j - 1] = path[k];
    }
    k++;
  }
  dir[j] = '\0';
  file[i - j - 1] = '\0';
  result[0] = dir;
  result[1] = file;
  return result;
}

//Find the inum for a path, or create the item if it doesn't exist
int find_or_create(const char *path, mode_t mode)
{
  int inum = tree_lookup(path);
  if (inum != -1)
  {
    return inum;
  }
  inum = alloc_inode(mode);
  char **sp = split_path(path);
  int inum_dir = tree_lookup(sp[0]);
  if (inum_dir == -1)
  {
    return -1;
  }
  directory_put(get_inode(inum_dir), sp[1], inum);
  free(sp[0]);
  free(sp[1]);
  free(sp);
  return inum;
}

// gets the details on the file at path and sets them in the stat struct
int storage_stat(const char *path, struct stat *st)
{
  int inum = tree_lookup(path);
  if (inum == -1)
  { // containing directory does not exist
    return -1;
  }
  inode_t *inode = get_inode(inum);
  st->st_mode = inode->mode;
  st->st_size = inode->size;
  st->st_nlink = inode->ref_count;
  st->st_uid = getuid(); // From demo code
  return 0;
}

// reads size bytes from the file at path, offset from the beginning of the file, into the buffer buf
int storage_read(const char *path, char *buf, size_t size, off_t offset)
{
  int inum = tree_lookup(path);
  if (inum == -1)
  {
    return -1;
  }
  inode_t *inode = get_inode(inum);
  if (offset > inode->size)
  {
    return 0;
  }
  char *start = ((char *)blocks_get_block(inode->block)) + offset;
  size_t size_to_read = size + offset < 4096 ? size : 4096 - offset;
  fprintf(stderr, "+ read %d bytes from %p\n", (int) size_to_read, start);
  memcpy(buf, start, size_to_read);
  return size_to_read;
}

// writes size bytes to the file at path, offset from the beginning of the file, from the buffer buf
int storage_write(const char *path, const char *buf, size_t size, off_t offset)
{
  int inum = tree_lookup(path);
  if (inum == -1)
  { // file does not exist yet
    inum = find_or_create(path, FILE_MODE);
    if (inum == -1)
    { // containing directory does not exist
      return -1;
    }
    // tries to set the new file size to be large enough for the size after offsetting, at most one block
    get_inode(inum)->size = offset + size < 4096 ? offset + size : 4096;
  }
  inode_t *inode = get_inode(inum);
  if (offset > inode->size)
  {
    return -1;
  }
  char *start = ((char *)blocks_get_block(inode->block)) + offset;
  size_t size_to_write = size + offset < 4096 ? size : 4096 - offset; // clamp SIZE_TO_WRITE in legal values
  inode->size = size_to_write + offset;
  fprintf(stderr, "+ write %d bytes to %p\n", (int) size_to_write, start);
  memcpy(start, buf, size_to_write);
  return size_to_write;
}

// changes the file at path's size to the given size, at most one block
int storage_truncate(const char *path, off_t size)
{
  int inum = tree_lookup(path);
  if (inum == -1)
  {
    return -1;
  }
  inode_t *inode = get_inode(inum);
  if (size > 4096)
  {
    return -1;
  }
  inode->size = size;
  return 0;
}

// removes a link to a file from a directory
int storage_unlink(const char *path)
{
  int inum = tree_lookup(path);
  if (inum == -1)
  {
    return -1;
  }

  char **sp = split_path(path);

  int inum_dir = tree_lookup(sp[0]);
  if (inum_dir == -1)
  {
    return -1;
  }
  directory_delete(get_inode(inum_dir), sp[1]);
  free(sp[0]);
  free(sp[1]);
  free(sp);
  return 0;
}

// adds a link to a file in a directory
int storage_link(const char *from, const char *to)
{
  int inumfrom = tree_lookup(from);
  if (inumfrom == -1)
  {
    return -1;
  }
  int inumto = tree_lookup(to);
  if (inumto != -1)
  {
    return -1;
  }
  char **sp = split_path(to);
  int inumto_dir = tree_lookup(sp[0]);
  if (inumto_dir == -1)
  {
    return -1;
  }
  directory_put(get_inode(inumto_dir), sp[1], inumfrom);
  free(sp[0]);
  free(sp[1]);
  free(sp);
  return 0;
}

// renames the given file
int storage_rename(const char *from, const char *to)
{
  if (storage_link(from, to) == -1)
  {
    return -1;
  }
  if (storage_unlink(from) == -1)
  {
    return -1;
  }
  return 0;
}
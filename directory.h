// Directory manipulation functions.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48
#define ROOT_BLOCK 2

#include "blocks.h"
#include "inode.h"
#include "slist.h"

typedef struct dirhead {
  int num_entries;
  int _reserved[15];
} dirhead_t;

typedef struct direntry {
  char name[DIR_NAME_LENGTH];
  int inum;
  int present;
  char _reserved[10];
} direntry_t;

void root_init();
int directory_lookup(inode_t *dd, const char *name);
int tree_lookup(const char *path);
int directory_put(inode_t *dd, const char *name, int inum);
int directory_delete(inode_t *dd, const char *name);
slist_t *directory_list_path(const char *path);
int get_inum_from_block(void * block);
void print_directory(inode_t *dd);

#endif

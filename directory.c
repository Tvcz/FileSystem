#include "inode.h"
#include "directory.h"
#include <assert.h>
#include <string.h>


// set up the root inode and confirm it is at the correct block
void root_init() {
  int inum = alloc_inode(DIRECTORY_MODE);
  inode_t *root = get_inode(inum);
  fprintf(stderr, "+ Root block -> %d\n",root->block);
  assert(root->block == ROOT_BLOCK);
  ((dirhead_t *) (blocks_get_block(root->block)))->num_entries = 0;
  directory_put(root, ".", inum);
}

// get the inum of a file of the given name in a directory if it exists, -1 if not
int directory_lookup(inode_t *dd, const char *name) {
  if (dd->mode != DIRECTORY_MODE) {
    return -1;
  }
  dirhead_t *dir = (dirhead_t *) blocks_get_block(dd->block); 
  int end_index = dir->num_entries;
  direntry_t *entries_start = (direntry_t *) (dir + 1);
  for (int i = 0; i < end_index; i++) {
    direntry_t *entry = entries_start + i;
    if (entry->present == 1 && strcmp(entry->name, name) == 0) {
      return entry->inum;
    }
    else if (entry->present == 0) {
      end_index++;
    }
  }
  return -1;
}

// get the file at the given path if it exists, or -1 if not
int tree_lookup(const char *path) {
  char *path_copy = strdup(path);
  char *token = strtok(path_copy, "/"); 
  int inum = get_inum_from_block(blocks_get_block(ROOT_BLOCK));
  assert(inum != -1); // root must have . entry
  while (token != NULL) {
    inode_t *dd = get_inode(inum);
    inum = directory_lookup(dd, token);
    if (inum == -1) {
      return -1;
    }
    token = strtok(NULL, "/");
  }
  return inum;
}

// add a file with the given name and inum to the given directory
int directory_put(inode_t *dd, const char *name, int inum) {
  fprintf(stderr, "+ directory_put: %s -> %d\n", name, inum);
  dirhead_t *dir = (dirhead_t *) blocks_get_block(dd->block); 
  int num_entries = dir->num_entries;
  direntry_t *entries_start = (direntry_t *) (dir + 1);
  direntry_t *new_entry = NULL;
  for (int i = 0; i < num_entries; i++) {
    direntry_t *entry = entries_start + i;
    if (entry->present == 0) {
      new_entry = entry;
      break;
    }
  }
  if (new_entry == NULL) {
    new_entry = entries_start + num_entries;
  }

  strcpy(new_entry->name, name);
  new_entry->inum = inum;
  new_entry->present = 1;
  dir->num_entries++;
  get_inode(inum)->ref_count++;
  return 0;
}

// remove a file with the given name from the given directory
int directory_delete(inode_t *dd, const char *name) {
  dirhead_t *dir = (dirhead_t *) blocks_get_block(dd->block); 
  int end_index = dir->num_entries;
  direntry_t *entries_start = (direntry_t *) (dir + 1);
  for (int i = 0; i < end_index; i++) {
    direntry_t *entry = entries_start + i;
    if (entry->present == 1 && strcmp(entry->name, name) == 0) {
      entry->present = 0;
      dir->num_entries--;
      get_inode(entry->inum)->ref_count--;
      if (get_inode(entry->inum)->ref_count == 0) {
        free_inode(entry->inum);
      }
      return 0;
    }
    else if (entry->present == 0) {
      end_index++;
    }
  }
  return 1;
}

// return an slist of the files in the given directory
slist_t *directory_list(inode_t *dd) {
  dirhead_t *dir = (dirhead_t *) blocks_get_block(dd->block); 
  int end_index = dir->num_entries;
  direntry_t *entries_start = (direntry_t *) (dir + 1);
  slist_t *list = NULL;  
  for (int i = 0; i < end_index; i++) {
    direntry_t *entry = entries_start + i;
    if (entry->present == 1) {
      list = s_cons(entry->name, list);
    }
    else if (entry->present == 0) {
      end_index++;
    }
  }
  return list;
}

// return an slist of the files in the directory at the given path
slist_t *directory_list_path(const char *path) {
    int inum = tree_lookup(path);
    if (inum == -1) { // non-existent directory
      // behavior identical for non-existent path and empty directory
      return NULL;
    }
    inode_t *dd = get_inode(inum);
    return directory_list(dd);
}

// return the inum of a directory that is at the given block
int get_inum_from_block(void * block) {
  dirhead_t *dir = (dirhead_t *) block;
  int end_index = dir->num_entries;
  direntry_t *entries_start = (direntry_t *) (dir + 1);
  for (int i = 0; i < end_index; i++) {
    direntry_t *entry = entries_start + i;
    if (entry->present == 1) {
      if (strcmp(entry->name, ".") == 0) {
        return entry->inum;
      }
    }
    else if (entry->present == 0) {
      end_index++;
    }
  }
  return -1;
}

// print the files in the given directory
void print_directory(inode_t *dd) {
  slist_t *list = directory_list(dd);
  slist_t *curr = list;
  while (curr != NULL) {
    printf("%s\n", curr->data);
  }
}

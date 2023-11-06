// Inode manipulation routines.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H
#define DIRECTORY_MODE 040755 
#define FILE_MODE 0100644
#define INODE_COUNT 128
#define INODE_BLOCK 1

#include <sys/types.h>
#include "blocks.h"


typedef struct inode {
  u_int16_t mode;       // permission & type
	u_int16_t ref_count;  // Number of references to the data refered to by this inode
	u_int32_t size;       // Size of Data 
	u_int32_t block;  
	u_int32_t iblock;     // Used for file splitting - 
	u_int32_t iiblock;
  u_int16_t padding[6]; // Pad inode struct size to 32 bytes
} inode_t;

#define INODE_SIZE sizeof(inode_t)

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode(mode_t mode);
void free_inode(int inum);
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);

#endif

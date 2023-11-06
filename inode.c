#include <assert.h>

#include "inode.h"
#include "blocks.h"
#include "bitmap.h"


// Print all recorded info about a given inode
void print_inode(inode_t *node)
{
  printf("Inode: \n\n"
         "mode: %d\n"
         "ref_count: %d\n"
         "size: %d\n"
         "blocks: %d\n"
         "iblock: %d\n"
         "iiblock: %d\n",
         node->mode,
         node->ref_count,
         node->size,
         node->block,
         node->iblock,
         node->iiblock);
}

// return the inode of the given inum
inode_t *get_inode(int inum)
{
  inode_t *inode_block = (inode_t *) blocks_get_block(INODE_BLOCK);
  inode_t *inode = inode_block + inum;
  return inode;
}

// Allocate a new inode with a given mode. Return associated inum or -1 on error
int alloc_inode(mode_t mode)
{
  void *ibm = get_inode_bitmap();

  for (int ii = 0; ii < INODE_COUNT; ++ii)
  {
    if (!bitmap_get(ibm, ii))
    {
      bitmap_put(ibm, ii, 1);
      printf("+ alloc_inode() -> %d\n", ii);

      inode_t *inode = get_inode(ii);
      inode->mode = mode;
      inode->ref_count = 0;
      inode->size = 0;
      inode->block = alloc_block();
      // 0 means no storage block since it is the bitmap
      inode->iblock = 0;
      inode->iiblock = 0;
      return ii;
    }
  }

  return -1;
}

// free the inode of the given inum
void free_inode(int inum)
{
  inode_t *inode = get_inode(inum);
  // assumption about no Extra Credit (max file size 4k)
  assert(inode->iblock == 0);
  assert(inode->iiblock == 0);
  free_block(inode->block);
}

// grow the size of the given inode by the given amount
int grow_inode(inode_t *node, int size)
{
  // no Extra Credit, can't grow inode above one block 
  assert(node->size + size <= BLOCK_SIZE);
  node->size += size;
  return node->size;
}

// shrink the size of the given inode by the given amount
int shrink_inode(inode_t *node, int size)
{
  if (node->size - size >= 0)
  {
    node->size -= size;
  }
  else
  {
    node->size = 0;
  }
  return node->size;
}

/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "tnwb",
    /* First member's full name */
    "jqx",
    /* First member's email address */
    "258101143@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 基本常量和宏定义 */

#define WSIZE 4 /* 字和头部/脚部大小 (4 bytes) */
#define DSIZE 8 /* 双字宽度 (8 bytes) */
#define CHUNKSIZE (1<<12) /* 扩展堆的默认大小 (4096 bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* 设置头部和脚部的值, 块大小+分配位 */
#define PACK(size, alloc) ((size) | (alloc))

/* 读写指针p的位置 */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* 从地址p获取大小或分配位*/
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* 给定有效载荷指针, 找到头部和脚部 */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 给定有效载荷指针, 找到前一块或下一块 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);

static char *heap_listp;
static char *pre_listp;

/*
 * 扩展heap, 传入的是字节数
*/
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* 根据传入字节数奇偶, 考虑对齐 */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* 初始化，序言块头部和脚部均设置为空闲，还有结尾块 */
    PUT(HDRP(bp), PACK(size, 0)); /* 填充序言块头部 */
    PUT(FTRP(bp), PACK(size, 0)); /* 填充序言块脚部 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* 填充结尾块 */

    /* 判断相邻块是否是空闲块, 进行合并 */
    return coalesce(bp);
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 创建初始化空堆 */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
	
    PUT(heap_listp, 0); /* 对齐 */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* 填充序言块 */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* 填充序言块 */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* 结尾块 */
    heap_listp += (2*WSIZE);
    pre_listp = heap_listp;
	
    /* 扩展空闲空间 */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}


static void *find_fit(size_t asize)
{
    char *bp = heap_listp;
    size_t alloc;
    size_t size;
    while (GET_SIZE(HDRP(NEXT_BLKP(bp))) > 0) {
        bp = NEXT_BLKP(bp);
        alloc = GET_ALLOC(HDRP(bp));
        if (alloc) continue;
        size = GET_SIZE(HDRP(bp));
        if (size < asize) continue;
        return bp;
    } 
    return NULL;
}


static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    
    if ((size - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        PUT(HDRP(NEXT_BLKP(bp)),PACK(size - asize,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size - asize,0));
    } else {
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
    }
    pre_listp = bp;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; /* 对齐后的块大小 */
    size_t extendsize; /* 找不到则需要扩展堆的大小 */
    char *bp;
    
    /* 过滤不合法的请求 */
    if (size == 0)
        return NULL;
    
    /* 调整块大小以包括开销和对齐要求 */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    
    /* 在空闲列表中搜索适合的 */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* 找不到则扩展堆  */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
	
    //int isPre = (pre_listp == bp);
    if (prev_alloc && next_alloc) { // Case 1 
        pre_listp = bp;
        return bp;
    }

    if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        
    }

    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    pre_listp = bp;
    return bp;
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
       return mm_malloc(size);
    if (size == 0) 
       mm_free(ptr);

    void *newptr;
    size_t copySize;
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    size = GET_SIZE(HDRP(ptr));
    copySize = GET_SIZE(HDRP(newptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, ptr, copySize - WSIZE);
    mm_free(ptr);
    return newptr;
}
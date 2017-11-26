//
//  malloc.c
//  allocator
//
//  Created by 张雪遥 on 25/11/2017.
//  Copyright © 2017 张雪遥. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
//#include <sys/types.h>

#define BLOCK_SIZE 24       //由于存在虚拟的data字段，sizeof不能正确计算meta长度，这里手工设置

void *first_block = NULL;       //链表的表头

typedef struct s_block *t_block;
struct s_block{
    size_t size;        //数据区大小
    t_block next;       //指向下个块的指针
    int free;       //是否是空闲块，1表示空闲，0表示占用
    int padding;        //填充4字节，保证meta块长度为8的倍数
    char data[1];       //虚拟字段，表示数据块的第一个字段，长度不应计入meta
};

/* First fit */
t_block find_block(t_block *last, size_t size){
    t_block b = first_block;
    while (b && !(b->free && b->size >= size)) {
        *last = b;
        b = b->next;
    }
    return b;
}

/* 开辟新的堆栈 */
t_block extend_heap(t_block last, size_t s){
    t_block b;
    b = sbrk(0);
    
    if (sbrk(BLOCK_SIZE + s) == (void *)-1) {
        return NULL;
    }
    
    b->size = s;
    b->next = NULL;
    
    if(last){
        last->next = b;
    }
    b->free = 0;
    return b;
}

/* 分裂block */
void split_block(t_block b, size_t s){
    t_block new;
    new = b->data + s;      //-----------
    new->size = b->size - s - BLOCK_SIZE;
    new->next = b->next;
    new->free = 1;
    b->size = s;
    b->next = new;
}

/* 使分配区按照8字节对齐 */
size_t align8(size_t s){
    if ((s & 7) == 0) {
        return s;
    }
    return ((s>>3) + 1)<<3;
}

/* malloc */
void *malloc(size_t size){
    t_block b;
    t_block last;
    size_t s;
    
    s = align8(size);       //对齐地址
    
    if (first_block) {
        last = first_block;
        b = find_block(&last, s);
        
        if (b) {
            /* 判断分裂条件 */
            if ((b->size - s) >= (BLOCK_SIZE + 8)) {
                split_block(b, s);
            }
            b->free = 0;
        }else{
            /* 没有合适的block则开辟一个新的 */
            b = extend_heap(last, s);
            if (!b) {
                return NULL;
            }
        }
    }else{
        b = extend_heap(last, s);
        if (!b) {
            return NULL;
        }
        first_block = b;
    }
    
    return b->data;
}

/* calloc */





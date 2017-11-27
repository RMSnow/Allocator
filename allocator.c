//
//  malloc.c
//  allocator
//
//  Created by 张雪遥 on 25/11/2017.
//  Copyright © 2017 张雪遥. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>

#define BLOCK_SIZE 40       //由于存在虚拟的data字段，不能直接通过sizeof获取

void *first_block = NULL;       //链表的表头

typedef struct s_block *t_block;

//struct s_block{
//    size_t size;        //数据区大小
//    t_block next;       //指向下个块的指针
//    int free;       //是否是空闲块，1表示空闲，0表示占用
//    int padding;        //填充4字节，保证meta块长度为8的倍数
//    char data[1];       //虚拟字段，表示数据块的第一个字段，长度不应计入meta
//};

/* 增加magic pointer */
//struct s_block{
//    size_t size;        //数据区大小
//    t_block next;       //指向下个块的指针
//    int free;       //是否是空闲块，1表示空闲，0表示占用
//    int padding;        //填充4字节，保证meta块长度为8的倍数
//    void *ptr;      //Magic pointer，指向data
//    char data[1];       //虚拟字段，表示数据块的第一个字段，长度不应计入meta
//};

/* 改为双向链表 */
struct s_block{
    size_t size;        //数据区大小
    t_block prev;       //指向上个块的指针
    t_block next;       //指向下个块的指针
    int free;       //是否是空闲块，1表示空闲，0表示占用
    int padding;        //填充4字节，保证meta块长度为8的倍数
    void *ptr;      //Magic pointer，指向data
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
void *calloc(size_t number, size_t size){
    size_t *new;        //提高效率，可以将每8个字节设置为0
    size_t s8, i;
    
    new = malloc(number * size);
    if(new){
        s8 = align8(number * size) >> 3;
        for (i = 0; i < s8; i++) {
            new[i] = 0;
        }
    }
    return new;
}

/* 检查地址合法性 */
t_block get_block(void *p){
    char *tmp = p;
    return (p = tmp -= BLOCK_SIZE);
}

int valid_addr(void *p){
    if (first_block) {
        if(p > first_block && p < sbrk(0)){
            return p == (get_block(p))->ptr;
        }
    }
    return 0;
}

/* 合并相邻块方法 */
t_block fusion(t_block b){
    if(b->next && b->next->free){
        b->size += BLOCK_SIZE + b->next->size;
        b->next = b->next->next;
        if (b->next) {
            b->next->prev = b;
        }
    }
    return b;
}

/* free */
void free(void *p){
    t_block b;
    if(valid_addr(p)){      //检查参数地址合法性
        b = get_block(p);
        b->free = 1;
        
        if (b->prev && b->prev->free) {
            b = fusion(b->prev);
        }
        
        if (b->next) {
            fusion(b);
        }else{      //若当前是最后一个block
            if (b->prev) {
                b->prev->prev = NULL;       //----------------
            }else{
                first_block = NULL;
            }
            brk(b);     //回退break指针释放进程内存
        }
    }
}

/* 内存复制方法 */
void copy_block(t_block src, t_block dst){
    size_t *sdata, *ddata;      //以8字节为单位复制
    size_t i;
    
    sdata = src->ptr;
    ddata = dst->ptr;
    
    for(i = 0; (i * 8) < src->size && (i * 8) < dst->size; i++){
        ddata[i] = sdata[i];
    }
}

/* realloc */
void *realloc(void *p, size_t size){
    size_t s;
    t_block b, new;
    void *newp;
    
    if(!p){
        return malloc(size);        //当p为NULL时，调用malloc
    }
    
    if (valid_addr(p)) {
        s = align8(size);
        b = get_block(p);
        
        if (b->size >= s) {     //当新的size变小时，考虑split
            if(b->size - s >= (BLOCK_SIZE + 8)){
                split_block(b, s);
            }
        }else{      //若与后继block合并后满足size条件
            if (b->next && b->next->free && (b->size + BLOCK_SIZE + b->next->size) >= s) {
                fusion(b);
                
                if (b->size -s >= (BLOCK_SIZE + 8)) {       //考虑split
                    split_block(b, s);
                }
            }else{
                newp = malloc(s);
                if (!newp) {
                    return NULL;
                }
                new = get_block(newp);
                copy_block(b, new);
                free(p);
                return newp;
            }
        }
        return p;
    }
    
    return NULL;
}

//
//  main.c
//  allocator
//
//  Created by 张雪遥 on 25/11/2017.
//  Copyright © 2017 张雪遥. All rights reserved.
//

#include <stdio.h>

typedef struct s_block *t_block;

struct s_block{
    size_t size;        //数据区大小
    t_block next;       //指向下个块的指针
    int free;       //是否是空闲块，1表示空闲，0表示占用
    int padding;        //填充4字节，保证meta块长度为8的倍数
    char data[1];       //虚拟字段，表示数据块的第一个字段，长度不应计入meta
};

int main(int argc, const char * argv[]) {
    size_t a;
    t_block b;
    char carray[1] = "c";
    char *c;
    int i;
    
    printf("sizeof(a) = %d\n", sizeof(a));
    printf("sizeof(b) = %d\n", sizeof(b));
    printf("sizeof(carray) = %d, carray = %c\n", sizeof(carray), carray);
    printf("sizeof(c) = %d\n", sizeof(c));
    printf("sizeof(*c) = %d\n", sizeof(*c));
    printf("sizeof(i) = %d\n", sizeof(i));


    printf("\nsizeof(s_block) = %d\n", sizeof(struct s_block));
    
    return 0;
}

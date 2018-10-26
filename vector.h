#include <stdio.h>

struct vec
{
    unsigned int size;
    unsigned int capacity;
    unsigned char *data;
};

void vector_alloc(struct vec *);
void vector_delete(struct vec *);
void vector_push_back(struct vec *, char *, int);
void vector_jmp_update(struct vec *, unsigned int, int);

#include <stdio.h>
#define CAPACITY 200

struct stack 
{
    int size_of_elements;
    int elements[CAPACITY];
};

void stack_push(struct stack *, int);
void stack_pop(struct stack *, int *);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct vec
{
    unsigned int size;  // index
    unsigned int capacity;
    char *data;
};

void vector_alloc(struct vec *vector)
{
    vector->size = 0;
    vector->capacity = 100;
    vector->data = (char *)malloc(vector->capacity * sizeof(char));

    assert(vector->data != NULL);
}

void vector_delete(struct vec *vector)
{
    assert(vector != NULL || vector->data != NULL);

    vector->size = 0;
    vector->capacity = 0;
    free(vector->data);
}

void vector_push_back(struct vec *vector, char *bytes, int size_of_bytes)
{
    if (vector->size + size_of_bytes > vector->capacity) 
    {
        vector->capacity *= 2;
        vector->data = (char *)realloc(vector->data, vector->capacity * sizeof(char));
        
        assert(vector->data != NULL);
    }
   
    memcpy(vector->data + vector->size, bytes, size_of_bytes);
    vector->size += size_of_bytes;
}

void vector_jmp_update(struct vec *vector, unsigned int instruction_offset, int jmp_offset)
{
    vector->data[instruction_offset + 3] = (jmp_offset & 0xFF000000) >> 24;
    vector->data[instruction_offset + 2] = (jmp_offset & 0x00FF0000) >> 16;
    vector->data[instruction_offset + 1] = (jmp_offset & 0x0000FF00) >> 8;
    vector->data[instruction_offset] = (jmp_offset & 0x000000FF);
}

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include "vector.h"
#include "stack.h"

struct bf_state
{
    int source_ptr;
    uint8_t *memory_segment;
    size_t memory_size;
};

size_t get_file_size(FILE *file_with_bf_code)
{
    fseek(file_with_bf_code, 0, SEEK_END);
    return ftell(file_with_bf_code);
}

struct bf_state bf_init(size_t memory_size)
{
    struct bf_state result = 
    { 
        0, (uint8_t *)malloc(memory_size), memory_size
    };
    return result;
}

void jit(struct bf_state *state, char *source)
{
    assert(state != NULL && source != NULL);
    struct vec op_instructions;
    struct stack relocation_table = { .size_of_elements = 0, .elements = {0} }; // relocation_table from     asm language
    int relocation_site = 0, relative_offset = 0;
 
    vector_alloc(&op_instructions);

    char jit_prologue[] = 
    {
        0x55,   // push rbp
        0x48, 0x89, 0xe5,   // mov rbp, rsp
        0x50,   // push rax
        0x51,   // push rcx
        0x52,   // push rdx
        0x48, 0x89, 0xf8,   // mov rax, rdi
        0x48, 0x8b, 0x48, 0x08, // mov rcx, [rax+8]
    };
    vector_push_back(&op_instructions, jit_prologue, sizeof(jit_prologue));
    
    while (source[state->source_ptr] != '\0')
    {
        switch (source[state->source_ptr])
        {
            case '>': 
            {
                char jit_inc_ptr[] = 
                {
                    0x48, 0xff, 0xc1    // inc rcx
                }; 
                vector_push_back(&op_instructions, jit_inc_ptr, sizeof(jit_inc_ptr));       
            }
            break;
            case '<':
            {
                char jit_dec_ptr[] = 
                {
                    0x48, 0xff, 0xc9 // dec rcx
                };
                vector_push_back(&op_instructions, jit_dec_ptr, sizeof(jit_dec_ptr));
            }
            break;
            case '+':
            {
                char jit_inc_value[] = 
                {
                    0xfe, 0x01 // inc byte [rcx]
                };
                vector_push_back(&op_instructions, jit_inc_value, sizeof(jit_inc_value));
            }
            break;
            case '-':
            {
                char jit_dec_value[] = 
                {
                    0xfe, 0x09 // dec byte [rcx]
                };
                vector_push_back(&op_instructions, jit_dec_value, sizeof(jit_dec_value));
            }
            break;  
            case '.':
            {
                char jit_putchar[] = 
                {
                    // using linux syscall write
                    0xba, 0x01, 0x00, 0x00, 0x00, // mov rdx, 1
                    0xbb, 0x01, 0x00, 0x00, 0x00, // mov rbx, 1
                    0xb8, 0x04, 0x00, 0x00, 0x00, // mov rax, 4
                    0xcd, 0x80, // int 0x80
                };
                vector_push_back(&op_instructions, jit_putchar, sizeof(jit_putchar));
            }
            break;
            case ',':
            {
                char jit_getchar[] = 
                {
                    // using linux syscall write
                    0xba, 0x01, 0x00, 0x00, 0x00, // mov edx, 1
                    0xbb, 0x00, 0x00, 0x00, 0x00, // mov ebx, 0
                    0xb8, 0x03, 0x00, 0x00, 0x00, // mov eax, 4
                    0xcd, 0x80, // int 0x80
                };
                vector_push_back(&op_instructions, jit_getchar, sizeof(jit_getchar));
            }
            break;  
            case '[': 
            {
                char jit_start_loop[] = 
                {
                    0x80, 0x39, 0x00, // cmp byte [rcx], 0
                    0x0F, 0x84, 0x00, 0x00, 0x00, 0x00 // je <relative offset>
                };
                vector_push_back(&op_instructions, jit_start_loop, sizeof(jit_start_loop));
            }
            stack_push(&relocation_table, op_instructions.size);
            break;
            case ']': 
            {
                char jit_end_loop[] = 
                {
                    0x80, 0x39, 0x00, // cmp byte [rcx], 0
                    0x0F, 0x85, 0x00, 0x00, 0x00, 0x00 // jne <relative offset>
                };
                vector_push_back(&op_instructions, jit_end_loop, sizeof(jit_end_loop));
            }
            stack_pop(&relocation_table, &relocation_site);
            relative_offset = op_instructions.size - relocation_site;
            vector_jmp_update(&op_instructions, op_instructions.size - 4, -relative_offset);
            vector_jmp_update(&op_instructions, relocation_site - 4, relative_offset);
            break; 
          }
           state->source_ptr++;
        }

    char jit_epilogue[] =
    {
        0x5a, // pop rdx
        0x59, // pop rcx
        0x58, // pop rax
        0x5d, // pop rbp
        0xc3  // ret
    };
    vector_push_back(&op_instructions, jit_epilogue, sizeof(jit_epilogue));

    void *jit_mem = mmap(NULL, op_instructions.size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(jit_mem, op_instructions.data, op_instructions.size);
    void (*execute_func)(struct bf_state *) = jit_mem;
 
    execute_func(state);
    munmap(jit_mem, op_instructions.size);
    vector_delete(&op_instructions);
} 

int get_number_of_instructions(char *source)
{
    const char bf_alphabet[] = "><+-,.[]";
    int number_of_bf_instr = 0;
    
    for (unsigned int i = 0; i < strlen(source); i++)
    {
        char *is_bf_instruction = memchr(bf_alphabet, source[i], sizeof(bf_alphabet));
        if (is_bf_instruction != NULL)
            number_of_bf_instr++;
    }

    return number_of_bf_instr;
}

int main(int argc, char **argv)
{
    if (argc != 2) 
    {
        fprintf(stderr, "File not specified.\n");
        puts("usage: ./bf_interpreter <filename.bf>");
        return EX_USAGE;
    }

    const char *filename = argv[1];
    FILE *file_with_bf_code = fopen(filename, "r");

    if (file_with_bf_code == NULL) 
    {
        perror(filename);
        return EX_NOINPUT;
    }

    const int memory_size = 30000;

    size_t file_size = get_file_size(file_with_bf_code);
    fseek(file_with_bf_code, 0, SEEK_SET);
    
    char *source = (char *)malloc(file_size);
    assert(source != NULL);

    size_t read_elements = fread(source, 1, file_size, file_with_bf_code);
    if (!read_elements) 
    {
        fprintf(stderr, "Brainfuck code not found.\n");
        fclose(file_with_bf_code);
        return EX_DATAERR;
    }
    
    int number_of_bf_instructions = get_number_of_instructions(source);
    if (number_of_bf_instructions > memory_size) 
    {
        fprintf(stderr, "You cannot read more than %d instructions.", memory_size);
        fclose(file_with_bf_code);
        return EX_DATAERR;
    }
    
    fclose(file_with_bf_code);

    struct bf_state values = bf_init(memory_size);
    jit(&values, source);
    free(source);
    
    return 0;
}

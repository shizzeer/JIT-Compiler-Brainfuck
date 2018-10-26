#include "stack.h"
#include <assert.h>

void stack_push(struct stack *stack_state, int element)
{
	assert(stack_state->size_of_elements < CAPACITY);
	
	stack_state->elements[stack_state->size_of_elements] = element;
	stack_state->size_of_elements++;
}

void stack_pop(struct stack *stack_state, int *relocation_site)
{
	assert(stack_state->size_of_elements > 0);

	*relocation_site = stack_state->elements[--stack_state->size_of_elements];
}

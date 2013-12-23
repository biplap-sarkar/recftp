/*  
    Implementation of stack.
    Copyright (C) 2011  Biplap Sarkar

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

struct stack *createstack()
{
  struct stack *newstack = (struct stack *)malloc(sizeof(struct stack)); 
  memset(newstack, 0, sizeof(newstack));
  newstack->top==NULL;
  return newstack;
}

void deletestack(struct stack *stk){
  free(stk);
}

void push(struct stack *stk, char *data)
{
  struct node *newnode = (struct node *)malloc(sizeof(struct node));
  char *val = (char *)malloc(sizeof(char)*(100));
  memset(val,0,strlen(data));
  strcpy(val,data);
  newnode->value = val;
  newnode->next = stk->top;
  stk->top = newnode;
}
char* pop(struct stack *stk)
{
  static char val[1024];
  char *ptr;
  if(stk->top == NULL){
    printf("top pointing to NULL\n");
    return NULL;
  }
  struct node *topnode = stk->top;
  ptr = stk->top->value;
  if(ptr == NULL)
    return NULL;
  memset(val, 0, 1024);
  strcpy(val,ptr);
  stk->top = stk->top->next;
  free(ptr);
  free(topnode);
  return val;
}

/*  
    Header file for stack.
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

struct stack{
  struct node *top;
};

struct node{
  void *value;
  struct node *next;
};
struct stack *createstack();
void deletestack(struct stack *stk);
void push(struct stack *stk, char *data);
char * pop(struct stack *stk);

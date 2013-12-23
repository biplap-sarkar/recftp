/*  
    Implementation of terminal utilites.
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

#include <termios.h>

static struct termios save;

void echoOff()
{
  struct termios term;
  
  tcgetattr(0, &save);
  term = save;
  term.c_lflag &= (~ECHO);
  tcsetattr(0, TCSANOW, &term);
}

void echoOn()
{
  tcsetattr(0, TCSANOW, &save);
}

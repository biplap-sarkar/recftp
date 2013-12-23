/*  
    Main file.
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
#include <errno.h>
#include <string.h>
#include "recftp.h"
int main(int argc, char **argv)
{
  char port[10];
  char user[50],passwd[50];
  int res;
  if(argc == 3)
    strcpy(port, argv[2]);
  else if(argc == 2)
    strcpy(port, "21");
  else{
    printf("Usage: recftp server port\n");
    return 0;
  }
  if(ftp_connect(argv[1], port) > -1)
    ftp_login(argv[1], port);
  serve();
  /*
  printf("Username: ");
  scanf("%s",user);
  printf("Password: ");
  scanf("%s",passwd);
  if(ftp_login(argv[1], port, user, passwd)==-1){
    printf("\nerror: %d\n",errno);
  }
  */
  return 0;
}

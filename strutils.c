/*  
    Implementation of string utilites.
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

#include <stdlib.h>
#include <string.h>

void buildparam(char **bparam, int bsize, char **param, int size, char esc)
{
  int i,j=0,len;
  for(i=0;i<bsize;i++){
    if(j==(size-1))
	break;
    len = strlen(param[j]);
    if(len > 0)
	if(param[j][len-1]==esc)
	  param[j][len-1] = ' ';
      else
	  j++;
    strcat(param[j],bparam[i]);
  }
  int k;
  for(k=i;k<bsize;k++){
    if(strlen(param[j]) > 0)
  	strcat(param[j]," ");
    strcat(param[j],bparam[k]);
  }
}


int splitstr(char ***strs, char *str, char esc)
{
  int strln = strlen(str);
  int strcnt = 0;
  int idx;
  char *ptr = str;
  for(idx=0;idx<=strln;idx++){
    if(idx%20 == 0)
      *strs = (char **)realloc(*strs, sizeof(char *)*(strcnt+20));
    
    if(*ptr == esc){
      if(idx < strln){
	ptr = &str[idx+1];
	continue;
      }
      else
	break;
    }
  
    if(str[idx]==esc || str[idx]==0){
      str[idx]=0;
      if(*ptr != esc)
	*(*strs+strcnt++)=ptr;
      if(idx < strln)
	ptr = &str[idx+1];
      else
	break;
    }
  }
  return strcnt;
}

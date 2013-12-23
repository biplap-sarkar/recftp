/*  
    Implementation of commands provided by recftp.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <dirent.h>
#include "termutils.h"
#include "strutils.h"
#include "recftp.h"
#include "stack.h"
#include "mathutils.h"

char hostname[100];
int debug=1;
int retries = 3;
static int serv_sd;
int serve();
static issuecmd(int, char**, int);
static int pasvport();
static int socketconnect(const char *, const char *);
static void parseresponse(char *resp, char **prelim, char **final);
int isresponsecomplete(char *resp);

int ftp_connect(const char *ip, const char *port)
{
  int ret;
  char *buff = (char *)malloc(sizeof(char)*DATABUFFSIZE);

  strcpy(hostname,ip);

  serv_sd = socketconnect(ip,port);
  if(serv_sd == -1){
    free(buff);
    return -1;
  }
  printf("Connected with %s at %s\n",ip,port);
  do{
    ret = recv(serv_sd, buff, DATABUFFSIZE, 0);
    if(ret > 0)
      printf(buff);
  }while(!isresponsecomplete(buff));
  free(buff);
}

static int socketconnect(const char *host, const char *port){
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sockd;
  int ret;

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;
  
  int res = getaddrinfo(host, port, &hints, &result);
  if(res){
    printf("Host not found\n");
    return -1;
  }
  
  for(rp = result; rp != NULL; rp = rp->ai_next){
    sockd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if(sockd == -1)
      continue;
    if(connect(sockd, rp->ai_addr, rp->ai_addrlen) != -1){
      ret = sockd;
      break;
    }
    close(sockd);
  }
  if(rp == NULL){
    printf("Error connecting the socket\n");
    ret = -1;
    goto exit;
  }
 exit:
  freeaddrinfo(result);
  return ret;
}


int ftp_login(const char *ip, const char *port)
{
  struct pollfd loginpfd[2];
  char *sendbuff = (char *)malloc(sizeof(char) *  COMBUFFSIZE);
  char *recvbuff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char *buff = (char *)malloc(sizeof(char)*DATABUFFSIZE);
  bzero(buff, DATABUFFSIZE);
  bzero(sendbuff, COMBUFFSIZE);
  bzero(recvbuff, COMBUFFSIZE); 
  printf("Username for ftp@%s: ",ip);
  fflush(stdout);
  strcat(buff, "USER ");
  fgets(buff+strlen("USER "),DATABUFFSIZE,stdin);
  int ret = send(serv_sd, buff, strlen(buff), 0);
  if(ret == -1){
    if(errno == ENOTCONN || ECONNRESET){
      free(sendbuff);
      free(recvbuff);
      free(buff);
      ftp_connect(ip,port);
      return(ftp_login(ip,port));
    }
    else{
      goto outside;
    }
  }
  do{
     bzero(buff, DATABUFFSIZE);
    ret = recv(serv_sd, buff, DATABUFFSIZE, 0);
    printf(buff);
  }while(!isresponsecomplete(buff));
  bzero(buff, DATABUFFSIZE);
  strcat(buff, "PASS ");
  
  printf("Password: ");
  fflush(stdout);
  echoOff();
  fgets(buff+strlen("PASS "),DATABUFFSIZE,stdin);
  echoOn();

  ret = send(serv_sd, buff, strlen(buff), 0);
  if(ret == -1){
    if(errno == ENOTCONN || ECONNRESET){
      free(sendbuff);
      free(recvbuff);
      ftp_connect(ip,port);
      return (ftp_login(ip,port));
    }
    else{
      goto outside;
    }
  }
  do{
     bzero(buff, DATABUFFSIZE);
    ret = recv(serv_sd, buff, DATABUFFSIZE, 0);
    printf(buff);
  }while(!isresponsecomplete(buff));
  
  
 outside:
  free(buff);
  free(recvbuff);
  free(sendbuff);
  return ret;
}

int isresponsecomplete(char *resp)
{
  int res=0;
  int probe = 1;
  int len = strlen(resp);
  int idx;
  int endidx = len-2;
  if(resp[len-1]!='\n')
    res = 0;
  else{
    while(probe){
      for(idx=endidx;resp[idx]!='\n' && idx > 0;idx--)
	continue;
      if(resp[idx]=='\n'){
	endidx = idx-1;
	idx++;
      }
      else 
	probe=0;
      if(isnumber(resp[idx]) && isnumber(resp[idx+1]) && isnumber(resp[idx+2])){
	if(resp[idx+3]==' ')
	  res = 1;
	probe=0;
      }
    }
  }
  return res;
}

int serve(){
  char request[COMBUFFSIZE];
  char **tokens;
  char *ptr;
  int cmd = -1;
  int cnt,i;
  char delim = ' ';
  while(cmd != QUIT){
    tokens=NULL;
    printf("ftp>");
    fflush(stdout);
    bzero(request, COMBUFFSIZE);
    fgets(request, COMBUFFSIZE-1, stdin);
    i=strlen(request);
    request[i-1]=0;
    if(strcmp(request,"")==0)
      continue;
    cnt=splitstr(&tokens,request,delim);
    ptr = *tokens;
   
    if(strcmp(ptr,"cd")==0)
      cmd = CD;
    else if(strcmp(ptr,"ls")==0)
      cmd = LS;
    else if(strcmp(ptr,"dir")==0)
      cmd = LS;
    else if(strcmp(ptr,"pwd")==0)
      cmd = PWD;
    else if(strcmp(ptr,"get")==0)
      cmd = GET;
    else if(strcmp(ptr,"rget")==0)
      cmd = RGET;
    else if(strcmp(ptr,"put")==0)
      cmd = PUT;
    else if(strcmp(ptr,"rput")==0)
      cmd = RPUT;
    else if(strcmp(ptr,"quit")==0)
      cmd = QUIT;
    else if(strcmp(ptr,"open")==0)
      cmd = OPEN;
    else if(strcmp(ptr,"help")==0)
      cmd = HELP;
    else
      cmd = -1;
    issuecmd(cmd, tokens, cnt);
    free(tokens);
  }
}

						 

static int pasvport(){
  char buff[100];
  char *saveptr = (char *)malloc(sizeof(char)*100);
  memset(saveptr,0,100);
  char *ptr;
  char **ret=NULL;
  int cnt;
  char delim = ' ';
  int port = -1;
  strcpy(buff,"PASV\n");
  send(serv_sd,buff,strlen(buff),0);
  bzero(buff,100);
  recv(serv_sd,buff,100,0);
  if(debug)
    printf("%s",buff);
  cnt = splitstr(&ret,buff,delim);
  strcpy(saveptr,ret[cnt-1]);
  free(ret);
  ret = NULL;
  cnt = splitstr(&ret,saveptr,',');
  int len = strlen(ret[cnt-1]);
  ret[cnt-1][len-3]=0;
  port = atoi(ret[4])*256 + atoi(ret[5]);
  free(saveptr);
  free(ret);
  return port;
}



static int ftp_chtype(int type)
{
  int res = -1;
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  memset(buff,0,COMBUFFSIZE);
  switch(type){
  case BINARY:{
    strcat(buff,"TYPE I\n");
    break;
  }
  case ASCII:{
    strcat(buff,"TYPE A\n");
    break;
  }
  }
  if(strlen(buff)>0){
    send(serv_sd, buff, strlen(buff), 0);
    recv(serv_sd, buff, COMBUFFSIZE, 0);
    if(debug)
      printf("%s",buff);
    res = 0;
  } 
  free(buff);
  return res;
}

static int ftp_putdir(char *path, char *rempath)
{
  struct stack *dirstack = createstack();
  DIR *dirp;
  struct dirent *dent;
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char *ptr;
  memset(buff,0,COMBUFFSIZE);
  chdir(path);
  dirp = opendir("./");
  if(dirp == NULL){
    printf("error %d while opening directory %s\n",errno,path);
    return -1;
  }
  strcat(buff,"MKD ");
  strcat(buff,rempath);
  strcat(buff,"\n");
  send(serv_sd, buff, strlen(buff), 0);
  memset(buff, 0, COMBUFFSIZE);
  recv(serv_sd, buff, COMBUFFSIZE, 0);
  if(debug)
    printf("%s\n",buff);
  memset(buff, 0, COMBUFFSIZE);
  ftp_chdir(rempath);
  while(dent = readdir(dirp)){
    if(dent->d_type == DT_DIR){
      if((strcmp(dent->d_name,".")==0) || (strcmp(dent->d_name,"..")==0))
	continue;
      push(dirstack, dent->d_name);
    }
    else if(dent->d_type == DT_REG){
      ftp_putfile(dent->d_name, dent->d_name);
    }
    else
      continue;
  }
  free(buff);
  while(ptr=pop(dirstack))
    ftp_putdir(ptr,ptr);
  deletestack(dirstack);
  closedir(dirp);
  chdir("../");
  ftp_chdir("../");
  return 0;
}

static int ftp_getdir(char *path, char *localpath)
{
  struct stack *dirstack = createstack();
  struct stack *filestack = createstack();
  char *sbuff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char *residue = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE*2);
  memset(sbuff,0,COMBUFFSIZE);
  memset(residue,0,COMBUFFSIZE);
  memset(buff,0,COMBUFFSIZE*2);
  ftp_chdir(path);
  int port = pasvport();
  if(debug)
    printf("port received: %d\n",port);
  send(serv_sd,"LIST\n",strlen("LIST\n"),0);
  char sport[10];
  sprintf(sport,"%d",port);
  int i,j;
  int sockd;
  for(i=0;i<retries;i++){
    sockd = socketconnect(hostname,sport);
    if(sockd > 0)
      break;
  }
  if(i==retries && sockd == -1)
    return -1;
  int bufflen;
  int entrycnt;
  int dirnamelen,filenamelen;
  char *ptr;
  char **direntries=NULL;
  char **entryattr=NULL;
  char *entryname = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char delim = '\n';
  char attrdelim = ' ';
  int attrcnt;
  mkdir(localpath,S_IXOTH|S_IXGRP|S_IRWXU);
  chdir(localpath);
  int res=1;
  
  while(res){
    memset(sbuff, 0, COMBUFFSIZE);
    res = recv(sockd, sbuff, COMBUFFSIZE-1, 0);
    strcat(buff, residue);
    memset(residue, 0, COMBUFFSIZE);
    strcat(buff, sbuff);
    bufflen = strlen(buff);
    if(bufflen>0){
      for(i=bufflen-1;buff[i]!='\n';i--)
	continue;
    }
    strcpy(residue,buff+i+1);
    buff[i+1]=0;
    //printf("%s",buff);
    if(direntries != NULL)
      free(direntries);
    direntries = NULL;
    entrycnt = splitstr(&direntries,buff,delim);
    for(i=0;i<entrycnt;i++){
      if(entryattr != NULL)
	free(entryattr);
      entryattr=NULL;
      attrcnt = splitstr(&entryattr,direntries[i],attrdelim);
      if(strcmp(entryattr[0],"total")==0)
	continue;
      memset(entryname,0,COMBUFFSIZE);
      if(attrcnt>=8){
	strcpy(entryname, entryattr[8]);
	for(j=9;j<attrcnt;j++){
	  strcat(entryname, " ");
	  strcat(entryname,entryattr[j]);
	}
      }
      if(strlen(entryname)>0){
	if(entryattr[0][0]=='d'){
	  dirnamelen = strlen(entryname);
	  entryname[dirnamelen-1]=0;
	  push(dirstack,entryname);
	}
	else{
	  filenamelen = strlen(entryname);
	  entryname[filenamelen-1]=0;
	  push(filestack,entryname);
	}
      }
    }
    memset(sbuff, 0, COMBUFFSIZE);
    memset(buff,0,COMBUFFSIZE*2);
  }
  recv(serv_sd,buff,COMBUFFSIZE,0);
  recv(serv_sd,buff,COMBUFFSIZE,0);
  while(ptr=pop(filestack)){
    printf("trying to get %s\n",ptr);
    ftp_getfile(ptr,ptr);
  }
  deletestack(filestack);
  free(entryname);
  free(entryattr);
  free(direntries);
  free(buff);
  free(sbuff);
  free(residue);
  while((ptr=pop(dirstack))!=NULL){
    ftp_getdir(ptr,ptr);
  }
  deletestack(dirstack);
  ftp_chdir("../");
  chdir("../");
  return 0;
}

static int ftp_list(char *path)
{
  int res = -1;
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  memset(buff,0,COMBUFFSIZE);
  int port = pasvport();
  strcat(buff,"LIST ");
  (strlen(path)==0)?strcat(buff,"./"):strcat(buff,path);
  strcat(buff,"\n");
  send(serv_sd, buff, strlen(buff), 0);
  getpasvdata(port, STDOUT_FILENO);
  free(buff);
}

static int ftp_pwd()
{
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  memset(buff,0,COMBUFFSIZE);
  strcat(buff, "PWD\n");
  send(serv_sd, buff, strlen(buff), 0);
  recv(serv_sd, buff, 1024, 0);
  printf("%s",buff);
  free(buff);
}

static int ftp_chdir(char *path)
{
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  memset(buff,0,COMBUFFSIZE);
  strcat(buff,"CWD ");
  strcat(buff,path);
  strcat(buff,"\n");
  send(serv_sd, buff, strlen(buff), 0);
  memset(buff,0,COMBUFFSIZE);
  recv(serv_sd, buff, COMBUFFSIZE, 0);
  printf("%s",buff);
  free(buff);
  return 0;
}

static int ftp_quit()
{
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  memset(buff,0,COMBUFFSIZE);
  strcat(buff,"QUIT\n");
  send(serv_sd, buff, strlen(buff), 0);
  recv(serv_sd, buff, COMBUFFSIZE, 0);
  printf("%s",buff);
  free(buff);
  return 0;
}

static int ftp_putfile(char *file, char *remfile)
{
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  memset(buff,0,COMBUFFSIZE);
  strcat(buff, "STOR ");
  strcat(buff, remfile);
  strcat(buff, "\n");
  int fd = open(file, O_RDONLY);
  if(fd == -1){
    if(errno == EACCES){
      printf("local file %s does not exist\n",file);
      return -1;
    }
    printf("error %d while opening local file %s\n",errno,file);
    return -1;
  }
  int port = pasvport();
  ftp_chtype(BINARY);
  send(serv_sd, buff, strlen(buff), 0);
  int res,i;
  for(i=0;i<retries;i++){
    res = putpasvdata(port, fd);
    if(res == -1)
      continue;
    else
      break;
  }
  ftp_chtype(ASCII);
  close(fd);
}
static int ftp_getfile(char *file, char *localfile)
{
  char *buff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char choice[4];
  memset(buff,0,COMBUFFSIZE);
  int fd = open(localfile, O_CREAT|O_EXCL|O_WRONLY, S_IXOTH|S_IXGRP|S_IRWXU);
  if(fd == -1){
    if(errno == EEXIST){
      printf("File already exists, overwrite? (y/n): ");
      fgets(choice,3,stdin);
      if(choice[0] == 'y'|| choice[0]=='Y')
	fd = open(file, O_CREAT|O_WRONLY|O_TRUNC, S_IXOTH|S_IXGRP|S_IRWXU);
      else{
	free(buff);
	return -1;
      }
    }
    else{
      printf("Error while creating the file %s\n",localfile);
      free(buff);
      return -1;
    }
  }
  int port = pasvport();
  printf("port received %d\n",port);
  ftp_chtype(BINARY);
  strcat(buff,"RETR ");
  strcat(buff,file);
  strcat(buff,"\n");
  send(serv_sd, buff, strlen(buff), 0);
  int res,i;
  for(i=0;i<retries;i++){
    res = getpasvdata(port, fd);
    if(res == -1)
      continue;
    else
      break;
  }
  close(fd);
  ftp_chtype(ASCII);
  free(buff);
  return 0;
}

static int ftp_open(char *hostname, int port)
{
  char sport[10];
  sprintf(sport,"%d", port);
  ftp_connect(hostname, sport);
  if(serv_sd > 0)
    ftp_login(hostname, sport);
  else
    return -1;
  return 0;
}

static int putpasvdata(int port, int fd)
{
  char sport[10];
  int ret,res=0,conret;
  char *buff = (char *)malloc(sizeof(char)*DATABUFFSIZE);
  char *cbuff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  memset(buff,0,DATABUFFSIZE);
  memset(cbuff,0,COMBUFFSIZE);
  
  sprintf(sport,"%d",port);
  int sockd = socketconnect(hostname,sport);
  conret = recv(serv_sd, cbuff, COMBUFFSIZE, 0);
  if(conret == -1){
    res = -1;
    goto exit;
  }
  printf("%s\n",cbuff);
  while((ret=read(fd,buff,DATABUFFSIZE))>0){
    send(sockd, buff, ret,0);
    memset(buff,0,DATABUFFSIZE);
  }
  if(ret == -1){
    res = -1;
    goto exit;
  }
  memset(cbuff,0,COMBUFFSIZE);
  conret = recv(serv_sd, cbuff, COMBUFFSIZE, 0);
  if(conret == -1){
    res = -1;
    goto exit;
  }
 exit:
  close(sockd);
  free(cbuff);
  free(buff);
  return res;
}

static int getpasvdata(int port, int fd)
{
  char sport[10];
  char *buff = (char *)malloc(sizeof(char)*DATABUFFSIZE);
  char *cbuff = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char *prelimres = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char *finalres = (char *)malloc(sizeof(char)*COMBUFFSIZE);
  char **results = (char **)malloc(sizeof(char *)*2);
  char *ptr;
  int res=0;
  results[0]=prelimres;
  results[1]=finalres;
  char delim = '\n';
  
  memset(buff,0,DATABUFFSIZE);
  memset(cbuff,0,COMBUFFSIZE);
  memset(prelimres,0,COMBUFFSIZE);
  memset(finalres,0,COMBUFFSIZE);
  sprintf(sport,"%d",port);
  int sockd = socketconnect(hostname,sport);
  int ret,conret;
  ret = recv(sockd, buff, DATABUFFSIZE, 0);
  if(ret == -1){
    res = -1;
    goto exit;
  }
  conret = recv(serv_sd,cbuff,COMBUFFSIZE,0);
  if(conret == -1){
    res == -1;
    goto exit;
  }
  parseresponse(cbuff, &prelimres, &finalres);
  printf("%s", prelimres);
  write(fd, buff, ret);
  while((ret=recv(sockd,buff,DATABUFFSIZE,0))>0){
    write(fd, buff, ret);
    memset(buff,0,DATABUFFSIZE);
  }
  memset(cbuff,0,COMBUFFSIZE);
  if(strlen(finalres)==0)
    recv(serv_sd,finalres,COMBUFFSIZE,0);
  printf("%s",finalres);
 exit:
  close(sockd);
  free(prelimres);
  free(finalres);
  free(results);
  free(cbuff);
  free(buff);
  return res;
}

static void parseresponse(char *resp, char **prelim, char **final)
{
  char **strs=NULL;
  char esc='\n';
  int strcnt = splitstr(&strs, resp, esc);
  int i;
  for(i=0;i<strcnt;i++){
    if(strs[i][0]=='1'){
      strcat(*prelim,strs[i]);
      strcat(*prelim,"\n");
    }
    else if(strs[i][0]=='2'){
      strcat(*final,strs[i]);
      strcat(*final,"\n");
    }
  }
  if(strs != NULL)
    free(strs);
}

static int ftp_help(char **params, int cnt)
{
  if(cnt == 1)
    printf("cd\tget\thelp\topen\tput\npwd\tquit\trget\trput\n");
  if(cnt > 1){
    if(strcmp(params[1],"cd")==0)
      printf("cd\tchange remote working directory\n");
    else if(strcmp(params[1],"get")==0)
      printf("get\treceive file\n");
    else if(strcmp(params[1],"put")==0)
      printf("put\tsend one file\n");
    else if(strcmp(params[1],"pwd")==0)
      printf("pwd\tprint working directory on remote machine\n");
    else if(strcmp(params[1],"quit")==0)
      printf("quit\tterminate ftp session and quit\n");
    else if(strcmp(params[1],"rget")==0)
      printf("rget\treceive directory\n");
    else if(strcmp(params[1],"rput")==0)
      printf("rput\tsend one directory\n");
    else if(strcmp(params[1],"help")==0)
      printf("help\tprint local help information\n");
    else if(strcmp(params[1],"open")==0)
      printf("open\tconnect to remote ftp\n");
    else
      printf("%s\tunknown command\n",params[1]);
  }
}

static issuecmd(int cmd, char **param, int paramc){
  char *buff = (char *)malloc(sizeof(char)*1024);
  char *path = (char *)malloc(sizeof(char)*4096);
  char *file = (char *)malloc(sizeof(char)*4096);
  char *xtra = (char *)malloc(sizeof(char)*4096);
  char **bparam = (char **)malloc(sizeof(char *)*3);
  bparam[0] = path;
  bparam[1] = file;
  bparam[2] = xtra;
  memset(path, 0, 4096);
  memset(file, 0, 4096);
  memset(xtra, 0, 4096);
  buildparam(param+1,paramc-1,bparam,3,'\\');
  switch(cmd){
  case HELP:{
    ftp_help(param,paramc);
    break;
  }
  case GET:{
    if(serv_sd <= 0){
      printf("Not connected\n");
      goto exit;
    }
    if(strlen(file)==0)
      strcpy(file,path);
    ftp_getfile(path, file);
    break;
  }
  case RGET:{
    if(serv_sd <= 0){
      printf("Not connected\n");
      goto exit;
    }
    if(strlen(file)==0)
      strcpy(file,path);
    ftp_getdir(path, file);
    break;
  }
  case PUT:{
    if(serv_sd <= 0){
      printf("Not connected\n");
      goto exit;
    }
    if(strlen(file)==0)
      strcpy(file,path);
    ftp_putfile(path,file);
    break;
  }
  case RPUT:{
    if(serv_sd <= 0){
      printf("Not connected\n");
      goto exit;
    }
    if(strlen(file)==0)
      strcpy(file,path);
    ftp_putdir(path,file);
    break;
  }
  case LS:{
    if(serv_sd <= 0){
      printf("Not connected\n");
      goto exit;
    }
    ftp_list(path);
    break;
  }
  case PWD:{
    if(serv_sd <= 0){
      printf("Not connected\n");
      goto exit;
    }
    ftp_pwd();
    break;
  }
  case CD:{
    if(serv_sd <= 0){
      printf("Not connected\n");
      goto exit;
    }
    ftp_chdir(path);
    break;
  }
  case OPEN:{
    ftp_open(path, FTP_PORT);
    break;
  }
  case QUIT:{
    if(serv_sd <= 0){
      goto exit;
    }
    ftp_quit();
    break;
  }
  default:{
    printf("Invalid command, cann't you even type properly?\n");
    break;
  }
  }
 exit:
  free(buff);
  free(path);
  free(file);
  free(xtra);
  free(bparam);
}


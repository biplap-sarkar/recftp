/*  
    Header file for commands provided by recftp.
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

#include <sys/socket.h>
#define FTP_PORT 21
#define DATABUFFSIZE 4096
#define COMBUFFSIZE 1024
#define GET 1
#define PUT 2
#define LS 3
#define PWD 4
#define RGET 5
#define RPUT 6
#define CD 7
#define QUIT 8
#define HELP 9
#define OPEN 10
#define BINARY 90
#define ASCII 91
int ftp_connect(const char *ip, const char *port);
int ftp_login(const char *ip, const char *port);
static int ftp_chtype(int mode);
static int ftp_getfile(char *remfile, char *locfile);
static int ftp_getdir(char *remdir, char *locdir);
static int ftp_putfile(char *locfile, char *remfile);
static int ftp_putdir(char *locdir, char *remdir);
static int ftp_list(char *path);
static int ftp_pwd();
static int ftp_chdir(char *path);
static int ftp_help(char **params, int cnt);
static int ftp_quit();
static int getpasvdata(int port, int fd);
static int putpasvdata(int port, int fd);

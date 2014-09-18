#ifndef EPOLL_EXAMPLE_H
#define EPOLL_EXAMPLE_H

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#define MAXEVENTS 20
#define MAXLISTEN 20
void add_active_fd(int _fd);// put the actived tcp accepted socket fd in to an array
void del_active_fd(int _fd);
int make_socket_blocking (int _sfd);
int make_socket_non_blocking (int _sfd);
int create_and_connect(char*host,char*port);
int create_and_bind (char *port);
int net_epoll_listen(char* port, int conns);
int net_epoll_wait ();
int net_epoll_accept();
int net_epoll_read (int _fd, char*buf, int read_length);
int net_epoll_write(int _fd, char*buf, int length);

void epoll_set_fd_index (int n);
int epoll_get_fd_index ();

int net_epoll_get_fd();
void net_epoll_close();
int epoll_check_rd_hup();
int epoll_check_error();

int epoll_sfd_cmp( int );
int epoll_ctl_add_fd( int _fd);
int epoll_ctl_del_fd( int _fd);

int kill_fd(int );
int epoll_init();
int socket_check(int fd);

void epoll_close( int _fd);

  int sfd, s;
  int efd;
  
  int fd_index;
  struct epoll_event  event;
  struct epoll_event *events;

  int cfd;// for connections to the log server
#endif
  
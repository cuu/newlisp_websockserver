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
int make_socket_blocking (int _sfd);
int make_socket_non_blocking (int _sfd);
int create_and_bind (char *port);
int net_epoll_listen(char* port, int conns);
int net_epoll_wait ();
int net_epoll_accept();
int net_epoll_read (char*buf, int read_length);
int net_epoll_write(int _fd, char*buf, int length);

void epoll_set_fd_index (int n);
int epoll_get_fd_index ();

int net_epoll_get_fd();
void net_epoll_close();
int epoll_check_events_flags();
int epoll_sfd_cmp( int );

int kill_fd(int );

  int sfd, s;
  int efd;
  
  int fd_index;
  struct epoll_event  event;
  struct epoll_event *events;

#endif
  
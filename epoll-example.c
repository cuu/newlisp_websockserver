/*
 * epoll to newlisp wrapper .so
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "epoll-example.h"

int active_fds[MAXEVENTS];
void add_active_fd(int _fd)
{
  int i,u;
  u = 0;
  for(i=0;i<MAXEVENTS;i++)
  {
    if( active_fds[i] == _fd ) { u = 1; break;}
  }
  if(u == 0)
  {
    for(i=0;i<MAXEVENTS;i++)
    {
      if(active_fds[i] == 0)
      {
        active_fds[i] = _fd;
        u = i;
        break;
      }
    }
  }
}

void del_active_fd(int _fd)
{
  int i;
  for(i=0;i<MAXEVENTS;i++)
  {
    if(active_fds[i] == _fd)
    {
      active_fds[i] = 0;
    }
  }
}

int make_socket_blocking (int _sfd)
{
  int flags, ret;

  flags = fcntl (_sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl ");
      return -1;
    }

  flags &= ~O_NONBLOCK;
  ret = fcntl (_sfd, F_SETFL, flags);
  if (ret == -1)
    {
      perror ("fcntl ~nonblock");
      return -1;
    }

  return 0;
}

int make_socket_non_blocking (int _sfd)
{
  int flags, ret;

  flags = fcntl (_sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  ret = fcntl (_sfd, F_SETFL, flags);
  if (ret == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}

int create_and_connect(char*host,char*port)
{
  int sock;
  struct sockaddr_in echoserver; 
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
  {
    fprintf(stderr,"Failed to create socket");
    exit(-1);
  }

  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(host);  /* IP address */
  echoserver.sin_port = htons(atoi(port));       /* server port */
            /* Establish connection */
  if (connect(sock,(struct sockaddr *) &echoserver, sizeof(echoserver)) < 0) 
  {
        fprintf(stderr,"Failed to connect with server");
        return -1;
  }

  return sock;
}

int create_and_bind (char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int _s, _sfd;
	int flag ;
	
  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  _s = getaddrinfo (NULL, port, &hints, &result);
  if (_s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (_s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      _sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (_sfd == -1)
        continue;
	  /*
		flag = 1;
		if(setsockopt( _sfd, SOL_SOCKET, SO_REUSEADDR,
                          (char *) &flag, sizeof(flag)) < 0) {
                fprintf(stderr, "Can't set SO_REUSEADDR.\n");
	        exit(1001);
	    }
	    */
      _s = bind (_sfd, rp->ai_addr, rp->ai_addrlen);
      if (_s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }

      close (_sfd); // error occured if
    }

  if (rp == NULL)
    {
      fprintf (stderr, "Could not bind\n"); exit(-1);
      return -1;
    }

  freeaddrinfo (result);

  return _sfd;
}

int epoll_init()
{
  efd = epoll_create1 (0);
  if (efd == -1)
  {
    perror ("epoll_create");
    exit(efd);
  }

  events = calloc (MAXEVENTS, sizeof event);
  if(!events) { perror ("epoll events calloc"); exit(-2); }

  return efd;
}

int net_epoll_listen(char* port, int conns)
{
  sfd = create_and_bind (port);
  if (sfd == -1)
    return sfd;
  
  s = make_socket_non_blocking (sfd);
  if (s == -1)
    return s;
  
  s = listen (sfd,conns);
  if (s == -1)
    {
      perror ("listen");
      return s;
    }

  efd = epoll_create1 (0);
  if (efd == -1)
    {
      perror ("epoll_create");
      return efd;
    }

  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET; // only for listenning 
  s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
  if (s == -1)
    {
      perror ("epoll_ctl");
	  return s;
    }

  events = calloc (MAXEVENTS, sizeof event);
  if(!events)
	  return -2;
  else
  return 0;
  
}

int net_epoll_wait ()
{
	 return epoll_wait (efd, events, MAXEVENTS, -1);
}

int net_epoll_accept( )
{
				  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];	
                  struct sockaddr in_addr;
                  socklen_t in_len;
                  int infd;
                  
                  in_len = sizeof (in_addr);
                  infd = accept (sfd, &in_addr, &in_len);
                  if (infd == -1)
                    {
                      if ((errno == EAGAIN) ||  (errno == EWOULDBLOCK))
                        {
                          /* We have processed all incoming
                             connections. */
                          return infd;
                        }
                      else
                        {
                          perror ("accept");
                          return infd;
                        }
                    }

                  s = getnameinfo (&in_addr, in_len,
                                   hbuf, sizeof hbuf,
                                   sbuf, sizeof sbuf,
                                   NI_NUMERICHOST | NI_NUMERICSERV);
                  if (s == 0)
                    {
                      printf("Accepted connection on descriptor %d "
                             "(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }

                  /* Make the incoming socket non-blocking and add it to the
                     list of fds to monitor. */
                  s = make_socket_non_blocking (infd);
                  if (s == -1) return s;

                  event.data.fd = infd;
                  event.events =  EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET; 
                  s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                  if (s == -1)
                    {
                      perror ("epoll_ctl");
                      return s;
                    }
                    
                    //net_epoll_write( infd, ">\n", 2); /// first time to write 
                    add_active_fd(infd);
                    return 0;
}

int net_epoll_read (int _fd, char*buf, int read_length)
{
                 ssize_t count;
 
                 count = read ( _fd, buf, read_length );
                  if (count == -1)
                    {
                      /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                      if (errno != EAGAIN)
                        {
                          perror ("read");
                        }
                    }             
                    return count;
                    
}

int net_epoll_write(int _fd, char*buf, int length)
{
                 ssize_t count;
 
                 count = write ( _fd, buf, length );
                  if (count == -1)
                    {
                      if (errno != EAGAIN)
                        {
                          perror ("write");
                        }
                    }             
                    return count;
                    
}

int net_epoll_get_fd()
{
	return events[fd_index].data.fd;
}
void net_epoll_close()
{
  free (events);
  close (sfd);	
}

void epoll_close( int _fd)
{
  if( close(_fd) < 0)
    perror("epoll_close");
}

void epoll_set_fd_index (int n)
{
	fd_index = n;
}
int epoll_get_fd_index ()
{
	return fd_index;
}

int epoll_check_rd_hup()
{
  if( (events[fd_index].events & EPOLLRDHUP) || (events[fd_index].events & EPOLLHUP) ) return 1;
	//if( (events[fd_index].events & EPOLLRDHUP) ||  (events[fd_index].events & EPOLLERR) || (events[fd_index].events & EPOLLHUP) || (!(events[fd_index].events & EPOLLIN))) return 1;
	else return 0;
}

int epoll_check_error()
{
  if( events[fd_index].events & EPOLLERR ) return 1;
  else return 0;
}

int epoll_sfd_cmp( int cmp_sfd)
{
	if( sfd == cmp_sfd) return 1;
	else return 0;
}

int epoll_ctl_add_fd( int _fd)
{
	event.data.fd = _fd;
    event.events =  EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET; 
	s = epoll_ctl (efd, EPOLL_CTL_ADD, _fd, &event);
	if (s == -1)
	{
		perror ("epoll_ctl add");
		return s;
	}
	return 0;
}

int epoll_ctl_del_fd( int _fd)
{
	// This maybe will hang the epoll_wait forever,as I am not sure of it
	s = epoll_ctl (efd, EPOLL_CTL_DEL, _fd, NULL); // since kernel 2.6.9, can use NULL 
	if (s == -1)
	{
		perror ("epoll_ctl del ");
		return s;
	}
	return 0;
}

int kill_fd(int _fd)
{
	shutdown(_fd, SHUT_RDWR);
}

void spit_to_all_active_clients(char*buf, int length)
{
  int i,_s;
  for(i=0;i<MAXEVENTS;i++)
  {
    if(active_fds[i] != 0)
    {
      if(socket_check(active_fds[i]) == 0)
      _s = net_epoll_write( active_fds[i], buf, length);
      else 
        perror("socket check spit_to_all_active_clients ");

    }
  }
  return;  
}


/* reading waiting errors on the socket
 * return 0 if there's no, 1 otherwise
 */
int socket_check(int fd)
{
   int ret;
   int code;
   size_t len = sizeof(int);

   ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &code, &len);

   if ((ret || code)!= 0)
      return 1;

   return 0;
}

void show_socketfd_error(int _fd)
{
  int error = 0;
  socklen_t errlen = sizeof(error);
  if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
  {
    fprintf(stderr,"error = %s\n", strerror(error));
  }
}

#ifdef TEST

int main (int argc, char *argv[])
{
  FILE*pipe;
  int pfd;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s [port]\n", argv[0]);
      exit (EXIT_FAILURE);
    }

	net_epoll_listen( argv[1], MAXLISTEN);

  cfd = create_and_connect("127.0.0.1", "9001");
  if(cfd < 0 ) exit(cfd);
  s = make_socket_non_blocking(cfd);
  if(s<0) return s;
  else epoll_ctl_add_fd(cfd);

  /* The event loop */
  while (1)
    {
      int n, i;

	 n = net_epoll_wait();
     for (i = 0; i < n; i++)
	 {
		 epoll_set_fd_index(i);
	  if (epoll_check_rd_hup())
	    {
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
        perror("epoll error check_event flags");
        fprintf (stderr, "%d\n", net_epoll_get_fd());
        show_socketfd_error(net_epoll_get_fd());
	      close (net_epoll_get_fd());
        del_active_fd(net_epoll_get_fd());
	      //continue;
	    }else if (epoll_check_error())
      {
        perror("epoll error");
        continue;
      }
      else if(net_epoll_get_fd() == cfd) 
      {
        printf("in log server section %d\n",cfd);
          int done2= -1;
          while(1)
          {
            char buf2[512];
            memset(buf2,0,512);
            done2 =  net_epoll_read(net_epoll_get_fd() , buf2, sizeof(buf2));
            if(done2 > 0)
            {
              printf("here from log:\n");
              s = write (1, buf2, done2);
              spit_to_all_active_clients(buf2,done2);
            }else if (done2<=0) break;
          }
          if (done2==0)
          {
            printf("pipe closed\n");
            close(cfd);
          }

      }
		else if ( epoll_sfd_cmp( net_epoll_get_fd()))
	    {
              /* We have a notification on the listening socket, which
                 means one or more incoming connections. */
              while (1)
                {
					if( net_epoll_accept() != 0) break;
                }
				
            }
          else 
            {
              int done = -1;
			  
              while (1)
                {
					char buf[512];
          memset(buf,0,512);
					done = net_epoll_read( net_epoll_get_fd(), buf, sizeof(buf));
					if(done > 0)
					{
						s = write (1, buf, done);
						s = net_epoll_write( net_epoll_get_fd(), buf, strlen(buf));
						s = net_epoll_write( net_epoll_get_fd(), ">\n", 2);
					}
					else if(done <= 0 ) break;
                }

              if (done==0)
                {
                  printf ("Closed connection on descriptor %d\n",net_epoll_get_fd());
                  close (net_epoll_get_fd());
                }
            }
        }
    }

  pclose(pipe);
  net_epoll_close();
  return EXIT_SUCCESS;
}
#elif defined(TEST2)
int file_exist (char *filename)
{
  struct stat buffer;   
  return (stat (filename, &buffer) == 0);
}

int main (int argc, char *argv[])
{
  FILE*pipe;
  int pfd;
  int i;
  char cmdstring[1024];

  memset(active_fds,0, MAXEVENTS);
  if (argc != 3)
    {
      fprintf (stderr, "Usage: %s [port] [logfile]\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  net_epoll_listen( argv[1], MAXLISTEN);

  if(!file_exist(argv[2]))
  {
    fprintf(stderr, "Cannot open %s file. Try again later.\n",argv[2]);
    exit(-1);
  }
  sprintf(cmdstring,"tail -f %s", argv[2]);
  pipe = popen(cmdstring,"r");
  if (pipe == NULL)
  {
    fprintf(stderr, "Cannot popen %s file. Try again later.\n",argv[2]);
    exit(-1);
  }
  pfd = fileno(pipe);
  make_socket_non_blocking(pfd);
  epoll_ctl_add_fd( pfd);
  /* The event loop */
  while (1)
    {
      int n, i;

   n = net_epoll_wait();
     for (i = 0; i < n; i++)
   {
     epoll_set_fd_index(i);
    if (epoll_check_rd_hup())
      {
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
        fprintf (stderr, "epoll rd hup error,%d\n", net_epoll_get_fd());
        show_socketfd_error(net_epoll_get_fd());
        close (net_epoll_get_fd());
        del_active_fd(net_epoll_get_fd());
        //continue;
      }else if (epoll_check_error())
      {
        perror("epoll error");
        continue;
      }
      else if(net_epoll_get_fd() == pfd) 
      {
        printf("in log section %d\n",pfd);
          int done2= -1;
          while(1)
          {
            char buf2[1024];
            memset(buf2,0,1024);
            done2 =  net_epoll_read(net_epoll_get_fd() , buf2, sizeof(buf2));
            if(done2 > 0)
            {
              printf("here from log:\n");
              s = write (1, buf2, done2);
              spit_to_all_active_clients(buf2,strlen(buf2));
            }else if (done2<=0) break;
          }
          if (done2==0)
          {
            printf("pipe closed\n");
            close(pfd);
            pclose(pipe);
          }

      }
    else if ( epoll_sfd_cmp( net_epoll_get_fd()))
      {
              /* We have a notification on the listening socket, which
                 means one or more incoming connections. */
              while (1)
                {
                  if( net_epoll_accept() != 0) break;
                }
        
      }
      else 
      {
              int done = -1;
        
              while (1)
                {
                  char buf[512];
                  memset(buf,0,512);
                  done = net_epoll_read( net_epoll_get_fd(), buf, sizeof(buf));
                 if(done > 0)
                  {
                    /*
                      s = write (1, buf, done);
                      s = net_epoll_write( net_epoll_get_fd(), buf, strlen(buf));
                      s = net_epoll_write( net_epoll_get_fd(), ">\n", 2);
                    */
                  }
                  else if(done <= 0 ) break;
                }

              if (done==0)
                {
                  printf ("Closed connection on descriptor %d\n",net_epoll_get_fd());
                  close (net_epoll_get_fd());
                  del_active_fd(net_epoll_get_fd());
                }
            }
        }
    }

  pclose(pipe);
  net_epoll_close();
  return EXIT_SUCCESS;
}

#endif

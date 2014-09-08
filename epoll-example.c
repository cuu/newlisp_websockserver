/*
 * epoll to newlisp wrapper .so
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include "epoll-example.h"

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

int create_and_bind (char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int _s, _sfd;

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
  event.events = EPOLLIN | EPOLLET;
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
                    
                    return 0;
}

int net_epoll_read (char*buf, int read_length)
{
                 ssize_t count;
 
                 count = read ( net_epoll_get_fd(), buf, read_length );
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

void epoll_set_fd_index (int n)
{
	fd_index = n;
}
int epoll_get_fd_index ()
{
	return fd_index;
}

int epoll_check_events_flags()
{
	if( (events[fd_index].events & EPOLLERR) || (events[fd_index].events & EPOLLHUP) || (!(events[fd_index].events & EPOLLIN))) return 1;
	else return 0;
}

int epoll_sfd_cmp( int cmp_sfd)
{
	if( sfd == cmp_sfd) return 1;
	else return 0;
}

int kill_fd(int _fd)
{
	shutdown(_fd, SHUT_RDWR);
}
#ifdef TEST

int main (int argc, char *argv[])
{

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s [port]\n", argv[0]);
      exit (EXIT_FAILURE);
    }

	net_epoll_listen( argv[1], MAXLISTEN);

  /* The event loop */
  while (1)
    {
      int n, i;

	 n = net_epoll_wait();
     for (i = 0; i < n; i++)
	 {
		 epoll_set_fd_index(i);
	  if (epoll_check_events_flags())
	    {
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
	      fprintf (stderr, "epoll error\n");
	      close (net_epoll_get_fd());
	      //continue;
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
					done = net_epoll_read( buf, sizeof(buf));
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

  net_epoll_close();
  return EXIT_SUCCESS;
}

#endif

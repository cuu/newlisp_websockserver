(setq sofile "./libepoll-example.so")

(import sofile "net_epoll_listen")
(import sofile "net_epoll_wait")
(import sofile "net_epoll_accept")
(import sofile "net_epoll_read")
(import sofile "net_epoll_write")
(import sofile "epoll_set_fd_index")
(import sofile "epoll_get_fd_index")
(import sofile "net_epoll_get_fd")
(import sofile "net_epoll_close")
(import sofile "epoll_check_events_flags")
(import sofile "epoll_sfd_cmp")
(import sofile "kill_fd")
(import sofile "make_socket_blocking")
(import sofile "make_socket_non_blocking")
(import sofile "del_active_fd")
(import sofile "create_and_connect")

(import sofile "epoll_ctl_add_fd")
(import sofile "spit_to_all_active_clients")

(load "/home/cuu/Documents/openwrt_zigbee_lsp_file/myfunc.lsp")
(load "/home/cuu/Documents/newlisp_websockserver/websocket.lsp")

(setq while_break nil)

(net_epoll_listen port 10)

(setq cfd (create_and_connect "127.0.0.1" "9001"))
(if (< cfd 0) (exit -1))
(setq s (make_socket_non_blocking cfd))
(if (< s 0) (println "make cfd non blocking failed") (epoll_ctl_add_fd cfd))


(define (epoll_websocket_process, buff (msg "") n rvlen)
(while true

	(setq n (net_epoll_wait))
	(for (i 0 (- n 1))
		(epoll_set_fd_index i)
		(if (= (epoll_check_events_flags) 1)
			(begin
				(println "epoll error "  (net_epoll_get_fd) "\n")
				(close (net_epoll_get_fd))
				(del_active_fd (net_epoll_get_fd))
			)
			(= (net_epoll_get_fd) cfd)
			(begin
				(setq rvlen -1)

				(while (nil? while_break)
					;(set 'buff (dup "\000" 1024))
					(setq buff "")
					(setq rvlen  (read (net_epoll_get_fd) buff 1024 ))

					(println "rvlen: " rvlen)
					(println "buff:" buff)
					(setq buff (ws_send (replace "\n" buff "<br />")))
					(if (> (length buff) 0)
						(spit_to_all_active_clients buff (length buff))
					)
					(if (<= rvlen 0) 
						(begin
							(println "break while " rvlen)
							(setq while_break true)
						)
					)
				)
				(setq while_break nil) ;; reset while break flag
				(if (or (nil? buff) (= rvlen 0)) ;; nil or 0
					(begin
						(println "Closed connection on descriptor " (net_epoll_get_fd) "\n")
						(close (net_epoll_get_fd))
						(del_active_fd (net_epoll_get_fd))
						;(kill_fd (net_epoll_get_fd))
					)
				)

			)
			(=  (epoll_sfd_cmp (net_epoll_get_fd)) 1)
			(begin
				(while (nil? while_break)
					(if (!=  (net_epoll_accept) 0) (setq while_break true)
					)
				)
				(setq while_break nil)
			)
			(begin
				(setq rvlen -1)

				(while (nil? while_break)
					;(set 'buff (dup "\000" 1024))
					(setq buff "")
					(setq rvlen  (read (net_epoll_get_fd) buff 1024 ))
					(while (> (peek (net_epoll_get_fd)) 0)
						(setq rvlen  (read  (net_epoll_get_fd) buff 1024))
						(if-not (nil? buff) (setq buff (append buff buff)) )
					)
					(println "rvlen: " rvlen)
					(println "buff:" buff)
					(if-not (nil? buff)
						(if (and (starts-with buff "GET ") (ends-with buff "\r\n\r\n"))
							(handle_shake buff)
							(handle_data buff)
						)
					)

					(if (<= rvlen 0) 
						(begin
							(println "break while " rvlen)
							(setq while_break true)
						)
					)
				)
				(setq while_break nil) ;; reset while break flag
				(if (or (nil? buff) (= rvlen 0)) ;; nil or 0
					(begin
						(println "Closed connection on descriptor " (net_epoll_get_fd) "\n")
						(close (net_epoll_get_fd))
						(del_active_fd (net_epoll_get_fd))
						;(kill_fd (net_epoll_get_fd))
					)
				)
			)

		)
		
	)
)
	(net_epoll_close)
)


(epoll_websocket_process)

(exit -1)

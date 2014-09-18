(load "load_and_import.lsp")

(setq while_break nil)

(net_epoll_listen port 10)

(setq cfd (create_and_connect "127.0.0.1" "9001"))
(if (< cfd 0) (exit -1))
(setq s (make_socket_non_blocking cfd))
(if (< s 0) (println "make cfd non blocking failed") (epoll_ctl_add_fd cfd))


(define (epoll_websocket_process, (while_break1 nil)(while_break2 nil) (while_break3 nil) buff buff2 (msg "") n rvlen rvlen2)
(while true

	(setq n (net_epoll_wait))
	(for (i 0 (- n 1))
		(epoll_set_fd_index i)
		(if (= (epoll_check_rd_hup) 1)
			(begin
				(println "epoll rd hup error " (net_epoll_get_fd))
				(show_socketfd_error (net_epoll_get_fd))
				(epoll_close (net_epoll_get_fd))
				(del_active_fd (net_epoll_get_fd))
			)
			(= (epoll_check_error) 1)
			(begin
				(println "epoll error");
			)
			(= (net_epoll_get_fd) cfd)
			(begin
				(setq rvlen2 -1)

				(while (nil? while_break3)
					;(set 'buff (dup "\000" 1024))
					(setq buff2 "")
					(setq rvlen2 (read (net_epoll_get_fd) buff2 1024 ))
					(while (> (peek (net_epoll_get_fd)) 0)
						(setq rvlen2  (read  (net_epoll_get_fd) buff2 1024))
						(if-not (nil? buff2) (setq buff2 (append buff2 buff2)) )
					)
					(println "rvlen2: " rvlen2)
					(println "buff2:" buff2)
					(if-not (nil? buff2)
						(begin
							(setq buff2 (wsiencode (replace "\n" (xxd_filter_ascii buff2) "<br />")))
							(if (> (length buff2) 0)
								(spit_to_all_active_clients buff2 (length buff2))
							)
						)
					)
					(if (<= rvlen2 0) 
						(begin
							(println "break while " rvlen2)
							(setq while_break3 true)
						)
					)
				)
				(setq while_break3 nil) ;; reset while break flag
				(if (or (nil? buff2) (= rvlen2 0)) ;; nil or 0
					(begin
						(println "Closed connection on descriptor " (net_epoll_get_fd) "\n")
						(epoll_close (net_epoll_get_fd))
						(del_active_fd (net_epoll_get_fd))
						;(kill_fd (net_epoll_get_fd))
					)
				)

			)
			(=  (epoll_sfd_cmp (net_epoll_get_fd)) 1)
			(begin
				(while (nil? while_break2)
					(if (!=  (net_epoll_accept) 0) (setq while_break2 true)
					)
				)
				(setq while_break2 nil)
			)
			(begin
				(setq rvlen -1)

				(while (nil? while_break1)
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
							(setq while_break1 true)
						)
					)
				)
				(setq while_break1 nil) ;; reset while break flag
				(if (or (nil? buff) (= rvlen 0)) ;; nil or 0
					(begin
						(println "Closed connection on descriptor " (net_epoll_get_fd) "\n")
						(epoll_close (net_epoll_get_fd))
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

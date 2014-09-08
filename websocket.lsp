;; 2014 04 02 
;; tested in linux chromium
;; 
(define-macro (build_uint16 hi low) (+ (& low 0x00ff) (<< (& hi 0x00ff) 8)))

(setq port "8080")

(if-not  (file? "/home/cuu/Documents/newlisp_websockserver/sha1.so")
	(begin (println "no sha1 library") (exit) )
)
(import "/home/cuu/Documents/newlisp_websockserver/sha1.so" "sha1_string")

(setq websocket_guid "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
(setq flag "\n")
(setq global_status 1);;; global commiunication status 1 means normal text transfer, other means controling ,like close,ping,pong,etc

(define (hex2string hex_buf)
	(if-not (nil? hex_buf)
		(join (map (fn (x) (format "%02x" x)) (unpack (dup "b "  (length hex_buf)) hex_buf)) " ")
	)
)

(define (hex2str hex_buf);; no space version
   (if-not (nil? hex_buf)
      (join (map (fn (x) (format "%02x" x)) (unpack (dup "b "  (length hex_buf)) hex_buf)) )
   )
)

(define (string2hex str_buf, map_buff)
   (if-not (nil? str_buf)
      (begin
         (setq map_buff (map (fn (x) (int x 0 16)) (explode str_buf 2)))
         (pack (dup "b " (length map_buff)) map_buff)
      )
   )
)


(define (hextodecstr hex_buf); 
	(if-not (nil? hex_buf)
		(join (map (fn (x) (format "%d" x)) (unpack (dup "b "  (length hex_buf)) hex_buf)) " ")
	)
)

(define (handle_shake buff, ret tmplst tmplst2 key_code combine_key sha1_code accept_code)
	(if (not (nil? buff))
		(begin
			(setq tmplst (parse buff "\r\n"))
			(dolist (x tmplst)
				(if
				(= $idx (- (length tmplst) 1) ) 
					(begin
					(println "start handle shake")
					(setq combine_key (string key_code websocket_guid))

					(println "combine_key: " combine_key)
					;(setq sha1_code (nth 0 (exec (string "./sha1test " combine_key))))

					(setq sha1_code (join (map (fn (x) (char x)) (unpack (dup "c " 20) (sha1_string combine_key)) )))

					(println "sha1_code: " (hex2string sha1_code))
					;(setq accept_code (base64-enc (string2hex sha1_code)))
					(setq accept_code (base64-enc sha1_code))
					(println "accept_code: " accept_code)
					(setq ret "")
					(setq ret (append ret "HTTP/1.1 101 Switching Protocols\r\n"))
					(setq ret (append ret "Upgrade: websocket\nConnection: Upgrade\r\n"))
					(setq ret (append ret (string "Sec-WebSocket-Accept: " accept_code "\r\n\r\n")))
					;(setq ret (append ret "Sec-WebSocket-Protocol: chat\r\n\r\n")) ;; here is what we called additional protocol
					;;;not send back this Sec-WebSocket-Protocol when the client did not send 
					(println ret)
					;(net-send connection ret )
					(net_epoll_write  (net_epoll_get_fd) ret (length ret))
					(println "shake over")	
					)
				(find "Sec-WebSocket-Key: " x)
					(begin
						(setq tmplst2 (parse x " "))
						(if (> (length tmplst2) 1)
							(begin
								(setq key_code  (nth 1 tmplst2) )
								(println "key_code: " key_code " :" (length key_code))
							)
						)	
			
					)
				)
			)
		)
	)	
)
(define (handle_data buff,fin opcode payload_start mask static_status (dataret  "") return_str msg_len msg_len_flag)
				(println "handle_data: " buff " length: " (length buff))
				(if (> (length buff) 3)
							(begin
								(setq fin (& (char (slice buff 0 1) ) 0x80))
								(setq opcode (& (char (slice buff 0 1)) 0x0f))
								(setq payload_start 2)
								(println "opcode: " opcode)
								(if (and (= fin 128) (< opcode 3))
								(begin
									(setq mask (& (nth 0 (unpack "b" (slice buff 1 1))) 0x80))
									(setq msg_len_flag (& (nth 0 (unpack "b" (slice buff 1 1))) 0x7f))
									(println "mask: " mask  " " "msg_len_flag: " msg_len_flag)
									(if (> (length buff) 6)
										(begin
											(if (= msg_len_flag 126)
												(begin
													(setq msg_len (get-int (reverse (slice buff 2 2))))
													(setq payload_start (+ payload_start 2))
												)
											(= msg_len_flag 127)
												(begin
													(setq msg_len (get-int (reverse (slice buff 2 4))))
													(setq payload_start (+ payload_start 4))
												)
												(setq msg_len msg_len_flag)
											)
											(if (> mask 0)
												(begin
													(setq mask_bytes (unpack "b b b b" (slice buff payload_start 4)))
													(println "mask bytes: " mask_bytes)
													(setq payload_start (+ payload_start 4))		
												)
											)
											(if (< (length buff) (+ payload_start msg_len))
												(println "msg not completed: " payload_start " + " msg_len)
												(begin
													(if (> mask 0)
														(begin
															(setq payload (slice buff payload_start (+ payload_start msg_len)))
															(if (= opcode 0x01)
																(begin
																	(for (i 0 (- msg_len 1))
																		(catch (extend dataret  (char (^ (nth (% i 4) mask_bytes) (nth 0 (unpack "b" (slice payload i 1)))))) 'result)
																	)
																)
															)	 
														)
													)
												)
											)
										(setq static_status opcode)		
										)
									)
								)
								(and (= fin 128) (= opcode 8))
								(begin
									(println "closing wesocket")
									;(net-close connection)
									(make_socket_blocking (net_epoll_get_fd))
									;(close (net_epoll_get_fd))
									(kill_fd (net_epoll_get_fd))
								)
								(begin
									(println "fin not 129: " fin " " opcode)
									;(close (net_epoll_get_fd))
									;(kill_fd (net_epoll_get_fd))
									
								)
								)

								(if (= static_status 0x01)
									(begin
										(println  dataret)
										(println (hextodecstr dataret))
										(setq return_str (string dataret " back"))
										(setq dataret (ws_send return_str))
										;(net-send connection dataret)
										(net_epoll_write (net_epoll_get_fd) dataret (length dataret))
										(println (hextodecstr dataret))
									)
								)
							)
						)
)

(define (ws_send buff, b1 message b2 len n )
	(println "ws_send: " buff)
	(setq message "")
	(setq b1 0x80)
	(setq b1 (| b1 0x01))
	(setq message (append  message (char b1)))
	(setq b2 0)
	(setq len (length buff))
	(if (< len 126)
		(begin
			(setq b2 (| b2 len))
			(setq message (append message (char b2)))
		)
		(and (> len 126) (< len 65535))
		(begin
			(setq b2 (| b2 126))
			(setq n (pack "d" len))
			(setq message (append message (char b2)))	
			(setq message (append message n))
		)
		(begin
			(setq b2 (| b2 127))
			(setq n (pack "ld" len))
			(setq message (append message (char b2)))
			(setq message (append message n))
		)
	)
	(setq message (append message buff))
					
)

;(set 'listen (net-listen port))
;(unless listen (begin
;   (print "listening failed\n")
;    (exit)))

;(print "Waiting for connection on: " port "\n")

(define (websocket_server ,connection buff)
(while 1
	(setq connection (net-accept listen))
	(println (net-peer connection))
	(while (not (net-select connection "r" 100000))  (println "waiting for reading...") )
		;(while (net-select connection "w" 100000)
		(while (not (net-error))
			(setq buff "")
			
			(if (not (nil? flag))
				(begin
					(setq rvlen  (net-receive connection buff 1024 flag))
					(handle_shake buff)
				)
				(nil? flag)
				(begin
					(println "net-receive flag nil")
					(setq rvlen  (net-receive connection buff 1024))
					(println "net-receive flag nil rvlen " rvlen)
					(extend msg  buff)
				)
			)
			(println "net-error: " (net-error))

			(println "peek now " (net-peek connection))
			(while (> (net-peek connection) 0)
				(println "flag: " (char flag))
			(if (not (nil? flag))
				(begin
					(setq rvlen  (net-receive connection buff 1024 flag))
					(println buff)
					(handle_shake buff)
				)
				(nil? flag)
				(begin
					(setq rvlen  (net-receive connection buff 1024))
					(extend msg buff)
				)
			)	
			
			)
			
			(if (nil? flag )
					(begin
						(setq buff msg)
						(setq msg nil)
						(println "nil flag")
						(if (> (length buff) 0)
							(handle_data buff)
							(println "(length buff)<= 0 " (length buff))
						)
					)
			)
			
			);; end this connection
			
		
	(println "close connect")
	(setq flag "\n")
	(net-close connection)	
)
)




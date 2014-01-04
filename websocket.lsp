(define-macro (build_uint16 hi low) (+ (& low 0x00ff) (<< (& hi 0x00ff) 8)))

(setq port 80)

(setq websocket_guid "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
(setq flag "\n")

(define (hextostring hex_buf); xxxxxxx to xx xx xx xx
	(setq nr (length hex_buf))
	;(assert "hextostring length: " nr "\n")
	(setq ret nil)
	(setq dupstr (dup "b " nr))
	
	(if (> (length dupstr) 1)
		(begin
			(setq dupstr (chop dupstr 1))
	
		(setq upk_lst (unpack dupstr hex_buf))

		(dolist (y upk_lst)
			(extend ret (format "%02x " y))
		)
	
		(setq ret (chop ret 1))
		)
	)
	ret
)

(define (hextodecstr hex_buf); 
        (setq nr (length hex_buf))
        (setq ret nil)
        (setq dupstr (dup "b " nr))

        (if (> (length dupstr) 1)
                (begin
                        (setq dupstr (chop dupstr 1))

                (setq upk_lst (unpack dupstr hex_buf))

                (dolist (y upk_lst)
                        (extend ret (format "%d " y))
                )

                (setq ret (chop ret 1))
                )
        )
	ret
)

(define (stringtohex str)
        (setq len (length str))
        (setq pos 0)
        (setq ret nil)
        (while (< pos len)
                (setq tmp (slice str pos 2))
                (extend ret (string (int tmp 0 16)))
                (extend ret " ")
                (++ pos) (++ pos)
        )
        (setq dup_str (dup "b " (/ len 2)))
        (setq pack_str (string "(pack \"" dup_str "\" " ret ")"))
        (setq ret (eval-string pack_str))
        ret
)
(setq msg_len 0)
(setq msg nil)
(define (handle_shake buff)
				(if
				(= buff "\r\n") 
					(begin
					(setq combine_key (string key_code websocket_guid))

					(println "combine_key: " combine_key)
					(setq sha1_code (nth 0 (exec (string "./sha1test " combine_key))))
					(println "sha1_code: " sha1_code)
					(setq accept_code (base64-enc (stringtohex sha1_code)))
					(println "accept_code: " accept_code)
					(setq ret "")
					(setq ret (append ret "HTTP/1.1 101 Switching Protocols\r\n"))
					(setq ret (append ret "Upgrade: websocket\nConnection: Upgrade\r\n"))
					(setq ret (append ret (string "Sec-WebSocket-Accept: " accept_code "\r\n")))
					(setq ret (append ret "Sec-WebSocket-Protocol: chat\r\n\r\n"))
					(print ret)
					(net-send connection ret )
					(setq ret "")
					(setq flag nil)				
					)
				(find "Sec-WebSocket-Key: " buff)
					(begin
						(setq tmplst (parse buff " "))
						(if (> (length tmplst) 1)
							(begin
								(setq key_code (chop (nth 1 tmplst) 2) )
								;(setq key_code (nth 1 tmplst))
								;(print "key_code: " key_code " " (length key_code))
							)
						)	
			
					)
				)	
)
(define (handle_data buff)
				(println "handle_data:" buff " length: " (length buff))
				(if (> (length buff) 3)
							(begin
								(setq fin (& (char (slice buff 0 1) ) 0x80))
								(setq opcode (& (char (slice buff 0 1)) 0x0f))
								(setq payload_start 2)
								(if (= fin 128)
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
												
										)
									)
								)
								(println "fin not 129:" fin)
								)
								
								(println  dataret)
								(println (hextodecstr dataret))
								(setq return_str (string dataret " back"))
								(setq dataret (ws_send return_str))
								(net-send connection dataret)
								(println (hextodecstr dataret))
								(setq dataret "")
							)
						)
)

(define (ws_send buff)
	(println buff)
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

(set 'listen (net-listen port))
(unless listen (begin
    (print "listening failed\n")
    (exit)))

(print "Waiting for connection on: " port "\n")
(while 1
	(setq connection (net-accept listen))
	(while (not (net-select connection "r" 1000)))
		(while (net-select connection "w" 1000)
			(setq buff "")
			
			(if (not (nil? flag))
				(begin
					(setq rvlen  (net-receive connection buff 1024 flag))
					(handle_shake buff)
				)
				(nil? flag)
				(begin
					(setq rvlen  (net-receive connection buff 1024))
					(extend msg  buff)
				)
			)	
			(println "peek now" (net-peek connection))
			(while (> (net-peek connection) 0)
				(println "flag: " (char flag))
			(if (not (nil? flag))
				(begin
					(setq rvlen  (net-receive connection buff 1024 flag))
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
						(handle_data buff)
					)
			)
			
			);; end this connection
			
		
	(println "close connect")
	(setq flag "\n")
	(net-close connection)	
)

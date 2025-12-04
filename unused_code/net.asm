;  A complete set of functions for handling UDP networking.
;  By Richard Cantzler, 2002
;
; $Id: net.asm,v 1.1 2002/07/31 12:34 rmc Exp $
%include "lib291.inc"
%include "ppe.inc"
;%include "net.inc"


        BITS 32

        GLOBAL _app_net_SetNetworked, _app_net_GetNetworked
        GLOBAL _app_net_Init, _app_net_Uninit, _app_net_CloseSockets, _app_net_ClearCallback
        GLOBAL _app_net_MakeServer, _app_net_MakeClient
        GLOBAL _app_net_Connect, _app_net_Disconnect
        GLOBAL _app_net_ProcessLoop, _app_net_SetupServer, _app_net_SetupClient
        GLOBAL _app_net_ReceiveData
        GLOBAL _app_net_ServerEvent, _app_net_PlayerEvent, _app_net_EnemyEvent, _app_net_ParticleEvent
        GLOBAL _TXbuf, _RXbuf

        GLOBAL SERVER_CLI_CONN, SERVER_CLI_DIS, SERVER_SRV_DIS, SERVER_SRV_AUTH
        GLOBAL PLAYER_SPAWN, PLAYER_MOVE, PLAYER_DIE
        GLOBAL ENEMY_SPAWN, ENEMY_MOVE, ENEMY_DIE
        GLOBAL PPE_SPAWN, PPE_MOVE, PPE_DIE

SECTION .data

;STRUC   SOCKADDR
;.Port           resw 1  ; Port number
;.Address        resd 1  ; 32-bit IP address
;ENDSTRUC

_isnetworked          db      0       ; enable/disable network

; network addresses/sockets
_mynum                db      0       ; 0 for server, 1-3 for client.

_listensock           dd      0
_listenaddr           dw      0,0,0   ; SOCKADDR structure
_sendsock             dd      0
_sendaddr             dw      0,0,0
_recvaddr             dw      0,0,0   ; w,d

; for server
_clientnum            db      0
_clientsock           dd      0
_clientaddr           dw      0,0,0

; packet structure : 16 dw's (dw15 ... dw0)
; dw15 = b3 b2 b1 b0
; b3 = program i.d. (for auth)
; b2 = program i.d. (for auth)
; b1 = client i.d. (for auth)
; b0 = primary data type
; dw14 ... dw0 = data

_program_id           equ     4242h    ; unique program i.d., identifies our packets

; data types
SERVER_EVENT          equ     00010000b    ; mask for server event
SERVER_CLI_CONN       equ     00010001b    ; from client to server, a client is connecting (please auth)
SERVER_CLI_DIS        equ     00010010b    ; from client to server, a client is disconnecting
SERVER_SRV_DIS        equ     00010100b    ; from server to everyone, server is disconnecting
SERVER_SRV_AUTH       equ     00011000b    ; from server to client, server has authed you!

PLAYER_EVENT          equ     00100000b    ; mask for player event
PLAYER_SPAWN          equ     00100001b    ; from server to clients, player has spawned
PLAYER_MOVE           equ     00100010b    ; from server to clients, player has moved
PLAYER_DIE            equ     00100100b    ; from server to clients, player has died

ENEMY_EVENT           equ     01000000b    ; mask for enemy event
ENEMY_SPAWN           equ     01000001b    ; from server to clients, enemy has spawned
ENEMY_MOVE            equ     01000010b    ; from server to clients, enemy has moved
ENEMY_DIE             equ     01000100b    ; from server to clients, enemy has died

PPE_EVENT             equ     10000000b    ; mask for particle events
PPE_SPAWN             equ     10000001b    ; from server to clients, particle has spawned
PPE_MOVE              equ     10000010b    ; from server to clients, particle has moved
PPE_DIE               equ     10000100b    ; from server to clients, particle has died


CR      EQU 0Dh
LF      EQU 0Ah


_MESSAGES
                      dd     .m0, (.m1-.m0)
                      dd     .m1, (.m2-.m1)
                      dd     .m2, (.m3-.m2)
                      dd     .m3, (.m4-.m3)
                      dd     .m4, (.m5-.m4)
                      dd     .m5, (.m6-.m5)
                      dd     .m6, (.m7-.m6)
                      dd     .m7, (.m8-.m7)
                      dd     .m8, (.m9-.m8)
                      dd     .m9, (.m10-.m9)
                      dd     .m10, (.m11-.m10)
                      dd     .m11, (.m12-.m11)
                      dd     .m12, (.m13-.m12)
      .m0             db     'Player # connected.',CR,LF
      .m1             db     'Player # disconnected.',CR,LF
      .m2             db     'Server disconnected.',CR,LF
      .m3             db     'Player # has been authed.',CR,LF
      .m4             db     'Player # has spawned.',CR,LF
      .m5             db     'Player # has moved.',CR,LF
      .m6             db     'Player # has died.',CR,LF
      .m7             db     'Enemy has spawned.',CR,LF
      .m8             db     'Enemy has moved.',CR,LF
      .m9             db     'Enemy has died.',CR,LF
      .m10            db     'Particle has spawned.',CR,LF
      .m11            db     'Particle has moved.',CR,LF
      .m12            db     'Particle has died.',CR,LF
      .m13

MSG_CLIENT_CONNECT        equ     0*8
MSG_CLIENT_CONNECT_LEN    equ     0*8 + 4
MSG_CLIENT_DISCONNECT     equ     1*8
MSG_CLIENT_DISCONNECT_LEN equ     1*8 + 4
MSG_SERVER_DISCONNECT     equ     2*8
MSG_SERVER_DISCONNECT_LEN equ     2*8 + 4
MSG_CLIENT_AUTH           equ     3*8
MSG_CLIENT_AUTH_LEN       equ     3*8 + 4
MSG_PLAYER_SPAWN          equ     4*8
MSG_PLAYER_SPAWN_LEN      equ     4*8 + 4
MSG_PLAYER_MOVE           equ     5*8
MSG_PLAYER_MOVE_LEN       equ     5*8 + 4
MSG_PLAYER_DIE            equ     6*8
MSG_PLAYER_DIE_LEN        equ     6*8 + 4
MSG_ENEMY_SPAWN           equ     7*8
MSG_ENEMY_SPAWN_LEN       equ     7*8 + 4
MSG_ENEMY_MOVE            equ     8*8
MSG_ENEMY_MOVE_LEN        equ     8*8 + 4
MSG_ENEMY_DIE             equ     9*8
MSG_ENEMY_DIE_LEN         equ     9*8 + 4
MSG_PPE_SPAWN             equ     10*8
MSG_PPE_SPAWN_LEN         equ     10*8 + 4
MSG_PPE_MOVE              equ     11*8
MSG_PPE_MOVE_LEN          equ     11*8 + 4
MSG_PPE_DIE               equ     12*8
MSG_PPE_DIE_LEN           equ     12*8 + 4



; for client & server
_serverport		dw	3039h			; port number (host order)
_clientport		dw	3029h			; port number (host order)

; -------------------------------------------------------------------------------------------------

SECTION .bss

; for client & server
_socket		resd	1
_gotdatagram	resb	1

buf_len		equ	16*1024         ; max size
_TXbuf		resb	buf_len
_RXbuf		resb	buf_len

_isclient       resb    1
_isserver       resb    1


; -------------------------------------------------------------------------------------------------

SECTION .text


;;;
;;; (void) _app_net_SetNetworked (unsigned int enable)
;;;  Inputs:  enable tells us whether to enable the network, or not.
;;; Outputs:  sets [_isnetworked] to 1 if it is networked, 0 if not.
;;; Purpose:  This function allows us to run in networked/non-networked modes.
;;;   Calls:  None.
;;;
proc _app_net_SetNetworked
.enable         arg       4
        mov     eax, [ebp + .enable]
        mov     [_isnetworked], al

        ret
endproc
;_app_net_SetNetworked_arglen   equ 4

; -------------------------------------------------------------------------------------------------

;;;
;;; (unsigned int event) _app_net_GetNetworked (void)
;;;  Inputs:  None.
;;; Outputs:  Sets event (eax) to 1 if it is networked, 0 if not.
;;; Purpose:  This function allows us to tell if we're in networked/non-networked modes.
;;;   Calls:  None.
;;;
proc _app_net_GetNetworked
        xor     eax, eax
        mov     al, [_isnetworked]

        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_Init (void)
;;;  Inputs:  None
;;; Outputs:  None
;;; Purpose:  This function allocates a socket for the main network communication.
;;;   Calls:  _InitSocket (lib), _Socket_create (lib)
;;;
proc _app_net_Init
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .error

	; Initialize the socket library
	call	_InitSocket
	test	eax, eax
	jnz	short .error

	; Create a tcp socket for listening to data, (0.0.0.0)
	invoke	_Socket_create, dword SOCK_DGRAM
	test	eax, eax
	js	short .bad_sock
        mov	[_listensock], eax

        mov     eax, 1
        ret
.bad_sock
        call	_ExitSocket
.error
        xor     eax, eax
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_Uninit (void)
;;;  Inputs:  None, assumes _app_net_Init has been called
;;; Outputs:  None
;;; Purpose:  This function deallocates the socket for the main network communication.
;;;   Calls:  _ExitSocket (lib)
;;;
proc _app_net_Uninit
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .notnet

        call	_ExitSocket
.notnet
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_CloseSockets (void)
;;;  Inputs:  None
;;; Outputs:  None
;;; Purpose:  This function closes all of the server and client sockets.
;;;   Calls:  _Socket_close (lib)
;;;
proc _app_net_CloseSockets
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .notnet

	invoke	_Socket_close, dword [_listensock]
        invoke  _Socket_close, dword [_sendsock]     ; if client?
	invoke	_Socket_close, dword [_clientsock]   ; if server?
.notnet
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_ClearCallback (void)
;;;  Inputs:  None, assumes we bound a socket (where AddCallback is internally called).
;;; Outputs:  None
;;; Purpose:  This function clears our socket callback.
;;;   Calls:  _Socket_SetCallback (lib)
;;;
proc _app_net_ClearCallback
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .notnet

        invoke	_Socket_SetCallback, dword 0
.notnet
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_MakeServer (void)
;;;  Inputs:  None
;;; Outputs:  None
;;; Purpose:  This function sets the private variable letting us know to run as a server.
;;;   Calls:  None
;;;
proc _app_net_MakeServer
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .notnet

        mov     byte [_isserver], 1
        mov     byte [_isclient], 0
.notnet
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_MakeClient (void)
;;;  Inputs:  None
;;; Outputs:  None
;;; Purpose:  This function sets the private variable letting us know to run as a client.
;;;   Calls:  None
;;;
proc _app_net_MakeClient
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .notnet

        mov     byte [_isserver], 0
        mov     byte [_isclient], 1
.notnet
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_Connect (void)
;;;  Inputs:  None
;;; Outputs:  Sends SERVER_CLI_CONN packet to server if we're a client.
;;; Purpose:  This function allows us to connect to the server.
;;;   Calls:  _SendData_to_Server (net.asm)
;;;
proc _app_net_Connect
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .close

        ; auth with server (if client)
        cmp     byte [_isclient], 1
        jne     short .close

        ; send auth
        mov     byte [_mynum], 0

        mov     word [_TXbuf], _program_id
        mov     byte [_TXbuf + 2], 0                 ; request connection (client# must be 0)
        mov     byte [_TXbuf + 3], SERVER_CLI_CONN

        invoke  _SendData_to_Server, dword _TXbuf, dword 4

.close
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_Disconnect (void)
;;;  Inputs:  None
;;; Outputs:  Sends SERVER_CLI_DIS/SERVER_SRV_DIS packet depending on who we are.
;;; Purpose:  This function allows us to disconnect.
;;;   Calls:  _SendData_to_Server (net.asm), _SendData_to_Clients (net.asm)
;;;
proc _app_net_Disconnect
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .close

        cmp     byte [_isserver], 1
        jne     short .did_client_quit

        ; server quit
;        invoke  _app_net_ServerEvent, dword SERVER_SRV_DIS
        mov     word [_TXbuf], _program_id
        mov     al, byte [_mynum]
        mov     byte [_TXbuf + 2], al
        mov     byte [_TXbuf + 3], SERVER_SRV_DIS
        mov     byte [_TXbuf + 4], '!'
        invoke  _SendData_to_Clients, dword _TXbuf, dword 5

.did_client_quit
        cmp     byte [_isclient], 1
        jne     short .close

        ; client quit
;        invoke  _app_net_ServerEvent, dword SERVER_CLI_DIS
        mov     word [_TXbuf], _program_id
        mov     al, byte [_mynum]
        mov     byte [_TXbuf + 2], al
        mov     byte [_TXbuf + 3], SERVER_CLI_DIS
        mov     byte [_TXbuf + 4], '!'
        invoke  _SendData_to_Server, dword _TXbuf, dword 5

.close
        ret
endproc

; -------------------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_ProcessLoop (void)
;;;  Inputs:  None
;;; Outputs:  None
;;; Purpose:  This function processess the data coming in and going out (prints incoming data to screen).
;;;   Calls:  _SendData_to_Clients (net.asm), _SendData_to_Server (net.asm)
;;;
proc _app_net_ProcessLoop
        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     near .close

        ; _app_net_Connect
.close
        ret
endproc

; -------------------------------------------------------------------------------------

;;;
;;; (void) _SocketHandler (unsigned int Socket, unsigned int Event)
;;;  Inputs:  Socket is a socket number, Event is an event number.
;;; Outputs:  None
;;; Purpose:  This function sets the internal flag saying data has been received.
;;;   Calls:  None
;;;
proc _SocketHandler
.Socket		arg	4
.Event		arg	4

	; make sure we're getting an event on the socket we're interested in!
	; switch for each client/server socket
	mov	eax, [ebp+.Socket]
	cmp	eax, [_listensock]
	jne	short .done

        mov	eax, [ebp+.Event]
	cmp	eax, SOCKEVENT_READ
	jne	short .done

	mov	byte [_gotdatagram], 1
.done:
	ret
endproc
_SocketHandler_end

; -------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _ListenNet (unsigned int socket, SOCKADDR* name)
;;;  Inputs:  socket is a socket to listen on.
;;;           name is a pointer to a SOCKADDR structure containing the ipaddress and port to listen on.
;;; Outputs:  1 if executed smoothly, 0 on error
;;; Purpose:  This function allows us to listen to incoming data.
;;;   Calls:  _Socket_bind (lib)
;;;
proc _ListenNet
.socket        arg         4
.name          arg         4
	; Bind socket to that address and port
	invoke	_Socket_bind, dword [ebp + .socket], dword [ebp + .name]
	test	eax, eax
	jnz	near .error

	; Install a callback for incoming datagrams on the socket
	invoke	_LockArea, ds, dword _isclient, dword 1
	invoke	_LockArea, ds, dword _isserver, dword 1
	invoke	_LockArea, ds, dword _gotdatagram, dword 1
	invoke	_LockArea, cs, dword _SocketHandler, dword _SocketHandler_end-_SocketHandler
	invoke	_Socket_SetCallback, dword _SocketHandler
	test	eax, eax
	jnz	short .error

	invoke	_Socket_AddCallback, dword [ebp + .socket], dword SOCKEVENT_READ
	test	eax, eax
	jnz	short .error

        mov     eax, 1
        ret
.error
        xor     eax, eax      ; error
        ret
endproc
_ListenNet_arglen    equ      4

; -------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _app_net_SetupClient (char* srv_ipaddr_ptr, char* srv_port_ptr)
;;;  Inputs:  srv_ipaddr_ptr is a pointer to a string containing the server ipaddress.
;;;           srv_port_ptr is a pointer to a string containing the server port.
;;; Outputs:  1 if executed smoothly, 0 on error
;;; Purpose:  This function sets up the private data for connecting to the server.
;;;   Calls:  _Socket_htons (lib), _Socket_inet_addr (lib), _Socket_gethostbyaddr (lib)
;;;
proc _app_net_SetupClient
.srv_ipaddr_ptr    arg     4
.srv_port_ptr      arg     4

        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     near .error

        ; setup socket to send data on
	invoke	_Socket_create, dword SOCK_DGRAM
	test	eax, eax
	js	near .error
        mov	[_sendsock], eax

	; Set up address structure:
	;  First the port
;	mov     eax, [ebp + .srv_port_ptr]
	mov     eax, _serverport
	invoke	_Socket_htons, word [eax]
	mov	[_sendaddr+SOCKADDR.Port], ax

	mov	eax, [ebp + .srv_ipaddr_ptr]		; Get pointer to ipaddress
	invoke  _Socket_inet_addr, eax


	invoke	_Socket_gethostbyaddr, eax	; use .ipaddr
	test	eax, eax
	jz	near .error
	mov	eax, [eax+HOSTENT.AddrList]	; Get pointer to address list
	mov	eax, [eax]			; Get pointer to first address
	test	eax, eax			; Valid pointer?
	jz	near .error
	mov	eax, [eax]			; Get first address

	mov	[_sendaddr+SOCKADDR.Address], eax


	; Set up address structure:
	;  First the port
	invoke	_Socket_htons, word [_clientport]
	mov	[_listenaddr+SOCKADDR.Port], ax
	;  Then the address - INADDR_ANY means any address (don't care)
	mov	dword [_listenaddr+SOCKADDR.Address], INADDR_ANY

        ; bind socket to listen for server
        invoke  _ListenNet, dword [_listensock], dword _listenaddr
	test	eax, eax
	jz	short .error

        mov     eax, 1

        ret
.error
        xor     eax, eax	; error on send
        ret
endproc
;_SetupClient_arglen   equ   8

; -------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _app_net_SetupServer (void)
;;;  Inputs:  None
;;; Outputs:  1 if executed smoothly, 0 on error
;;; Purpose:  This function sets up the private data for listening to packets.
;;;           Also sets the socket Callback.
;;;   Calls:  _Socket_htons (lib), _Socket_bind (lib), _Socket_SetCallback (lib)
;;;           _LockArea (lib), _Socket_AddCallback (lib)
;;;
proc _app_net_SetupServer

        ; make sure in networked mode
        cmp     byte [_isnetworked], 1
        jne     short .error

	; Set up address structure:
	;  First the port
	invoke	_Socket_htons, word [_serverport]
	mov	[_listenaddr+SOCKADDR.Port], ax
	;  Then the address - INADDR_ANY means any address (don't care)
	mov	dword [_listenaddr+SOCKADDR.Address], INADDR_ANY

        invoke  _ListenNet, dword [_listensock], dword _listenaddr
	test	eax, eax
	jz	short .error

        mov     eax, 1
        ret
.error
        xor     eax, eax      ; error
        ret
endproc

; -------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _SendData_to_Server (char* ptrData, unsigned int Data_len)
;;;  Inputs:  ptrData is a pointer to the data to be sent.  Data_len is the length of said data.
;;; Outputs:  1 if executed smoothly, 0 on error
;;; Purpose:  This function sends data pointed to by ptrData to the server.
;;;   Calls:  _Socket_sendto (lib)
;;;
proc _SendData_to_Server
.ptrData          arg      4
.Data_len         arg      4

	invoke	_Socket_sendto, dword [_sendsock], dword [ebp + .ptrData], dword [ebp + .Data_len], dword 0, dword _sendaddr
	test	eax, eax
        js      short .error

        mov     eax, 1
        ret
.error
        xor     eax, eax    ; error
        ret
endproc
_SendData_to_Server_arglen   equ   8

; ---------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _SendData_to_Clients (char* ptrData, unsigned int Data_len)
;;;  Inputs:  ptrData is a pointer to the data to be sent.  Data_len is the length of said data.
;;; Outputs:  1 if executed smoothly, 0 on error.
;;; Purpose:  This function sends data pointed to by ptrData to all the clients.
;;;   Calls:  _Socket_sendto (lib)
;;;
proc _SendData_to_Clients
.ptrData          arg      4
.Data_len         arg      4

	invoke	_Socket_sendto, dword [_clientsock], dword [ebp + .ptrData], dword [ebp + .Data_len], dword 0, dword _clientaddr
	test	eax, eax
        js      short .error

        mov     eax, 1
        ret
.error
        xor     eax, eax    ; error
        ret
endproc
_SendData_to_Clients_arglen   equ   8

; --------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _app_net_ReceiveData (void)
;;;  Inputs:  None
;;; Outputs:  Received data in _RXBuf
;;; Purpose:  This function puts the incoming data into the buffer _RXbuf.
;;;           It also stores the senders data into _recvaddr
;;;   Calls:  _Socket_recvfrom (lib)
;;;
proc _app_net_ReceiveData

        ; is there incoming data?
        cmp	byte [_gotdatagram], 1
	jne	.server_notdone

        ; data has arrived
	mov	byte [_gotdatagram], 0

       	invoke	_Socket_recvfrom, dword [_listensock], dword _RXbuf, dword buf_len-1, dword 0, dword _recvaddr
	test	eax, eax
	js	short .server_notdone	; error on receive
	jz	short .server_notdone	; no data?

        invoke  _ProcessReceived, dword eax

        ret
.server_notdone
        xor     eax, eax    ; error
        ret
endproc
;_app_net_ReceiveData_arglen   equ   8

; --------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _ProcessReceived (unsigned int BufLen)
;;;  Inputs:  Received data in _RXbuf, address in _recvaddr,
;;;           BufLen is the number of bytes in the buffer to process.
;;; Outputs:  Performs operations specified by received data.
;;; Purpose:  This function gets the data in buffer _RXbuf,
;;;           and process it appropriately.
;;;   Calls:  _SendData_to_Clients
;;;
proc _ProcessReceived
.BufLen              arg  4

        ; make sure the packet has the right form . . .
	cmp     dword [ebp + .BufLen], 4
	jb      near .Exit                   ; if the length is too short, skip

        mov     eax, [_RXbuf]           ; get packet header : al ah eal eah
        mov     edx, eax
        shr     edx, 16                 ; high word in ax, low word in dx : al ah dl dh

        cmp     ax, _program_id
        jne     near .Exit                   ; if the unique i.d. is not right . . . invalid packet.

        ; make sure client # is acceptable.
        ; in client mode, clientnum=0
        cmp     dl, [_clientnum]
        jg      near .Exit                   ; not authorized client
        cmp     dl, 0
        jl      near .Exit                   ; not authorized client (auth with client # = 0)

        cmp     byte [_isserver], 1
        jne     near .client

; ----------------------------------------- SERVER --------------------------------------------
        ; now we're dealing with strictly SERVER data cases

        cmp     dl, 0
        jne     .FromCurrentClient          ; if client number is zero (needs auth)

        test    dh, SERVER_EVENT
        jz      near .Exit
        cmp     dh, SERVER_CLI_CONN
        jne     near .Exit                   ; if client # = 0 and not asking for CONN, ignore

        mov al, '0'
        mov byte [_RXbuf + 2], al

        call    _AddClient              ; if client # = 0 and is asking for CONN, add client
        test    eax, eax
        jz      near .Exit

        ; send authorization acknowledgement
        mov     word [_TXbuf], _program_id
        mov     byte [_TXbuf + 2], 0
        mov     byte [_TXbuf + 3], SERVER_SRV_AUTH

        mov     byte [_TXbuf + 4], 1              ; new client number
        invoke	_SendData_to_Clients, dword _TXbuf, dword 5

        mov     byte [_RXbuf + 2], 'C'
        mov     byte [_RXbuf + 3], 'C'

        jmp     near .DoneProcessing
.FromCurrentClient

        test    dh, SERVER_EVENT
        jz      near .client_player_event   ; we process player/enemy/ppe data the same way for client & server

        cmp     dh, SERVER_CLI_DIS
        jne     near .Exit                  ; was server event, but not one we care about . . .

        mov     byte [_RXbuf + 2], 'C'
        mov     byte [_RXbuf + 3], 'D'
        mov     byte [_clientnum], 0
        jmp     near .DoneProcessing

; ----------------------------------------- CLIENT --------------------------------------------

.client
        ; deal with all data cases

        ; did we receive server authentication?
        test    dh, SERVER_EVENT         ; begin SERVER_EVENT
        jz      near .client_player_event

        cmp     dh, SERVER_SRV_AUTH
        jne     short .client_srv_dis

        mov     al, [_RXbuf + 4]
        mov     [_mynum], al
        mov     byte [_RXbuf + 2], 'S'
        mov     byte [_RXbuf + 3], 'A'
        jmp     near .DoneProcessing
.client_srv_dis
        ; did the server disconnect?
        cmp     dh, SERVER_SRV_DIS
        jne     near .Exit               ; was server event , no matching server events though . . .

        mov     byte [_RXbuf + 2], 'S'
        mov     byte [_RXbuf + 3], 'D'
        jmp     near .DoneProcessing
.client_player_event                     ; begin PLAYER_EVENT
        test    dh, PLAYER_EVENT
        jz      near .client_enemy_event

        cmp     dh, PLAYER_SPAWN
        jne     short .client_player_move

        mov     byte [_RXbuf + 2], 'P'
        mov     byte [_RXbuf + 3], 'S'
        jmp     near .DoneProcessing
.client_player_move
        cmp     dh, PLAYER_MOVE
        jne     short .client_player_die

        mov     byte [_RXbuf + 2], 'P'
        mov     byte [_RXbuf + 3], 'M'
        jmp     near .DoneProcessing
.client_player_die
        cmp     dh, PLAYER_DIE
        jne     near .Exit              ; was player event, no matching player events though . . .

        mov     byte [_RXbuf + 2], 'P'
        mov     byte [_RXbuf + 3], 'D'
        jmp     near .DoneProcessing
.client_enemy_event                     ; begin ENEMY_EVENT
        test    dh, ENEMY_EVENT
        jz      near .client_ppe_event

        cmp     dh, ENEMY_SPAWN
        jne     short .client_enemy_move

        mov     byte [_RXbuf + 2], 'E'
        mov     byte [_RXbuf + 3], 'S'
        jmp     near .DoneProcessing
.client_enemy_move
        cmp     dh, ENEMY_MOVE
        jne     short .client_enemy_die

        mov     byte [_RXbuf + 2], 'E'
        mov     byte [_RXbuf + 3], 'M'
        jmp     near .DoneProcessing
.client_enemy_die
        cmp     dh, ENEMY_DIE
        jne     near .Exit              ; was enemy event, no matching enemy events though . . .

        mov     byte [_RXbuf + 2], 'E'
        mov     byte [_RXbuf + 3], 'D'
        jmp     near .DoneProcessing
.client_ppe_event                       ; begin PPE_EVENT
        test    dh, PPE_EVENT
        jz      near .Exit              ; didn't match any event types . . .

        cmp     dh, PPE_SPAWN
        jne     short .client_ppe_move

;        mov     byte [_RXbuf + 2], 'o'
;        mov     byte [_RXbuf + 3], 'S'

        invoke  _AddParticle, dword 0, dword [_RXbuf + 8], dword [_RXbuf + 12], dword [_RXbuf + 16], dword [_RXbuf + 20], dword [_RXbuf + 24], dword [_RXbuf + 28], dword [_RXbuf + 32], dword [_RXbuf + 36]
        
        jmp     near .DoneProcessing
.client_ppe_move
        cmp     dh, PPE_MOVE
        jne     short .client_ppe_die

        mov     byte [_RXbuf + 2], 'o'
        mov     byte [_RXbuf + 3], 'M'
        jmp     near .DoneProcessing
.client_ppe_die
        cmp     dh, PPE_DIE
        jne     near .Exit              ; was ppe event, no matching ppe events though . . .

        mov     byte [_RXbuf + 2], 'o'
        mov     byte [_RXbuf + 3], 'D'
        jmp     near .DoneProcessing

; ----------------------------------------- DONE PROCESSING -----------------------------------

.DoneProcessing
jmp     .SkipPrint
	; Print out the data
	mov	ecx, [ebp + .BufLen]
	mov     ecx, 5
;	xor	ebx, ebx
        mov     ebx, 2                           ; start printing on third byte
.recvdata_loop_print:
        mov     ah, 06h
	mov	dl, [_RXbuf+ebx]
	int	21h

	inc	ebx
	cmp	ebx, ecx
	jb	short .recvdata_loop_print

        ; print endline
        mov     ah, 06h
        mov     dl, CR
        int     21h
        mov     dl, LF
        int     21h

.SkipPrint


.Exit

        ret
endproc
_ProcessReceived_arglen   equ   4

; --------------------------------------------------------------------------------------

;;;
;;; (unsigned int) _AddClient (void)
;;;  Inputs:  None
;;; Outputs:  Sets up new client (address and socket).
;;; Purpose:  This function allows us to setup client sockets
;;;           so the server can communicate with the clients.
;;;   Calls:  _Socket_htons (lib), _Socket_create (lib)
;;;
proc _AddClient

        mov     al, [_clientnum]
        cmp     al, 1
        jge     .error                        ; clients_full

        ; update client #
        inc     byte [_clientnum]                            ; will be new client number

        ; store clients address
        mov     eax, [_recvaddr + SOCKADDR.Address]
        mov     [_clientaddr + SOCKADDR.Address], eax

        invoke	_Socket_htons, word [_clientport]
        mov     [_clientaddr + SOCKADDR.Port], ax

        ; open socket to send to client
	invoke	_Socket_create, dword SOCK_DGRAM
	test	eax, eax
        js	near .error                   ; bad_sock

	mov	[_clientsock], eax


        mov     eax, 1
        ret
.error
        xor     eax, eax    ; error
        ret
endproc

; --------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_ServerEvent (unsigned int event)
;;;  Inputs:  event is an event type (CLI_CONN, SRV_AUTH, CLI_DIS, SRV_DIS).
;;; Outputs:  Sends the server event data to Client/Server depending on who we are.
;;; Purpose:  This function allows us to connect to the client/server.
;;;   Calls:  _SendData_to_Clients (net.asm), _SendData_to_Server (net.asm)
;;;
proc _app_net_ServerEvent
.event                     arg     4
        cmp     byte [_isnetworked], 1
        jne     near .Exit

        ; which event?
        mov     eax, [ebp + .event]


        ; set packet up
        mov     word [_TXbuf], _program_id
        mov     bl, [_mynum]
        mov     byte [_TXbuf + 2], bl
        mov     byte [_TXbuf + 3], al
        mov     byte [_TXbuf + 4], 0

        cmp     byte [_isserver], 1
        jne     short .weRclient

        ; send event to client (cause we are a server)
        invoke	_SendData_to_Clients, dword _TXbuf, dword 5

.weRclient
        cmp     byte [_isclient], 1
        jne     short .Exit

        ; send event to server (cause we are a client)
        invoke	_SendData_to_Server, dword _TXbuf, dword 5

.Exit
        ret
endproc
;_app_net_ServerEvent_arglen equ 4

; --------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_PlayerEvent (PLAYER* stud, unsigned int event)
;;;  Inputs:  stud is a pointer to a PLAYER structure.
;;;           event is an event type (PLAYER_SPAWN/PLAYER_MOVE/PLAYER_DIE).
;;; Outputs:  Sends the player data to Client/Server depending on who we are.
;;; Purpose:  This function allows us to send player data back and forth.
;;;   Calls:  _SendData_to_Clients (net.asm), _SendData_to_Server (net.asm)
;;;
proc _app_net_PlayerEvent
.stud                      arg     4
.event                     arg     4
        cmp     byte [_isnetworked], 1
        jne     near .Exit

        ; which event?
        mov     eax, [ebp + .event]
        mov     al, PLAYER_MOVE


        ; set packet up
        mov     word [_TXbuf], _program_id
        mov     bl, [_mynum]
        mov     byte [_TXbuf + 2], bl
        mov     byte [_TXbuf + 3], al
        ;mov     byte [_TXbuf + 4], 0             ; [ebp + .stud]

        cmp     byte [_isserver], 1
        jne     short .weRclient

        ; send player data to client (cause we are a server)
        invoke	_SendData_to_Clients, dword _TXbuf, dword 5

.weRclient
        cmp     byte [_isclient], 1
        jne     short .Exit

        ; send player data to server (cause we are a client)
        invoke	_SendData_to_Server, dword _TXbuf, dword 5

.Exit
        ret
endproc
;_app_net_PlayerEvent_arglen equ 8

; --------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_EnemyEvent (ENEMY* wuss, unsigned int event)
;;;  Inputs:  wuss is a pointer to an ENEMY structure.
;;;           event is an event type (ENEMY_SPAWN/ENEMY_MOVE/ENEMY_DIE).
;;; Outputs:  Sends the enemy data to Client/Server depending on who we are.
;;; Purpose:  This function allows us to send enemy data back and forth.
;;;   Calls:  _SendData_to_Clients (net.asm), _SendData_to_Server (net.asm)
;;;
proc _app_net_EnemyEvent
.wuss                      arg     4
.event                     arg     4
        cmp     byte [_isnetworked], 1
        jne     near .Exit

        ; which event?
        mov     eax, [ebp + .event]
        mov     al, PLAYER_MOVE


        ; set packet up
        mov     word [_TXbuf], _program_id
        mov     bl, [_mynum]
        mov     byte [_TXbuf + 2], bl
        mov     byte [_TXbuf + 3], al
        ;mov     byte [_TXbuf + 4], 0             ; [ebp + .wuss]

        cmp     byte [_isserver], 1
        jne     short .weRclient

        ; send enemy data to client (cause we are a server)
        invoke	_SendData_to_Clients, dword _TXbuf, dword 5

.weRclient
        cmp     byte [_isclient], 1
        jne     short .Exit

        ; send enemy data to server (cause we are a client)
        invoke	_SendData_to_Server, dword _TXbuf, dword 5

.Exit
        ret
endproc
;_app_net_EnemyEvent_arglen equ 8

; --------------------------------------------------------------------------------------

;;;
;;; (void) _app_net_ParticleEvent (PARTICLE* rock, unsigned int event)
;;;  Inputs:  rock is a pointer to a PARTICLE structure.
;;;           event is an event type (Spawn/Move/Del).
;;; Outputs:  Sends the particle data to Client/Server depending on who we are.
;;; Purpose:  This function allows us to send particle data back and forth.
;;;   Calls:  _SendData_to_Clients (net.asm), _SendData_to_Server (net.asm)
;;;
proc _app_net_ParticleEvent
.rock                      arg     4
.event                     arg     4
        cmp     byte [_isnetworked], 1
        jne     near .Exit

        ; which event?
        mov     eax, [ebp + .event]

        ; set packet up
        mov     word [_TXbuf], _program_id
        mov     bl, [_mynum]
        mov     byte [_TXbuf + 2], bl
        mov     byte [_TXbuf + 3], al
       ; mov     byte [_TXbuf + 4], 0             ; [ebp + .rock]

        cmp     byte [_isserver], 1
        jne     short .weRclient

        ; send particle to client (cause we are a server)
        invoke	_SendData_to_Clients, dword _TXbuf, dword 40

.weRclient
        cmp     byte [_isclient], 1
        jne     short .Exit

        ; send particle to server (cause we are a client)
        invoke	_SendData_to_Server, dword _TXbuf, dword 40

.Exit
        ret
endproc
;_app_net_ParticleEvent_arglen equ 8



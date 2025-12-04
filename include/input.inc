
		  EXTERN _UpdateInput
                  EXTERN _FlushKeyboard

  		  EXTERN _KEYBOARD
		  EXTERN _MOUSE_X
		  EXTERN _MOUSE_Y
		  EXTERN _MOUSE_LBUTTON
		  EXTERN _MOUSE_RBUTTON
		  EXTERN _MOUSE_MBUTTON
		  EXTERN _MOUSE_BUTTON

                  EXTERN _QUIT_SIGNAL
		
		  SDLM_BUTTON1			EQU	    1
		  SDLM_BUTTON2			EQU	    2
		  SDLM_BUTTON3			EQU	    4		  		

		  SDLK_UNKNOWN                	EQU         0
		  SDLK_FIRST		    	EQU         0
		  SDLK_BACKSPACE		EQU         8
		  SDLK_TAB      		EQU         9
		  SDLK_CLEAR    		EQU         12
		  SDLK_RETURN   		EQU         13
		  SDLK_PAUSE    		EQU         19
		  SDLK_ESCAPE   		EQU         27
		  SDLK_SPACE    		EQU         32
		  SDLK_EXCLAIM		        EQU         33
		  SDLK_QUOTEDBL		        EQU         34
		  SDLK_HASH		        EQU         35
		  SDLK_DOLLAR		        EQU         36
		  SDLK_AMPERSAND		EQU         38
		  SDLK_QUOTE		        EQU         39
		  SDLK_LEFTPAREN		EQU         40
		  SDLK_RIGHTPAREN		EQU         41
		  SDLK_ASTERISK		        EQU         42
		  SDLK_PLUS		        EQU         43
		  SDLK_COMMA		        EQU         44
		  SDLK_MINUS		        EQU         45
		  SDLK_PERIOD		        EQU         46
		  SDLK_SLASH		       	EQU         47
		  SDLK_0			EQU         48
		  SDLK_1			EQU         49
		  SDLK_2			EQU         50
		  SDLK_3			EQU         51
		  SDLK_4			EQU         52
		  SDLK_5			EQU	    53
		  SDLK_6			EQU         54
		  SDLK_7			EQU         55
		  SDLK_8			EQU	    56
		  SDLK_9			EQU         57
		  SDLK_COLON		        EQU         58
		  SDLK_SEMICOLON		EQU         59
		  SDLK_LESS		        EQU         60
		  SDLK_EQUALS		        EQU         61
		  SDLK_GREATER		        EQU         62
		  SDLK_QUESTION		        EQU         63
		  SDLK_AT			EQU         64
		  SDLK_LEFTBRACKET	        EQU         91
		  SDLK_BACKSLASH		EQU         92
		  SDLK_RIGHTBRACKET	        EQU         93
		  SDLK_CARET		       	EQU         94
		  SDLK_UNDERSCORE		EQU         95
		  SDLK_BACKQUOTE		EQU         96
		  SDLK_a			EQU	    97
		  SDLK_b			EQU         98
		  SDLK_c			EQU	    99
		  SDLK_d			EQU         100
		  SDLK_e			EQU         101
		  SDLK_f			EQU         102
		  SDLK_g			EQU         103
		  SDLK_h			EQU         104
		  SDLK_i			EQU	    105
		  SDLK_j			EQU         106
		  SDLK_k			EQU         107
		  SDLK_l			EQU	    108
		  SDLK_m			EQU         109
		  SDLK_n			EQU	    110
		  SDLK_o			EQU         111
		  SDLK_p			EQU         112
		  SDLK_q			EQU         113
		  SDLK_r			EQU	    114
		  SDLK_s			EQU         115
		  SDLK_t			EQU         116
		  SDLK_u			EQU         117
		  SDLK_v			EQU         118
		  SDLK_w			EQU         119
		  SDLK_x			EQU         120
		  SDLK_y			EQU         121
		  SDLK_z			EQU         122
		  SDLK_DELETE			EQU         127
		  SDLK_KP0			EQU         256
		  SDLK_KP1			EQU         257
		  SDLK_KP2			EQU         258
		  SDLK_KP3			EQU         259
		  SDLK_KP4			EQU         260
		  SDLK_KP5			EQU         261
		  SDLK_KP6			EQU         262
		  SDLK_KP7		 	EQU         263
		  SDLK_KP8		      	EQU    	    264
		  SDLK_KP9			EQU	    265
		  SDLK_KP_PERIOD		EQU         266
		  SDLK_KP_DIVIDE		EQU         267
		  SDLK_KP_MULTIPLY	        EQU         268
		  SDLK_KP_MINUS		        EQU         269
		  SDLK_KP_PLUS		        EQU         270
		  SDLK_KP_ENTER		        EQU         271
		  SDLK_KP_EQUALS	        EQU         272
		  SDLK_UP			EQU         273
		  SDLK_DOWN		    	EQU         274
		  SDLK_RIGHT		 	EQU         275
		  SDLK_LEFT		 	EQU         276
		  SDLK_INSERT		     	EQU         277
		  SDLK_HOME			EQU         278
		  SDLK_END			EQU         279
		  SDLK_PAGEUP	               	EQU         280
		  SDLK_PAGEDOWN	                EQU         281
		  SDLK_F1			EQU         282
		  SDLK_F2			EQU         283
		  SDLK_F3			EQU         284
		  SDLK_F4			EQU         285
		  SDLK_F5			EQU         286
		  SDLK_F6			EQU         287
		  SDLK_F7			EQU         288
		  SDLK_F8			EQU         289
		  SDLK_F9			EQU         290
		  SDLK_F10		    	EQU         291
		  SDLK_F11		    	EQU         292
		  SDLK_F12		    	EQU         293
		  SDLK_F13		        EQU	    294
		  SDLK_F14		        EQU         295
		  SDLK_F15			EQU         296
		  SDLK_NUMLOCK	            	EQU         300
		  SDLK_CAPSLOCK	                EQU         301
		  SDLK_SCROLLOCK               	EQU         302
		  SDLK_RSHIFT	      		EQU         303
		  SDLK_LSHIFT	       		EQU         304
		  SDLK_RCTRL	       		EQU         305
		  SDLK_LCTRL			EQU         306
		  SDLK_RALT			EQU         307
		  SDLK_LALT			EQU         308
		  SDLK_RMETA			EQU         309
		  SDLK_LMETA			EQU         310
		  SDLK_LSUPER			EQU         311		
		  SDLK_RSUPER			EQU         312		
		  SDLK_MODE			EQU         313		
		  SDLK_COMPOSE	              	EQU         314		
		  SDLK_HELP			EQU         315
		  SDLK_PRINT			EQU         316
		  SDLK_SYSREQ			EQU         317
		  SDLK_BREAK			EQU         318
		  SDLK_MENU		       	EQU         319


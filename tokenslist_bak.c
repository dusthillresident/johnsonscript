// DO NOT EDIT THIS, instead edit tokenslist_src.c


#define t_nul		0
#define t_plus		1	// 	+		built in function: addition.		takes at least 2 parameters
#define t_minus		2	//	-		built in function: subtraction.		takes at least 2 parameters
#define t_slash		3	// 	/		built in function: division.		takes at least 2 parameters
#define t_star		4	//	*		built in function: multiplication.	takes at least 2 parameters
#define t_mod		5	//	%		built in function: modulo.		takes 2 parameters
#define t_shiftleft	6	//	<<		built in function: shift left.		takes 2 parameters
#define t_shiftright	7	//	>>		built in function: shift right.		takes 2 parameters
#define t_lshiftright	8	//	>>>		built in function: logical shift right.	takes 2 parameters
#define t_and		9	//	&		built in function: bitwise and.		takes at least 2 parameters
#define t_or		10	//	|		built in function: bitwise or.		takes at least 2 parameters
#define t_eor		11	//	^		built in function: bitwise exclusive or. takes at least 2 parameters
#define t_not		12	//	~		built in function: bitwise not.		takes 1 parameter

#define t_abs		13	//	abs		built in function: absolute value	takes 1 parameter
#define t_int		14	//	int		built in function: integer		takes 1 parameter
#define t_sgn		15	//	sgn		built in function: signum		takes 1 parameter
#define t_neg		16	//	neg		built in function: negative		takes 1 parameter

#define t_lessthan	17	//	<		built in function: less than		takes 2 parameters
#define t_morethan	18	//	>		built in function: greater than		takes 2 parameters
#define t_lesseq	19	//	<=		built in function: less than or equal	takes 2 parameters
#define t_moreeq	20	//	>=		built in function: greater than or eq.	takes 2 parameters
#define t_equal		21	//	=		built in function: equal		takes 2 parameters

#define t_land		22	//	&&		built in function: logical and		takes at least 2 parameters
#define t_landf		23	//	&&		fast version of logical and with index to stop position in token.data.i
#define t_lor		24	//	||		built in function: logical or		takes at least 2 parameters
#define t_lorf		25	//	||		fast version of logical or with index to stop position in token.data.i
#define t_lnot		26	//	!		built in function: logical not		takes 1 parameter

#define t_D		27	//	D		variable access (access the vars array)		takes 1 parameter
#define t_A		28	//	A		array access (indexed access of the vars array)	takes 2 parameters
#define t_L		29	//	L		local variable access (access values on the stack >= the stack pointer)
#define t_P		30	//	P		function parameter variables (access values on the stack above the local variables)
#define t_F		31	//	F		function call
#define t_SS		32	//	S		create new unnamed stringvar (or find first unclaimed one)
#define t_C		33	//	C		character access (work with strings as byte arrays) takes two parameters, a stringvalue and a value
#define t_V		34	//	V		value access (use strings as vectors, aka arrays of values) takes two parameters, a stringvalue and a value

#define t_Df		35	//			fast variable access (basically D but with a direct pointer to the variable in token.data.pointer)
#define t_Af		36	//			fast array access (basically A but with the array start index in token.data.i)
#define t_stackaccess	37	//			access a value on the stack relative to the stack pointer, with token.data.i as the offset
#define t_Ff		38	//			fast function call (basically F but with pointer to func_info in token.data.pointer)

#define t_number	39	//			number constant
#define t_id		40	//			identifier
#define t_getref	41	//	@		'get reference'. returns function number for named functions, variable array index for variables and stack accesses

// ===== string related functions that return numbers ======
				//	EXAMPLE				RETURNS		DESCRIPTION
#define t_ascS		42	//	asc$ [string]			NUM		return number for first character of string
#define t_valS		43	//	val$ [string]			NUM		return value of number in string
#define t_lenS		44	//	len$ [string]			NUM		return length of string
#define t_vlenS		45	//	vlen$ [string]			NUM		return length of string div 8, for use with vectors aka V (value access)
#define t_cmpS		46	//	cmp$ [string] [string]		NUM		strcmp 
#define t_instrS	47	//	instr$ [stringa] [stringb]	NUM		return the position of stringb in stringa or -1 if not found
// =========================================================

#define t_rnd		48	//	rnd [number]	built in function: pseudo random numbers	takes one parameter

#define t_alloc		49	//	alloc [n]	allocate [n] number of items from the vars array

#define t_referredstringconst 50	// getref "stringconst"		creates an unnamed string variable which the stringconstant is copied to, then returns the reference number of the new string variable. note that the copy happens only once so any modifications to the string variable are permanent

// ------- maths -----------

#define t_tan		51	//	tan
#define t_tanh		52	//	tanh
#define t_atan		53	//	atan
#define t_atan2		54	//	atan2
#define t_acos		55	//	acos
#define t_cos		56	//	cos
#define t_cosh		57	//	cosh
#define t_asin		58	//	asin
#define t_sin		59	//	sin
#define t_sinh		60	//	sinh
#define t_exp		61	//	exp
#define t_log		62	//	log
#define t_log10		63	//	log10
#define t_pow		64	//	pow
#define t_sqr		65	//	sqr
#define t_ceil		66	//	ceil
#define t_floor		67	//	floor
#define t_fmod		68	//	fmod

// ------- functions related to file handling -----------
// these return the 'file number' of the file that was opened or 0 if there was an error
#define t_openin	69	//	openin [stringval]		open a file in read only mode. 
#define t_openout	70	//	openout [stringval]		open a file in write only mode
#define t_openup	71	//	openup [stringval]		open a file in read/write mode
// ----				
#define t_eof		72	//	eof [filenumber]		check if reached end of file
#define t_bget		73	//	bget [filenumber]		read byte from file
#define t_vget		74	//	vget [filenumber]		read 8 byte double from file
#define t_ptr		75	//	ptr  [filenumber]		check current position in file
#define t_ext		76	//	ext [filenumber]		check the current length of the file
// ------------------------------------------------------

#define t_extfun	77	//	external function, t.data.pointer contains a function pointer, used for extensions

#define t_error_line	78	//	line number for last error
#define t_error_column	79	//	column number for last error
#define t_error_number	80	//	code number for last error

#define t_leftb		81	//	(

#define VALUES_END	t_leftb

#ifdef enable_graphics_extension
 // graphics extension functions 
 #define t_winw		82		// takes no parameters, returns current window width
 #define t_winh		83		// takes no parameters, returns current window height
 #define t_mousex	84		// takes no parameters, returns current mouse x position	
 #define t_mousey	85		// takes no parameters, returns current mouse y position
 #define t_mousez	86		// takes no parameters, returns a value that changes when the mouse button is scrolled
 #define t_mouseb	87		// takes no parameters, returns bitfield of the mouse buttons currently pressed
 #define t_readkey	88		// takes no parameters, pulls a byte from the keyboard buffer and returns it
 #define t_keypressed	89		// takes one parameter, checks the iskeypressed array and returns info about whether or not that key is currently pressed
 #define t_expose	90		// takes no parameters, returns the state of a flag variable that gets set whenever there's an expose xwindows event, the flag is cleared after reading
 #define t_wmclose	91		// takes no parameters, returns the state of a flag that gets set when the window manager close button has been clicked, flag is cleared after reading
 #define t_keybuffer	92		// return the number of characters in the keyboard buffer
 #undef VALUES_END
 #define VALUES_END t_keybuffer
#endif

// =============================================================================================================================
// =============================================================================================================================
// ============ beyond here: not a 'value' =====================================================================================
// =============================================================================================================================
// =============================================================================================================================

// misc syntax stuff

#define t_rightb	93		//	)
#define t_endstatement	94		//	;		end of statement marker
#define t_comma		95		//	,

#define t_deffn		96		//	function [F value] ([ P num_params L num_locals] or [ param_id_one param_id_two [...] [local local_id_one local_id_two] ] ) ;
#define t_local		97		//	local				used with 'function' keyword. ids after this are local variable names
#define t_ellipsis	98		//	...				used with 'function' keyword. when put at the end of param list, specifies that function takes unlimited parameters.

// ---------------------
#define t_caseof	99
#define t_when		100
#define t_otherwise	101
#define t_endcase	102

#define t_caseofV	103
#define t_caseofS	104
// ---------------------

#define t_label		105		//	.label		place label

// ========= commands ========= 

#define t_return		106	//	return
#define t_while			107	//	while
#define t_endwhile		108	//	endwhile
#define t_if			109	//	if
#define t_else			110	//	else
#define t_endif			111	//	endif
#define t_for			112	//	for [variable_name] [start_value] [end_value] [step_value]
#define t_endfor		113	//	endfor
#define t_endloop		114	//	endloop ([level])	break out of loops
#define t_continue		115	//	continue ([level])	skip to the next iteration of a loop
#define t_restart		116	//	restart ([level])	return to the beginning of a loop
#define t_set			117	//	set
#define t_var			118	//	variable [identifier] ([identifier]) ... ;		declare variables	
#define t_arr			119	//	array [identifier] value;	declare an array
#define t_const			120	//	constant [identifier] value;	declare a constant
#define t_stringvar		121	//	stringvar [identifier] ([number]) ... ; declare string variables. Optionally specify the starting bufsize
#define t_print			122	//	print, will print string constants and/or values until it finds ';'
#define t_endfn			123	//	endfunction	return from a function without returning a value (will return 0)
#define t_goto			124	//	goto [string];		will search for a label with matching string and execution will continue from there
#define t_option		125	//	option [string] [value] [etc];	this will be used to set options like stack size, variable array size, and possibly other things
#define t_extopt		126	//	a processed form of 'option' that has a pointer to a function, used for extensions
#define t_wait			127	//	wait [value];		usleep value*1000
#define t_oscli			128	//	oscli [stringvalue];	system("string");
#define t_quit			129	//	quit ([value]);		exit(value);
#define t_unclaim		130	//	unclaim [string reference number];		Unclaim (release) strings so they can be reused by "S"
#define t_catch			131	//	catch [commands]				run some commands, if an error happens during that then it'll be caught. Also works as a pseudo-value in the same way that oscli does
#define t_throw			132	//	throw "error message"
#define t_endcatch		133	//	terminate the code block where errors are being trapped
#define t_catchf		134	//	processed catch that knows where the corresponding 'endcatch' is
// ---------------------------------------------
#define t_appendS		135	//	append$ [stringvar] [stringvalue]		append to string variables
#define t_extcom		136	//	external command, used for extensions
// ----- commands related to file handling -----
#define t_sptr			137	//	sptr [filenumber] [value] ;			set position in file to [value]
#define t_bput			138	//	bput [filenumber] [value] [...] ;		write bytes to file
#define t_vput			139	//	vput [filenumber] [value] [...] ;		write 8 byte doubles to file
#define t_sput			140	//	sput [filenumber] [stringvalue] [...] ;		write null terminated strings to file
#define t_close			141	//	close [filenumber] ;				close an open file
// ---------------------------------------------
// ===== end of commands ======

// ===== fast modified versions of loop/control flow commands =====
#define t_gotof			142	//      jump to position in token.i
#define t_whilef		143	//	position of matching 'endwhile' in token.i
#define t_endwhilef		144	//	position of matching 'while'+1 in token.i
#define t_whileff		145	//	 while loops that were like "while 0" or "while 1" that have been optimised into simple jumps, they are essentially the same as t_gotof
#define t_endwhileff		146	//	 but they preserve the symbolic info that they represent the start & end of a loop, used by 'endloop' and 'continue'
#define t_iff			147	//	position of matching else/endif in token.i
#define t_elsef			148	//	position of matching endif in token.i
#define t_endiff		149	//	matched endifs must be changed to avoid confusing the matching process for other if/else/endif blocks
#define t_forf			150	//	processed 'for' with pointer to forinfo in token.data.pointer
#define t_endforf		151	//	processed 'endfor' with pointer to forinfo in token.data.pointer
// ===================================

// ===== fast modified versions of 'set' =======================

#define t_set_Df		152	//	fast set for t_Df
#define t_set_stackaccess	153	//	fast set for t_stackaccess

// ===== string functions & stuff that's a 'string value' ======
					//		EXAMPLE				RETURNS		DESCRIPTION
#define t_stringconst		154	//	"string"					string constant
#define t_stringconstf		155	//							string constant (fast), eliminates the need to call strlen()
#define t_rightS		156	//	right$ [string] [n]		STR		get the last n characters of string
#define t_leftS			157	//	left$  [string] [n]		STR		get the first n characters of string
#define t_midS			158	//	mid$ [string] [pos] [n]		STR		get n characters from string starting at pos
#define t_chrS			159	//	chr$ [num]			STR		return a string with the character [num]
#define t_strS			160	//	str$ [num]			STR		return string containing representation of [num]
#define t_catS			161	//	cat$ [string] [string] ...	STR		concatenate strings
#define t_stringS		162	//	string$ [number] [string]	STR		Function returning multiple copies of a string.
#define t_S			163	//	$		string variable dereference
#define t_Sf			164	//			fast string variable access (like $ but with pointer to stringvar in token.data.pointer)
#define t_sget			165	//	sget [filenumber] [(num_bytes)]	read strings from files. if num_bytes is not given, it reads until it finds 0x0A
#define t_vectorS		166	//	vector$ [num] [...]		STR		return string containing vector string, meant for use with 'V' (vectors)
#define t_extsfun		167	//	external string function, used for extensions
#define t_error_message		168	//	error message string from last encountered error
#define t_error_file		169	//	name of the program text file from the last encountered error
#define STRINGVALS_START t_stringconst
#define STRINGVALS_END   t_extsfun
#ifdef enable_graphics_extension // graphics extension stringvalues
 #define t_readkeyS		170	// takes no parameters, pulls a byte from the keyboard buffer and returns it as a stringval
 #undef STRINGVALS_END
 #define STRINGVALS_END t_readkeyS
#endif


// ===== end of string functions & stuff that's a 'string value' ======

#ifdef enable_graphics_extension
 // ===== graphics extension commands =====
 // commands
 #define t_startgraphics	171	// startgraphics		winwidth winheight ;
 #define t_stopgraphics		172	// stopgraphics			;
 #define t_winsize		173	// winsize			W H ;
 #define t_pixel		174	// pixel			X Y ([X Y] ...) ;
 #define t_line			175	// line				X Y X Y ([X Y] ...) ;
 #define t_circlef		176	// circlef			X Y R ;
 #define t_circle		177	// circle			X Y R ;
 #define t_arcf			178	// arcf				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_arc			179	// arc				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_rectanglef		180	// rectanglef			X Y W [H] ;
 #define t_rectangle		181	// rectangle			X Y W [H] ;
 #define t_triangle		182	// triangle			X Y X Y X Y ;
 #define t_drawtext		183	// drawtext			X Y S (stringval);
 #define t_drawscaledtext	184	// drawscaledtext		X Y XS YS (stringval);
 #define t_refreshmode		185	// refreshmode			(mode) ;    (0 refresh on, 1 refresh off)
 #define t_refresh		186	// refresh 			;
 #define t_gcol			187	// gcol				(rgb) ;  or it can be like this: (r) (g) (b) ;
 #define t_bgcol		188	// bgcol			(rgb) ;  or it can be like this: (r) (g) (b) ;  background colour
 #define t_cls			189	// cls				;
 #define t_drawmode		190	// drawmode			dm ;		set the drawing mode to 'dm'
#endif


#if allow_debug_commands
 #define t_tb		191	//	testbeep
 #define t_printstackframe 192	//	print everything in the current stack frame
 #define t_printentirestack 193	//	print everything in the stack up to the current stack frame
#endif

#define t_bad		255	//			bad data

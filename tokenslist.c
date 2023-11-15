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
#define t_log2		63	//	log2
#define t_log10		64	//	log10
#define t_pow		65	//	pow
#define t_sqr		66	//	sqr
#define t_ceil		67	//	ceil
#define t_floor		68	//	floor
#define t_fmod		69	//	fmod

// ------- functions related to file handling -----------
// these return the 'file number' of the file that was opened or 0 if there was an error
#define t_openin	70	//	openin [stringval]		open a file in read only mode. 
#define t_openout	71	//	openout [stringval]		open a file in write only mode
#define t_openup	72	//	openup [stringval]		open a file in read/write mode
// ----				
#define t_eof		73	//	eof [filenumber]		check if reached end of file
#define t_bget		74	//	bget [filenumber]		read byte from file
#define t_vget		75	//	vget [filenumber]		read 8 byte double from file
#define t_ptr		76	//	ptr  [filenumber]		check current position in file
#define t_ext		77	//	ext [filenumber]		check the current length of the file
// ------------------------------------------------------

#define t_extfun	78	//	external function, t.data.pointer contains a function pointer, used for extensions

#define t_error_line	79	//	line number for last error
#define t_error_column	80	//	column number for last error
#define t_error_number	81	//	code number for last error

#define t_evalexpr	82	//	evalexpr [string]		evaluate string as program text that is expected to be a numerical expression
#define t_eval		83	//	eval	[string]		evalute string as program text that is expected to be code, the return value of the code is returned

#define t_leftb		84	//	(

#define VALUES_END	t_leftb

#ifdef enable_graphics_extension
 // graphics extension functions 
 #define t_winw		85		// takes no parameters, returns current window width
 #define t_winh		86		// takes no parameters, returns current window height
 #define t_mousex	87		// takes no parameters, returns current mouse x position	
 #define t_mousey	88		// takes no parameters, returns current mouse y position
 #define t_mousez	89		// takes no parameters, returns a value that changes when the mouse button is scrolled
 #define t_mouseb	90		// takes no parameters, returns bitfield of the mouse buttons currently pressed
 #define t_readkey	91		// takes no parameters, pulls a byte from the keyboard buffer and returns it
 #define t_keypressed	92		// takes one parameter, checks the iskeypressed array and returns info about whether or not that key is currently pressed
 #define t_expose	93		// takes no parameters, returns the state of a flag variable that gets set whenever there's an expose xwindows event, the flag is cleared after reading
 #define t_wmclose	94		// takes no parameters, returns the state of a flag that gets set when the window manager close button has been clicked, flag is cleared after reading
 #define t_keybuffer	95		// return the number of characters in the keyboard buffer
 #undef VALUES_END
 #define VALUES_END t_keybuffer
#endif

// =============================================================================================================================
// =============================================================================================================================
// ============ beyond here: not a 'value' =====================================================================================
// =============================================================================================================================
// =============================================================================================================================

// misc syntax stuff

#define t_rightb	96		//	)
#define t_endstatement	97		//	;		end of statement marker
#define t_comma		98		//	,

#define t_deffn		99		//	function [F value] ([ P num_params L num_locals] or [ param_id_one param_id_two [...] [local local_id_one local_id_two] ] ) ;
#define t_local		100		//	local				used with 'function' keyword. ids after this are local variable names
#define t_ellipsis	101		//	...				used with 'function' keyword. when put at the end of param list, specifies that function takes unlimited parameters.

// ---------------------
#define t_caseof	102
#define t_when		103
#define t_otherwise	104
#define t_endcase	105

#define t_caseofV	106
#define t_caseofS	107
// ---------------------

#define t_label		108		//	.label		place label

// ========= commands ========= 

#define t_return		109	//	return
#define t_while			110	//	while
#define t_endwhile		111	//	endwhile
#define t_if			112	//	if
#define t_else			113	//	else
#define t_endif			114	//	endif
#define t_for			115	//	for [variable_name] [start_value] [end_value] [step_value]
#define t_endfor		116	//	endfor
#define t_endloop		117	//	endloop ([level])	break out of loops
#define t_continue		118	//	continue ([level])	skip to the next iteration of a loop
#define t_restart		119	//	restart ([level])	return to the beginning of a loop
#define t_set			120	//	set
#define t_var			121	//	variable [identifier] ([identifier]) ... ;		declare variables	
#define t_arr			122	//	array [identifier] value;	declare an array
#define t_const			123	//	constant [identifier] value;	declare a constant
#define t_stringvar		124	//	stringvar [identifier] ([number]) ... ; declare string variables. Optionally specify the starting bufsize
#define t_print			125	//	print, will print string constants and/or values until it finds ';'
#define t_endfn			126	//	endfunction	return from a function without returning a value (will return 0)
#define t_goto			127	//	goto [string];		will search for a label with matching string and execution will continue from there
#define t_option		128	//	option [string] [value] [etc];	this will be used to set options like stack size, variable array size, and possibly other things
#define t_extopt		129	//	a processed form of 'option' that has a pointer to a function, used for extensions
#define t_wait			130	//	wait [value];		usleep value*1000
#define t_oscli			131	//	oscli [stringvalue];	system("string");
#define t_quit			132	//	quit ([value]);		exit(value);
#define t_unclaim		133	//	unclaim [string reference number];		Unclaim (release) strings so they can be reused by "S"
#define t_catch			134	//	catch [commands]				run some commands, if an error happens during that then it'll be caught. Also works as a pseudo-value in the same way that oscli does
#define t_throw			135	//	throw "error message"
#define t_endcatch		136	//	terminate the code block where errors are being trapped
#define t_catchf		137	//	processed catch that knows where the corresponding 'endcatch' is
#define t_increment		138	//	increment [name of numerical variable]		increases variable by 1
#define t_increment_stackaccess	139	//	processed increment that increments a local variable
#define t_increment_df		140	//	processed increment that increments a global variable
#define t_decrement		141	//	decrement [name of numerical variable]		decreases variable by 1
#define t_decrement_stackaccess	142	//	
#define t_decrement_df		143
// ---------------------------------------------
#define t_appendS		144	//	append$ [stringvar] [stringvalue]		append to string variables
#define t_extcom		145	//	external command, used for extensions
#define t_prompt		146	//	_prompt						enter the interactive prompt
// ----- commands related to file handling -----
#define t_sptr			147	//	sptr [filenumber] [value] ;			set position in file to [value]
#define t_bput			148	//	bput [filenumber] [value] [...] ;		write bytes to file
#define t_vput			149	//	vput [filenumber] [value] [...] ;		write 8 byte doubles to file
#define t_sput			150	//	sput [filenumber] [stringvalue] [...] ;		write null terminated strings to file
#define t_close			151	//	close [filenumber] ;				close an open file
// ---------------------------------------------
// ===== end of commands ======

// ===== fast modified versions of loop/control flow commands =====
#define t_gotof			152	//      jump to position in token.i
#define t_whilef		153	//	position of matching 'endwhile' in token.i
#define t_endwhilef		154	//	position of matching 'while'+1 in token.i
#define t_whileff		155	//	 while loops that were like "while 0" or "while 1" that have been optimised into simple jumps, they are essentially the same as t_gotof
#define t_endwhileff		156	//	 but they preserve the symbolic info that they represent the start & end of a loop, used by 'endloop' and 'continue'
#define t_iff			157	//	position of matching else/endif in token.i
#define t_elsef			158	//	position of matching endif in token.i
#define t_endiff		159	//	matched endifs must be changed to avoid confusing the matching process for other if/else/endif blocks
#define t_forf			160	//	processed 'for' with pointer to forinfo in token.data.pointer
#define t_endforf		161	//	processed 'endfor' with pointer to forinfo in token.data.pointer
// ===================================

// ===== fast modified versions of 'set' =======================

#define t_set_Df		162	//	fast set for t_Df
#define t_set_stackaccess	163	//	fast set for t_stackaccess

// ===== string functions & stuff that's a 'string value' ======
					//		EXAMPLE				RETURNS		DESCRIPTION
#define t_stringconst		164	//	"string"					string constant
#define t_stringconstf		165	//							string constant (fast), eliminates the need to call strlen()
#define t_rightS		166	//	right$ [string] [n]		STR		get the last n characters of string
#define t_leftS			167	//	left$  [string] [n]		STR		get the first n characters of string
#define t_midS			168	//	mid$ [string] [pos] [n]		STR		get n characters from string starting at pos
#define t_chrS			169	//	chr$ [num]			STR		return a string with the character [num]
#define t_strS			170	//	str$ [num]			STR		return string containing representation of [num]
#define t_catS			171	//	cat$ [string] [string] ...	STR		concatenate strings
#define t_stringS		172	//	string$ [number] [string]	STR		Function returning multiple copies of a string.
#define t_S			173	//	$		string variable dereference
#define t_Sf			174	//			fast string variable access (like $ but with pointer to stringvar in token.data.pointer)
#define t_sget			175	//	sget [filenumber] [(num_bytes)]	read strings from files. if num_bytes is not given, it reads until it finds 0x0A
#define t_vectorS		176	//	vector$ [num] [...]		STR		return string containing vector string, meant for use with 'V' (vectors)
#define t_error_message		177	//	error message string from last encountered error
#define t_error_file		178	//	name of the program text file from the last encountered error
#define t_rnd_state		179	//	obtain the current state value of the random number generator
#define t_evalS			180	//	eval$ [string]			STR		evaluates string as prog text that is expected to be a string value
// ----- this one has to be the last one in the list of 'string value' tokens cos it's used for STRINGVALS_END -------------------------
#define t_extsfun		181	//	external string function, used for extensions
#define STRINGVALS_START t_stringconst
#define STRINGVALS_END   t_extsfun
#ifdef enable_graphics_extension // graphics extension stringvalues
 #define t_readkeyS		182	// takes no parameters, pulls a byte from the keyboard buffer and returns it as a stringval
 #undef STRINGVALS_END
 #define STRINGVALS_END t_readkeyS
#endif


// ===== end of string functions & stuff that's a 'string value' ======

#ifdef enable_graphics_extension
 // ===== graphics extension commands =====
 // commands
 #define t_startgraphics	183	// startgraphics		winwidth winheight ;
 #define t_stopgraphics		184	// stopgraphics			;
 #define t_winsize		185	// winsize			W H ;
 #define t_pixel		186	// pixel			X Y ([X Y] ...) ;
 #define t_line			187	// line				X Y X Y ([X Y] ...) ;
 #define t_circlef		188	// circlef			X Y R ;
 #define t_circle		189	// circle			X Y R ;
 #define t_arcf			190	// arcf				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_arc			191	// arc				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_rectanglef		192	// rectanglef			X Y W [H] ;
 #define t_rectangle		193	// rectangle			X Y W [H] ;
 #define t_triangle		194	// triangle			X Y X Y X Y ;
 #define t_drawtext		195	// drawtext			X Y S (stringval);
 #define t_drawscaledtext	196	// drawscaledtext		X Y XS YS (stringval);
 #define t_refreshmode		197	// refreshmode			(mode) ;    (0 refresh on, 1 refresh off)
 #define t_refresh		198	// refresh 			;
 #define t_gcol			199	// gcol				(rgb) ;  or it can be like this: (r) (g) (b) ;
 #define t_bgcol		200	// bgcol			(rgb) ;  or it can be like this: (r) (g) (b) ;  background colour
 #define t_cls			201	// cls				;
 #define t_drawmode		202	// drawmode			dm ;		set the drawing mode to 'dm'
#endif


#if allow_debug_commands
 #define t_tb		203	//	testbeep
 #define t_printstackframe 204	//	print everything in the current stack frame
 #define t_printentirestack 205	//	print everything in the stack up to the current stack frame
#endif

#define t_bad		255	//			bad data

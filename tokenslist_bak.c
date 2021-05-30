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
#define t_lor		23	//	||		built in function: logical or		takes at least 2 parameters
#define t_lnot		24	//	!		built in function: logical not		takes 1 parameter

#define t_D		25	//	D		variable access (access the vars array)		takes 1 parameter
#define t_A		26	//	A		array access (indexed access of the vars array)	takes 2 parameters
#define t_L		27	//	L		local variable access (access values on the stack >= the stack pointer)
#define t_P		28	//	P		function parameter variables (access values on the stack above the local variables)
#define t_F		29	//	F		function call
#define t_SS		30	//	S		create new unnamed stringvar (or find first unclaimed one)

#define t_Df		31	//			fast variable access (basically D but with a direct pointer to the variable in token.data.pointer)
#define t_Af		32	//			fast array access (basically A but with the array start index in token.data.i)
#define t_stackaccess	33	//			access a value on the stack relative to the stack pointer, with token.data.i as the offset
#define t_Ff		34	//			fast function call (basically F but with pointer to func_info in token.data.pointer)

#define t_number	35	//			number constant
#define t_id		36	//			identifier
#define t_getref	37	//	getref		'get reference'. returns function number for named functions, variable array index for variables and stack accesses

// ===== string related functions that return numbers ======
				//	EXAMPLE				RETURNS		DESCRIPTION
#define t_ascS		38	//	asc$ [string]			NUM		return number for first character of string
#define t_valS		39	//	val$ [string]			NUM		return value of number in string
#define t_lenS		40	//	len$ [string]			NUM		return length of string
#define t_cmpS		41	//	cmp$ [string] [string]		NUM		strcmp 
#define t_instrS	42	//	instr$ [stringa] [stringb]	NUM		return the position of stringb in stringa or -1 if not found
// =========================================================

#define t_rnd		43	//	rnd [number]	built in function: pseudo random numbers	takes one parameter

#define t_alloc		44	//	alloc [n]	allocate [n] number of items from the vars array

#define t_referredstringconst 45	// getref "stringconst"		creates an unnamed string variable which the stringconstant is copied to, then returns the reference number of the new string variable. note that the copy happens only once so any modifications to the string variable are permanent

// ------- maths -----------

#define t_tan		46	//	tan
#define t_tanh		47	//	tanh
#define t_atan		48	//	atan
#define t_atan2		49	//	atan2
#define t_acos		50	//	acos
#define t_cos		51	//	cos
#define t_cosh		52	//	cosh
#define t_asin		53	//	asin
#define t_sin		54	//	sin
#define t_sinh		55	//	sinh
#define t_exp		56	//	exp
#define t_log		57	//	log
#define t_log10		58	//	log10
#define t_pow		59	//	pow
#define t_sqr		60	//	sqr
#define t_ceil		61	//	ceil
#define t_floor		62	//	floor
#define t_fmod		63	//	fmod

// ------- functions related to file handling -----------
// these return the 'file number' of the file that was opened or 0 if there was an error
#define t_openin	64	//	openin [stringval]		open a file in read only mode. 
#define t_openout	65	//	openout [stringval]		open a file in write only mode
#define t_openup	66	//	openup [stringval]		open a file in read/write mode
// ----				
#define t_eof		67	//	eof [filenumber]		check if reached end of file
#define t_bget		68	//	bget [filenumber]		read byte from file
#define t_vget		69	//	vget [filenumber]		read 8 byte double from file
#define t_ptr		70	//	ptr  [filenumber]		check current position in file
#define t_ext		71	//	ext [filenumber]		check the current length of the file
// ------------------------------------------------------

#define t_leftb		72	//	(

#define VALUES_END	t_leftb

#ifdef enable_graphics_extension
 // graphics extension functions 
 #define t_winw		73		// takes no parameters, returns current window width
 #define t_winh		74		// takes no parameters, returns current window height
 #define t_mousex	75		// takes no parameters, returns current mouse x position	
 #define t_mousey	76		// takes no parameters, returns current mouse y position
 #define t_mousez	77		// takes no parameters, returns a value that changes when the mouse button is scrolled
 #define t_mouseb	78		// takes no parameters, returns bitfield of the mouse buttons currently pressed
 #define t_readkey	79		// takes no parameters, pulls a byte from the keyboard buffer and returns it
 #define t_keypressed	80		// takes one parameter, checks the iskeypressed array and returns info about whether or not that key is currently pressed
 #define t_expose	81		// takes no parameters, returns the state of a flag variable that gets set whenever there's an expose xwindows event, the flag is cleared after reading
 #define t_wmclose	82		// takes no parameters, returns the state of a flag that gets set when the window manager close button has been clicked, flag is cleared after reading
 #define t_keybuffer	83		// return the number of characters in the keyboard buffer
 #undef VALUES_END
 #define VALUES_END t_keybuffer
#endif

// =============================================================================================================================
// =============================================================================================================================
// ============ beyond here: not a 'value' =====================================================================================
// =============================================================================================================================
// =============================================================================================================================

// misc syntax stuff

#define t_rightb	84		//	)
#define t_endstatement	85		//	;		end of statement marker
#define t_comma		86		//	,

#define t_deffn		87		//	function [F value] ([ P num_params L num_locals] or [ param_id_one param_id_two [...] [local local_id_one local_id_two] ] ) ;
#define t_local		88		//	local				used with 'function' keyword. ids after this are local variable names
#define t_ellipsis	89		//	...				used with 'function' keyword. when put at the end of param list, specifies that function takes unlimited parameters.

// ---------------------
#define t_caseof	90
#define t_when		91
#define t_otherwise	92
#define t_endcase	93

#define t_caseofV	94
#define t_caseofS	95
// ---------------------

#define t_label		96		//	.label		place label

// ========= commands ========= 

#define t_return		97	//	return
#define t_while			98	//	while
#define t_endwhile		99	//	endwhile
#define t_if			100	//	if
#define t_else			101	//	else
#define t_endif			102	//	endif
#define t_set			103	//	set
#define t_var			104	//	variable [identifier] ([identifier]) ... ;		declare variables	
#define t_arr			105	//	array [identifier] value;	declare an array
#define t_const			106	//	constant [identifier] value;	declare a constant
#define t_stringvar		107	//	stringvar [identifier] ([number]) ... ; declare string variables. Optionally specify the starting bufsize
#define t_print			108	//	print, will print string constants and/or values until it finds ';'
#define t_endfn			109	//	endfunction	return from a function without returning a value (will return 0)
#define t_goto			110	//	goto [string];		will search for a label with matching string and execution will continue from there
#define t_option		111	//	option [string] [value] [etc];	this will be used to set options like stack size, variable array size, and possibly other things
#define t_wait			112	//	wait [value];		usleep value*1000
#define t_oscli			113	//	oscli [stringvalue];	system("string");
#define t_quit			114	//	quit ([value]);		exit(value);
// ----- commands related to file handling -----
#define t_sptr			115	//	sptr [filenumber] [value] ;			set position in file to [value]
#define t_bput			116	//	bput [filenumber] [value] [...] ;		write bytes to file
#define t_vput			117	//	vput [filenumber] [value] [...] ;		write 8 byte doubles to file
#define t_sput			118	//	sput [filenumber] [stringvalue] [...] ;		write null terminated strings to file
#define t_close			119	//	close [filenumber] ;				close an open file
// ---------------------------------------------
// ===== end of commands ======

// ===== fast modified versions of loop/control flow commands =====
#define t_gotof			120	//      jump to position in token.i
#define t_whilef		121	//	position of matching 'endwhile' in token.i
#define t_endwhilef		122	//	position of matching 'while'+1 in token.i
#define t_iff			123	//	position of matching else/endif in token.i
#define t_elsef			124	//	position of matching endif in token.i
#define t_endiff		125	//	matched endifs must be changed to avoid confusing the matching process for other if/else/endif blocks
// ===================================

// ===== string functions & stuff that's a 'string value' ======
					//	EXAMPLE				RETURNS		DESCRIPTION
#define t_stringconst		126	//	"string"					string constant
#define t_stringconstf		127	//							string constant (fast), eliminates the need to call strlen()
#define t_rightS		128	//	right$ [string] [n]		STR		get the last n characters of string
#define t_leftS			129	//	left$  [string] [n]		STR		get the first n characters of string
#define t_midS			130	//	mid$ [string] [pos] [n]		STR		get n characters from string starting at pos
#define t_chrS			131	//	chr$ [num]			STR		return a string with the character [num]
#define t_strS			132	//	str$ [num]			STR		return string containing representation of [num]
#define t_catS			133	//	cat$ [string] [string] ...	STR		concatenate strings
#define t_S			134	//	$		string variable dereference
#define t_Sf			135	//			fast string variable access (like $ but with pointer to stringvar in token.data.pointer)
#define t_sget			136	//	sget [filenumber] [(num_bytes)]	read strings from files. if num_bytes is not given, it reads until it finds 0x0A
#define STRINGVALS_START t_stringconst
#define STRINGVALS_END   t_sget
#ifdef enable_graphics_extension // graphics extension stringvalues
 #define t_readkeyS		137	// takes no parameters, pulls a byte from the keyboard buffer and returns it as a stringval
 #undef STRINGVALS_END
 #define STRINGVALS_END t_readkeyS
#endif


// ===== end of string functions & stuff that's a 'string value' ======

#ifdef enable_graphics_extension
 // ===== graphics extension commands =====
 // commands
 #define t_startgraphics	138	// startgraphics		winwidth winheight ;
 #define t_stopgraphics		139	// stopgraphics			;
 #define t_winsize		140	// winsize			W H ;
 #define t_pixel		141	// pixel			X Y ([X Y] ...) ;
 #define t_line			142	// line				X Y X Y ([X Y] ...) ;
 #define t_circlef		143	// circlef			X Y R ;
 #define t_circle		144	// circle			X Y R ;
 #define t_rectanglef		145	// rectanglef			X Y W [H] ;
 #define t_rectangle		146	// rectangle			X Y W [H] ;
 #define t_triangle		147	// triangle			X Y X Y X Y ;
 #define t_drawtext		148	// drawtext			X Y S (stringval);
 #define t_refreshmode		149	// refreshmode			(mode) ;    (0 refresh on, 1 refresh off)
 #define t_refresh		150	// refresh 			;
 #define t_gcol			151	// gcol				(rgb) ;  or it can be like this: (r) (g) (b) ;
 #define t_bgcol		152	// bgcol			(rgb) ;  or it can be like this: (r) (g) (b) ;  background colour
 #define t_cls			153	// cls				;
 #define t_drawmode		154	// drawmode			dm ;		set the drawing mode to 'dm'
#endif


#if allow_debug_commands
 #define t_tb		155	//	testbeep
 #define t_printstackframe 156	//	print everything in the current stack frame
 #define t_printentirestack 157	//	print everything in the stack up to the current stack frame
#endif

#define t_bad		255	//			bad data

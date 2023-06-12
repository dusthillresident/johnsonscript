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
#define t_C		31	//	C		character access (work with strings as byte arrays) takes two parameters, a stringvalue and a value

#define t_Df		32	//			fast variable access (basically D but with a direct pointer to the variable in token.data.pointer)
#define t_Af		33	//			fast array access (basically A but with the array start index in token.data.i)
#define t_stackaccess	34	//			access a value on the stack relative to the stack pointer, with token.data.i as the offset
#define t_Ff		35	//			fast function call (basically F but with pointer to func_info in token.data.pointer)

#define t_number	36	//			number constant
#define t_id		37	//			identifier
#define t_getref	38	//	@		'get reference'. returns function number for named functions, variable array index for variables and stack accesses

// ===== string related functions that return numbers ======
				//	EXAMPLE				RETURNS		DESCRIPTION
#define t_ascS		39	//	asc$ [string]			NUM		return number for first character of string
#define t_valS		40	//	val$ [string]			NUM		return value of number in string
#define t_lenS		41	//	len$ [string]			NUM		return length of string
#define t_cmpS		42	//	cmp$ [string] [string]		NUM		strcmp 
#define t_instrS	43	//	instr$ [stringa] [stringb]	NUM		return the position of stringb in stringa or -1 if not found
// =========================================================

#define t_rnd		44	//	rnd [number]	built in function: pseudo random numbers	takes one parameter

#define t_alloc		45	//	alloc [n]	allocate [n] number of items from the vars array

#define t_referredstringconst 46	// getref "stringconst"		creates an unnamed string variable which the stringconstant is copied to, then returns the reference number of the new string variable. note that the copy happens only once so any modifications to the string variable are permanent

// ------- maths -----------

#define t_tan		47	//	tan
#define t_tanh		48	//	tanh
#define t_atan		49	//	atan
#define t_atan2		50	//	atan2
#define t_acos		51	//	acos
#define t_cos		52	//	cos
#define t_cosh		53	//	cosh
#define t_asin		54	//	asin
#define t_sin		55	//	sin
#define t_sinh		56	//	sinh
#define t_exp		57	//	exp
#define t_log		58	//	log
#define t_log10		59	//	log10
#define t_pow		60	//	pow
#define t_sqr		61	//	sqr
#define t_ceil		62	//	ceil
#define t_floor		63	//	floor
#define t_fmod		64	//	fmod

// ------- functions related to file handling -----------
// these return the 'file number' of the file that was opened or 0 if there was an error
#define t_openin	65	//	openin [stringval]		open a file in read only mode. 
#define t_openout	66	//	openout [stringval]		open a file in write only mode
#define t_openup	67	//	openup [stringval]		open a file in read/write mode
// ----				
#define t_eof		68	//	eof [filenumber]		check if reached end of file
#define t_bget		69	//	bget [filenumber]		read byte from file
#define t_vget		70	//	vget [filenumber]		read 8 byte double from file
#define t_ptr		71	//	ptr  [filenumber]		check current position in file
#define t_ext		72	//	ext [filenumber]		check the current length of the file
// ------------------------------------------------------

#define t_extfun	73	//	external function, t.data.pointer contains a function pointer, used for extensions

#define t_leftb		74	//	(

#define VALUES_END	t_leftb

#ifdef enable_graphics_extension
 // graphics extension functions 
 #define t_winw		75		// takes no parameters, returns current window width
 #define t_winh		76		// takes no parameters, returns current window height
 #define t_mousex	77		// takes no parameters, returns current mouse x position	
 #define t_mousey	78		// takes no parameters, returns current mouse y position
 #define t_mousez	79		// takes no parameters, returns a value that changes when the mouse button is scrolled
 #define t_mouseb	80		// takes no parameters, returns bitfield of the mouse buttons currently pressed
 #define t_readkey	81		// takes no parameters, pulls a byte from the keyboard buffer and returns it
 #define t_keypressed	82		// takes one parameter, checks the iskeypressed array and returns info about whether or not that key is currently pressed
 #define t_expose	83		// takes no parameters, returns the state of a flag variable that gets set whenever there's an expose xwindows event, the flag is cleared after reading
 #define t_wmclose	84		// takes no parameters, returns the state of a flag that gets set when the window manager close button has been clicked, flag is cleared after reading
 #define t_keybuffer	85		// return the number of characters in the keyboard buffer
 #undef VALUES_END
 #define VALUES_END t_keybuffer
#endif

// =============================================================================================================================
// =============================================================================================================================
// ============ beyond here: not a 'value' =====================================================================================
// =============================================================================================================================
// =============================================================================================================================

// misc syntax stuff

#define t_rightb	86		//	)
#define t_endstatement	87		//	;		end of statement marker
#define t_comma		88		//	,

#define t_deffn		89		//	function [F value] ([ P num_params L num_locals] or [ param_id_one param_id_two [...] [local local_id_one local_id_two] ] ) ;
#define t_local		90		//	local				used with 'function' keyword. ids after this are local variable names
#define t_ellipsis	91		//	...				used with 'function' keyword. when put at the end of param list, specifies that function takes unlimited parameters.

// ---------------------
#define t_caseof	92
#define t_when		93
#define t_otherwise	94
#define t_endcase	95

#define t_caseofV	96
#define t_caseofS	97
// ---------------------

#define t_label		98		//	.label		place label

// ========= commands ========= 

#define t_return		99	//	return
#define t_while			100	//	while
#define t_endwhile		101	//	endwhile
#define t_if			102	//	if
#define t_else			103	//	else
#define t_endif			104	//	endif
#define t_set			105	//	set
#define t_var			106	//	variable [identifier] ([identifier]) ... ;		declare variables	
#define t_arr			107	//	array [identifier] value;	declare an array
#define t_const			108	//	constant [identifier] value;	declare a constant
#define t_stringvar		109	//	stringvar [identifier] ([number]) ... ; declare string variables. Optionally specify the starting bufsize
#define t_print			110	//	print, will print string constants and/or values until it finds ';'
#define t_endfn			111	//	endfunction	return from a function without returning a value (will return 0)
#define t_goto			112	//	goto [string];		will search for a label with matching string and execution will continue from there
#define t_option		113	//	option [string] [value] [etc];	this will be used to set options like stack size, variable array size, and possibly other things
#define t_extopt		114	//	a processed form of 'option' that has a pointer to a function, used for extensions
#define t_wait			115	//	wait [value];		usleep value*1000
#define t_oscli			116	//	oscli [stringvalue];	system("string");
#define t_quit			117	//	quit ([value]);		exit(value);
// ---------------------------------------------
#define t_extcom		118	//	external command, used for extensions
// ----- commands related to file handling -----
#define t_sptr			119	//	sptr [filenumber] [value] ;			set position in file to [value]
#define t_bput			120	//	bput [filenumber] [value] [...] ;		write bytes to file
#define t_vput			121	//	vput [filenumber] [value] [...] ;		write 8 byte doubles to file
#define t_sput			122	//	sput [filenumber] [stringvalue] [...] ;		write null terminated strings to file
#define t_close			123	//	close [filenumber] ;				close an open file
// ---------------------------------------------
// ===== end of commands ======

// ===== fast modified versions of loop/control flow commands =====
#define t_gotof			124	//      jump to position in token.i
#define t_whilef		125	//	position of matching 'endwhile' in token.i
#define t_endwhilef		126	//	position of matching 'while'+1 in token.i
#define t_iff			127	//	position of matching else/endif in token.i
#define t_elsef			128	//	position of matching endif in token.i
#define t_endiff		129	//	matched endifs must be changed to avoid confusing the matching process for other if/else/endif blocks
// ===================================

// ===== string functions & stuff that's a 'string value' ======
					//		EXAMPLE				RETURNS		DESCRIPTION
#define t_stringconst		130	//	"string"					string constant
#define t_stringconstf		131	//							string constant (fast), eliminates the need to call strlen()
#define t_rightS		132	//	right$ [string] [n]		STR		get the last n characters of string
#define t_leftS			133	//	left$  [string] [n]		STR		get the first n characters of string
#define t_midS			134	//	mid$ [string] [pos] [n]		STR		get n characters from string starting at pos
#define t_chrS			135	//	chr$ [num]			STR		return a string with the character [num]
#define t_strS			136	//	str$ [num]			STR		return string containing representation of [num]
#define t_catS			137	//	cat$ [string] [string] ...	STR		concatenate strings
#define t_stringS		138	//	string$ [number] [string]	STR		Function returning multiple copies of a string.
#define t_S			139	//	$		string variable dereference
#define t_Sf			140	//			fast string variable access (like $ but with pointer to stringvar in token.data.pointer)
#define t_sget			141	//	sget [filenumber] [(num_bytes)]	read strings from files. if num_bytes is not given, it reads until it finds 0x0A
#define t_extsfun		142	//	external string function, used for extensions
#define STRINGVALS_START t_stringconst
#define STRINGVALS_END   t_extsfun
#ifdef enable_graphics_extension // graphics extension stringvalues
 #define t_readkeyS		143	// takes no parameters, pulls a byte from the keyboard buffer and returns it as a stringval
 #undef STRINGVALS_END
 #define STRINGVALS_END t_readkeyS
#endif


// ===== end of string functions & stuff that's a 'string value' ======

#ifdef enable_graphics_extension
 // ===== graphics extension commands =====
 // commands
 #define t_startgraphics	144	// startgraphics		winwidth winheight ;
 #define t_stopgraphics		145	// stopgraphics			;
 #define t_winsize		146	// winsize			W H ;
 #define t_pixel		147	// pixel			X Y ([X Y] ...) ;
 #define t_line			148	// line				X Y X Y ([X Y] ...) ;
 #define t_circlef		149	// circlef			X Y R ;
 #define t_circle		150	// circle			X Y R ;
 #define t_arcf			151	// arcf				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_arc			152	// arc				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_rectanglef		153	// rectanglef			X Y W [H] ;
 #define t_rectangle		154	// rectangle			X Y W [H] ;
 #define t_triangle		155	// triangle			X Y X Y X Y ;
 #define t_drawtext		156	// drawtext			X Y S (stringval);
 #define t_drawscaledtext	157	// drawscaledtext		X Y XS YS (stringval);
 #define t_refreshmode		158	// refreshmode			(mode) ;    (0 refresh on, 1 refresh off)
 #define t_refresh		159	// refresh 			;
 #define t_gcol			160	// gcol				(rgb) ;  or it can be like this: (r) (g) (b) ;
 #define t_bgcol		161	// bgcol			(rgb) ;  or it can be like this: (r) (g) (b) ;  background colour
 #define t_cls			162	// cls				;
 #define t_drawmode		163	// drawmode			dm ;		set the drawing mode to 'dm'
#endif


#if allow_debug_commands
 #define t_tb		164	//	testbeep
 #define t_printstackframe 165	//	print everything in the current stack frame
 #define t_printentirestack 166	//	print everything in the stack up to the current stack frame
#endif

#define t_bad		255	//			bad data

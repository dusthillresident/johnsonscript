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
#define t_V		32	//	V		value access (use strings as arrays of values) takes two parameters, a stringvalue and a value

#define t_Df		33	//			fast variable access (basically D but with a direct pointer to the variable in token.data.pointer)
#define t_Af		34	//			fast array access (basically A but with the array start index in token.data.i)
#define t_stackaccess	35	//			access a value on the stack relative to the stack pointer, with token.data.i as the offset
#define t_Ff		36	//			fast function call (basically F but with pointer to func_info in token.data.pointer)

#define t_number	37	//			number constant
#define t_id		38	//			identifier
#define t_getref	39	//	@		'get reference'. returns function number for named functions, variable array index for variables and stack accesses

// ===== string related functions that return numbers ======
				//	EXAMPLE				RETURNS		DESCRIPTION
#define t_ascS		40	//	asc$ [string]			NUM		return number for first character of string
#define t_valS		41	//	val$ [string]			NUM		return value of number in string
#define t_lenS		42	//	len$ [string]			NUM		return length of string
#define t_vlenS		43	//	vlen$ [string]			NUM		return length of string div 8, for use with V (value access)
#define t_cmpS		44	//	cmp$ [string] [string]		NUM		strcmp 
#define t_instrS	45	//	instr$ [stringa] [stringb]	NUM		return the position of stringb in stringa or -1 if not found
// =========================================================

#define t_rnd		46	//	rnd [number]	built in function: pseudo random numbers	takes one parameter

#define t_alloc		47	//	alloc [n]	allocate [n] number of items from the vars array

#define t_referredstringconst 48	// getref "stringconst"		creates an unnamed string variable which the stringconstant is copied to, then returns the reference number of the new string variable. note that the copy happens only once so any modifications to the string variable are permanent

// ------- maths -----------

#define t_tan		49	//	tan
#define t_tanh		50	//	tanh
#define t_atan		51	//	atan
#define t_atan2		52	//	atan2
#define t_acos		53	//	acos
#define t_cos		54	//	cos
#define t_cosh		55	//	cosh
#define t_asin		56	//	asin
#define t_sin		57	//	sin
#define t_sinh		58	//	sinh
#define t_exp		59	//	exp
#define t_log		60	//	log
#define t_log10		61	//	log10
#define t_pow		62	//	pow
#define t_sqr		63	//	sqr
#define t_ceil		64	//	ceil
#define t_floor		65	//	floor
#define t_fmod		66	//	fmod

// ------- functions related to file handling -----------
// these return the 'file number' of the file that was opened or 0 if there was an error
#define t_openin	67	//	openin [stringval]		open a file in read only mode. 
#define t_openout	68	//	openout [stringval]		open a file in write only mode
#define t_openup	69	//	openup [stringval]		open a file in read/write mode
// ----				
#define t_eof		70	//	eof [filenumber]		check if reached end of file
#define t_bget		71	//	bget [filenumber]		read byte from file
#define t_vget		72	//	vget [filenumber]		read 8 byte double from file
#define t_ptr		73	//	ptr  [filenumber]		check current position in file
#define t_ext		74	//	ext [filenumber]		check the current length of the file
// ------------------------------------------------------

#define t_extfun	75	//	external function, t.data.pointer contains a function pointer, used for extensions

#define t_leftb		76	//	(

#define VALUES_END	t_leftb

#ifdef enable_graphics_extension
 // graphics extension functions 
 #define t_winw		77		// takes no parameters, returns current window width
 #define t_winh		78		// takes no parameters, returns current window height
 #define t_mousex	79		// takes no parameters, returns current mouse x position	
 #define t_mousey	80		// takes no parameters, returns current mouse y position
 #define t_mousez	81		// takes no parameters, returns a value that changes when the mouse button is scrolled
 #define t_mouseb	82		// takes no parameters, returns bitfield of the mouse buttons currently pressed
 #define t_readkey	83		// takes no parameters, pulls a byte from the keyboard buffer and returns it
 #define t_keypressed	84		// takes one parameter, checks the iskeypressed array and returns info about whether or not that key is currently pressed
 #define t_expose	85		// takes no parameters, returns the state of a flag variable that gets set whenever there's an expose xwindows event, the flag is cleared after reading
 #define t_wmclose	86		// takes no parameters, returns the state of a flag that gets set when the window manager close button has been clicked, flag is cleared after reading
 #define t_keybuffer	87		// return the number of characters in the keyboard buffer
 #undef VALUES_END
 #define VALUES_END t_keybuffer
#endif

// =============================================================================================================================
// =============================================================================================================================
// ============ beyond here: not a 'value' =====================================================================================
// =============================================================================================================================
// =============================================================================================================================

// misc syntax stuff

#define t_rightb	88		//	)
#define t_endstatement	89		//	;		end of statement marker
#define t_comma		90		//	,

#define t_deffn		91		//	function [F value] ([ P num_params L num_locals] or [ param_id_one param_id_two [...] [local local_id_one local_id_two] ] ) ;
#define t_local		92		//	local				used with 'function' keyword. ids after this are local variable names
#define t_ellipsis	93		//	...				used with 'function' keyword. when put at the end of param list, specifies that function takes unlimited parameters.

// ---------------------
#define t_caseof	94
#define t_when		95
#define t_otherwise	96
#define t_endcase	97

#define t_caseofV	98
#define t_caseofS	99
// ---------------------

#define t_label		100		//	.label		place label

// ========= commands ========= 

#define t_return		101	//	return
#define t_while			102	//	while
#define t_endwhile		103	//	endwhile
#define t_if			104	//	if
#define t_else			105	//	else
#define t_endif			106	//	endif
#define t_set			107	//	set
#define t_var			108	//	variable [identifier] ([identifier]) ... ;		declare variables	
#define t_arr			109	//	array [identifier] value;	declare an array
#define t_const			110	//	constant [identifier] value;	declare a constant
#define t_stringvar		111	//	stringvar [identifier] ([number]) ... ; declare string variables. Optionally specify the starting bufsize
#define t_print			112	//	print, will print string constants and/or values until it finds ';'
#define t_endfn			113	//	endfunction	return from a function without returning a value (will return 0)
#define t_goto			114	//	goto [string];		will search for a label with matching string and execution will continue from there
#define t_option		115	//	option [string] [value] [etc];	this will be used to set options like stack size, variable array size, and possibly other things
#define t_extopt		116	//	a processed form of 'option' that has a pointer to a function, used for extensions
#define t_wait			117	//	wait [value];		usleep value*1000
#define t_oscli			118	//	oscli [stringvalue];	system("string");
#define t_quit			119	//	quit ([value]);		exit(value);
// ---------------------------------------------
#define t_extcom		120	//	external command, used for extensions
// ----- commands related to file handling -----
#define t_sptr			121	//	sptr [filenumber] [value] ;			set position in file to [value]
#define t_bput			122	//	bput [filenumber] [value] [...] ;		write bytes to file
#define t_vput			123	//	vput [filenumber] [value] [...] ;		write 8 byte doubles to file
#define t_sput			124	//	sput [filenumber] [stringvalue] [...] ;		write null terminated strings to file
#define t_close			125	//	close [filenumber] ;				close an open file
// ---------------------------------------------
// ===== end of commands ======

// ===== fast modified versions of loop/control flow commands =====
#define t_gotof			126	//      jump to position in token.i
#define t_whilef		127	//	position of matching 'endwhile' in token.i
#define t_endwhilef		128	//	position of matching 'while'+1 in token.i
#define t_iff			129	//	position of matching else/endif in token.i
#define t_elsef			130	//	position of matching endif in token.i
#define t_endiff		131	//	matched endifs must be changed to avoid confusing the matching process for other if/else/endif blocks
// ===================================

// ===== string functions & stuff that's a 'string value' ======
					//		EXAMPLE				RETURNS		DESCRIPTION
#define t_stringconst		132	//	"string"					string constant
#define t_stringconstf		133	//							string constant (fast), eliminates the need to call strlen()
#define t_rightS		134	//	right$ [string] [n]		STR		get the last n characters of string
#define t_leftS			135	//	left$  [string] [n]		STR		get the first n characters of string
#define t_midS			136	//	mid$ [string] [pos] [n]		STR		get n characters from string starting at pos
#define t_chrS			137	//	chr$ [num]			STR		return a string with the character [num]
#define t_strS			138	//	str$ [num]			STR		return string containing representation of [num]
#define t_catS			139	//	cat$ [string] [string] ...	STR		concatenate strings
#define t_stringS		140	//	string$ [number] [string]	STR		Function returning multiple copies of a string.
#define t_S			141	//	$		string variable dereference
#define t_Sf			142	//			fast string variable access (like $ but with pointer to stringvar in token.data.pointer)
#define t_sget			143	//	sget [filenumber] [(num_bytes)]	read strings from files. if num_bytes is not given, it reads until it finds 0x0A
#define t_extsfun		144	//	external string function, used for extensions
#define STRINGVALS_START t_stringconst
#define STRINGVALS_END   t_extsfun
#ifdef enable_graphics_extension // graphics extension stringvalues
 #define t_readkeyS		145	// takes no parameters, pulls a byte from the keyboard buffer and returns it as a stringval
 #undef STRINGVALS_END
 #define STRINGVALS_END t_readkeyS
#endif


// ===== end of string functions & stuff that's a 'string value' ======

#ifdef enable_graphics_extension
 // ===== graphics extension commands =====
 // commands
 #define t_startgraphics	146	// startgraphics		winwidth winheight ;
 #define t_stopgraphics		147	// stopgraphics			;
 #define t_winsize		148	// winsize			W H ;
 #define t_pixel		149	// pixel			X Y ([X Y] ...) ;
 #define t_line			150	// line				X Y X Y ([X Y] ...) ;
 #define t_circlef		151	// circlef			X Y R ;
 #define t_circle		152	// circle			X Y R ;
 #define t_arcf			153	// arcf				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_arc			154	// arc				X Y XR YR START_ANGLE EXTENT_ANGLE ;
 #define t_rectanglef		155	// rectanglef			X Y W [H] ;
 #define t_rectangle		156	// rectangle			X Y W [H] ;
 #define t_triangle		157	// triangle			X Y X Y X Y ;
 #define t_drawtext		158	// drawtext			X Y S (stringval);
 #define t_drawscaledtext	159	// drawscaledtext		X Y XS YS (stringval);
 #define t_refreshmode		160	// refreshmode			(mode) ;    (0 refresh on, 1 refresh off)
 #define t_refresh		161	// refresh 			;
 #define t_gcol			162	// gcol				(rgb) ;  or it can be like this: (r) (g) (b) ;
 #define t_bgcol		163	// bgcol			(rgb) ;  or it can be like this: (r) (g) (b) ;  background colour
 #define t_cls			164	// cls				;
 #define t_drawmode		165	// drawmode			dm ;		set the drawing mode to 'dm'
#endif


#if allow_debug_commands
 #define t_tb		166	//	testbeep
 #define t_printstackframe 167	//	print everything in the current stack frame
 #define t_printentirestack 168	//	print everything in the stack up to the current stack frame
#endif

#define t_bad		255	//			bad data

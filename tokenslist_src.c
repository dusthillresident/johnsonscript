

#define t_nul		COCKSPENIS
#define t_plus		COCKSPENIS	// 	+		built in function: addition.		takes at least 2 parameters
#define t_minus		COCKSPENIS	//	-		built in function: subtraction.		takes at least 2 parameters
#define t_slash		COCKSPENIS	// 	/		built in function: division.		takes at least 2 parameters
#define t_star		COCKSPENIS	//	*		built in function: multiplication.	takes at least 2 parameters
#define t_mod		COCKSPENIS	//	%		built in function: modulo.		takes 2 parameters
#define t_shiftleft	COCKSPENIS	//	<<		built in function: shift left.		takes 2 parameters
#define t_shiftright	COCKSPENIS	//	>>		built in function: shift right.		takes 2 parameters
#define t_lshiftright	COCKSPENIS	//	>>>		built in function: logical shift right.	takes 2 parameters
#define t_and		COCKSPENIS	//	&		built in function: bitwise and.		takes at least 2 parameters
#define t_or		COCKSPENIS	//	|		built in function: bitwise or.		takes at least 2 parameters
#define t_eor		COCKSPENIS	//	^		built in function: bitwise exclusive or. takes at least 2 parameters
#define t_not		COCKSPENIS	//	~		built in function: bitwise not.		takes 1 parameter

#define t_abs		COCKSPENIS	//	abs		built in function: absolute value	takes 1 parameter
#define t_int		COCKSPENIS	//	int		built in function: integer		takes 1 parameter
#define t_sgn		COCKSPENIS	//	sgn		built in function: signum		takes 1 parameter
#define t_neg		COCKSPENIS	//	neg		built in function: negative		takes 1 parameter

#define t_lessthan	COCKSPENIS	//	<		built in function: less than		takes 2 parameters
#define t_morethan	COCKSPENIS	//	>		built in function: greater than		takes 2 parameters
#define t_lesseq	COCKSPENIS	//	<=		built in function: less than or equal	takes 2 parameters
#define t_moreeq	COCKSPENIS	//	>=		built in function: greater than or eq.	takes 2 parameters
#define t_equal		COCKSPENIS	//	=		built in function: equal		takes 2 parameters

#define t_land		COCKSPENIS	//	&&		built in function: logical and		takes at least 2 parameters
#define t_lor		COCKSPENIS	//	||		built in function: logical or		takes at least 2 parameters
#define t_lnot		COCKSPENIS	//	!		built in function: logical not		takes 1 parameter

#define t_D		COCKSPENIS	//	D		variable access (access the vars array)		takes 1 parameter
#define t_A		COCKSPENIS	//	A		array access (indexed access of the vars array)	takes 2 parameters
#define t_L		COCKSPENIS	//	L		local variable access (access values on the stack >= the stack pointer)
#define t_P		COCKSPENIS	//	P		function parameter variables (access values on the stack above the local variables)
#define t_F		COCKSPENIS	//	F		function call
#define t_SS		COCKSPENIS	//	S		create new unnamed stringvar (or find first unclaimed one)
#define t_C		COCKSPENIS	//	C		character access (work with strings as byte arrays) takes two parameters, a stringvalue and a value

#define t_Df		COCKSPENIS	//			fast variable access (basically D but with a direct pointer to the variable in token.data.pointer)
#define t_Af		COCKSPENIS	//			fast array access (basically A but with the array start index in token.data.i)
#define t_stackaccess	COCKSPENIS	//			access a value on the stack relative to the stack pointer, with token.data.i as the offset
#define t_Ff		COCKSPENIS	//			fast function call (basically F but with pointer to func_info in token.data.pointer)

#define t_number	COCKSPENIS	//			number constant
#define t_id		COCKSPENIS	//			identifier
#define t_getref	COCKSPENIS	//	getref		'get reference'. returns function number for named functions, variable array index for variables and stack accesses

// ===== string related functions that return numbers ======
				//	EXAMPLE				RETURNS		DESCRIPTION
#define t_ascS		COCKSPENIS	//	asc$ [string]			NUM		return number for first character of string
#define t_valS		COCKSPENIS	//	val$ [string]			NUM		return value of number in string
#define t_lenS		COCKSPENIS	//	len$ [string]			NUM		return length of string
#define t_cmpS		COCKSPENIS	//	cmp$ [string] [string]		NUM		strcmp 
#define t_instrS	COCKSPENIS	//	instr$ [stringa] [stringb]	NUM		return the position of stringb in stringa or -1 if not found
// =========================================================

#define t_rnd		COCKSPENIS	//	rnd [number]	built in function: pseudo random numbers	takes one parameter

#define t_alloc		COCKSPENIS	//	alloc [n]	allocate [n] number of items from the vars array

#define t_referredstringconst COCKSPENIS	// getref "stringconst"		creates an unnamed string variable which the stringconstant is copied to, then returns the reference number of the new string variable. note that the copy happens only once so any modifications to the string variable are permanent

// ------- maths -----------

#define t_tan		COCKSPENIS	//	tan
#define t_tanh		COCKSPENIS	//	tanh
#define t_atan		COCKSPENIS	//	atan
#define t_atan2		COCKSPENIS	//	atan2
#define t_acos		COCKSPENIS	//	acos
#define t_cos		COCKSPENIS	//	cos
#define t_cosh		COCKSPENIS	//	cosh
#define t_asin		COCKSPENIS	//	asin
#define t_sin		COCKSPENIS	//	sin
#define t_sinh		COCKSPENIS	//	sinh
#define t_exp		COCKSPENIS	//	exp
#define t_log		COCKSPENIS	//	log
#define t_log10		COCKSPENIS	//	log10
#define t_pow		COCKSPENIS	//	pow
#define t_sqr		COCKSPENIS	//	sqr
#define t_ceil		COCKSPENIS	//	ceil
#define t_floor		COCKSPENIS	//	floor
#define t_fmod		COCKSPENIS	//	fmod

// ------- functions related to file handling -----------
// these return the 'file number' of the file that was opened or 0 if there was an error
#define t_openin	COCKSPENIS	//	openin [stringval]		open a file in read only mode. 
#define t_openout	COCKSPENIS	//	openout [stringval]		open a file in write only mode
#define t_openup	COCKSPENIS	//	openup [stringval]		open a file in read/write mode
// ----				
#define t_eof		COCKSPENIS	//	eof [filenumber]		check if reached end of file
#define t_bget		COCKSPENIS	//	bget [filenumber]		read byte from file
#define t_vget		COCKSPENIS	//	vget [filenumber]		read 8 byte double from file
#define t_ptr		COCKSPENIS	//	ptr  [filenumber]		check current position in file
#define t_ext		COCKSPENIS	//	ext [filenumber]		check the current length of the file
// ------------------------------------------------------

#define t_leftb		COCKSPENIS	//	(

#define VALUES_END	t_leftb

#ifdef enable_graphics_extension
 // graphics extension functions 
 #define t_winw		COCKSPENIS		// takes no parameters, returns current window width
 #define t_winh		COCKSPENIS		// takes no parameters, returns current window height
 #define t_mousex	COCKSPENIS		// takes no parameters, returns current mouse x position	
 #define t_mousey	COCKSPENIS		// takes no parameters, returns current mouse y position
 #define t_mousez	COCKSPENIS		// takes no parameters, returns a value that changes when the mouse button is scrolled
 #define t_mouseb	COCKSPENIS		// takes no parameters, returns bitfield of the mouse buttons currently pressed
 #define t_readkey	COCKSPENIS		// takes no parameters, pulls a byte from the keyboard buffer and returns it
 #define t_keypressed	COCKSPENIS		// takes one parameter, checks the iskeypressed array and returns info about whether or not that key is currently pressed
 #define t_expose	COCKSPENIS		// takes no parameters, returns the state of a flag variable that gets set whenever there's an expose xwindows event, the flag is cleared after reading
 #define t_wmclose	COCKSPENIS		// takes no parameters, returns the state of a flag that gets set when the window manager close button has been clicked, flag is cleared after reading
 #define t_keybuffer	COCKSPENIS		// return the number of characters in the keyboard buffer
 #undef VALUES_END
 #define VALUES_END t_keybuffer
#endif

// =============================================================================================================================
// =============================================================================================================================
// ============ beyond here: not a 'value' =====================================================================================
// =============================================================================================================================
// =============================================================================================================================

// misc syntax stuff

#define t_rightb	COCKSPENIS		//	)
#define t_endstatement	COCKSPENIS		//	;		end of statement marker
#define t_comma		COCKSPENIS		//	,

#define t_deffn		COCKSPENIS		//	function [F value] ([ P num_params L num_locals] or [ param_id_one param_id_two [...] [local local_id_one local_id_two] ] ) ;
#define t_local		COCKSPENIS		//	local				used with 'function' keyword. ids after this are local variable names
#define t_ellipsis	COCKSPENIS		//	...				used with 'function' keyword. when put at the end of param list, specifies that function takes unlimited parameters.

// ---------------------
#define t_caseof	COCKSPENIS
#define t_when		COCKSPENIS
#define t_otherwise	COCKSPENIS
#define t_endcase	COCKSPENIS

#define t_caseofV	COCKSPENIS
#define t_caseofS	COCKSPENIS
// ---------------------

#define t_label		COCKSPENIS		//	.label		place label

// ========= commands ========= 

#define t_return		COCKSPENIS	//	return
#define t_while			COCKSPENIS	//	while
#define t_endwhile		COCKSPENIS	//	endwhile
#define t_if			COCKSPENIS	//	if
#define t_else			COCKSPENIS	//	else
#define t_endif			COCKSPENIS	//	endif
#define t_set			COCKSPENIS	//	set
#define t_var			COCKSPENIS	//	variable [identifier] ([identifier]) ... ;		declare variables	
#define t_arr			COCKSPENIS	//	array [identifier] value;	declare an array
#define t_const			COCKSPENIS	//	constant [identifier] value;	declare a constant
#define t_stringvar		COCKSPENIS	//	stringvar [identifier] ([number]) ... ; declare string variables. Optionally specify the starting bufsize
#define t_print			COCKSPENIS	//	print, will print string constants and/or values until it finds ';'
#define t_endfn			COCKSPENIS	//	endfunction	return from a function without returning a value (will return 0)
#define t_goto			COCKSPENIS	//	goto [string];		will search for a label with matching string and execution will continue from there
#define t_option		COCKSPENIS	//	option [string] [value] [etc];	this will be used to set options like stack size, variable array size, and possibly other things
#define t_wait			COCKSPENIS	//	wait [value];		usleep value*1000
#define t_oscli			COCKSPENIS	//	oscli [stringvalue];	system("string");
#define t_quit			COCKSPENIS	//	quit ([value]);		exit(value);
// ----- commands related to file handling -----
#define t_sptr			COCKSPENIS	//	sptr [filenumber] [value] ;			set position in file to [value]
#define t_bput			COCKSPENIS	//	bput [filenumber] [value] [...] ;		write bytes to file
#define t_vput			COCKSPENIS	//	vput [filenumber] [value] [...] ;		write 8 byte doubles to file
#define t_sput			COCKSPENIS	//	sput [filenumber] [stringvalue] [...] ;		write null terminated strings to file
#define t_close			COCKSPENIS	//	close [filenumber] ;				close an open file
// ---------------------------------------------
// ===== end of commands ======

// ===== fast modified versions of loop/control flow commands =====
#define t_gotof			COCKSPENIS	//      jump to position in token.i
#define t_whilef		COCKSPENIS	//	position of matching 'endwhile' in token.i
#define t_endwhilef		COCKSPENIS	//	position of matching 'while'+1 in token.i
#define t_iff			COCKSPENIS	//	position of matching else/endif in token.i
#define t_elsef			COCKSPENIS	//	position of matching endif in token.i
#define t_endiff		COCKSPENIS	//	matched endifs must be changed to avoid confusing the matching process for other if/else/endif blocks
// ===================================

// ===== string functions & stuff that's a 'string value' ======
					//		EXAMPLE				RETURNS		DESCRIPTION
#define t_stringconst		COCKSPENIS	//	"string"					string constant
#define t_stringconstf		COCKSPENIS	//							string constant (fast), eliminates the need to call strlen()
#define t_rightS		COCKSPENIS	//	right$ [string] [n]		STR		get the last n characters of string
#define t_leftS			COCKSPENIS	//	left$  [string] [n]		STR		get the first n characters of string
#define t_midS			COCKSPENIS	//	mid$ [string] [pos] [n]		STR		get n characters from string starting at pos
#define t_chrS			COCKSPENIS	//	chr$ [num]			STR		return a string with the character [num]
#define t_strS			COCKSPENIS	//	str$ [num]			STR		return string containing representation of [num]
#define t_catS			COCKSPENIS	//	cat$ [string] [string] ...	STR		concatenate strings
#define t_stringS		COCKSPENIS	//	string$ [number] [string]	STR		Function returning multiple copies of a string.
#define t_S			COCKSPENIS	//	$		string variable dereference
#define t_Sf			COCKSPENIS	//			fast string variable access (like $ but with pointer to stringvar in token.data.pointer)
#define t_sget			COCKSPENIS	//	sget [filenumber] [(num_bytes)]	read strings from files. if num_bytes is not given, it reads until it finds 0x0A
#define STRINGVALS_START t_stringconst
#define STRINGVALS_END   t_sget
#ifdef enable_graphics_extension // graphics extension stringvalues
 #define t_readkeyS		COCKSPENIS	// takes no parameters, pulls a byte from the keyboard buffer and returns it as a stringval
 #undef STRINGVALS_END
 #define STRINGVALS_END t_readkeyS
#endif


// ===== end of string functions & stuff that's a 'string value' ======

#ifdef enable_graphics_extension
 // ===== graphics extension commands =====
 // commands
 #define t_startgraphics	COCKSPENIS	// startgraphics		winwidth winheight ;
 #define t_stopgraphics		COCKSPENIS	// stopgraphics			;
 #define t_winsize		COCKSPENIS	// winsize			W H ;
 #define t_pixel		COCKSPENIS	// pixel			X Y ([X Y] ...) ;
 #define t_line			COCKSPENIS	// line				X Y X Y ([X Y] ...) ;
 #define t_circlef		COCKSPENIS	// circlef			X Y R ;
 #define t_circle		COCKSPENIS	// circle			X Y R ;
 #define t_rectanglef		COCKSPENIS	// rectanglef			X Y W [H] ;
 #define t_rectangle		COCKSPENIS	// rectangle			X Y W [H] ;
 #define t_triangle		COCKSPENIS	// triangle			X Y X Y X Y ;
 #define t_drawtext		COCKSPENIS	// drawtext			X Y S (stringval);
 #define t_refreshmode		COCKSPENIS	// refreshmode			(mode) ;    (0 refresh on, 1 refresh off)
 #define t_refresh		COCKSPENIS	// refresh 			;
 #define t_gcol			COCKSPENIS	// gcol				(rgb) ;  or it can be like this: (r) (g) (b) ;
 #define t_bgcol		COCKSPENIS	// bgcol			(rgb) ;  or it can be like this: (r) (g) (b) ;  background colour
 #define t_cls			COCKSPENIS	// cls				;
 #define t_drawmode		COCKSPENIS	// drawmode			dm ;		set the drawing mode to 'dm'
#endif


#if allow_debug_commands
 #define t_tb		COCKSPENIS	//	testbeep
 #define t_printstackframe COCKSPENIS	//	print everything in the current stack frame
 #define t_printentirestack COCKSPENIS	//	print everything in the stack up to the current stack frame
#endif

#define t_bad		255	//			bad data

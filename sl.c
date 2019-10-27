#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mylib.c"
#include <ctype.h> // isspace
#include <unistd.h> // usleep
#include <math.h>
#ifdef enable_graphics_extension
 #define myxlib_notstandalonetest
 #include "NewBase.c"
#endif

// memory problem debugging stuff
#if 0
 void* mymallocfordebug(size_t size    ,int n, char *f){
  void *out = malloc(size);
  printf("line %d,	%s:	MALLOC	%x\n",n,f,out);
  return out;
 }
 void* mycallocfordebug(size_t nmemb, size_t size    ,int n, char *f){
  void *out = calloc(nmemb,size);
  printf("line %d,	%s:	CALLOC	%x\n",n,f,out); 
  return out;
 }
 void* myreallocfordebug(void *ptr, size_t size    ,int n, char *f){
  void *out = realloc(ptr,size);
  printf("line %d,	%s:	REALLOC	%x\n",n,f,out);
  return out;
 }
 void myfreefordebug(void *ptr,      int n, char *f){
  printf("line %d,	%s:	FREEING	%x\n",n,f,ptr);
  free(ptr);
 }
 #define malloc(a) mymallocfordebug(a,__LINE__,__FILE__)
 #define calloc(a,b) mycallocfordebug(a,b,__LINE__,__FILE__)
 #define realloc(a,b) myreallocfordebug(a,b,__LINE__,__FILE__)
 #define free(p) myfreefordebug(p,__LINE__,__FILE__)
#endif

#define allow_debug_commands 1

struct token {
 unsigned char type;
 union {
  double number;
  void *pointer;
  int i; 
  //int i2[2];
 } data;
};
typedef struct token token;

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

#define t_Df		30	//			fast variable access (basically D but with a direct pointer to the variable in token.data.pointer)
#define t_Af		31	//			fast array access (basically A but with the array start index in token.data.i)
#define t_stackaccess	32	//			access a value on the stack relative to the stack pointer, with token.data.i as the offset
#define t_Ff		33	//			fast function call (basically F but with pointer to func_info in token.data.pointer)

#define t_number	34	//			number constant
#define t_id		35	//			identifier
#define t_getref	36	//	getref		'get reference'. returns function number for named functions, variable array index for variables and stack accesses

// ===== string related functions that return numbers ======
				//	EXAMPLE				RETURNS		DESCRIPTION
#define t_ascS		37	//	asc$ [string]			NUM		return number for first character of string
#define t_valS		38	//	val$ [string]			NUM		return value of number in string
#define t_lenS		39	//	len$ [string]			NUM		return length of string
#define t_cmpS		40	//	cmp$ [string] [string]		NUM		strcmp 
// =========================================================

#define t_rnd		41	//	rnd [number]	built in function: pseudo random numbers	takes one parameter

#define t_alloc		42	//	alloc [n]	allocate [n] number of items from the vars array

#define t_referredstringconst 43	// getref "stringconst"		creates an unnamed string variable which the stringconstant is copied to, then returns the reference number of the new string variable. note that the copy happens only once so any modifications to the string variable are permanent

// ------- maths -----------

#define t_tan		44	//	tan
#define t_tanh		45	//	tanh
#define t_atan		46	//	atan
#define t_atan2		47	//	atan2
#define t_acos		48	//	acos
#define t_cos		49	//	cos
#define t_cosh		50	//	cosh
#define t_asin		51	//	asin
#define t_sin		52	//	sin
#define t_sinh		53	//	sinh
#define t_exp		54	//	exp
#define t_log		55	//	log
#define t_log10		56	//	log10
#define t_pow		57	//	pow
#define t_sqr		58	//	sqr
#define t_ceil		59	//	ceil
#define t_floor		60	//	floor
#define t_fmod		61	//	fmod

#define t_leftb		62	//	(

#define VALUES_END	t_leftb

#ifdef enable_graphics_extension
 // graphics extension functions 
 #define t_winw			63	// takes no parameters, returns current window width
 #define t_winh			64	// takes no parameters, returns current window height
 #define t_mousex		65	// takes no parameters, returns current mouse x position	
 #define t_mousey		66	// takes no parameters, returns current mouse y position
 #define t_mousez		67	// takes no parameters, returns a value that changes when the mouse button is scrolled
 #define t_mouseb		68	// takes no parameters, returns bitfield of the mouse buttons currently pressed
 #define t_readkey		69	// takes no parameters, pulls a byte from the keyboard buffer and returns it
 #define t_keypressed		70	// takes one parameter, checks the iskeypressed array and returns info about whether or not that key is currently pressed
 #define t_expose		71	// takes no parameters, returns the state of a flag variable that gets set whenever there's an expose xwindows event, the flag is cleared after reading
 #define t_wmclose		72	// takes no parameters, returns the state of a flag that gets set when the window manager close button has been clicked, flag is cleared after reading
 #define t_keybuffer		73	// return the number of characters in the keyboard buffer
 #undef VALUES_END
 #define VALUES_END t_keybuffer
#endif

// =============================================================================================================================
// =============================================================================================================================
// ============ beyond here: not a 'value' =====================================================================================
// =============================================================================================================================
// =============================================================================================================================

// misc syntax stuff

#define t_rightb		74	//	)
#define t_endstatement		75	//	;		end of statement marker
#define t_comma			76	//	,

#define t_deffn			77	//	function [F value] ([ P num_params L num_locals] or [ param_id_one param_id_two [...] [local local_id_one local_id_two] ] ) ;
#define t_local			78	//	local				used with 'function' keyword. ids after this are local variable names
#define t_ellipsis		79	//	...				used with 'function' keyword. when put at the end of param list, specifies that function takes unlimited parameters.

#define t_label			80	//	.label		place label

// ========= commands ========= 

#define t_return		81	//	return
#define t_while			82	//	while
#define t_endwhile		83	//	endwhile
#define t_if			84	//	if
#define t_else			85	//	else
#define t_endif			86	//	endif
#define t_set			87	//	set
#define t_var			88	//	variable [identifier] ([identifier]) ... ;		declare variables	
#define t_arr			89	//	array [identifier] value;	declare an array
#define t_const			90	//	constant [identifier] value;	declare a constant
#define t_stringvar		91	//	stringvar [identifier] ([number]) ... ; declare string variables. Optionally specify the starting bufsize
#define t_print			92	//	print, will print string constants and/or values until it finds ';'
#define t_endfn			93	//	endfunction	return from a function without returning a value (will return 0)
#define t_goto			94	//	goto [string];		will search for a label with matching string and execution will continue from there
// --- not implemented yet ---
#define t_option		95	//	option [string] [value] [etc];	this will be used to set options like stack size, variable array size, and possibly other things
// ---------------------------
#define t_wait			96	//	wait [value];		usleep value*1000
// ===== end of commands ======

// ===== fast modified versions of loop/control flow commands =====
#define t_gotof			97      //      jump to position in token.i
#define t_whilef		98	//	position of matching 'endwhile' in token.i
#define t_endwhilef		99	//	position of matching 'while'+1 in token.i
#define t_iff			100	//	position of matching else/endif in token.i
#define t_elsef			101	//	position of matching endif in token.i
#define t_endiff		102	//	matched endifs must be changed to avoid confusing the matching process for other if/else/endif blocks
// ===================================

// ===== string functions & stuff that's a 'string value' ======
					//	EXAMPLE				RETURNS		DESCRIPTION
#define t_stringconst		103	//	"string"					string constant
#define t_stringconstf		104	//							string constant (fast), eliminates the need to call strlen()
#define t_rightS		105	//	right$ [string] [n]		STR		get the last n characters of string
#define t_leftS			106	//	left$  [string] [n]		STR		get the first n characters of string
#define t_midS			107	//	mid$ [string] [pos] [n]		STR		get n characters from string starting at pos
#define t_chrS			108	//	chr$ [num]			STR		return a string with the character [num]
#define t_strS			109	//	str$ [num]			STR		return string containing representation of [num]
#define t_catS			110	//	cat$ [string] [string] ...	STR		concatenate strings
// ----
#define t_S			111	//	$		string variable dereference
#define t_Sf			112	//			fast string variable access (like $ but with pointer to stringvar in token.data.pointer)
#define STRINGVALS_START t_stringconst
#define STRINGVALS_END   t_Sf
#ifdef enable_graphics_extension // graphics extension stringvalues
 #define t_readkeyS		113	// takes no parameters, pulls a byte from the keyboard buffer and returns it as a stringval
 #undef STRINGVALS_END
 #define STRINGVALS_END t_readkeyS
#endif


// ===== end of string functions & stuff that's a 'string value' ======

#ifdef enable_graphics_extension
 // ===== graphics extension commands =====
 // commands
 #define t_startgraphics	114	// startgraphics		winwidth winheight ;
 #define t_stopgraphics		115	// stopgraphics			;
 #define t_winsize		116	// winsize			W H ;
 #define t_pixel		117	// pixel			X Y ([X Y] ...) ;
 #define t_line			118	// line				X Y X Y ([X Y] ...) ;
 #define t_circlef		119	// circlef			X Y R ;
 #define t_circle		120	// circle			X Y R ;
 #define t_rectanglef		121	// rectanglef			X Y W [H] ;
 #define t_rectangle		122	// rectangle			X Y W [H] ;
 #define t_triangle		123	// triangle			X Y X Y X Y ;
 #define t_drawtext		124	// drawtext			X Y S (stringval);
 #define t_refreshmode		125	// refreshmode			(mode) ;    (0 refresh on, 1 refresh off)
 #define t_refresh		126	// refresh 			;
 #define t_gcol			127	// gcol				(rgb) ;  or it can be like this: (r) (g) (b) ;
 #define t_bgcol		128	// bgcol			(rgb) ;  or it can be like this: (r) (g) (b) ;  background colour
 #define t_cls			129	// cls				;
 #define t_drawmode		130	// drawmode			dm ;		set the drawing mode to 'dm'
#endif


#if allow_debug_commands
 #define t_tb		200	//	testbeep
 #define t_printstackframe 201	//	print everything in the current stack frame
 #define t_printentirestack 202	//	print everything in the stack up to the current stack frame
#endif

#define t_bad		255	//			bad data

//--------------------------
struct id_info;
typedef struct id_info id_info;
struct id_info {
 char *name; // the string of this identifier
 token t; // the kind of token it's meant to represent
 // ----
 id_info *next; // linked list
};
//--------------------------

//--------------------------
#define p_exact 0
#define p_atleast 1
struct func_info;
typedef struct func_info func_info;
struct func_info {
 int function_number; // used by 'getref'
 int params_type; // exact, at least
 int num_params;
 int num_locals;
 int start_pos; // starting position in the tokens array
 id_info *ids; // the names of the parameters & local variables
};
func_info initial_function = { -1, p_exact, 0, 0, 0, NULL };
//--------------------------

//--------------------------
#define DEFAULT_STRING_ACCUMULATOR_LEVELS 4
#define DEFAULT_NEW_STRINGVAR_BUFSIZE 256
struct stringvar { // string variable
 char *string;
 size_t len, bufsize;
 int string_variable_number; // used by 'getref'
};
typedef struct stringvar stringvar;
struct stringval { // string value
 char *string;
 size_t len;
};
typedef struct stringval stringval;
stringvar* newstringvar(size_t bufsize){
 stringvar *out = calloc(1,sizeof(stringvar));
 out->string = calloc(bufsize,sizeof(char));
 out->bufsize = bufsize;
 return out;
}
//--------------------------

//--------------------------
// this is for keeping track of all the strings that are part of the program data (string constants, ids, labels, etc) so that they can all be freed when the program is unloaded.
// but also note that for convenience, it may also be used for other kinds of pointer that aren't text strings, just because it's a convenient way to keep track of pointers that would need to be freed when the program is unloaded. for example, it's used in the case of t_stringconstf to keep track of a pointer to a stringval
struct stringslist;
typedef struct stringslist stringslist;
struct stringslist {
 char *string;
 stringslist *next; // linked list
};
void free_stringslist(stringslist *s){
 if(s == NULL) return;
 if(s->next) free_stringslist(s->next);
 if(s->string != NULL) free(s->string);
 free(s);
}
void stringslist_addstring(stringslist *s,char *string){
 if(s->next) stringslist_addstring(s->next,string);
 s->next = calloc(1,sizeof(stringslist));
 s->next->string = string;
}
//--------------------------

//--------------------------
#define MAX_FUNCS 256
struct program {
 token *tokens;
 int length;
 int maxlen;
 // ------------
 double *vars;
 int vsize;
 int next_free_var;
 // ------------
 double *stack;
 int ssize,sp,spo;
 // ------------
 func_info *functions[MAX_FUNCS];
 func_info *current_function;
 // ------------
 int getstringvalue_level;
 int max_string_accumulator_levels;
 stringvar **string_accumulator;
 int max_stringvars;
 stringvar **stringvars;
 //int next_free_stringvar;
 // ------------
 id_info *ids;
 stringslist *program_strings;
};
typedef struct program program;
//--------------------------

// -------------------------------------------------------------------------------------------
// ----------------------- PROTOTYPES --------------------------------------------------------
// -------------------------------------------------------------------------------------------

double getvalue(int *p, program *prog);
double interpreter(int p, program *prog);
void free_ids(id_info *ids);
token* tokenise(stringslist *progstrings, char *text, int *length_return);
token* loadtokensfromtext(stringslist *progstrings, char *path,int *length_return);
void process_function_definitions(program *prog);
void error(char *s);
stringval getstringvalue( program *prog, int *pos );
int isstringvalue(unsigned char type);
int isThisBracketAStringValue(program *prog, int p);

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

program* newprog(int maxlen, int vsize, int ssize){
 program *out = calloc(1,sizeof(program));
 // allocate memory for tokens
 if(maxlen){
  out->tokens = calloc(maxlen,sizeof(token));
  out->maxlen=maxlen;
 }
 // allocate memory for variables and stack
 if(vsize||ssize){
  out->vars = calloc(vsize+ssize,sizeof(double));
  out->stack= out->vars + vsize;
  out->vsize=vsize+ssize;
  out->ssize=ssize;
 }
 // initialise function context
 out->current_function = &initial_function;
 // initialise string accumulator 
 out->max_string_accumulator_levels = DEFAULT_STRING_ACCUMULATOR_LEVELS;
 out->string_accumulator = calloc(DEFAULT_STRING_ACCUMULATOR_LEVELS,sizeof(void*));
 int i;
 for(i=0; i<DEFAULT_STRING_ACCUMULATOR_LEVELS; i++){
  out->string_accumulator[ i ] = newstringvar(DEFAULT_NEW_STRINGVAR_BUFSIZE);
 }
 // initialise id list
 out->ids = calloc(1,sizeof(id_info));
 return out;
}

token maketoken( unsigned char type ){
 token out;
 out.type=type;
 out.data.pointer = NULL;
 return out;
}
token maketoken_num( double n ){
 token out;
 out.type = t_number;
 out.data.number = n;
 return out;
}
token maketoken_stackaccess( int sp_offset ){
 token out;
 out.type = t_stackaccess;
 out.data.i = sp_offset;
 return out; 
}
token maketoken_Df( double *pointer ){
 token out;
 out.type = t_Df;
 out.data.pointer = (void*)pointer;
 return out;
}
token maketoken_Sf( stringvar *pointer ){
 token out;
 out.type = t_Sf;
 out.data.pointer = (void*)pointer;
 return out;
}

void unloadprog(program *prog){
 int i;
 // free the strings that are part of the program data (string constants, ids, labels, etc)
 free_stringslist(prog->program_strings);
 // free the tokens, variable+stack array, identifier information
 free(prog->tokens);
 free(prog->vars);// this line also frees the stack array, because the stack array is simply the upper part of the variable array
 free_ids(prog->ids);
 // free the function information structures (and their identifier information)
 for(i=0; i<MAX_FUNCS; i++){ 
  if(prog->functions[i]){
   free_ids(prog->functions[i]->ids);
   free(prog->functions[i]);
  }
 }
 // free the string accumulator
 for(i=0; i<prog->max_string_accumulator_levels; i++){
  free( prog->string_accumulator[i]->string );
  free( prog->string_accumulator[i] );
 }
 free(prog->string_accumulator);
 // free the string variables
 for(i=0; i<prog->max_stringvars; i++){
  free( prog->stringvars[i]->string );
  free( prog->stringvars[i] );
 }
 free( prog->stringvars );
 // free the program struct itself
 free( prog );
}

#define DEFAULT_VSIZE 256
#define DEFAULT_SSIZE 256
program* init_program( char *str,int str_is_filepath ){
 token *tokens; int tokens_length; stringslist *program_strings = calloc(1,sizeof(stringslist));
 // get the tokens, either from program text in 'str', or by using 'str' as a filepath to load program text from a file.
 // at the same time, we also get the stringslist 
 if( str_is_filepath){
  tokens = loadtokensfromtext(program_strings,str,&tokens_length);
 }else{
  tokens = tokenise(program_strings,str,&tokens_length);
 }
 // check for 'option' commands that set things like stack size, variable array size, etc.
 int vsize = DEFAULT_VSIZE;
 int ssize = DEFAULT_SSIZE;
  //[put here]
 // optimise the tokens, searching for "D [literal constant]", "P [literal constant]", etc, and replacing them with the fast single token equivalents
  // [put here]
 // create the 'program' structure, either with default stack/variable array size, or with sizes that were set by option commands
 program *prog = newprog(0,vsize,ssize);
 prog->tokens = tokens; prog->length = tokens_length; prog->maxlen = tokens_length;
 prog->program_strings = program_strings;
 // search for function definitions and process them
 process_function_definitions(prog);
 return prog;
}

// ----------------------------------------------------------------------------------------------------------------

// this returns the allocated area as an index into the variable array
int allocate_variable_data(program *prog, int amount ){
 if( amount <= 0 ) error("allocate_variable_data: bad allocation request\n");
 if( prog->next_free_var+amount > (prog->vsize - prog->ssize) ) error("allocate_variable_data: run out of space\n");
 int index = prog->next_free_var; prog->next_free_var += amount;
 return index;
}

// ----------------------------------------------------------------------------------------------------------------

id_info* find_id(id_info *ids, char *id_string){
 if(ids == NULL){
  return NULL;
 }
 if( ids->name && !strcmp(id_string, ids->name) ){
  return ids; 
 }else{
  return find_id(ids->next,id_string);
 }
}

void add_id(id_info *ids, id_info *new_id){
 // check if this id name is already used in this id list
 if( ids->name && !strcmp(new_id->name, ids->name) ){
  printf("add_id: id was '%s'\n",new_id->name);
  error("add_id: this id is already used in this id list\n");
 }
 // ----------
 if( ids->next == NULL){
  ids->next = new_id;
 }else{
  add_id( ids->next, new_id);
 }
}

id_info* make_id(char *id_string, token t){
 id_info *out = calloc(1, sizeof(id_info));
 out->t = t;
 out->name = id_string;
 return out;
}

void free_ids(id_info *ids){
 if(ids == NULL ) return;
 free_ids(ids->next);
 free(ids);
}

void process_id(program *prog, token *t){
 id_info *foundid=NULL;
 // first, check current function's id list
 foundid = find_id( prog->current_function->ids, (char*)t->data.pointer );
 // if nothing was found, check the global id list
 if(foundid==NULL) foundid = find_id( prog->ids, (char*)t->data.pointer );
 // if nothing was still found, error
 if(foundid==NULL){
  printf("process_id: id was '%s'\n",(char*)t->data.pointer);
  error("process_id: unknown id\n");
 }
 // --- something was found ---
 // overwrite the token with the kind of token in the found id_info
 *t = foundid->t;
}

void list_ids(id_info *ids){
 if(ids == NULL ) return;
 if(ids->name)printf(" id: '%s'\n",ids->name);
 list_ids(ids->next);
}

char* getfuncname(program *prog,func_info *f){
 id_info *id = prog->ids;
 while(1){
  if( id->t.type == t_Ff && id->t.data.pointer == f ) return id->name;
  id = id->next;
  if( id == NULL ) return NULL;
 }
}
void list_all_ids(program *prog){
 printf("Global IDs\n");
 list_ids(prog->ids);
 int i;
 char *funcname;
 for(i=0;i<MAX_FUNCS;i++){
  if(prog->functions[i]){
   funcname = getfuncname( prog, prog->functions[i] );
   if(funcname)
    printf("The IDs of function %d ('%s')\n",i,funcname);
   else
    printf("The IDs of function %d\n",i);
   list_ids(prog->functions[i]->ids);
  }//endif function
 }//next i
}//endproc

// ----------------------------------------------------------------------------------------------------------------

// example of the two different kinds of function definitions. 
// the kind without named parameters or locals:
//  function F1 [P2['...']] [L3] ;
// the kind with named parameters or locals:
//  function myfunction [param_a param_b etc] ['...'] [[local] [localvariable_a localvariable_b etc]] ;
void process_function_definition(int pos,program *prog){
 int func_number=-1;
 func_info *out = calloc(1,sizeof(func_info));
 out->ids = calloc(1,sizeof(id_info));
 char *func_id=NULL;
 token tok = maketoken( t_Ff ); tok.data.pointer = (void*)out;
 
 // is this an 'unnamed and referred to by number'-style function or a 'named and referred to by name' function?
 switch( prog->tokens[pos].type ){
 case t_F:
  pos+=1;
  func_number = getvalue(&pos,prog);
  if(prog->functions[func_number]){ printf("process_function_definition: this function number is already used\n"); goto process_function_definition_errorout; }
  break;
 case t_id:
  // check if the id is allowed, eg, if it's not already used
   if( find_id(prog->ids, (char*)prog->tokens[pos].data.pointer) ){ printf("process_function_definition: this identifier is already used\n"); goto process_function_definition_errorout; }
  // find the next unused function number, error if there are no unused function numbers
   {int i;
    for(i=0; i<MAX_FUNCS; i++){
     if( !prog->functions[i] ){ func_number = i; i=MAX_FUNCS; }
    }
    if(func_number == -1){ printf("process_function_definition: too many functions have already been defined\n"); goto process_function_definition_errorout; }
   }
  // save the id's string in func_id, to be later added to the id list 
  func_id = (char*)prog->tokens[pos].data.pointer;
  pos +=1;
  break;
 default: printf("process_function_definition: expected F or identifier\n"); goto process_function_definition_errorout;
 }
 
 // is this a function with named parameters, or a function with unnamed parameters, or a function that has no parameters at all?
 switch( prog->tokens[pos].type ){
 case t_id: { // function with named parameters. 
  //get parameter ids, store them in the function's id list
   int num_ps = 0;
   while( prog->tokens[pos].type == t_id ){
    add_id( out->ids, make_id( (char*)prog->tokens[pos].data.pointer,  maketoken_stackaccess( num_ps | 0x80000000 ) ) ); // needs to be adjusted later once we know how many locals there are
    num_ps +=1;
    pos+=1;
   }
   //save the number of parameters
   out->num_params = num_ps;
   if( prog->tokens[pos].type == t_ellipsis ){ //ellipsis found, this function can take a variable number of parameters. set 'at least' parameters type
    out->params_type = p_atleast;
    pos+=1;
   }
   break;
  }
 case t_P: // function with unnamed parameters.
  pos+=1;
  out->num_params = (int)getvalue(&pos,prog);
  if( prog->tokens[pos].type == t_ellipsis ){ //ellipsis found, this function can take a variable number of parameters. set 'at least' parameters type
   out->params_type = p_atleast;
   pos+=1;
  }
  break;
 case t_endstatement: // function with no parameters (and also no locals)
  goto process_function_definition_normalout;
 case t_ellipsis: // function with at least 0 parameters
  out->params_type = p_atleast; pos+=1;
 }

 // is this a function with named locals, or a function with unnamed locals, or a function with no locals at all?
 switch( prog->tokens[pos].type ){
 case t_local: {// function with named locals
   pos+=1;
   //put here: get local ids, store them in the function's id list
   int num_ls = 0;
   while( prog->tokens[pos].type == t_id ){
    add_id( out->ids, make_id( (char*)prog->tokens[pos].data.pointer,  maketoken_stackaccess( num_ls /* make extra room for _num_params */ +(out->params_type==p_atleast) ) ) ); 
    num_ls +=1;
    pos+=1;
   }
   if( num_ls == 0 ){ printf("process_function_definition: local: expected at least one identifier\n"); goto process_function_definition_errorout; }
   //save the number of locals
   out->num_locals = num_ls;
   break;
  }
 case t_L: // function with unnamed locals
  pos+=1;
  out->num_locals = (int)getvalue(&pos,prog);
  break;
 case t_endstatement: // function with no locals
  goto process_function_definition_normalout;
 default: printf("process_function_definition: expected L or identifier\n"); goto process_function_definition_errorout;
 }


 
 // ready to end the process of defining the function
 process_function_definition_normalout:
 // --- store function number ---
 out->function_number = func_number;
 // --- one extra local ---
 // if this is a function that takes a variable number of parameters, then it must have space allocated for one extra local variable, that is _num_params (L0)
 out->num_locals += (out->params_type == p_atleast);
 // --- adjust parameter accesses ---
 // the local variables are placed on the stack before the parameters. But the parameters are defined before the local variables are. 
 // so once the local variables have been defined, it's necessary to adjust the quick references of the parameters.
 // parameters were marked earlier by ORing them with 0x80000000 so that it would be possible to tell them apart from the locals
 {
  id_info *idp = out->ids->next;
  while( idp && (idp->t.data.i & 0x80000000)){
   idp->t.data.i = (idp->t.data.i^0x80000000) + (out->num_locals);
   idp = idp->next;
  }
 }

 if( prog->tokens[pos].type != t_endstatement ){
  printf("process_function_definition: the function definition is complete but ';' was not found\n"); goto process_function_definition_errorout;
 }
 pos+=1;
 // set function execution start position
 out->start_pos = pos;
 // save id information if this is a named function
 if( func_id ) add_id(prog->ids, make_id( func_id, tok ) );
 prog->functions[func_number] = out;
 return;

 process_function_definition_errorout:
 // something went wrong, so abandon defining this function.
 // free all the stuff that was created for it and just fuckin return
 free_ids( out->ids );
 free( out );
 return; 
}
void process_function_definitions(program *prog){
 int i;
 for(i=0; i<prog->length; i++){ 
  if( prog->tokens[i].type == t_deffn ) process_function_definition(i+1,prog);
 }
}


// ----------------------------------------------------------------------------------------------------------------
// ----- OPTION ---------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// process the 'option' command, which allows you to set things like the variable array size, the stack array size,
// and other stuff

// option works like this:
// option [string or number] [parameters ...] ;
// you can specify options by string or by number. options may or may not take parameters, the number of parameters depends on each option

#define opt_vsize	0	// set variable array size
#define opt_ssize	1	// set stack array size

void option( program *prog, int *p ){
 *p += 1; // advance p out of the way of the 'option' command itself
 int id_stringconst_pos = *p;
 
 if( prog->tokens[*p].type == t_id ) process_id(prog, &prog->tokens[*p]);

 int opt_number=-1;

 if(  isThisBracketAStringValue(prog, *p)  ){ // identify option string
  stringval id_string = getstringvalue( prog, p );
  if( !strncmp( "vsize", id_string.string, id_string.len ) ){ opt_number=0; goto option__identify_option_string_out;}	//	vsize		Set the size of the variables array
  if( !strncmp( "ssize", id_string.string, id_string.len ) ){ opt_number=1; goto option__identify_option_string_out;}	//	ssize		Set the size of the stack array
  //if( !strncmp( "", id_string.string, id_string.len ) ){ opt_number=;	goto option__identify_option_string_out;}	//	
  option__identify_option_string_out:
  if( opt_number != -1 && prog->tokens[id_stringconst_pos].type == t_stringconst ){
   prog->tokens[id_stringconst_pos] = maketoken_num(opt_number);
  }
 }else{
  opt_number = getvalue(p,prog);
 }

 option_process_id_number:
 
 switch(opt_number){
 case 0: // vsize
 {
  int new_vsize = getvalue(p,prog);
  //printf("new_vsize %d \n",new_vsize);
  if( new_vsize <= 0 ) error("option: vsize: new vsize is less than or equal to 0\n");
  if( prog->next_free_var ) error("option: vsize: it's not possible to change the vsize after variables have been allocated\n");
  double *newvarray = calloc(new_vsize + prog->ssize, sizeof(double));
  if( newvarray == NULL ) error("option: vsize: failed to allocate memory\n");
  size_t oldsize = sizeof(double) * (prog->vsize-prog->ssize);
  size_t newsize = sizeof(double) * new_vsize;
  memcpy( (void*)newvarray, (void*)prog->vars, oldsize < newsize ? oldsize : newsize ); // copy variables
  memcpy( (void*)newvarray + new_vsize, (void*)prog->stack, sizeof(double) * prog->ssize );    // copy stack
  prog->vsize = prog->ssize + new_vsize;  // update the information in the program struct and also free the old vars+stack array
  free(prog->vars);
  prog->vars = newvarray;
  prog->stack = newvarray + new_vsize;
  break;
 }
 case 1: // ssize
 {
  int new_ssize = getvalue(p,prog);
  if( new_ssize <= 0 ) error("option: ssize: new ssize is less than or equal to 0\n");
  if( prog->next_free_var ) error("option: ssize: it's not possible to change the ssize after variables have been allocated\n");
  if( new_ssize <= prog->sp + (prog->current_function->num_locals + (prog->current_function->params_type==p_atleast ? prog->stack[ prog->sp ] : prog->current_function->num_params )) ) error("option: ssize: new stack size is too small to contain the current contents of the stack\n");
  prog->vsize -= prog->ssize;  prog->ssize = new_ssize;  prog->vsize += new_ssize; //update size information
  prog->vars = realloc( prog->vars, sizeof(double) * prog->vsize ); // reallocate vars+stack array
  if( prog->vars == NULL ) error("option: ssize: realloc failed\n");
  prog->stack = prog->vars + (prog->vsize - prog->ssize);
  break;
 }
 default: error("option: unrecognised option\n");
 }

}


// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

#if 0
int Ext(FILE *fp){
 int result,currentposition;
 currentposition=ftell(fp);
 fseek(fp, 0, SEEK_END); result=ftell(fp);
 fseek(fp, currentposition, SEEK_SET);
 return result;
}
#endif

int wordmatch(int *pos, unsigned char *word, unsigned char *text){
 int i,l;
 l=strlen(word);
 for(i=0; i<l; i++){
  if( text[*pos + i] != word[i] ) return 0;
 }
 *pos += l;
 return 1;
}

int patternmatch(int pos, unsigned char *chars, unsigned char *text){
 int i,l,L,p, check;
 p=pos;
 l=strlen(chars);
 while(1){
  check=1;
  for(i=0; i<l; i++){
   if( text[p] == chars[i] ){
    check=0;
    break;
   }//endif
  }//next
  if(check) return p-pos;
  p+=1;
 }//endwhile
}//endproc

int patterncontains(int pos, unsigned char *chars, unsigned char *text){
 int i,l,L,p, check;
 p=pos;
 l=strlen(chars);
 while( text[p] && !isspace(text[p]) ){
  for(i=0; i<l; i++){
   if( text[p] == chars[i] ){
    return 1;
   }//endif
  }//next
  p+=1;
 }//endwhile
 return 0;
}//endproc

void _gettoken_setstring(stringslist *progstrings, token *t, unsigned char *text, int l){
 //printf("FUCK PENIS FUCK\n");
 // allocate memory for string and copy string to it
 t->data.pointer = calloc(l+1,sizeof(char));
 strncpy((char*)t->data.pointer, (char*)text, l);
 // keep track of this string so it can be freed later when the program is unloaded
 if(progstrings != NULL)stringslist_addstring(progstrings,(char*)t->data.pointer);
}
void _gettoken_skippastcomments(int *pos, unsigned char *text){
 if( isspace(text[*pos]) ) return;
 // block comment
 if( wordmatch( pos,"/*", text) ){		
  while( !wordmatch( pos,"*/", text) ){
   *pos += 1;
   if( !text[*pos] ) error("missing */\n");
  }//endwhile
 }//endif
 // line comment
 if( wordmatch( pos,"REM", text) ){	
  while( text[*pos] && text[*pos]!=10 ){
   *pos += 1;
  }//endwhile
 }//endif
}//endproc
token gettoken(stringslist *progstrings, int test_run, int *pos, unsigned char *text){
 // if it's a test run, the token will be discarded, so strings must not be allocated

 token out;

 gettoken_start:
 _gettoken_skippastcomments(pos,text);
 while( isspace(text[*pos]) ){
  *pos+=1;
  _gettoken_skippastcomments(pos,text);
 }
 //printf("COCK ROCK %c\n",text[*pos]);

 int l;

#if allow_debug_commands
 if( wordmatch( pos,"testbeep", text) ){	//	
  out = maketoken( t_tb ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"printentirestack", text) ){	//	
  out = maketoken( t_printentirestack ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"printstackframe", text) ){	//	
  out = maketoken( t_printstackframe ); goto gettoken_normalout;
 }
#endif

 // ellipsis
 if( wordmatch( pos,"...", text) ){	//	...
  out =  maketoken( t_ellipsis ); goto gettoken_normalout;
 }

 // label
 if( text[*pos]=='.' ){	
  //printf("here\n");
  l = patternmatch( *pos+1,"_qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890.", text);
  if(l){
   // put here: allocate memory for identifier string and store it wherever
   out.type = t_label;
   if(!test_run) _gettoken_setstring(progstrings, &out,text+*pos,l+1);
   *pos += l+1;
   goto gettoken_normalout;
  }
 }

 // number literal/constant
 l = patternmatch( *pos,"-123456789.0", text);
 if( l && !(l==1 && text[*pos] == '-') && patterncontains( *pos,"1234567890", text) ){	
  out=maketoken_num( strtod(text+*pos, NULL) );
  *pos += l;
  goto gettoken_normalout;
 }

 if( wordmatch( pos,"%", text) ){	//	%
  out = maketoken( t_mod ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"<<", text) ){	//	<<
  out = maketoken( t_shiftleft ); goto gettoken_normalout;
 }
 if( wordmatch( pos,">>>", text) ){	//	>>>
  out = maketoken( t_lshiftright ); goto gettoken_normalout;
 }
 if( wordmatch( pos,">>", text) ){	//	>>
  out = maketoken( t_shiftright ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"&&", text) ){	//	&&
  out =  maketoken( t_land ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"||", text) ){	//	||
  out =  maketoken( t_lor ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"!", text) ){	//	!
  out =  maketoken( t_lnot ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"+", text) ){	//	+
  out =  maketoken( t_plus ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"-", text) ){	//	-
  out =  maketoken( t_minus ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"/", text) ){	//	/
  out =  maketoken( t_slash ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"*", text) ){	//	*
  out =  maketoken( t_star ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"&", text) ){	//	&
  out =  maketoken( t_and ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"|", text) ){	//	|
  out =  maketoken( t_or ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"^", text) ){	//	^
  out =  maketoken( t_eor ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"~", text) ){	//	~
  out =  maketoken( t_not ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"abs", text) ){	//	abs
  out =  maketoken( t_abs ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"int", text) ){	//	int
  out =  maketoken( t_int ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"sgn", text) ){	//	sgn
  out =  maketoken( t_sgn ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"neg", text) ){	//	neg
  out =  maketoken( t_neg ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"<=", text) ){	//	<=
  out =  maketoken( t_lesseq ); goto gettoken_normalout;
 }
 if( wordmatch( pos,">=", text) ){	//	>=
  out =  maketoken( t_moreeq ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"<", text) ){	//	<
  out =  maketoken( t_lessthan ); goto gettoken_normalout;
 }
 if( wordmatch( pos,">", text) ){	//	>
  out =  maketoken( t_morethan ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"=", text) ){	//	=
  out =  maketoken( t_equal ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"D", text) ){	//	D
  out =  maketoken( t_D ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"A", text) ){	//	A
  out =  maketoken( t_A ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"L", text) ){	//	A
  out =  maketoken( t_L ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"P", text) ){	//	A
  out =  maketoken( t_P ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"F", text) ){	//	A
  out =  maketoken( t_F ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"$", text) ){	//	$
  out =  maketoken( t_S ); goto gettoken_normalout;
 }

 if( wordmatch( pos,";", text) ){	//	;
  out =  maketoken( t_endstatement ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"(", text) ){	//	(
  out =  maketoken( t_leftb ); goto gettoken_normalout;
 }
 if( wordmatch( pos,")", text) ){	//	)
  out =  maketoken( t_rightb ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"return", text) ){	//	
  out = maketoken( t_return ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"endwhile", text) ){	//	
  out = maketoken( t_endwhile ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"while", text) ){	//	
  out = maketoken( t_while ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"endif", text) ){	//	
  out = maketoken( t_endif ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"if", text) ){	//	
  out = maketoken( t_if ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"else", text) ){	//	
  out = maketoken( t_else ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"set", text) ){	//	
  out = maketoken( t_set ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"function", text) ){	//	
  out = maketoken( t_deffn ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"local", text) ){	//	
  out = maketoken( t_local ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"variable", text) ){	//	
  out = maketoken( t_var ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"constant", text) ){	//	
  out = maketoken( t_const ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"stringvar", text) ){	//	
  out = maketoken( t_stringvar ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"endfunction", text) ){	//	endfunction
  out = maketoken( t_endfn ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"goto", text) ){	//	
  out = maketoken( t_goto ); goto gettoken_normalout;
 }

 // '_num_params', alias for L0, which is the parameter telling you the number of parameters a function has
 if( wordmatch( pos,"_num_params", text) ){	//	
  out = maketoken_stackaccess( 0 ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"getref", text) ){	//	getref
  out = maketoken( t_getref ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"@", text) ){	//	getref shorthand '@'
  out = maketoken( t_getref ); goto gettoken_normalout;
 }

 // string constant
 if( text[*pos]=='"' ){
  *pos += 1;
  l=0;
  while( text[*pos]!='"' ){
   switch( text[*pos] ){
   case 0: error("missing \"\n");
   case 10: error("missing \" on this line\n");
   }//endswitch
   l+=1;
   *pos += 1;
  }//endwhile
  *pos += 1;
  out.type = t_stringconst;
  if(!test_run) _gettoken_setstring(progstrings, &out,text+*pos-l-1,l);
  goto gettoken_normalout;
 }//endif

 if( wordmatch( pos,"print", text) ){		
  out = maketoken( t_print ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"right$", text) ){	//	right$
  out = maketoken( t_rightS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"left$", text) ){	//	left$
  out = maketoken( t_leftS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"mid$", text) ){	//	mid$
  out = maketoken( t_midS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"chr$", text) ){	//	chr$
  out = maketoken( t_chrS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"str$", text) ){	//	str$
  out = maketoken( t_strS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"cat$", text) ){	//	cat$
  out = maketoken( t_catS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"asc$", text) ){	//	asc$
  out = maketoken( t_ascS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"val$", text) ){	//	val$
  out = maketoken( t_valS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"len$", text) ){	//	len$
  out = maketoken( t_lenS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"cmp$", text) ){	//	cmp$ (strcmp)
  out = maketoken( t_cmpS ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"rnd", text) ){	//	rnd
  out = maketoken( t_rnd ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"wait", text) ){	//	
  out = maketoken( t_wait ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"alloc", text) ){	//	
  out = maketoken( t_alloc ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"option", text) ){	//	
  out = maketoken( t_option ); goto gettoken_normalout;
 }

 #ifdef enable_graphics_extension 
 // ============================================================================================
 // ======= GRAPHICS EXTENSION =================================================================
 // ============================================================================================
 
 if( wordmatch( pos,"startgraphics", text) ){	//	startgraphics
  out = maketoken( t_startgraphics ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"stopgraphics", text) ){	//	
  out = maketoken( t_stopgraphics ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"winsize", text) ){	//	winsize
  out = maketoken( t_winsize ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"pixel", text) ){	//	
  out = maketoken( t_pixel ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"line", text) ){	//	
  out = maketoken( t_line ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"circlef", text) ){	//	
  out = maketoken( t_circlef ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"circle", text) ){	//	
  out = maketoken( t_circle ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"rectanglef", text) ){	//	
  out = maketoken( t_rectanglef ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"rectangle", text) ){	//	
  out = maketoken( t_rectangle ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"triangle", text) ){	//	
  out = maketoken( t_triangle ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"drawtext", text) ){	//	
  out = maketoken( t_drawtext ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"refreshmode", text) ){	//	
  out = maketoken( t_refreshmode ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"refresh", text) ){	//	
  out = maketoken( t_refresh ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"gcol", text) ){	//	
  out = maketoken( t_gcol ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"bgcol", text) ){	//	
  out = maketoken( t_bgcol ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"cls", text) ){	//	
  out = maketoken( t_cls ); goto gettoken_normalout;
 }
 // -----
 if( wordmatch( pos,"winw", text) ){	//	
  out = maketoken( t_winw ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"winh", text) ){	//	
  out = maketoken( t_winh ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"mousex", text) ){	//	
  out = maketoken( t_mousex ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"mousey", text) ){	//	
  out = maketoken( t_mousey ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"mouseb", text) ){	//	
  out = maketoken( t_mouseb ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"mousez", text) ){	//	
  out = maketoken( t_mousez ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"readkey$", text) ){	//	
  out = maketoken( t_readkeyS ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"readkey", text) ){	//	
  out = maketoken( t_readkey ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"keypressed", text) ){	//	
  out = maketoken( t_keypressed ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"expose", text) ){	//	
  out = maketoken( t_expose ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"wmclose", text) ){	//	
  out = maketoken( t_wmclose ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"keybuffer", text) ){	//	
  out = maketoken( t_keybuffer ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"drawmode", text) ){	//	
  out = maketoken( t_drawmode ); goto gettoken_normalout;
 }
 // ============================================================================================
 // ======= END OF GRAPHICS EXTENSION ==========================================================
 // ============================================================================================
 #endif

 // ========= maths ==========
 if( wordmatch( pos,"tanh", text) ){	//	
  out = maketoken( t_tanh ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"tan", text) ){	//	
  out = maketoken( t_tan ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"atan2", text) ){	//	
  out = maketoken( t_atan2 ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"atan", text) ){	//	
  out = maketoken( t_atan ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"acos", text) ){	//	
  out = maketoken( t_acos ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"cosh", text) ){	//	
  out = maketoken( t_cosh ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"cos", text) ){	//	
  out = maketoken( t_cos ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"asin", text) ){	//	
  out = maketoken( t_asin ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"sinh", text) ){	//	
  out = maketoken( t_sinh ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"sin", text) ){	//	
  out = maketoken( t_sin ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"exp", text) ){	//	
  out = maketoken( t_exp ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"log10", text) ){	//	
  out = maketoken( t_log10 ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"log", text) ){	//	
  out = maketoken( t_log ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"pow", text) ){	//	
  out = maketoken( t_pow ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"sqr", text) ){	//	
  out = maketoken( t_sqr ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"ceil", text) ){	//	
  out = maketoken( t_ceil ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"floor", text) ){	//	
  out = maketoken( t_floor ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"fmod", text) ){	//	
  out = maketoken( t_fmod ); goto gettoken_normalout;
 }
 // ========= end of maths ========

 // identifier
 if( text[*pos]=='_' || (text[*pos]>='a' && text[*pos]<='z') ){	
  l = patternmatch( *pos+1,"_qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890", text);
  //if(l){
   // put here: allocate memory for identifier string and store it wherever
   out.type = t_id;
   if(!test_run) _gettoken_setstring(progstrings, &out,text+*pos,l+1);
   *pos += l+1;
   goto gettoken_normalout;
  //}
 }

/*
 if( wordmatch( pos,"", text) ){	//	
  out = maketoken( t_ ); goto gettoken_normalout;
 }
*/

 //printf("FUCK (%c) %d   pos %d\n",text[*pos],text[*pos], *pos);

// printf("*pos == %d: %d '%c'\n",*pos,text[*pos],text[*pos]);
// if(text[*pos])goto gettoken_start;

 if(!text[*pos]) printf("FUCK OFF!\n"); // this should never FUCKING happen

 gettoken_failout:
 printf("WARNING: garbage in input: \"");
 while( text[*pos] && !isspace(text[*pos]) ){
  printf("%c",text[*pos]);
  *pos+=1;
 }
 printf("\"\n");
 while( text[*pos] && isspace(text[*pos]) ){
  *pos+=1;
 }
 return maketoken( 255 );

 gettoken_normalout:
 while( text[*pos] && isspace(text[*pos]) ){
  *pos+=1;
  _gettoken_skippastcomments(pos,text);
 }

 return out;
}


int gettokens(stringslist *progstrings, token *tokens_return, int len, unsigned char *text){
 int count = 0, pos = 0;
 token t;
 while(pos<len){
  t = gettoken(progstrings,!tokens_return,&pos,text);
  //printf("gettoken: got: %d\n",t.type);  usleep(90000);
  if(tokens_return)tokens_return[count]=t;
  count +=1;
 }
 return count;
}

token* tokenise(stringslist *progstrings, char *text, int *length_return){
 int len = strlen(text);
 int count = gettokens(NULL,NULL,len,text);
 // about 'count+1': allocate 1 space for one extra token that goes unused (set to 0) so that there's less likely to be trouble with segmentation faults
 token *out = calloc(count+1,sizeof(token)); 
 gettokens(progstrings,out,len,text);
 if(length_return)*length_return = count;
 return out;
}

token* loadtokensfromtext(stringslist *progstrings, char *path,int *length_return){
 FILE *f = fopen(path,"rb");
 if(f==NULL) return NULL;
 int ext = Ext(f);
 unsigned char *text = calloc( ext+1, sizeof(unsigned char));
 fread((void*)text, sizeof(char), ext, f);
 token *out = tokenise(progstrings, text,length_return);
 fclose(f); free(text);
 return out;
}

// ----------------------------------------------------------------------------------------------------------------

char tokenstringbuf[256];

char* tokenstring(token t){
 switch(t.type){
 //case 0 : return "NUL";
 case t_plus:		return "+";
 case t_minus:		return "-";
 case t_slash:		return "/";
 case t_star:		return "*";

 case t_mod:		return "%";
 case t_shiftleft:	return "<<";
 case t_shiftright:	return ">>";
 case t_lshiftright:	return ">>>";

 case t_and:		return "&";
 case t_or:		return "|";
 case t_eor:		return "^";
 case t_not:		return "~";
 case t_abs:		return "abs";
 case t_int:		return "int";
 case t_sgn:		return "sgn";
 case t_neg:		return "neg";
 case t_lessthan:	return "<";
 case t_morethan:	return ">";
 case t_lesseq:		return "<=";
 case t_moreeq:		return ">=";
 case t_equal:		return "=";
 case t_D:		return "D";
 case t_A:		return "A";
 case t_L:		return "L";
 case t_P:		return "P";
 case t_F:		return "F";
 case t_deffn:		return "function";
 case t_local:		return "local";
 case t_ellipsis:	return "...";
 case t_number:
  {
   snprintf(tokenstringbuf,sizeof(tokenstringbuf),"%.2f",t.data.number);
   return tokenstringbuf;
  }
 case t_leftb:		return "(";
 case t_rightb:		return ")";
 case t_endstatement:	return ";";
 case t_goto:		return "goto";

 case t_return:		return "return";
 case t_while: case t_whilef:		return "while";
 case t_endwhile: case t_endwhilef:	return "endwhile";
 case t_if: case t_iff:			return "if";
 case t_else: case t_elsef:		return "else";
 case t_endif: case t_endiff:		return "endif";

 case t_set:		return "set";

 case t_land:		return "&&";
 case t_lor:		return "||";
 case t_lnot:		return "!";

 case t_var:		return "variable";
 case t_const:		return "constant";

 case t_getref:		return "getref";

 case t_print:		return "print";
 case t_rightS:		return "right$";
 case t_leftS:		return "left$";
 case t_midS:		return "mid$";
 case t_chrS:		return "chr$";
 case t_strS:		return "str$";
 case t_catS:		return "cat$";
 case t_ascS:		return "asc$";
 case t_valS:		return "val$";
 case t_lenS:		return "len$";
 case t_cmpS:		return "cmp$";

 case t_stringvar:	return "stringvar";
 case t_S:		return "$";

 case t_rnd:		return "rnd";
 case t_wait:		return "wait";
 case t_alloc:		return "alloc";
 case t_endfn:		return "endfunction";
 case t_option:		return "option";

 case  t_tanh:		return "tanh";
 case  t_tan:		return "tan";
 case  t_atan2:		return "atan2";
 case  t_atan:		return "atan";
 case  t_acos:		return "acos";
 case  t_cosh:		return "cosh";
 case  t_cos:		return "cos";
 case  t_asin:		return "asin";
 case  t_sinh:		return "sinh";
 case  t_sin:		return "sin";
 case  t_exp:		return "exp";
 case  t_log10:		return "log10";
 case  t_log:		return "log";
 case  t_pow:		return "pow";
 case  t_sqr:		return "sqr";
 case  t_ceil:		return "ceil";
 case  t_floor:		return "floor";
 case  t_fmod:		return "fmod";

#if allow_debug_commands
 case t_tb:	return "testbeep";
 case t_printstackframe: return "printstackframe";
 case t_printentirestack: return "printentirestack";
#endif
 
 case t_stackaccess:
 {
  if(t.data.i<=0)
   snprintf(tokenstringbuf,sizeof(tokenstringbuf),"stackaccess(P%d)",-t.data.i);
  else
   snprintf(tokenstringbuf,sizeof(tokenstringbuf),"stackaccess(L%d)",t.data.i);
  return tokenstringbuf;
 }
 case t_id:
 { 
  snprintf(tokenstringbuf,sizeof(tokenstringbuf),"ID(%s)",(char*)t.data.pointer);
  return tokenstringbuf;
 }
 case t_label:
 {
  snprintf(tokenstringbuf,sizeof(tokenstringbuf),"Label(%s)",(char*)t.data.pointer);
  return tokenstringbuf;
 }
 case t_stringconst:
 {
  snprintf(tokenstringbuf,sizeof(tokenstringbuf),"Stringconst(%s)",(char*)t.data.pointer);
  return tokenstringbuf;
 }
 case t_stringconstf:
 {
  snprintf(tokenstringbuf,sizeof(tokenstringbuf),"Stringconstf(%s)",((stringval*)t.data.pointer)->string);
  return tokenstringbuf;
 }

 #ifdef enable_graphics_extension 
 // ============================================================================================
 // ======= GRAPHICS EXTENSION =================================================================
 // ============================================================================================
 case t_startgraphics: return "startgraphics";
 case t_stopgraphics: return "stopgraphics";
 case t_winsize: return "winsize";
 case t_pixel: return "pixel";
 case t_line: return "line";
 case t_circlef: return "circlef";
 case t_circle: return "circle";
 case t_rectanglef: return "rectanglef";
 case t_rectangle: return "rectangle";
 case t_triangle: return "triangle";
 case t_drawtext: return "drawtext";
 case t_refreshmode: return "refreshmode";
 case t_refresh: return "refresh";
 case t_gcol: return "gcol";
 case t_bgcol: return "bgcol";
 case t_cls: return "cls";
 // ----
 case t_winw: return "winw";
 case t_winh: return "winh";
 case t_mousex: return "mousex";
 case t_mousey: return "mousey";
 case t_mousez: return "mousez";
 case t_mouseb: return "mouseb";
 case t_readkeyS: return "readkeyS";
 case t_readkey: return "readkey";
 case t_keypressed: return "keypressed";
 case t_expose:	return "expose";
 case t_wmclose: return "wmclose";
 case t_keybuffer: return "keybuffer";
 case t_drawmode: return "drawmode";
 // ============================================================================================
 // ======= END OF GRAPHICS EXTENSION ==========================================================
 // ============================================================================================
 #endif

 //case 255: return "BadData";
 default:
  {
   snprintf(tokenstringbuf,sizeof(tokenstringbuf),"BadData(%d)",t.type);
   return tokenstringbuf;
  }
 }
}

void detokenise(program *prog, int mark_position){
 int i;
 for( i=0; i<prog->length; i++){
  if(i==mark_position) printf("\nREM: here\n");
  printf( (prog->tokens[i].type==t_endstatement || prog->tokens[i].type==t_label) ? "%s\n":"%s ", tokenstring( prog->tokens[i] ) );
 }
}

// ----------------------------------------------------------------------------------------------------------------
// -------------------- STRING HANDLING ---------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

void mystrncpy(char *dest, char *src, size_t n){
 //strncpy(dest,src,n); return;
 size_t i;
 for(i=0; i<n /*&& src[i]*/; i++){
  dest[i]=src[i];
 }
 dest[i]=0;
}

void stringvar_adjustsizeifnecessary(stringvar *sv, int bufsize_required, int preserve){
 if(bufsize_required<sv->bufsize){
  tb();
  return;
 }
 int new_bufsize = bufsize_required+(256-bufsize_required % 256);
 if(preserve){
  sv->string = realloc(sv->string, new_bufsize);
  if(sv->string == NULL) error("stringvar_adjustsize: realloc failed\n");
 }else{
  free(sv->string);
  sv->bufsize = new_bufsize;
  sv->string = malloc(new_bufsize);
  *sv->string = 0;
 }
}

void copy_stringval_to_stringvar(stringvar *dest, stringval src){
 if( src.len+1 >= dest->bufsize ) stringvar_adjustsizeifnecessary(dest, src.len+1, 0);
 mystrncpy(dest->string, src.string, src.len);
 dest->len = src.len;
}

int isstringvalue(unsigned char type){
 return (type==t_leftb || type==t_id || (type>=STRINGVALS_START && type<=STRINGVALS_END)) ;
}
int isThisBracketAStringValue(program *prog, int p){
 while( prog->tokens[p].type == t_leftb ){
  p+=1;
 }
 return(isstringvalue(prog->tokens[p].type));
}

stringval getstringvalue( program *prog, int *pos ){
 int level = prog->getstringvalue_level;
 // check if there's a string accumulator for this level & if not, prepare one 
 if(level >= prog->max_string_accumulator_levels){
  prog->string_accumulator = realloc((void*)prog->string_accumulator, sizeof(void*)*(prog->max_string_accumulator_levels+1));
  prog->string_accumulator[prog->max_string_accumulator_levels] = newstringvar(DEFAULT_NEW_STRINGVAR_BUFSIZE);
  prog->max_string_accumulator_levels += 1;
 }
 stringvar *accumulator = prog->string_accumulator[level];
 token t;
 getstringvalue_start:
 t = prog->tokens[ *pos ]; *pos += 1;
 switch( t.type){
 case t_S:
 {
  int stringvar_num = (int)getvalue(pos,prog);
  if( stringvar_num<0 || stringvar_num >= prog->max_stringvars ){
   printf("stringvar_num: %d\n",stringvar_num);
   error("getstringvalue: bad stringvariable access\n");
  }
  stringvar *in = prog->stringvars[stringvar_num];
  stringval out;
  out.string = in->string;
  out.len = in->len;
  return out;
 }
 case t_Sf:
 {
  stringvar *in = t.data.pointer;
  stringval out;
  out.string = in->string;
  out.len = in->len;
  return out;
 }
 case t_id:
 {
  *pos-=1;
  process_id(prog, &prog->tokens[*pos]);
  goto getstringvalue_start;
 }
 case t_stringconst:
 {
  stringval out;
  out.string = (char*)t.data.pointer;
  out.len = strlen(out.string);
  // convert this string constant into a fast string constant, so we will not need to call strlen() for it ever again
  stringval *fval = calloc(1,sizeof(stringval)); *fval = out;
  prog->tokens[ *pos -1 ].type = t_stringconstf;
  prog->tokens[ *pos -1 ].data.pointer = (void*)fval;  
  // keep track of the pointer for this ready-made stringval using the stringslist, so it will be freed when the program is unloaded
  stringslist_addstring(prog->program_strings,(char*)fval);
  return out;
 }
 case t_stringconstf:
  return *((stringval*)t.data.pointer);
 case t_leftS:
 {
  stringval in = getstringvalue( prog,pos );
  int len = getvalue(pos, prog);
  if(len>in.len)len=in.len; if(len<0)len=0;
  in.len = len;
  return in;
 }
 case t_rightS:
 {
  stringval in = getstringvalue( prog,pos );
  int len = getvalue(pos, prog);
  if(len>in.len)len=in.len; if(len<0)len=0;
  in.string += (in.len-len);
  in.len = len;
  return in;
 }
 case t_midS:
 {
  stringval sv = getstringvalue( prog,pos );
  int midpos = getvalue(pos, prog);
  int len = getvalue(pos, prog);
  if( sv.len<=0 || midpos < 0 || midpos >= sv.len ){
   sv.len=0;
   return sv;
  }
  if( midpos + len >= sv.len ){
   len -= ((midpos+len) - sv.len);
  }
  sv.string += midpos;
  sv.len = len;
  return sv;
 }
 case t_catS:
 {
  prog->getstringvalue_level += 1;
  stringval in;
  int bufsize_required;
  accumulator->len=0;
  while( isstringvalue( prog->tokens[ *pos ].type ) ){
   in = getstringvalue( prog,pos );
   bufsize_required = accumulator->len + in.len+1;
   if( bufsize_required > accumulator->bufsize ) stringvar_adjustsizeifnecessary(accumulator, bufsize_required, 1);
   mystrncpy( accumulator->string + accumulator->len,  in.string, in.len );
   accumulator->len += in.len;
  }
  stringval out;
  out.len = accumulator->len;
  out.string = accumulator->string;
  prog->getstringvalue_level -= 1;
  return out;
 }
 case t_strS:
 {
  double val = getvalue(pos,prog);
  int snpf_return;
  if(!isnan(val) && val == (double)(int)val){
   snpf_return = snprintf(accumulator->string, accumulator->bufsize, "%d",(int)val);
  }else{
   snpf_return = snprintf(accumulator->string, accumulator->bufsize, "%f",val);
  }
  stringval out;
  out.len = snpf_return>=accumulator->bufsize ? accumulator->bufsize-1 : snpf_return;
  out.string = accumulator->string;
  return out;
 }
 case t_chrS:
 {
  accumulator->string[0] = (char)getvalue(pos,prog);
  accumulator->string[1] = 0;
  stringval out;
  out.len=1;
  out.string=accumulator->string;
  return out;  
 }
 case t_leftb:
 {
  stringval out = getstringvalue( prog,pos );
  if( prog->tokens[ *pos ].type != t_rightb ) error("getstringvalue: expected closing bracket\n");
  *pos += 1;
  return out;
 }
 #ifdef enable_graphics_extension 
 // ============================================================================================
 // ======= GRAPHICS EXTENSION =================================================================
 // ============================================================================================
 case t_readkeyS:
 {
  accumulator->string[0] = (GET()&255);
  stringval out;
  out.string = accumulator->string;
  out.len = 1;
  return out;
 }
 // ============================================================================================
 // ======= END OF GRAPHICS EXTENSION ==========================================================
 // ============================================================================================
 #endif
 default:
  printf("getstringvalue: token is %s\n",tokenstring(t));
  error("getstringvalue: didn't find a stringvalue\n");
 }//endswitch
}//endproc

stringvar* create_new_stringvar(program *prog,size_t bufsize){
 int svnum = prog->max_stringvars;
 prog->stringvars = realloc(prog->stringvars, sizeof(void*)*(svnum+1));
 prog->stringvars[svnum] = newstringvar(bufsize);
 prog->stringvars[svnum]->string_variable_number = svnum; // used by 'getref'
 prog->max_stringvars += 1;
 return prog->stringvars[svnum];
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

void error(char *s){
 #ifdef enable_graphics_extension
 if(newbase_is_running){
  Wait(1);
  MyCleanup();
 }
 #endif
 printf("%s",s);
 exit(0); 
}

int isvalue(unsigned char type){
 return ( type && (type <= VALUES_END) ) ;
}

// ========================================================
// =========  GETVALUE  ===================================
// ========================================================

// this __attribute__ seems to make the interpreter run faster (tested with "spdtest_")
double __attribute__((aligned(32))) getvalue(int *p, program *prog){
 token t;
 getvalue_start:
 t = prog->tokens[*p]; *p += 1;
 switch( t.type ){
 case t_number:
  return t.data.number;
 case t_plus:
 {
  double out=getvalue(p, prog) + getvalue(p, prog);
  while( isvalue( prog->tokens[*p].type ) ){
   out += getvalue(p, prog);
  }
  return out;
 }
 case t_minus:
 {
  double out=getvalue(p, prog) - getvalue(p, prog);
  while( isvalue( prog->tokens[*p].type ) ){
   out -= getvalue(p, prog);
  }
  return out;
 }
 case t_slash:
 {
  double out=getvalue(p, prog) / getvalue(p, prog);
  while( isvalue( prog->tokens[*p].type ) ){
   out /= getvalue(p, prog);
  }
  return out;
 }
 case t_star:
 {
  double out=getvalue(p, prog) * getvalue(p, prog);
  while( isvalue( prog->tokens[*p].type ) ){
   out *= getvalue(p, prog);
  }
  return out;
 }

 case t_mod:
 {
  int a,b;
  a = getvalue(p, prog); b = getvalue(p, prog);
  return (double)(a % b);
 }
 case t_shiftleft:
 {
  int a,b;
  a = getvalue(p, prog); b = getvalue(p, prog);
  return (double)(a<<b);
 }
 case t_shiftright:
 {
  int a,b;
  a = getvalue(p, prog); b = getvalue(p, prog);
  return (double)(a>>b);
 }
 case t_lshiftright:
 {
  int a,b;
  a = getvalue(p, prog); b = getvalue(p, prog);
  return (double)(int)((unsigned int)a >> (unsigned int)b);
 }

 case t_and:
 {
  int out = (int)getvalue(p, prog) & (int)getvalue(p, prog);
  while( isvalue( prog->tokens[*p].type ) ){
   out &= (int)getvalue(p, prog);
  }
  return (double)out;
 }
 case t_or:
 {
  int out=(int)getvalue(p, prog) | (int)getvalue(p, prog);
  while( isvalue( prog->tokens[*p].type ) ){
   out |= (int)getvalue(p, prog);
  }
  return (double)out;
 }
 case t_eor:
 {
  int out=(int)getvalue(p, prog) ^ (int)getvalue(p, prog);
  while( isvalue( prog->tokens[*p].type ) ){
   out ^= (int)getvalue(p, prog);
  }
  return (double)out;
 }
 case t_not:
 {
  int out=getvalue(p, prog);
  return (double)(~out);
 }

 case t_abs:
 {
  double out=getvalue(p, prog);
  return out<0 ? -out : out;
 }
 case t_int:
 {
  return (int)getvalue(p, prog);
 }
 case t_sgn:
 {
  double out=getvalue(p, prog);
  return ((out>0)-(out<0));
 }
 case t_neg:
 {
  return -getvalue(p, prog);
 }

 case t_lessthan:
 {
  double a,b;
  a=getvalue(p,prog);
  b=getvalue(p,prog);
  return a<b;
 }
 case t_morethan:
 {
  double a,b;
  a=getvalue(p,prog);
  b=getvalue(p,prog);
  return a>b;
 }
 case t_lesseq:
 {
  double a,b;
  a=getvalue(p,prog);
  b=getvalue(p,prog);
  return a<=b;
 }
 case t_moreeq:
 {
  double a,b;
  a=getvalue(p,prog);
  b=getvalue(p,prog);
  return a>=b;
 }
 case t_equal:
 {
  double a,b;
  a=getvalue(p,prog);
  b=getvalue(p,prog);
  return a==b;
 }

 case t_land:
 {
  int out=getvalue(p, prog);
  double v;
  while( isvalue( prog->tokens[*p].type ) ){
   v = getvalue(p, prog);
   out = out && (int)v;
  }
  return (double)out;
 }
 case t_lor:
 {
  int out=getvalue(p, prog);
  double v;
  while( isvalue( prog->tokens[*p].type ) ){
   v = getvalue(p, prog);
   out = out || (int)v;
  }
  return (double)out;
 }
 case t_lnot:
 {
  int out=getvalue(p, prog);
  return (double)(!out);
 }

 case t_rnd:
 {
  return (double)Rnd((int)getvalue(p,prog));
 }

 case t_leftb:
 {
  double out = getvalue(p, prog);
  if( prog->tokens[*p].type != t_rightb ) error("expected closing bracket\n");
  *p += 1;
  return out;
 }

 case t_D:
 {
  int index = getvalue(p, prog);
  if( index>=0 && index<prog->vsize) return prog->vars[ index ];
  error("getvalue: bad variable access\n");
  break;
 }
 case t_Df:
 {
  return *(double*)t.data.pointer;
 }
 case t_A:
 {
  int index = (int)getvalue(p,prog) + (int)getvalue(p,prog);
  if( index>=0 && index<prog->vsize) return prog->vars[ index ];
  error("getvalue: bad array access\n");
  break;
 }
 case t_Af:
 { 
  int index = t.data.i + (int)getvalue(p,prog);
  if( index>=0 && index<prog->vsize) return prog->vars[ index ];
  error("getvalue: bad array access\n");
  break; 
 }
 case t_P:
 {
  int index = prog->sp + prog->current_function->num_locals + (int)getvalue(p,prog);
  if(index<0 || index>=prog->ssize) error("getvalue: bad parameter access\n");
  return prog->stack[index];
 }
 case t_L:
 {
  int index = prog->sp + (int)getvalue(p,prog);
  if(index<0 || index>=prog->ssize) error("getvalue: bad local variable access\n");
  return prog->stack[index];
 }
 case t_stackaccess:
 {
  int index = prog->sp + t.data.i;
  //printf("FUCK %d\n",index);
  if(index<0 || index>=prog->ssize) error("getvalue: bad stack access\n");
  return prog->stack[index];
 }

 case t_id:
 {
  *p-=1;
  process_id(prog, &prog->tokens[*p]);
  goto getvalue_start;
 }

 case t_ascS:
 {
  stringval sv = getstringvalue(prog,p);
  if( sv.len == 0 ) return -1;
  return (double)(unsigned char)sv.string[0];
 }
 case t_valS:
 {
  stringval sv = getstringvalue(prog,p);
  return strtod(sv.string,NULL);
 }
 case t_lenS:
 {
  stringval sv = getstringvalue(prog,p);
  return (double)sv.len;
 }
 /*
 case t_cmpS:
 {
 }
 */

 /* ======== start of maths ======== */
 case  t_tan:
  return tan( getvalue(p,prog) );
 case  t_tanh:	
  return tanh( getvalue(p,prog) );
 case  t_atan:	
  return atan( getvalue(p,prog) );
 case  t_atan2:
 {
  double a,b;
  a=getvalue(p,prog); b=getvalue(p,prog);
  return atan2(a,b);
 }
 case  t_acos:	
  return acos(getvalue(p,prog));
 case  t_cos:	
  return cos(getvalue(p,prog));
 case  t_cosh:	
  return cosh(getvalue(p,prog));
 case  t_asin:	
  return asin(getvalue(p,prog));
 case  t_sin:	
  return sin(getvalue(p,prog));
 case  t_sinh:	
  return sinh(getvalue(p,prog));
 case  t_exp:	
  return exp( getvalue(p,prog) );
 case  t_log:	
  return log(getvalue(p,prog));
 case  t_log10:	
  return log10(getvalue(p,prog));
 case  t_pow:	
 {
  double a,b;
  a=getvalue(p,prog); b=getvalue(p,prog);
  return pow(a,b);
 }
 case  t_sqr:
  return sqrt(getvalue(p,prog));
 case  t_ceil:
  return ceil(getvalue(p,prog));
 case  t_floor:
  return floor(getvalue(p,prog));
 case  t_fmod:
 {
  double a,b;
  a=getvalue(p,prog); b=getvalue(p,prog);
  return fmod(a,b);
 }
 /* ======== end of maths ======== */
 

 case t_getref: getref_start:
 {
  t = prog->tokens[*p]; *p+=1;
  switch(t.type){
  case t_D: case t_F: case t_S:// this is something you wouldn't really do, but it's supported anyway
   return getvalue(p, prog);
  case t_Ff:
   return ((func_info*)t.data.pointer)->function_number;
  case t_Sf:
   return ((stringvar*)t.data.pointer)->string_variable_number;
  case t_Df:
   return (double)( ( (void*)t.data.pointer - (void*)prog->vars  )/sizeof(double) );
  case t_stackaccess:
   return (prog->vsize-prog->ssize) + (prog->sp + t.data.i);
  case t_P:
   return (prog->vsize-prog->ssize) + ( prog->sp + prog->current_function->num_locals + (int)getvalue(p, prog));
  case t_L:
   return (prog->vsize-prog->ssize) + ( prog->sp + (int)getvalue(p, prog));
  case t_id:
   *p-=1;
   process_id(prog, &prog->tokens[*p]);
   goto getref_start;
  case t_stringconst:
   {
    // move p back so the current token is the string constant
    *p-=1;
    // create new string variable and copy the string constant to it
    stringvar *referred_string_constant = create_new_stringvar(prog,DEFAULT_NEW_STRINGVAR_BUFSIZE);
    copy_stringval_to_stringvar(referred_string_constant, getstringvalue(prog, p) );
    // replace the getref token with a referredstringconst
    prog->tokens[*p-2].type = t_referredstringconst;
    prog->tokens[*p-2].data.i = referred_string_constant->string_variable_number;
    // return the reference number of the new string variable
    return referred_string_constant->string_variable_number;
   }
  default: error("getvalue: bad use of getref\n");
  }
  break;
 }
 case t_referredstringconst:
  *p+=1; // skip past the string constant
  return t.data.i; // return the reference number of the string variable that was created by getref for the string constant
 case t_alloc:
 {
  int amount = getvalue(p,prog);
  return allocate_variable_data(prog, amount);
 }

 // FUNCTION CALL
 {
  func_info *finfo_cur;
  func_info *finfo_new;
  int fnum;
  int oldsp,newsp,spp,ospo, minimum_stack_required, _num_params, i;
  double func_return;
  {
   case t_F:
   finfo_cur = prog->current_function;
   fnum = getvalue(p,prog); if( (fnum<0 || fnum>MAX_FUNCS) || !prog->functions[fnum] ) error("getvalue: bad function call\n");
   func_info *finfo_new = prog->functions[fnum];
   goto getvalue_functioncall;
   
   case t_Ff: 
   finfo_cur = prog->current_function;
   finfo_new = t.data.pointer;
 
   getvalue_functioncall:

   // move the stack pointer up out of the way of the stack frame underneath it (and also the stack frame that is currently being created, represented by spo)
   // need to know the current stack frame size (current number of locals + current number of parameters )
   ospo = prog->spo;
   oldsp = prog->sp; newsp = oldsp + finfo_cur->num_locals + (finfo_cur->params_type==p_atleast ? prog->stack[oldsp] : finfo_cur->num_params) + prog->spo;

   // check if there's space on the stack for the new stack frame (new number of locals + new number of params) and if not, error
   minimum_stack_required = finfo_new->num_locals + finfo_new->num_params;
   if( newsp + minimum_stack_required > prog->ssize ) error("interpreter: stack overflow\n");
   // make space for local variables
   spp = newsp + finfo_new->num_locals;
   prog->spo += finfo_new->num_locals;
   // read in the parameters
   _num_params = finfo_new->num_params;
   for(i=0; i<_num_params; i++){
    prog->stack[ spp ] = getvalue(p, prog);
    spp+=1;
    prog->spo += 1;
   }
   // if this is a function with variable number of parameters:
   //  continue reading parameters as long as there are more to add (checking that there is stack space for it with each one added, if not, error)
   //  store number of params in local variable 0 (_num_params)
   if( finfo_new->params_type == p_atleast ){
    while( isvalue( prog->tokens[*p].type ) ){
     if( spp >= prog->ssize ) error("interpreter: stack overflow\n");
     prog->stack[ spp ] = getvalue(p, prog);
     spp+=1;
     prog->spo += 1;
     _num_params+=1;
    }
    //spp+=1; prog->spo += 1; // is this line correct?
    prog->stack[ newsp ] = _num_params;
   }

   // set new context and call the function
   prog->sp = newsp;
   prog->current_function = finfo_new;
   prog->spo=0;
   func_return = interpreter( finfo_new->start_pos, prog );
   prog->spo = ospo;
   prog->current_function = finfo_cur;
   prog->sp = oldsp;
   return func_return;
  }//block2
 }//block1
 #ifdef enable_graphics_extension 
 // ============================================================================================
 // ======= GRAPHICS EXTENSION =================================================================
 // ============================================================================================
 case t_winw:
 {
  return WinW;
 }
 case t_winh:
 {
  return WinH;
 }
 case t_mousex:
 {
  return mouse_x;
 }
 case t_mousey:
 {
  return mouse_y;
 }
 case t_mousez:
 {
  return mouse_z;
 }
 case t_mouseb:
 {
  return mouse_b;
 }
 case t_readkey:
 {
  return (double)GET();
 }
 case t_keypressed:
 {
  int keynum = getvalue(p,prog);
  if(iskeypressed==NULL) return 0;
  return iskeypressed[keynum&0xff];
 }
 case t_expose:
 {
  int expo = exposed;
  exposed = 0;
  return expo;
 }
 case t_wmclose:
 {
  int wmc = wmclosed;
  wmclosed = 0;
  return wmc;
 }
 case t_keybuffer:
 {
  return KeyBufferUsedSpace();
 }
 // ============================================================================================
 // ======= END OF GRAPHICS EXTENSION ==========================================================
 // ============================================================================================
 #endif
 default:
  printf("getvalue: t.type == %d (%s)\n", t.type, tokenstring(t) );
  error("getvalue: expected a value\n");
 }//endswitch t.type
}//endproc getvalue

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

int _interpreter_ifsearch(int p, program *prog){
 int levelcount=1;
 while(levelcount){
  if(p>=prog->length) error("interpreter: missing endif or otherwise bad ifs\n");
  switch(prog->tokens[p].type){
  case t_else:
   if(levelcount==1){
    levelcount -=1;
   }
   break;
  case t_endif: levelcount-=1; break;
  case t_if: levelcount +=1; break; 
  }
  p+=1;
 }//endwhile
 return p-1;
}//endproc

int _interpreter_labelsearch(program *prog,char *labelstring, int labelstringlen){
 int i;
 for(i=0; i<prog->length; i++){
  if( prog->tokens[i].type == t_label && !strncmp((char*)prog->tokens[i].data.pointer, labelstring, labelstringlen) ){
   return i+1;
  }//endif
 }//next
 printf("_interpreter_labelsearch: label was %s\n",labelstring);
 error("interpreter: goto: couldn't find label\n");
 return 0;
}//endproc

double interpreter(int p, program *prog){
 token t;
 interpreter_start:
 t = prog->tokens[p];
 switch( t.type ){
 case t_return:
 {
  p+=1;
  return getvalue(&p, prog);
 }
 case t_endwhilef:
 {
  p=t.data.i; goto interpreter_start;
 } 
 case t_whilef:
 { // fast while/endwhile
  p+=1;
  if( !getvalue(&p, prog) ){
   p=t.data.i;
  }
  break;
 }
 case t_gotof:
 {
  p=t.data.i;
  goto interpreter_start;
 }
 case t_goto:
 {
  int newpos;
  if( prog->tokens[p+1].type == t_stringconst ){
   newpos = _interpreter_labelsearch(prog, (char*)prog->tokens[p+1].data.pointer, strlen((char*)prog->tokens[p+1].data.pointer) );
   prog->tokens[p].type = t_gotof;
   prog->tokens[p].data.i = newpos;
  }else{
   p+=1;
   stringval sv; sv = getstringvalue( prog, &p );
   p = _interpreter_labelsearch(prog, sv.string, sv.len);
  }
  goto interpreter_start;
 }
 case t_while:
 {
  int s_pos,e_pos;
  s_pos=p; p+=1;
  // search for matching endwhile
  int levelcount=1;
  while(levelcount){
   if(p>=prog->length) error("interpreter: missing endwhile\n");
   switch( prog->tokens[p].type ){
   case t_while: levelcount+=1; break;
   case t_endwhile: levelcount-=1; break;
   }
   e_pos=p;
   p+=1;
  }//endwhile
  //update matching while/endwhile 
  prog->tokens[s_pos].type = t_whilef;
  prog->tokens[s_pos].data.i = e_pos+1;
  prog->tokens[e_pos].type = t_endwhilef;
  prog->tokens[e_pos].data.i = s_pos+0;
  //go back to the position of the while so it can be executed as normal
  p=s_pos;
  break;
 }
 case t_endwhile:
 {
  int levelcount=1;
  while(levelcount){
   p-=1;
   if(p<0) error("interpreter: can't find while for this endwhile\n");
   switch( prog->tokens[p].type ){
   case t_while: levelcount-=1; break;
   case t_endwhile: levelcount+=1; break;
   }
  }
  break;
 }
 case t_iff:
  p+=1;
  if( !getvalue(&p, prog) ) p = t.data.i;
  break;
 case t_elsef:
  p = t.data.i; break;
 case t_if:
 {
  int ifpos,elsepos,endifpos;
  ifpos = p;
  elsepos = _interpreter_ifsearch(p+1, prog);
  if( prog->tokens[elsepos].type == t_endif ){
   endifpos = elsepos; elsepos=-1;
  }else{
   endifpos = _interpreter_ifsearch(elsepos+1, prog);
  }
  //if( prog->tokens[ifpos].type != t_if ) error("blah1\n"); 
  //if( elsepos>=0 && prog->tokens[elsepos].type != t_else ) error("blah2\n"); 
  //if( prog->tokens[endifpos].type != t_endif ) error("blah3\n"); 
  // fix if
  prog->tokens[ifpos].type = t_iff;
  prog->tokens[ifpos].data.i = ( elsepos==-1 ? endifpos+1 : elsepos+1 );
  // fix else
  if(elsepos != -1){
   prog->tokens[elsepos].type = t_elsef;
   prog->tokens[elsepos].data.i = endifpos+1;
  }
  // fix endif
  prog->tokens[endifpos].type = t_endiff;
  goto interpreter_start;
 }
 case t_else:
 {
  p = _interpreter_ifsearch(p+1, prog);
  break;
 }
 case t_set: t_set_start:
 {
  t=prog->tokens[p+1]; p+=2;
  int index;
  switch( t.type ){
  case t_D:
  {
   index = (int)getvalue(&p, prog);
   //printf("  set index: %d\n",index);
   if(index<0 || index>=prog->vsize) error("interpreter: set: bad variable access\n");
   prog->vars[index] = getvalue(&p, prog);
   break;
  }
  case t_Df:
  {
   *(double*)t.data.pointer = getvalue(&p, prog);
   break;
  }
  case t_A:
   index = (int)getvalue(&p,prog) + (int)getvalue(&p,prog);
   if(index<0 || index>=prog->vsize) error("interpreter: set: bad array access\n");
   prog->vars[index] = getvalue(&p, prog);
   break;
  case t_P:
   index = prog->sp + prog->current_function->num_locals + (int)getvalue(&p,prog);
   if(index<0 || index>=prog->ssize) error("interpreter: set: bad parameter access\n");
   prog->stack[index] = getvalue(&p, prog);
   break;
  case t_L:
   index = prog->sp + (int)getvalue(&p,prog);
   if(index<0 || index>=prog->ssize) error("interpreter: set: bad parameter access\n");
   prog->stack[index] = getvalue(&p, prog);
   break;
  case t_stackaccess:
   index = prog->sp + t.data.i;
   if(index<0 || index>=prog->ssize) error("interpreter: set: bad stack access\n");
   prog->stack[index] = getvalue(&p, prog);
   break;
  case t_id:
   process_id(prog, &prog->tokens[p - 1]);
   p -= 2;
   goto t_set_start;
  case t_S:
  {
   int string_number = getvalue(&p,prog);
   if(string_number<0 || string_number>=prog->max_stringvars) error("interpreter: set: bad stringvar access\n");
   stringvar *svr = prog->stringvars[string_number];
   copy_stringval_to_stringvar(svr, getstringvalue(prog, &p) );
   break;
  }
  case t_Sf:
  {
   stringvar *svr = (stringvar*)t.data.pointer;
   copy_stringval_to_stringvar(svr, getstringvalue(prog, &p) );
   break;
  }
  default: error("interpreter: set: bad use of 'set' command\n");
  }
  break; // Sat 14 Sep 12:35 - is this break necessary?
 }
 case t_var: // create a variable
 {
  p+=1;
  while( prog->tokens[p].type != t_endstatement ){
   if( prog->tokens[p].type != t_id ) error("interpreter: bad 'variable' command\n");
   if( find_id(prog->ids,     (char*)prog->tokens[p].data.pointer) ) error("interpreter: variable: this identifier is already used\n");
   add_id(prog->ids, make_id( (char*)prog->tokens[p].data.pointer, maketoken_Df( prog->vars + allocate_variable_data(prog, 1) ) ) );
   p+=1;
  }
  break;
 }
 case t_const: // create a constant
 {
  p+=1;
  if( prog->tokens[p].type != t_id ) error("interpreter: bad 'constant' command\n");
  char *idstring = (char*)prog->tokens[p].data.pointer;
  if( find_id(prog->ids, idstring) ) error("interpreter: constant: this identifier is already used\n");
  p+=1;
  add_id(prog->ids, make_id( idstring, maketoken_num( getvalue(&p, prog) ) ) );
  break;
 }
 case t_stringvar: // create a string variable
 {
  size_t bufsize; token idt;
  p+=1;
  t_stringvar_start:
  bufsize = DEFAULT_NEW_STRINGVAR_BUFSIZE;
  // get id for new string variable
  idt = prog->tokens[p]; p+=1;
  if( idt.type != t_id ) error("interpreter: bad 'stringvar' command\n");
  // if the optional bufsize parameter is present, then get it
  if( prog->tokens[p].type!=t_id && isvalue( prog->tokens[p].type ) ){
   bufsize = getvalue(&p,prog);
  }
  // create id for new string variable
  add_id(prog->ids, make_id( (char*)idt.data.pointer, maketoken_Sf( create_new_stringvar(prog,bufsize) ) ) );
  if( prog->tokens[p].type != t_endstatement ) goto t_stringvar_start;
  p+=1;
  break;
 }
 case t_print: // print
 {
  double value; stringval stringvalue; int i;
  p+=1;
  t_print_start: // =========================================================================
  if( prog->tokens[p].type == t_endstatement ){
   p+=1;
   printf("\n");
   break;
  }
  // ids can represent string values and numbers, so when an id is encountered,
  // it must be processed before anything else happens, so that we know whether it's a string or a number.
  if( prog->tokens[p].type == t_id ){
   process_id(prog, &prog->tokens[p]);
  }
  // similarly, string expressions and number expressions both use () brackets,
  // this means that a left bracket can represent either a string value or a number.
  // 'print' takes both string values and numbers, so when a left bracket is encountered,
  // it is necessary to check whether it's part of a string expression, or a numerical expression
  if( prog->tokens[p].type == t_leftb){
   i=p;
   while( prog->tokens[i].type == t_leftb ){ i+=1; }
   if( prog->tokens[i].type == t_id ) process_id(prog, &prog->tokens[i]);
   if( isstringvalue(prog->tokens[i].type) )
    goto t_print_stringval;
   else
    goto t_print_value;
  }//endif
  if( isstringvalue(prog->tokens[p].type) ) goto t_print_stringval;

  t_print_value: // =======================================================================
  value = getvalue(&p, prog);
  if( !isnan(value) && value == (double)(int)value){
   printf("%d",(int)value);
  }else{
   printf("%f",value);
  }
  goto t_print_start;
  t_print_stringval: //====================================================================
  stringvalue = getstringvalue( prog, &p );
  for(i=0; i<stringvalue.len; i++){
   putchar(stringvalue.string[i]);
  }
  goto t_print_start;
 }
 case t_wait:
 {
  p+=1;
  usleep((int)( getvalue(&p,prog)*1000 ));
  break;
 }
 case t_endfn:
  return 0.0;
 case t_id: // ids can represent functions, so they must be processed if encountered by 'interpreter'
  process_id(prog,&prog->tokens[p]);
  goto interpreter_start;
 case t_F: case t_Ff: // procedure call (a function is called and the return value is discarded)
  getvalue(&p,prog);
  break;
 case t_option:
 {
  option( prog, &p );
  break;
 }
 #ifdef enable_graphics_extension 
 // ============================================================================================
 // ======= GRAPHICS EXTENSION =================================================================
 // ============================================================================================
 case t_startgraphics:
 {
  p+=1;
  int w, h; w = getvalue(&p,prog); h = getvalue(&p,prog);
  start_newbase_thread(w,h);
  #ifdef NewBase_HaventRemovedThisYet
  xflush_for_every_draw=0;
  #endif
  break;
 }
 case t_stopgraphics:
 {
  p+=1;
  MyCleanup();
  break;
 }
 case t_winsize:
 {
  p+=1;
  int w, h; w = getvalue(&p,prog); h = getvalue(&p,prog);
  SetWindowSize(w,h);
  Wait(1);
  break;
 }
 case t_pixel:
 {
  p+=1; int x,y;
  do{
   x=getvalue(&p,prog);
   y=getvalue(&p,prog);
   Plot69(x,y);
  }while( isvalue( prog->tokens[p].type ) );
  break;
 }
 case t_line:
 {
  p+=1; int x1,y1,x2,y2, sw;
  x2=getvalue(&p,prog);
  y2=getvalue(&p,prog);
  do{
   sw=x2; x2=getvalue(&p,prog); x1=sw;
   sw=y2; y2=getvalue(&p,prog); y1=sw;
   Line(x1,y1,x2,y2);
  }while( isvalue( prog->tokens[p].type ) );
  break;
 }
 case t_circlef: case t_circle:
 {
  p+=1;
  int x,y,r; x=getvalue(&p,prog); y=getvalue(&p,prog); r=getvalue(&p,prog);
  if(t.type == t_circlef)
   CircleFill(x,y,r);
  else
   Circle(x,y,r);
  break;
 }
 case t_rectanglef: case t_rectangle:
 { 
  p+=1;
  int x,y,w,h;
  x=getvalue(&p,prog); y=getvalue(&p,prog); w=getvalue(&p,prog); if(isvalue( prog->tokens[p].type)) h=getvalue(&p,prog); else h=w;
  if(t.type == t_rectanglef)
   RectangleFill(x,y,w,h);
  else
   Rectangle(x,y,w,h);
  break;
 }
 case t_triangle:
 {
  p+=1;
  int x1,y1, x2,y2, x3,y3;
  x1=getvalue(&p,prog); y1=getvalue(&p,prog); x2=getvalue(&p,prog); y2=getvalue(&p,prog); x3=getvalue(&p,prog); y3=getvalue(&p,prog);
  Triangle(x1,y1, x2,y2, x3,y3);
  break;
 }
 case t_drawtext:
 {
  p+=1;
  int x,y,s; stringval sv;
  x=getvalue(&p,prog); y=getvalue(&p,prog); s=getvalue(&p,prog);
  sv = getstringvalue( prog, &p );
  char holdthis = sv.string[sv.len]; sv.string[sv.len]=0;
  drawtext_(x,y,s, sv.string);
  sv.string[sv.len]=holdthis;
  break;
 }
 case t_refreshmode:
 {
  p+=1; int m;
  m = getvalue(&p,prog);
  switch(m){
  case 0:
   RefreshOn();
   #ifdef NewBase_HaventRemovedThisYet
   xflush_for_every_draw=0;
   #endif
   break;
  case 1:
   #ifdef NewBase_HaventRemovedThisYet
   xflush_for_every_draw=0;
   #endif
   RefreshOff();
   break;
  #ifdef NewBase_HaventRemovedThisYet
  case 2:
   RefreshOn();
   xflush_for_every_draw=0;
   break;
  #endif
  }
  break;
 }
 case t_refresh:
 {
  p+=1;
  Refresh();
  break;
 }
 case t_gcol:
 {
  p+=1;
  int r,g,b;
  r=getvalue(&p,prog);
  if( !isvalue( prog->tokens[p].type ) ){
   GcolDirect( MyColour2( r ) );
   break;
  }
  g=getvalue(&p,prog); b=getvalue(&p,prog);
  Gcol(r,g,b);
  break;
 }
 case t_bgcol:
 {
  p+=1;
  int r,g,b;
  r=getvalue(&p,prog);
  if( !isvalue( prog->tokens[p].type ) ){
   GcolBGDirect( MyColour2( r ) );
   break;
  }
  g=getvalue(&p,prog); b=getvalue(&p,prog);
  GcolBG(r,g,b);
  break;
 }
 case t_cls:
 {
  p+=1;
  Cls();
  break;
 }
 case t_drawmode:
 {
  p+=1;
  SetPlottingMode(getvalue(&p,prog));
  break;
 }
 // ============================================================================================
 // ======= END OF GRAPHICS EXTENSION ==========================================================
 // ============================================================================================
 #endif
#if allow_debug_commands
 case t_printentirestack:
 {
  printf("---- STACK ----\n");
  int i;
  for(i=0; i<prog->sp; i++){ // print everything until the current stack frame.
   printf(" %f\n",prog->stack[ i ]);
  }  
 }// continue into t_printstackframe and let it print the current stack frame (as well as advance p past the printentirestack command)
 case t_printstackframe:
 {
  p+=1;
  int i;
  printf("STACK FRAME\n----- locals -----\n");
  for(i=0; i < prog->current_function->num_locals + (prog->current_function->params_type==p_atleast ? prog->stack[ prog->sp ] : prog->current_function->num_params ); i++){
   if(i==prog->current_function->num_locals) printf("----- params -----\n");
   printf(" %f\n",prog->stack[ prog->sp + i ]);
  }
  printf("------------------\n\n");
 } break;
 case t_tb: tb();
#endif
 case t_label: case t_endif: case t_endiff: case t_endstatement:
  p+=1;
  goto interpreter_start;
 default:
  printf("interpreter: token is %s\n",tokenstring(t));
  error("interpreter: unexpected token\n");
 }
 goto interpreter_start;
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

#define main_printstuff 0
int main(int argc, char **argv){
 SeedRng();
 //printf("sz %d\n",sizeof(token)); return 0;
 program *prg = NULL;

 if(argc>1){
  prg = init_program( argv[1], Exists(argv[1]) ); 
 }else{
  printf("%s [program text or path to a file containing program text]\n",argv[0]);
  return 0;
 }

#if main_printstuff
 printf("ok\n\n");
 detokenise(prg,-1);
 printf("\n");
#endif

#if main_printstuff
 printf("------- program output -------\n");
#endif
 double result = interpreter(0,prg);
#if main_printstuff
 printf("------------------------------\n");
 printf("\n");
#endif

#if main_printstuff
 list_all_ids(prg); printf("\n");
#endif

 printf("result: %f\n",result);

 unloadprog(prg);
 printf("ended\n");
 return 0;
}

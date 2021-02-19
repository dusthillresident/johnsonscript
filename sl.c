#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mylib.c"
#include <ctype.h> // isspace
#include <unistd.h> // usleep
#include <math.h>

// Apparently isnan() returns 0 for -nan when GCC optimisations are enabled.
// So I needed to find my own replacement for isnan() that behaves how I want.
//#define MyIsnan(value) (isnan(value))
#define MyIsnan(value) ( (int)value && value==0.0 )

#ifdef enable_graphics_extension
 #define myxlib_notstandalonetest
 #include "NewBase.c"
#endif

#ifndef DISABLE_ALIGN_STUFF
 // this is for testing. I discovered that messing with __attribute__((aligned(x))) can make a difference to how fast things run
 #define ALIGN_ATTRIB_CONSTANT 1
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


#include "tokenslist.c"


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
 int unclaimed; // used by 'S'
};
typedef struct stringvar stringvar;
// ----
struct stringval { // string value
 char *string;
 size_t len;
};
typedef struct stringval stringval;
// ----
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
// file management
struct file;
typedef struct file file;
struct file {
 int open;
 int read_access;
 int write_access;
 FILE *fp;
};
#define DEFAULT_MAX_FILES 8 // must be at least 3 to make space for stdin, stdout, stderr
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
 int max_files;
 file **files;
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
void process_function_definitions(program *prog,int startpos);
void error(char *s);
stringval getstringvalue( program *prog, int *pos );
int isstringvalue(unsigned char type);
int isvalue(unsigned char type);
int determine_valueorstringvalue(program *prog, int p);

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
 // initialise file array
 out->max_files = DEFAULT_MAX_FILES;
 out->files = calloc(DEFAULT_MAX_FILES,sizeof(void*));
 for(i=0; i<DEFAULT_MAX_FILES; i++){
  out->files[i] = calloc(1,sizeof(file));
 }
 out->files[0]->open=1; out->files[0]->read_access=1;  out->files[0]->fp = stdin;  // stdin
 out->files[1]->open=1; out->files[1]->write_access=1; out->files[1]->fp = stdout; // stout
 out->files[2]->open=1; out->files[2]->write_access=1; out->files[2]->fp = stderr; // sterr
 // initialise id list
 out->ids = calloc(1,sizeof(id_info));
 return out;
}

// -------------------------------------------------------------------------------------------
int get_free_fileslot(program *prog){
 int i;
 // look for the next free fileslot.
 // start at 3 because 0,1,2 are reserved for stdin, stdout, stderr
 for(i=3; i < prog->max_files; i++){
  if(!prog->files[i]->open) return i;
 }
 // if one was not found, create a new one
 prog->max_files += 1;
 prog->files = realloc(prog->files, sizeof(void*) * prog->max_files);
 if(prog->files == NULL) error("get_free_fileslot: realloc failed\n");
 prog->files[prog->max_files-1] = calloc(1,sizeof(file));
 return prog->max_files-1;
}
file* getfile(program *prog, int file_reference_number, int read, int write){
 int fileindex = file_reference_number - 1;
 if( fileindex<0 || fileindex>=prog->max_files ) error("bad file access (filenumber out of range)\n");
 file *out = prog->files[fileindex];
 if( ! out->open ) error("bad file access (not open)\n");
 if( read  && ! out->read_access  ) error("bad file access (not readable)\n");
 if( write && ! out->write_access ) error("bad file access (not writable)\n");
 return out;
}
// -------------------------------------------------------------------------------------------

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
 // close all open files (but ONLY if they're not stdin or stdout or stderr), free the file structs, and finally free the file array
 for(i=0; i<prog->max_files; i++){
  if( prog->files[i]->open  &&  prog->files[i]->fp != stdin  &&  prog->files[i]->fp != stdout  &&  prog->files[i]->fp != stderr ){
   fclose( prog->files[i]->fp );
  }
  free( prog->files[i] );
 }
 free( prog->files );
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
 process_function_definitions(prog,0);
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
   if( find_id(prog->ids, (char*)prog->tokens[pos].data.pointer) ){ printf("process_function_definition: this identifier '%s' is already used\n",(char*)prog->tokens[pos].data.pointer); goto process_function_definition_errorout; }
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
void process_function_definitions(program *prog,int startpos){
 int i;
 for(i=startpos; i<prog->length; i++){ 
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
#define opt_import	2	// import functions from another file
#define opt_seedrnd	3	// seed RNG
#define opt_unclaim	4	// unclaim a string (so it can be re-used by 'S')

void option( program *prog, int *p ){
 *p += 1; // advance p out of the way of the 'option' command itself
 int id_stringconst_pos = *p;
 
 if( prog->tokens[*p].type == t_id ) process_id(prog, &prog->tokens[*p]);

 int opt_number=-1;


 if(  determine_valueorstringvalue(prog, *p)  ){ // identify option string
  stringval id_string = getstringvalue( prog, p );
  if( id_string.len == 0 ){ opt_number=-1; goto option__identify_option_string_out; } // FUCK OFF! for FUCK'S sake...
  if( !strncmp( "vsize", id_string.string, id_string.len ) ){ opt_number = opt_vsize; goto option__identify_option_string_out;}	//	vsize		Set the size of the variables array
  if( !strncmp( "ssize", id_string.string, id_string.len ) ){ opt_number = opt_ssize; goto option__identify_option_string_out;}	//	ssize		Set the size of the stack array
  if( !strncmp( "import", id_string.string, id_string.len )){opt_number = opt_import; goto option__identify_option_string_out;}	//	import		Import functions from another file
  if( !strncmp( "seedrnd", id_string.string, id_string.len)){opt_number= opt_seedrnd; goto option__identify_option_string_out;}	//	seedrnd		Seed RNG
  if( !strncmp( "unclaim", id_string.string, id_string.len)){opt_number= opt_unclaim; goto option__identify_option_string_out;} //	unclaim [string ref num]	Unclaim a string
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
 case opt_vsize: // vsize
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
 case opt_ssize: // ssize
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
 case opt_import: // import
 {
  token *tokens; int tokens_length; stringval filename;
  // get filename
  filename = getstringvalue(prog,p);
  char holdthis = filename.string[filename.len]; filename.string[filename.len]=0;
  // load text file and process it, getting the tokens (program code data) and program_strings
  tokens = loadtokensfromtext(prog->program_strings,filename.string,&tokens_length);
  filename.string[filename.len]=holdthis;
  if( tokens == NULL ) error("import: couldn't open file\n");
  // resize array and append new code
  prog->tokens = realloc(prog->tokens, (prog->maxlen + tokens_length)*sizeof(token));
  memcpy(prog->tokens + prog->maxlen, tokens, sizeof(token) * tokens_length);
  prog->length += tokens_length; prog->maxlen+=tokens_length;
  // process function definitions for imported code
  process_function_definitions(prog,prog->maxlen - tokens_length);
  break;
 }
 case opt_seedrnd: // seed random number generator
 {
  int vp=0;
  int v[3] = { 0, 0, 0 };
  do{
   v[vp] = getvalue(p,prog); vp++;
  }while( isvalue( prog->tokens[*p].type ) );
  XRANDrand = v[0];
  XRANDranb = v[1];
  XRANDranc = v[2];
  break;
 }
 case opt_unclaim:
 {
  int stringvar_num = (int)getvalue(p,prog);
  if( stringvar_num<0 || stringvar_num >= prog->max_stringvars ){
   printf("stringvar_num: %d\n",stringvar_num);
   error("unclaim: bad stringvariable access\n");
  }
  prog->stringvars[stringvar_num]->unclaimed=1;
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
 if( wordmatch( pos,"REM", text) || wordmatch( pos,"#", text) ){	
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
 if( wordmatch( pos,"S", text) ){	//	$
  out =  maketoken( t_SS ); goto gettoken_normalout;
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

 if( wordmatch( pos,"oscli", text) ){	//	
  out = maketoken( t_oscli ); goto gettoken_normalout;
 }

 // '_num_params', alias for L0, which is the parameter telling you the number of parameters a function has
 if( wordmatch( pos,"_num_params", text) ){	//	
  out = maketoken_stackaccess( 0 ); goto gettoken_normalout;
 }
 // '_stdin', alias for '1'. '1' reserved as the file reference number for stdin
 if( wordmatch( pos,"_stdin", text) ){
  out = maketoken_num( 1 ); goto gettoken_normalout;
 }
 // '_stdout', alias for '2'. '2' reserved as the file reference number for stdout
 if( wordmatch( pos,"_stdout", text) ){
  out = maketoken_num( 2 ); goto gettoken_normalout;
 }
 // '_stderr', alias for '3'. '3' reserved as the file reference number for stderr
 if( wordmatch( pos,"_stderr", text) ){
  out = maketoken_num( 3 ); goto gettoken_normalout;
 }
 // '_pi', alias for '3.14159265358979323846264338327950288'
 if( wordmatch( pos,"_pi", text) ){
  out = maketoken_num( 3.14159265358979323846264338327950288 ); goto gettoken_normalout;
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
   //case 10: error("missing \" on this line\n");
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
 if( wordmatch( pos,"instr$", text) ){	//	cmp$ (strcmp)
  out = maketoken( t_instrS ); goto gettoken_normalout;
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

 // ========= file related keywords ==========
 if( wordmatch( pos,"openin", text) ){	//	
  out = maketoken( t_openin ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"openout", text) ){	//	
  out = maketoken( t_openout ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"openup", text) ){	//	
  out = maketoken( t_openup ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"eof", text) ){	//	
  out = maketoken( t_eof ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"bget", text) ){	//	
  out = maketoken( t_bget ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"vget", text) ){	//	
  out = maketoken( t_vget ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"ptr", text) ){	//	
  out = maketoken( t_ptr ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"ext", text) ){	//	
  out = maketoken( t_ext ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"sget", text) ){	//	
  out = maketoken( t_sget ); goto gettoken_normalout;
 }

 if( wordmatch( pos,"sptr", text) ){	//	
  out = maketoken( t_sptr ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"bput", text) ){	//	
  out = maketoken( t_bput ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"vput", text) ){	//	
  out = maketoken( t_vput ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"sput", text) ){	//	
  out = maketoken( t_sput ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"close", text) ){	//	
  out = maketoken( t_close ); goto gettoken_normalout;
 }
 // ====== end of file related keywords ======

 if( wordmatch( pos,"caseof", text) ){	//	
  out = maketoken( t_caseof ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"when ", text) ){	//	
  out = maketoken( t_when ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"otherwise", text) ){	//	
  out = maketoken( t_otherwise ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"endcase", text) ){	//	
  out = maketoken( t_endcase ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"quit", text) ){	//	
  out = maketoken( t_quit ); goto gettoken_normalout;
 }

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
 size_t ReturnValue;
 ReturnValue = fread((void*)text, sizeof(char), ext, f);
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
 case t_SS:		return "S";
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
 case t_goto: return "goto";
 case t_gotof: return "gotoF";

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
 case t_instrS:		return "instr$";

 // ------ file stuff ------------------
 case t_openin:		return "openin";
 case t_openout:	return "openout";
 case t_openup:		return "openup";

 case t_eof:		return "eof";
 case t_bget:		return "bget";
 case t_vget:		return "vget";
 case t_ptr:		return "ptr";
 case t_ext:		return "ext";

 case t_sptr:		return "sptr";
 case t_bput:		return "bput";
 case t_vput:		return "vput";
 case t_sput:		return "sput";
 case t_close:		return "close";

 case t_sget:		return "sget"; 
 // ------------------------------------

 case t_stringvar:	return "stringvar";
 case t_S:		return "$";

 case t_rnd:		return "rnd";
 case t_wait:		return "wait";
 case t_alloc:		return "alloc";
 case t_endfn:		return "endfunction";
 case t_option:		return "option";
 case t_oscli:		return "oscli";

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
 
 case t_Df:
 {
  snprintf(tokenstringbuf,sizeof(tokenstringbuf),"Df(%p)",t.data.pointer);
  return tokenstringbuf;
 }
 case t_stackaccess:
 {
  /*
  if(t.data.i<=0)
   snprintf(tokenstringbuf,sizeof(tokenstringbuf),"stackaccess(P%d)",-t.data.i);
  else
   snprintf(tokenstringbuf,sizeof(tokenstringbuf),"stackaccess(L%d)",t.data.i);
  */
  snprintf(tokenstringbuf,sizeof(tokenstringbuf),"stackaccess(%d)",t.data.i);
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

 case t_caseof:		return "caseof";
 case t_caseofV:		return "caseofV";
 case t_caseofS:	return "caseofS";
 case t_when:		return "when";
 case t_otherwise:	return "otherwise";
 case t_endcase:	return "endcase";

 case t_quit:		return "quit";

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
 //if( new_bufsize <= bufsize_required ) error("still happening");
 if(preserve){
  sv->string = realloc(sv->string, new_bufsize);
  if(sv->string == NULL) error("stringvar_adjustsize: realloc failed\n");
  sv->bufsize = new_bufsize;
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

stringval
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
getstringvalue( program *prog, int *pos ){
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
  if(!MyIsnan(val) && val == (double)(int)val){
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
 case t_sget:
 {
  file *f = getfile(prog, getvalue(pos,prog), 1,0);
  int num_bytes_to_read = -10;
  if( isvalue( prog->tokens[*pos].type ) ) num_bytes_to_read = getvalue(pos,prog);
  accumulator->len=0;
  if( num_bytes_to_read <= 0 ){
   // ======= reading until a specified character is found =====
   int read_until = -num_bytes_to_read; // read until we find this specific character (or EOF)
   int ch;
   while( 1 ){
    ch = fgetc(f->fp);
    if( ch == read_until || ch == -1 ) break;
    accumulator->string[ accumulator->len ] = ch;
    accumulator->len += 1;
    if( accumulator->len >= accumulator->bufsize-1 ){
     //printf("fuck1 %d, %d\n",accumulator->len, accumulator->bufsize);
     stringvar_adjustsizeifnecessary(accumulator, accumulator->bufsize + 1, 1);
     //printf("fuck2 %d, %d\n",accumulator->len, accumulator->bufsize);
     //if( accumulator->len >= accumulator->bufsize ) error("FUCK PENIS FUCK\n"); //remove later
    }
   }
   // ==========================================================
  }else{ 
   // ======= reading a specific number of bytes ===============
   if( accumulator->bufsize <= num_bytes_to_read ){
    stringvar_adjustsizeifnecessary(accumulator, num_bytes_to_read, 1);
    //if( accumulator->bufsize <= num_bytes_to_read ) error("FUCK ###########################################\n"); //remove later
   }//endif bufsize
   accumulator->len = fread( (void*)accumulator->string, sizeof(char), num_bytes_to_read, f->fp );
   // ==========================================================
  }//endif whether or not we're reading a set number of bytes, or reading until a specific character
  stringval out;
  out.len = accumulator->len;
  out.string = accumulator->string;
  return out;
 }//end case block
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

stringvar* create_new_stringvar(program *prog,int bufsize){
 if( bufsize <= 0 ) error("create_new_stringvar: requested buffer size is less than or equal to 0\n");
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
double
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
getvalue(int *p, program *prog){
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
  char hold = sv.string[sv.len]; sv.string[sv.len]=0;
  double result = strtod(sv.string,NULL);
  sv.string[sv.len] = hold;
  return result;
 }
 case t_lenS:
 {
  stringval sv = getstringvalue(prog,p);
  return (double)sv.len;
 }
 case t_cmpS:
 {
  stringval sv1 = getstringvalue(prog,p);
  prog->getstringvalue_level += 1;
  stringval sv2 = getstringvalue(prog,p);
  prog->getstringvalue_level -= 1;
  
  if( sv1.len == sv2.len ){
   return strncmp( sv1.string, sv2.string, sv1.len );
  }
  int result = strncmp( sv1.string, sv2.string, sv1.len < sv2.len ? sv1.len : sv2.len );
  if( result == 0 ){
   return sv1.len > sv2.len ? 1 : -1; /*sv1.len > sv2.len ? sv1.string[sv1.len-1] : -sv2.string[sv2.len-1];*/
  }
  return result;
 }
 case t_instrS:
 {
  stringval sv1 = getstringvalue(prog,p);
  prog->getstringvalue_level += 1;
  stringval sv2 = getstringvalue(prog,p);
  prog->getstringvalue_level -= 1;

  if( sv1.len < sv2.len ) return -1;
  
  int i;
  for(i=0; i <= sv1.len - sv2.len; i++){
   if( (sv1.string[i] == sv2.string[0])
        &&
       (sv1.string[i+sv2.len-1] == sv2.string[sv2.len-1])
        &&
       (!strncmp( sv1.string + i, sv2.string, sv2.len))
   ) return i;
  }//next

  return -1;
 }//end case block t_instrS
 case t_SS:
 {
  int i;
  int stringgiven=0;
//  stringvar *svr = (stringvar*)t.data.pointer;
//  copy_stringval_to_stringvar(svr, getstringvalue(prog, &p) );
  stringvar *svr = NULL;
  for( i = prog->max_stringvars-1; i>=0; i-- ){

   if( prog->stringvars[i]->unclaimed ){ 
    prog->stringvars[i]->unclaimed = 0;
    svr = prog->stringvars[i];
    break;
   }
  }
  if( svr == NULL ){
   svr = create_new_stringvar(prog,DEFAULT_NEW_STRINGVAR_BUFSIZE);
  }
  if( isvalue( prog->tokens[*p].type ) && isstringvalue( prog->tokens[*p].type ) ){
   stringgiven = determine_valueorstringvalue(prog, *p);
  }else{
   stringgiven = isstringvalue( prog->tokens[*p].type );
  }
  if( stringgiven ){
   copy_stringval_to_stringvar(svr, getstringvalue(prog, p) );
  }else{
   svr->len = 0;
  }
  return svr->string_variable_number;
 }

 // --------- file related functions ---------
 case t_openin: case t_openup:  case t_openout:
 {
  stringval sv = getstringvalue(prog,p);
  char holdthis = sv.string[sv.len]; sv.string[sv.len]=0; // null terminate the string for fopen()
  FILE *fp; int read,write;
  // open the file
  switch( t.type ){
  case t_openin:
   read  = 1;
   write = 0;
   fp = fopen(sv.string,"rb");
   break;
  case t_openout:
   read  = 0;
   write = 1;
   fp = fopen(sv.string,"wb");
   break;
  case t_openup:
   read  = 1;
   write = 1;
   fp = fopen(sv.string,"rb+");
   break;
  }
  // undo null termination
  sv.string[sv.len]=holdthis;
  // if the file was opened successfully, add it to the list of open files and return a reference number for it. if not, return 0
  if( !fp ) return 0;
  int fileindex = get_free_fileslot(prog);
  prog->files[fileindex]->open = 1;
  prog->files[fileindex]->read_access = read;
  prog->files[fileindex]->write_access = write;
  prog->files[fileindex]->fp = fp;
  return fileindex+1;
 }

 case t_eof:
 {
  file *f = getfile(prog, getvalue(p,prog), 0,0);
  return feof(f->fp);
 }
 case t_bget:
 {
  file *f = getfile(prog, getvalue(p,prog), 1,0);
  return fgetc(f->fp);
 }
 case t_vget:
 {
  file *f = getfile(prog, getvalue(p,prog), 1,0);
  char db[8];
  #if 1
  db[0] = fgetc(f->fp);
  db[1] = fgetc(f->fp);
  db[2] = fgetc(f->fp);
  db[3] = fgetc(f->fp);
  db[4] = fgetc(f->fp);
  db[5] = fgetc(f->fp);
  db[6] = fgetc(f->fp);
  db[7] = fgetc(f->fp);
  #else
  fread( (void*)db, sizeof(double), 1, f->fp);
  #endif
  return *((double*)db);
 }
 case t_ptr:
 {
  file *f = getfile(prog, getvalue(p,prog), 0,0);
  return ftell(f->fp);
 }
 case t_ext:
 {
  file *f = getfile(prog, getvalue(p,prog), 0,0);
  return Ext(f->fp);
 }
 // ----- end of file related functions ------

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
  case t_A:
   return (int)getvalue(p, prog) + (int)getvalue(p, prog);
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
   fnum = getvalue(p,prog); if( (fnum<0 || fnum>(MAX_FUNCS-1)) || !prog->functions[fnum] ) error("getvalue: bad function call\n");
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
  printf("getvalue: t.type == %d '%s'\n", t.type, tokenstring(t) );
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

struct caseof {
 int num_whens;
 int *whens; // position of 'whens'
 int *whens_; // position of first ';' after each 'when'
 int otherwise; //position of otherwise (this will point to the 'endcase' if there was no otherwise'
};

int caseof_skippast(program *prog, int p){ // p must point to the starting 'caseof'
 int i = p;
 int level = 1; // 'level'
 while( (i < prog->length) && level){
  i+=1;
  switch( prog->tokens[i].type ){
  case t_caseof: level += 1; break;
  case t_endcase: level -= 1; break;
  }
 }
 if(i == prog->length) error("caseof_skippast: missing endcase\n");
 return i;
}
int caseof_numwhens(program *prog, int p,struct caseof *co){ // p must point to the starting 'caseof'
 int out=0;
 int i = p+1;
 while( i < prog->length ){
  switch( prog->tokens[i].type ){
  case 0:
   error("caseof_numwhens: missing endcase\n");
   break;
  case t_caseof:
   i = caseof_skippast( prog, i);
   break;
  case t_when:
   if(co){
    co->whens[out] = i;
    if( out>0 && !co->whens_[out-1] ){
     error("caseof_numwhens:1: 'when' list not terminated by ';'\n"); // this can't always be caught but at least in some cases it'll be nice to be notified by this error message
    }
   }
   out += 1;
   break;
  case t_endstatement:
   if(co && (out>0) && !co->whens_[out-1] ){
    co->whens_[out-1] = i;
   }
   break;
  case t_otherwise:
   if(co){
    co->otherwise = i;
    if( out>0 && !co->whens_[out-1] ){
     error("caseof_numwhens:2: 'when' list not terminated by ';'\n");
    }
   }
   break;
  case t_endcase:
   return out;
  }
  i+=1;
 }
 error("caseof_numwhens: this should never happen\n");
}
int determine_valueorstringvalue(program *prog, int p){// this returns 0 for a value, 1 for a stringvalue, or causes an error if neither is found
 int i; i=p; while( prog->tokens[i].type == t_leftb ){ i+=1; } // skip past any left brackets
 if( prog->tokens[i].type == t_id ) process_id(prog, &prog->tokens[i]); // process it if it's an id
 if( isstringvalue(prog->tokens[i].type) ) return 1; // return 1 if it's a stringvalue
 if( isvalue(prog->tokens[i].type) ) return 0; // return 0 if it's a value
 error("expected a value or a stringvalue\n"); // cause an error because we expected a value or a stringvalue
}
#define PROCESSCASEOF_RUBBISH 0
void _interpreter_processcaseof(program *prog, int p){ int i;
//  create caseof struct.
 struct caseof *co = calloc(1, sizeof(struct caseof));
 int caseofpos = p;
 int endcasepos = caseof_skippast(prog,p);
 #if PROCESSCASEOF_RUBBISH
 printf("fuckk %s\n",tokenstring(prog->tokens[ endcasepos ]));
 #endif
//  determine if this is a caseofV or caseofS, or cause an error if there's an inappropriate token after the caseof.
//  replace the caseof with caseofV_f or caseofS_f appropriately.
 prog->tokens[caseofpos].type = determine_valueorstringvalue(prog,p+1) ? t_caseofS : t_caseofV;
//  count the number of whens.
 int numwhens = caseof_numwhens(prog,p,NULL);
//  allocate arrays for whens and ';'.
 co->num_whens = numwhens;
 co->whens  = calloc(numwhens,sizeof(int));
 co->whens_ = calloc(numwhens,sizeof(int));
//  find all the 'whens' and the otherwise
 caseof_numwhens(prog,p,co);
 // debug
 #if PROCESSCASEOF_RUBBISH
 for(i=0; i<numwhens; i++){
  printf("ffuck %0d: %d(%s), %d(%s)\n", i, co->whens[i],tokenstring(prog->tokens[co->whens[i]]), co->whens_[i],tokenstring(prog->tokens[co->whens_[i]])  );
 }
 #endif
 // replace otherwise with goto
 if(co->otherwise){
  prog->tokens[ co->otherwise ].type = t_gotof;
  prog->tokens[ co->otherwise ].data.i = endcasepos;
 }else{
  co->otherwise = endcasepos;
 }
 // replace whens with gotos
 for(i=0; i<numwhens; i++){
  token *t = &prog->tokens[ co->whens[i] ]; 
  t->type = t_gotof;
  t->data.i = endcasepos;
 }
 // replace endcase with ';'
 prog->tokens[ endcasepos ].type = t_endstatement;
//  update the caseof token to point to the caseof struct.
 prog->tokens[caseofpos].data.pointer = co;

 // add the allocated pointers to the strings list
 //void stringslist_addstring(stringslist *s,char *string);
 stringslist_addstring(prog->program_strings, (char *) co );
 stringslist_addstring(prog->program_strings, (char *) co->whens );
 stringslist_addstring(prog->program_strings, (char *) co->whens_ );  
 // debug
 #if PROCESSCASEOF_RUBBISH
 for(i=0; i<numwhens; i++){
  printf("ffuck %0d: %d(%s), %d(%s)\n", i, co->whens[i],tokenstring(prog->tokens[co->whens[i]]), co->whens_[i],tokenstring(prog->tokens[co->whens_[i]])  );
 }
 #endif
 // just FUCKING do this right now so you don't need to do it later, save maybe 0.0000001 seconds of processing time god damn it.....
 for(i=0; i<numwhens; i++){
  co->whens[i] += 1;
  co->whens_[i]+= 1;
 }
 co->otherwise+=1;
//  return
 return;
}

double
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
interpreter(int p, program *prog){
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
   //if( find_id(prog->ids,     (char*)prog->tokens[p].data.pointer) ) error("interpreter: variable: this identifier is already used\n");
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
  //if( find_id(prog->ids, idstring) ) error("interpreter: constant: this identifier is already used\n");
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
  if( !MyIsnan(value) && value == (double)(int)value){
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
 case t_oscli:
 {
  p+=1;
  stringval sv = getstringvalue( prog, &p );
  char holdthis = sv.string[sv.len]; sv.string[sv.len]=0;
  //printf("oscli: '%s'\n",sv.string);
  int SystemReturnValue;
  SystemReturnValue = system(sv.string);
  sv.string[sv.len]=holdthis;
  break;
 }
 // ------------ file related commands ------------------
 case t_sptr:
 {
  p+=1; file *f = getfile(prog, getvalue(&p,prog), 0,0);
  SetPtr(f->fp, getvalue(&p,prog));
  break;
 }
 case t_bput:
 {
  p+=1; file *f = getfile(prog, getvalue(&p,prog), 0,1);
  do{
   fputc( getvalue(&p,prog), f->fp );
  }while( isvalue( prog->tokens[p].type ) );
  break;
 }
 case t_vput:
 {
  p+=1; file *f = getfile(prog, getvalue(&p,prog), 0,1);
  double val;
  do{
   val = getvalue(&p,prog);
   fwrite((void*)&val, sizeof(double), 1, f->fp);
  }while( isvalue( prog->tokens[p].type ) );
  break;
 }
 case t_sput:
 {
  p+=1; file *f = getfile(prog, getvalue(&p,prog), 0,1);
  stringval sv;
  do{
   sv = getstringvalue( prog, &p );
   fwrite( (void*)sv.string, sizeof(char), sv.len, f->fp );
   //fputc(0, f->fp);
  }while( isstringvalue( prog->tokens[p].type ) );
  break;
 }
 case t_close:
 {
  p+=1; file *f = getfile(prog, getvalue(&p,prog), 0,0);
  f->open = 0;
  fclose(f->fp);
  break;
 }
 // --------- end of file related commands --------------
 case t_caseof:
  _interpreter_processcaseof(prog,p);
  break;
 case t_caseofV:
  { 
   struct caseof *co = t.data.pointer;
   p+=1;
   double v = getvalue(&p,prog);
   #if PROCESSCASEOF_RUBBISH
   printf("v: %f\n",v);
   #endif
   int i;
   for(i=0; i < co->num_whens; i++){
    p=co->whens[i];
    #if PROCESSCASEOF_RUBBISH
    printf("shit %s\n",tokenstring(prog->tokens[p]));
    #endif
    double v2;
    while( isvalue( prog->tokens[p].type ) ){
     v2 = getvalue(&p,prog);
     #if PROCESSCASEOF_RUBBISH
     printf("v2: %f\n",v2);
     #endif
     if( v == v2 ){
      #if PROCESSCASEOF_RUBBISH
      tb(); printf("match %f, %f\n",v,v2);
      #endif
      p=co->whens_[i]; goto caseofV_out;
     }
    }//endwhile
   }//next
   p=co->otherwise;
  }
  caseofV_out:
  break;
 case t_caseofS:
  { 
   struct caseof *co = t.data.pointer;
   p+=1;
   stringval v = getstringvalue( prog, &p );
   int i;
   for(i=0; i < co->num_whens; i++){
    p=co->whens[i];
    stringval v2;
    while( isstringvalue( prog->tokens[p].type ) ){
     v2 = getstringvalue( prog, &p );
     if( (v.len == v2.len) && !strncmp(v.string, v2.string, v.len ) ){
      p=co->whens_[i]; goto caseofS_out;
     }
    }//endwhile
   }//next
   p=co->otherwise;
  }
  caseofS_out:
  break;
 case t_quit:
  {
   p+=1;
   #ifdef enable_graphics_extension
   if(newbase_is_running){
    Wait(1);
    MyCleanup();
   }
   #endif
   double ret_val = 0;
   if( isvalue( prog->tokens[p].type ) ){
    //printf("this is happening\n"); tb(); // test
    ret_val = getvalue(&p,prog);
   }
   exit( (int) ret_val );
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

#if 1
 add_id(prg->ids, make_id( "_argc", maketoken_num( argc-2 ) ) );
 int i;
 for(i=2; i<argc; i++){
  copy_stringval_to_stringvar(   create_new_stringvar(prg,strlen( argv[i] ) ), (stringval) { argv[i], strlen(argv[i]) } );
 }
#endif

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

#if main_printstuff
 printf("result: %f\n",result);
#endif

 unloadprog(prg);
#if main_printstuff
 printf("ended\n");
#endif
 return (int)result;
}

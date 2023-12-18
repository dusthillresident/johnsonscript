#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "mylib.c"
#include <ctype.h> // isspace
#include <unistd.h> // usleep
#include <math.h>
#include <setjmp.h>
#include <signal.h>

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
 #define ALIGN_ATTRIB_CONSTANT 16
#endif

// memory problem debugging stuff
#if 0
 void* mymallocfordebug(size_t size    ,int n, char *f,char *fun){
  void *out = malloc(size);
  fprintf(stderr, "line %d,	%s,	%s,	MALLOC	%p\n",n,f,fun,out);
  return out;
 }
 void* mycallocfordebug(size_t nmemb, size_t size    ,int n, char *f,char *fun){
  void *out = calloc(nmemb,size);
  fprintf(stderr, "line %d,	%s,	%s,	CALLOC	%p\n",n,f,fun,out); 
  return out;
 }
 void* myreallocfordebug(void *ptr, size_t size    ,int n, char *f,char *fun,char *name){
  void *out = realloc(ptr,size);
  fprintf(stderr, "line %d,	%s,	%s,	REALLOC	%p ( input %p )	'%s'\n",n,f,fun,out,ptr,name);
  return out;
 }
 void myfreefordebug(void *ptr,      int n, char *f,char *fun,char *name){
  fprintf(stderr, "line %d,	%s,	%s,	FREEING	%p	'%s'\n",n,f,fun,ptr,name);
  free(ptr);
 }
 #define malloc(a) mymallocfordebug(a,__LINE__,__FILE__,(char*)__FUNCTION__)
 #define calloc(a,b) mycallocfordebug(a,b,__LINE__,__FILE__,(char*)__FUNCTION__)
 #define realloc(a,b) myreallocfordebug(a,b,__LINE__,__FILE__,(char*)__FUNCTION__,#a)
 #define free(p) myfreefordebug(p,__LINE__,__FILE__,(char*)__FUNCTION__,#p)
#endif

#define allow_debug_commands 1

#define TOKENTYPE_TYPE unsigned int

enum eval_action { eval_interpreter, eval_getvalue, eval_getstringvalue };
enum determined_retval { determined_neither = -1, determined_value = 0, determined_stringvalue = 1 };

struct token {
 TOKENTYPE_TYPE type;
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
#define INITIAL_FUNCTION_DEFINITION (func_info){ -1, p_exact, 0, 0, 0, NULL };
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
stringvar* newstringvar(){
 stringvar *out = calloc(1,sizeof(stringvar));
 out->string = calloc(DEFAULT_NEW_STRINGVAR_BUFSIZE,sizeof(char));
 out->bufsize = DEFAULT_NEW_STRINGVAR_BUFSIZE;
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
 while(s->next) s=s->next;
 //fprintf(stderr, "stringslist_addstring: %s\n",string);
 s->next = calloc(1,sizeof(stringslist));
 s->next->string = string;
}
stringslist* stringslist_gettail(stringslist *s){
 if( ! s ) return NULL;
 while( s->next ) s=s->next;
 return s;
}
#if 0
void stringslist_debug_show_contents(stringslist *s){
 if( ! s ){
  fprintf( stderr, "null...\n");
 }
 int count = 0;
 fprintf( stderr, "stringslist contents: \n");
 while( s ){
  fprintf(stderr, " item %d, %p contains:	%p\n",count++,s,s->string);
  s=s->next;
 }
}
#endif
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
 func_info initial_function;
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
 id_info *external_options;
 id_info *extensions; // used for external extensions
 id_info *quit_procs; // used for extensions that need special routines in order to quit gracefully
 stringslist *program_strings;
 // ------------ these variables will hold error handler info, and info about the last error to be trapped
 jmp_buf *error_handler;
 stringvar *_error_message; 
 stringvar *_error_file;
 int _error_line;
 int _error_column;
 //int _error_number;
 // -----------------
 #ifdef enable_debugger
 //void *dbg;
 #endif
};
typedef struct program program;
//--------------------------

// -------------------------------------------------------------------------------------------
// ----------------------- PROTOTYPES --------------------------------------------------------
// -------------------------------------------------------------------------------------------

double getvalue(int *p, program *prog);
double interpreter(int p, program *prog);
void free_ids(id_info *ids);
token* tokenise( program *prog, char *text, int *length_return, char *name_of_source_file);
token* loadtokensfromtext(program *prog, char *path,int *length_return);
void process_function_definitions(program *prog,int startpos);
void error(program *prog, int p, char *format, ...);
stringval getstringvalue( program *prog, int *pos );
int isstringvalue(TOKENTYPE_TYPE type);
int isvalue(TOKENTYPE_TYPE type);
int determine_valueorstringvalue(program *prog, int p, int errorIfNeither);
char* tokenstring(token t);
void print_sourcetext_location( program *prog, int token_p);
void stringvar_adjustsizeifnecessary(program *prog, stringvar *sv, size_t bufsize_required, int preserve);
int add_id(id_info *ids, id_info *new_id);
void add_id__error_if_fail( program *prog, int p, char *errormessage,   id_info *ids, id_info *new_id);
id_info* make_id(char *id_string, token t);
id_info* find_id(id_info *ids, char *id_string);
void clean_stringvar( program *prog, stringvar *svr, int preserve );
void unclaim(program *prog, int *p);
int oscli(program *prog, int *p);
int catch( program *prog, int *p, int action, double *returnValue, stringval *returnStringValue );
void copy_stringval_to_stringvar(program *prog, stringvar *dest, stringval src);
void process_catch(program *prog, int p);
int interpreter_eval( program *prog,  int action, void *return_value, unsigned char *text );
token gettoken(program *prog, int test_run, int *pos, unsigned char *text);
void interactive_prompt(program *prog);

// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------
#ifdef enable_debugger
#include "_sl_debugger.c"
#endif
// -------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------

program* 
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
newprog(int maxlen, int vsize, int ssize){
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
 out->initial_function = INITIAL_FUNCTION_DEFINITION;
 out->current_function = &out->initial_function;
 // initialise string accumulator 
 out->max_string_accumulator_levels = DEFAULT_STRING_ACCUMULATOR_LEVELS;
 out->string_accumulator = calloc(DEFAULT_STRING_ACCUMULATOR_LEVELS,sizeof(void*));
 int i;
 for(i=0; i<DEFAULT_STRING_ACCUMULATOR_LEVELS; i++){
  out->string_accumulator[ i ] = newstringvar();
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
 // initialise stringslist
 out->program_strings = calloc(1,sizeof(stringslist));
 // initialise error info
 out->_error_message = newstringvar(); 
 out->_error_file    = newstringvar();
 copy_stringval_to_stringvar(out, out->_error_message, (stringval){"(C)2023 Johnsonscript",21} );
 return out;
}

// -------------------------------------------------------------------------------------------
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
get_free_fileslot(program *prog){
 int i;
 // look for the next free fileslot.
 // start at 3 because 0,1,2 are reserved for stdin, stdout, stderr
 for(i=3; i < prog->max_files; i++){
  if(!prog->files[i]->open) return i;
 }
 // if one was not found, create a new one
 prog->max_files += 1;
 prog->files = realloc(prog->files, sizeof(void*) * prog->max_files);
 if(prog->files == NULL) error(prog, -1, "get_free_fileslot: realloc failed");
 prog->files[prog->max_files-1] = calloc(1,sizeof(file));
 return prog->max_files-1;
}
file* getfile(program *prog, int file_reference_number, int read, int write){
 int fileindex = file_reference_number - 1;
 if( fileindex<0 || fileindex>=prog->max_files ) error(prog, -1, "bad file access (filenumber out of range)");
 file *out = prog->files[fileindex];
 if( ! out->open ) error(prog, -1, "bad file access (not open)");
 if( read  && ! out->read_access  ) error(prog, -1, "bad file access (not readable)");
 if( write && ! out->write_access ) error(prog, -1, "bad file access (not writable)");
 return out;
}
// -------------------------------------------------------------------------------------------

token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken( TOKENTYPE_TYPE type ){
 token out;
 out.type=type;
 out.data.pointer = NULL;
 return out;
}
token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_num( double n ){
 token out;
 out.type = t_number;
 out.data.number = n;
 return out;
}
token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_stackaccess( int sp_offset ){
 token out;
 out.type = t_stackaccess;
 out.data.i = sp_offset;
 return out; 
}
token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_Df( double *pointer ){
 token out;
 out.type = t_Df;
 out.data.pointer = (void*)pointer;
 return out;
}
token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_Sf( stringvar *pointer ){
 token out;
 out.type = t_Sf;
 out.data.pointer = (void*)pointer;
 return out;
}

// --------------------------------------------
// ---- External function/command routines ----
// --------------------------------------------

token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_extfun( double (*external_function)(int*,program*) ){
 token out;
 out.type = t_extfun;
 out.data.pointer = external_function; 
 return out;
}

token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_extsfun( stringval (*external_stringfunction)(program*,int*) ){
 token out;
 out.type = t_extsfun;
 out.data.pointer = external_stringfunction;
 return out;
}

token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_extc( void (*external_command)(int*,program*) ){
 token out;
 out.type = t_extcom;
 out.data.pointer = external_command;
 return out;
}

token
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
maketoken_extopt( void (*external_option_procedure)(program*,int*) ){
 token out;
 out.type = t_extopt;
 out.data.pointer = external_option_procedure;
 return out;
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
register_external_function( id_info *id_list, double (*external_function)(int*,program*), char *id ){
 if( ! add_id( id_list, make_id( id, maketoken_extfun( external_function ) ) ) ){
  error(NULL, -1, "register_external_function: id '%s' already exists",id);
 }
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
register_external_stringfunction( id_info *id_list, stringval (*external_stringfunction)(program*,int*), char *id ){
 if( ! add_id( id_list, make_id( id, maketoken_extsfun( external_stringfunction ) ) ) ){
  error(NULL, -1, "register_external_stringfunction: id '%s' already exists",id);
 }
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
register_external_command( id_info *id_list, void (*external_command)(int*,program*), char *id ){
 if( ! add_id( id_list, make_id( id, maketoken_extc( external_command ) ) ) ){
  error(NULL, -1, "register_external_command: id '%s' already exists",id);
 }
}

// the string 'char *id' must have the form "OPTION_blah", starting with OPTION_ and then having a name after
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
register_external_option( id_info *id_list, void (*external_option_procedure)(program*,int*), char *id){
 if( ! add_id( id_list, make_id( id, maketoken_extopt( external_option_procedure ) ) ) ){
  error(NULL, -1, "register_external_option: id '%s' already exists",id);
 }
}
// register a procedure to call when quitting.
// the string 'char *id' must have the form "QUIT_blah", starting with QUIT_ and then having a unique identifying name after
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
register_quit_procedure( id_info *id_list, void (*quit_procedure)(program*), char *id ){
 token t; t.type = t_bad; t.data.pointer = (void*) quit_procedure;
 if( ! add_id( id_list, make_id( id, t ) ) ){
  error(NULL, -1, "register_external_option: id '%s' already exists",id);
 }
}

// replace built-in stuff with extensions
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
register_replacement( id_info *id_list, TOKENTYPE_TYPE target, char *replacewith ){
 id_info *replacewith_idinfo;
 if( ! (replacewith_idinfo = find_id(id_list,replacewith) ) ){
  token t = {0}; t.type = target;
  error(NULL, -1, "extension '%s' for replacement of '%s' not found",replacewith,tokenstring(t));
 }
 token replacement = replacewith_idinfo->t;

 char *replacement_id_string = calloc(15,sizeof(char));
 strcpy(replacement_id_string, "__REPLACEMENT");
 *replacement_id_string = target;
 if( ! add_id( id_list, make_id( replacement_id_string, replacement ) ) ){
  fprintf(stderr, "tokentype number was '%d'\n",target);
  error(NULL, -1, "register_replacment: a replacement is already registered for this token type");
 }
}

// ---------------------------------------------------
// ---- End of external function/command routines ----
// ---------------------------------------------------

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
unloadprog(program *prog){
 int i;
 // free the stringslist (strings that are part of the program data (string constants, ids, labels, etc))
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
 // free extension-related IDs
 free_ids(prog->external_options);
 free_ids(prog->extensions);
 // free error-related string vars
 free( prog->_error_message->string); free( prog->_error_message );
 free( prog->_error_file->string); free( prog->_error_file );
 // free the program struct itself
 free( prog );
}

#define DEFAULT_VSIZE 256
#define DEFAULT_SSIZE 256
program*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
init_program( char *str,int str_is_filepath, id_info *extensions ){
 token *tokens; int tokens_length; 
 // create the 'program' structure, with default stack/variable array size
 program *prog = newprog(0,DEFAULT_VSIZE,DEFAULT_SSIZE);
 // install the extensions list (pass NULL for no extensions)
 prog->extensions = extensions;
 // --------------------------------------------------------
 // ----- process extension --------------------------------
 // --------------------------------------------------------
 while( extensions ){
  // --------- 'option' extensions ----------------
  if( extensions->name && !strncmp("OPTION_",extensions->name,7) ){
   char *extension_option_string_permanent_address = calloc( strlen(extensions->name)+1, sizeof(char));
   strcpy( extension_option_string_permanent_address, extensions->name+7 );
   //fprintf(stderr,"fuckin fuck %s\n",extension_option_string_permanent_address);//test, remove soon
   token extension_option_token = extensions->t;
   if( extension_option_token.type != t_extopt ){
    error(NULL, -1, "invalid extension configuration, please check");
   }
   if( ! prog->external_options ){
    prog->external_options = calloc(1,sizeof(id_info));
   }
   if( ! add_id( prog->external_options, make_id( extension_option_string_permanent_address, extension_option_token) ) ){
    error(NULL, -1, "init_program: external option id '%s' already exists",extension_option_string_permanent_address);
   }
   stringslist_addstring(prog->program_strings, extension_option_string_permanent_address);
  }
  if( extensions->name && !strncmp("QUIT_",extensions->name,5) ){
   if( ! prog->quit_procs ){
    prog->quit_procs = calloc(1,sizeof(id_info));
   }
   if( ! add_id( prog->quit_procs, make_id( extensions->name, extensions->t) ) ){
    error(NULL, -1, "there is already a quit procedure with this name '%s'",extensions->name);
   }
  }
  extensions = extensions->next;
 }
 // --------------------------------------------------------
 // --------------------------------------------------------
 // --------------------------------------------------------
 // get the tokens, either from program text in 'str', or by using 'str' as a filepath to load program text from a file.
 // at the same time, we also build the strings list
 if( str_is_filepath){
  tokens = loadtokensfromtext(prog,str,&tokens_length);
 }else{
  tokens = tokenise(prog,str,&tokens_length,"[command line input]");
 }
 prog->tokens = tokens; prog->length = tokens_length; prog->maxlen = tokens_length;
 // search for function definitions and process them
 process_function_definitions(prog,0);
 return prog;
}

// ----------------------------------------------------------------------------------------------------------------

// this returns the allocated area as an index into the variable array
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
allocate_variable_data(program *prog, int amount ){
 if( amount <= 0 ) error(prog, -1, "allocate_variable_data: bad allocation request %d",amount);
 if( prog->next_free_var+amount > (prog->vsize - prog->ssize) ) error(prog, -1, "allocate_variable_data: run out of space");
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

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
add_id(id_info *ids, id_info *new_id){
 while( ids->next ){
  // check if this id name is already used in this id list
  if( ids->name && !strcmp(new_id->name, ids->name) ){
   //fprintf(stderr,"add_id: id was '%s'\n",new_id->name);
   //error(NULL, "add_id: this id is already used in this id list");
   return 0;
  }
  ids=ids->next;
 }
 ids->next = new_id;
 return 1;
}
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
add_id__error_if_fail( program *prog, int p, char *errormessage,   id_info *ids, id_info *new_id){
 if( ! add_id( ids, new_id ) ){
  error(prog, p, "%s, id was '%s'",errormessage,new_id->name);
 }
}

id_info*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
make_id(char *id_string, token t){
 id_info *out = calloc(1, sizeof(id_info));
 out->t = t;
 size_t l = strlen(id_string);
 char *name_permanent_address = malloc(l+1); strcpy(name_permanent_address, id_string);
 out->name = name_permanent_address;
 return out;
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
free_ids(id_info *ids){
 if(ids == NULL ) return;
 free_ids(ids->next);
 free(ids->name);
 free(ids);
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
process_id(program *prog, token *t){
 id_info *foundid=NULL;
 // first, check current function's id list
 foundid = find_id( prog->current_function->ids, (char*)t->data.pointer );
 // if nothing was found, check the global id list
 if(foundid==NULL) foundid = find_id( prog->ids, (char*)t->data.pointer );
 // if nothing was still found, error
 if(foundid==NULL){
  error(prog, (int) (t - prog->tokens), "process_id: unknown id '%s'",(char*)t->data.pointer);
 }
 // --- something was found ---
 // overwrite the token with the kind of token in the found id_info
 *t = foundid->t;
}

id_info*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
peek_id(program *prog, char *searchforthis){
 id_info *foundid=NULL;
 // first, check current function's id list
 foundid = find_id( prog->current_function->ids, searchforthis );
 // if nothing was found, check the global id list
 if(foundid==NULL) foundid = find_id( prog->ids, searchforthis );
 return foundid;
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
list_ids(id_info *ids){
 if(ids == NULL ) return;
 if(ids->name)printf(" id: '%s'\n",ids->name);
 list_ids(ids->next);
}

char*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
getfuncname(program *prog,func_info *f){
 id_info *id = prog->ids;
 while(1){
  if( id->t.type == t_Ff && id->t.data.pointer == f ) return id->name;
  id = id->next;
  if( id == NULL ) return NULL;
 }
}
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
list_all_ids(program *prog){
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
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
process_function_definition(int pos,program *prog){
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
  if(prog->functions[func_number]){ fprintf(stderr,"process_function_definition: this function number is already used\n"); goto process_function_definition_errorout; }
  break;
 case t_id:
  // check if the id is allowed, eg, if it's not already used
   if( find_id(prog->ids, (char*)prog->tokens[pos].data.pointer) ){ fprintf(stderr,"process_function_definition: this identifier '%s' is already used\n",(char*)prog->tokens[pos].data.pointer); goto process_function_definition_errorout; }
  // find the next unused function number, error if there are no unused function numbers
   {int i;
    for(i=0; i<MAX_FUNCS; i++){
     if( !prog->functions[i] ){ func_number = i; i=MAX_FUNCS; }
    }
    if(func_number == -1){ fprintf(stderr,"process_function_definition: too many functions have already been defined\n"); goto process_function_definition_errorout; }
   }
  // save the id's string in func_id, to be later added to the id list 
  func_id = (char*)prog->tokens[pos].data.pointer;
  pos +=1;
  break;
 default: fprintf(stderr,"process_function_definition: expected F or identifier\n"); goto process_function_definition_errorout;
 }
 
 // is this a function with named parameters, or a function with unnamed parameters, or a function that has no parameters at all?
 switch( prog->tokens[pos].type ){
 case t_id: { // function with named parameters. 
  //get parameter ids, store them in the function's id list
   int num_ps = 0;
   while( prog->tokens[pos].type == t_id ){
    add_id__error_if_fail( prog, pos, "process_function_definition: id for this parameter is already used\n",
                           out->ids, make_id( (char*)prog->tokens[pos].data.pointer,  maketoken_stackaccess( num_ps | 0x80000000 ) ) ); // needs to be adjusted later once we know how many locals there are
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
    add_id__error_if_fail( prog, pos, "process_function_definition: id for this local is already used\n",
                           out->ids, make_id( (char*)prog->tokens[pos].data.pointer,  maketoken_stackaccess( num_ls /* make extra room for _num_params */ +(out->params_type==p_atleast) ) ) ); 
    num_ls +=1;
    pos+=1;
   }
   if( num_ls == 0 ){ fprintf(stderr,"process_function_definition: local: expected at least one identifier\n"); goto process_function_definition_errorout; }
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
 default: fprintf(stderr,"process_function_definition: expected L or identifier\n"); goto process_function_definition_errorout;
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
  fprintf(stderr,"process_function_definition: the function definition is complete but ';' was not found\n"); goto process_function_definition_errorout;
 }
 pos+=1;
 // set function execution start position
 out->start_pos = pos;
 // save id information if this is a named function
 if( func_id ) add_id__error_if_fail( prog, -1, "process_function_definition: the id for this function is already used\n",
                                      prog->ids, make_id( func_id, tok ) );
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
#define opt_cleanup	5	// free unused memory from all stringvars and string accs
#define opt_randomise	6	// get a new random seed based on the current clock time in seconds
//#define	opt_importreplace 7	// import, but existing functions with the same name are replaced
//#define opt_evalimport	8	// import, but treating string argument as code containing function definitions
//#define opt_evalimportreplace 9 // see above
//#define opt_clear	10	// forget all variables

#ifdef enable_graphics_extension
#define opt_wmclose	100	// specify action to take when the window close button is pressed
#define opt_wintitle	101	// set the window title string
#define opt_copytext	102	// put text on the clipboard
#define opt_pastetext	103	// read text from the clipboard
#define opt_setcliprect 104	// set graphics clipping rectangle
#define opt_clearcliprect 105	// reset the graphics clipping rectangle
#define opt_drawbmp	106	// treat the contents of a stringvalue as a microsoft 24bit .bmp image
#define opt_drawbmpadv  107	// drawbmp 'advanced' - with the full set of parameters
#define opt_copybmp     108     // put bmp on clipboard
#define opt_pastebmp    109     // read bmp from clipboard
#define opt_hascliptext 110	// check if the clipboard has text	
#define opt_hasclipbmp	111	// check if the clipboard has .bmp image
#define opt_customchar  112	// define a custom font character
#define opt_xresource	113	// get an X resource value
#define opt_startgraphics 114	// start graphics in single threaded mode, where the johnsonscript program itself will manually decide when to update (process events etc)
#define opt_xupdate	115	// process x events and update display etc
#define opt_pensize	116	// line width for drawing 
#endif

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
option( program *prog, int *p ){
 int starting_p = *p;
 *p += 1; // advance p out of the way of the 'option' command itself
 int id_stringconst_pos = *p;
 
 if( prog->tokens[*p].type == t_id ) process_id(prog, &prog->tokens[*p]);

 int opt_number=-1;

 if(  determine_valueorstringvalue(prog, *p, 1)  ){ // identify option string
  stringval id_string = getstringvalue( prog, p );
  if( id_string.len == 0 ){ opt_number=-1; goto option__identify_option_string_out; } // FUCK OFF! for FUCK'S sake...
  if( !strncmp( "vsize", id_string.string, id_string.len ) ){ opt_number = opt_vsize; goto option__identify_option_string_out;}	//	vsize		Set the size of the variables array
  if( !strncmp( "ssize", id_string.string, id_string.len ) ){ opt_number = opt_ssize; goto option__identify_option_string_out;}	//	ssize		Set the size of the stack array
  if( !strncmp( "import", id_string.string, id_string.len )){opt_number = opt_import; goto option__identify_option_string_out;}	//	import		Import functions from another file
  if( !strncmp( "seedrnd", id_string.string, id_string.len)){opt_number= opt_seedrnd; goto option__identify_option_string_out;}	//	seedrnd		Seed RNG
  if( !strncmp( "randomise", id_string.string, id_string.len)){opt_number= opt_randomise; goto option__identify_option_string_out;}	//	randomise
  if( !strncmp( "randomize", id_string.string, id_string.len)){opt_number= opt_randomise; goto option__identify_option_string_out;}	//	randomise
  if( !strncmp( "unclaim", id_string.string, id_string.len)){opt_number= opt_unclaim; goto option__identify_option_string_out;} //	unclaim [string ref num]	Unclaim a string
  if( !strncmp( "cleanup", id_string.string, id_string.len)){opt_number= opt_cleanup; goto option__identify_option_string_out;} //	cleanup		String var/acc garbage collection
#ifdef enable_graphics_extension
  if( !strncmp( "wintitle", id_string.string, id_string.len ) ){ opt_number=opt_wintitle;	goto option__identify_option_string_out;}	// 
  if( !strncmp( "wmclose", id_string.string, id_string.len ) ){ opt_number=opt_wmclose;	goto option__identify_option_string_out;}	//	
  if( !strncmp( "copytext", id_string.string, id_string.len ) ){ opt_number=opt_copytext;	goto option__identify_option_string_out;}	// copytext [stringvalue]
  if( !strncmp( "pastetext", id_string.string, id_string.len ) ){ opt_number=opt_pastetext;	goto option__identify_option_string_out;}	// pastetext [string variable reference number]
  if( !strncmp( "setcliprect", id_string.string, id_string.len ) ){ opt_number=opt_setcliprect;	goto option__identify_option_string_out;}	// setcliprect [x1] [y1] [w] [h]
  if( !strncmp( "clearcliprect", id_string.string, id_string.len ) ){ opt_number=opt_clearcliprect;	goto option__identify_option_string_out;} // clearcliprect
  if( !strncmp( "drawbmp", id_string.string, id_string.len ) ){ opt_number=opt_drawbmp;	goto option__identify_option_string_out;}	// drawbmp [stringvalue] [x] [y]
  if( !strncmp( "drawbmpadv", id_string.string, id_string.len ) ){ opt_number=opt_drawbmpadv;	goto option__identify_option_string_out;}	// drawbmp [stringvalue] [x] [y] [sx] [sy] [w] [h]
  if( !strncmp( "copybmp", id_string.string, id_string.len ) ){ opt_number=opt_copybmp;	goto option__identify_option_string_out;}	// copybmp [stringvalue]
  if( !strncmp( "pastebmp", id_string.string, id_string.len ) ){ opt_number=opt_pastebmp;goto option__identify_option_string_out;}	// pastebmp [string var reference number]
  if( !strncmp( "hascliptext", id_string.string, id_string.len ) ){ opt_number=opt_hascliptext;goto option__identify_option_string_out;}// hascliptext [variable reference number]
  if( !strncmp( "hasclipbmp", id_string.string, id_string.len ) ){ opt_number=opt_hasclipbmp;goto option__identify_option_string_out;}// hasclipbmp [variable reference number]
  if( !strncmp( "customchar", id_string.string, id_string.len ) ){ opt_number=opt_customchar;goto option__identify_option_string_out;}// hasclipbmp [variable reference number]
  if( !strncmp( "xresource",  id_string.string, id_string.len ) ){ opt_number=opt_xresource;goto option__identify_option_string_out;}// xresource [stringvar ref num] [itemname] [classname]
  if( !strncmp( "startgraphics",  id_string.string, id_string.len ) ){ opt_number=opt_startgraphics;goto option__identify_option_string_out;}// startgraphics [width] [height]
  if( !strncmp( "xupdate",  id_string.string, id_string.len ) ){ opt_number=opt_xupdate;goto option__identify_option_string_out;}// xupdate [enableblocking]
  if( !strncmp( "pensize",  id_string.string, id_string.len ) ){ opt_number=opt_pensize;goto option__identify_option_string_out;}// pensize [size]
#endif
  //if( !strncmp( "", id_string.string, id_string.len ) ){ opt_number=;	goto option__identify_option_string_out;}	//	
  // ------ check external options -----
  id_info *extopt = prog->external_options;
  if( extopt ) extopt = extopt->next;
  while( extopt ){
   int l = strlen( extopt->name );
   if( l == id_string.len && !strcmp(extopt->name, id_string.string) ){
    prog->tokens[starting_p] = extopt->t;
    *p = starting_p;
    return;
   }
   extopt = extopt->next;
  }
  // -----------------------------------
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
  if( new_vsize <= 0 ) error(prog, starting_p, "option: vsize: new vsize '%d' is less than or equal to 0",new_vsize);
  if( prog->next_free_var ) error(prog, starting_p, "option: vsize: it's not possible to change the vsize after variables have been allocated");
  double *newvarray = calloc(new_vsize + prog->ssize, sizeof(double));
  if( newvarray == NULL ) error(prog, starting_p, "option: vsize: failed to allocate memory");
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
  if( new_ssize <= 0 ) error(prog, starting_p, "option: ssize: new ssize '%d' is less than or equal to 0",new_ssize);
  if( prog->next_free_var ) error(prog, starting_p, "option: ssize: it's not possible to change the ssize after variables have been allocated");
  if( new_ssize <= prog->sp + (prog->current_function->num_locals + (prog->current_function->params_type==p_atleast ? prog->stack[ prog->sp ] : prog->current_function->num_params )) ) error(prog, starting_p, "option: ssize: new stack size is too small to contain the current contents of the stack");
  prog->vsize -= prog->ssize;  prog->ssize = new_ssize;  prog->vsize += new_ssize; //update size information
  prog->vars = realloc( prog->vars, sizeof(double) * prog->vsize ); // reallocate vars+stack array
  if( prog->vars == NULL ) error(prog, starting_p, "option: ssize: realloc failed");
  prog->stack = prog->vars + (prog->vsize - prog->ssize);
  break;
 }
 case opt_import: // import
 {
  if( prog->maxlen != prog->length ){
   error(prog, starting_p, "option: import: it's not possible to 'import' extra functions after using 'eval'");
  }
  token *tokens; int tokens_length; stringval filename;
  // get filename
  filename = getstringvalue(prog,p);
  char fname[filename.len+1];
  strncpy(fname, filename.string, filename.len);
  fname[filename.len]=0;
  // load text file and process it, getting the tokens (program code data) and program_strings
  tokens = loadtokensfromtext(prog, fname, &tokens_length);
  if( tokens == NULL ){
   error(prog, starting_p, "import: couldn't open file '%s'",fname);
  }
  // resize array and append new code
  prog->maxlen += tokens_length + 1;
  prog->tokens = realloc(prog->tokens, (prog->maxlen+1)*sizeof(token));
  memcpy(prog->tokens + (prog->maxlen - tokens_length), tokens, sizeof(token) * tokens_length);
  prog->length += tokens_length+1;
  // process function definitions for imported code
  process_function_definitions(prog, prog->maxlen - tokens_length);
  // Äkta dig för Rövar-Albin
  prog->tokens[prog->length]=(token){t_nul,0};
  // cleanup
  free(tokens);
  break;
 }
 case opt_seedrnd: // seed random number generator
 {
  if( determine_valueorstringvalue(prog, *p, 1) ){
   stringval state = getstringvalue(prog,p);
   int h = state.string[ state.len ];
   dhr_random__load_state( state.string );
   state.string[ state.len ]=h;
  }else{
   int count = 0;
   _rnd_v=0;
   do {
    unsigned long long int v = (unsigned long long int)getvalue(p,prog);
    _rnd_v = (_rnd_v<<32) ^ v;
    count++;
   } while( count<3 && isvalue( prog->tokens[*p].type ) );
  }
  break;
 }
 case opt_randomise:
 {
  SeedRng();
  break;
 }
 case opt_unclaim: // deprecated but preserved for compatibility, 'unclaim' is now a keyword
 {
  prog->tokens[starting_p].type = t_endstatement;
  prog->tokens[id_stringconst_pos].type = t_unclaim;
  *p = id_stringconst_pos;
  break;
 case opt_cleanup:
 {
  int i;
  for(i=0; i<prog->max_stringvars; i++){
   clean_stringvar( prog, prog->stringvars[i], 1 );
  }
  for(i=0; i<prog->max_string_accumulator_levels; i++){
   clean_stringvar( prog, prog->string_accumulator[i], 0 );
  }
 }
 break;
#ifdef enable_graphics_extension
 case opt_wmclose:
 {
  wmcloseaction = getvalue(p,prog);
  break;
 }
 case opt_wintitle:
 {
  stringval sv = getstringvalue( prog, p );
  char c = sv.string[sv.len]; sv.string[sv.len]=0;
  SetWindowTitle(sv.string);
  sv.string[sv.len]=c;
  break;
 }
 case opt_copytext: case opt_copybmp: {
  stringval sv = getstringvalue( prog, p );
  if( ! sv.len ) return;
  if( opt_number == opt_copytext )
   NB_CopyTextN( sv.string, sv.len );
  else if( sv.len > 54 )
   NB_CopyBmpN( sv.string, sv.len );
 } break;
 case opt_pastetext: case opt_pastebmp: {
  int stringvar_num = (int)getvalue(p,prog);
  if( stringvar_num<0 || stringvar_num >= prog->max_stringvars ){
   error(prog, starting_p, "pastetext: bad stringvariable access '%d'",stringvar_num);
  }
  // now we have the stringvar as prog->stringvars[stringvar_num]
  if( ( opt_number == opt_pastetext ) ? NB_PasteText() : NB_PasteBmp() ){
   if( prog->stringvars[stringvar_num]->bufsize < PasteBufferContentsSize ){ // reallocate string buffer if necessary
    prog->stringvars[stringvar_num]->string = realloc( prog->stringvars[stringvar_num]->string, PasteBufferContentsSize );
    if( prog->stringvars[stringvar_num]->string == NULL ){
     error(prog, starting_p, "option: memory allocation failuring during Paste");
    }//endif
    prog->stringvars[stringvar_num]->bufsize = PasteBufferContentsSize;
   }
   memcpy(prog->stringvars[stringvar_num]->string, (void*)PasteBuffer, PasteBufferContentsSize); //copy data
   prog->stringvars[stringvar_num]->len = PasteBufferContentsSize; // set data length
  }else{
   prog->stringvars[stringvar_num]->len = 0; // paste failed, return empty string
  }
 } break;
 case opt_setcliprect: {
  int x,y,w,h;
  x=(int)getvalue(p,prog);
  y=(int)getvalue(p,prog);
  w=(int)getvalue(p,prog);
  h=(int)getvalue(p,prog);
  SetClipRect(x,y,w,h);
 } break;
 case opt_clearcliprect: {
  ClearClipRect();
 } break;
 case opt_drawbmp: case opt_drawbmpadv: {
  stringval bmpstring;
  int x,y, sx=0,sy=0,w=-1,h=-1;
  bmpstring = getstringvalue( prog, p );
  x=(int)getvalue(p,prog);
  y=(int)getvalue(p,prog);
  if( opt_number == opt_drawbmpadv ){
   sx = (int)getvalue(p,prog);
   sy = (int)getvalue(p,prog);
   w  = (int)getvalue(p,prog);
   h  = (int)getvalue(p,prog);
  }
  if( bmpstring.len > 54 ){ // do some very basic validity checking
   unsigned int offset     = *(unsigned int*)(bmpstring.string + 2+4+2+2);
   unsigned int image_size = *(unsigned int*)(bmpstring.string + 2+4+2+2+ 4+4+4+4+2+2+4);
   if( offset + image_size > (unsigned int)bmpstring.len ){
    error(prog, starting_p, "option: drawbmp: invalid bitmap");
    return;
   }
   NB_DrawBmp( x, y, sx,sy,w,h, (Bmp*)bmpstring.string );
  }
 } break;
 case opt_hascliptext: case opt_hasclipbmp: {
  int variable_number = (int)getvalue(p,prog);
  if( variable_number < 0 || variable_number >= prog->vsize+prog->ssize ){
   error(prog, starting_p, "option: hasclip: bad variable access '%d'",variable_number);
  }
  prog->vars[ variable_number ] = ( opt_number == opt_hascliptext ? NB_HasClipText() : NB_HasClipBmp() );
 } break;
 case opt_customchar: {
  int character_number; unsigned char glyph_bytes[8]; int i;
  character_number = (int)getvalue(p,prog);
  for( i=0; i<8; i++ ){
   glyph_bytes[i] = (unsigned char)getvalue(p,prog);
  }
  if( !( character_number & ~0xff ) ){
   CustomChar( character_number,
               glyph_bytes[0],  glyph_bytes[1], glyph_bytes[2], glyph_bytes[3],
               glyph_bytes[4],  glyph_bytes[5], glyph_bytes[6], glyph_bytes[7] ); 
  }
 } break;
 case opt_xresource: {
  int stringvar_num = (int)getvalue(p,prog);
  if( stringvar_num<0 || stringvar_num >= prog->max_stringvars ){
   error(prog, starting_p, "pastetext: bad stringvariable access '%d'",stringvar_num);
  }
  stringvar *sv = prog->stringvars[stringvar_num];
  stringval itemname = getstringvalue( prog, p ); unsigned char h1 = itemname.string[itemname.len]; itemname.string[itemname.len]=0;
  stringval classname = getstringvalue( prog, p ); unsigned char h2 = classname.string[classname.len]; classname.string[classname.len]=0;
  char *result = NewBase_GetXResourceString(itemname.string, classname.string);
  sv->len = 0;
  if( result ){
   size_t l = strlen(result);
   stringvar_adjustsizeifnecessary(prog, sv, l+1, 0);
   strcpy(sv->string, result);
   sv->len = l;
   free(result);
  }
  // Never repost any of our correspondence without my permission.
  itemname.string[itemname.len]=h1;
  classname.string[classname.len]=h2;
 } break;
 case opt_startgraphics: {
  int w = (int)getvalue(p,prog);
  int h = (int)getvalue(p,prog);
  NewBase_MyInit(w,h,0);
  usleep(1000);
 } break;
 case opt_xupdate: {
  NewBase_HandleEvents( isvalue( prog->tokens[*p].type ) ? (int)getvalue(p,prog) : 0 );
  XFlush(Mydisplay);
 } break;
 case opt_pensize: {
  SetLineThickness( (int)getvalue(p,prog) );
 } break;
#endif
 }
 default: error(prog, starting_p, "option: unrecognised option");
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


int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
wordmatch(int *pos, unsigned char *word, unsigned char *text){
 int i=0;
 int p=*pos;
 while( word[i] ){
  if( text[p++] != word[i++] ) return 0;
 }
 *pos = p;
 return 1;
}

// do wordmatch but only return true if the word has whitespace after it
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
wordmatch_plus_whitespace(int *pos, unsigned char *word, unsigned char *text){
 int holdpos = *pos;
 int result = wordmatch(pos, word, text);
 if( result ){
  //tb();
  //fprintf(stderr,"fuckin shit %d\n",*pos);
  if( isspace(text[*pos]) || (ispunct(text[*pos]) && text[*pos]!='_') || !text[*pos] ) return 1;
  *pos = holdpos;
 }
 return 0;
}

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
patternmatch(int pos, unsigned char *chars, unsigned char *text){
 int i,p, check;
 p=pos;
 while(1){
  check=1;
  i=0;
  while(chars[i]){
   if( text[p] == chars[i++] ){
    check=0;
    break;
   }//endif
  }//next
  if(check) return p-pos;
  p+=1;
 }//endwhile
}//endproc

/*
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
patterncontains(int pos, unsigned char *chars, unsigned char *text){
 int i,l,p, check;
 p=pos;
 while( text[p] && !isspace(text[p]) ){
  i=0;
  while( chars[i] ){
   if( text[p] == chars[i++] ){
    return 1;
   }//endif
  }//next
  p+=1;
 }//endwhile
 return 0;
}//endproc
*/

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
_gettoken_setstring(stringslist *progstrings, token *t, unsigned char *text, int l){
 //fprintf(stderr,"FUCK PENIS FUCK\n");
 // allocate memory for string and copy string to it
 t->data.pointer = calloc(l+1,sizeof(char));
 strncpy((char*)t->data.pointer, (char*)text, l);
 // keep track of this string so it can be freed later when the program is unloaded
 if(progstrings != NULL)stringslist_addstring(progstrings,(char*)t->data.pointer);
}
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
_gettoken_skippastcomments(program *prog, int *pos, unsigned char *text){
 if( isspace(text[*pos]) ) return;
 // block comment
 if( text[*pos]=='/' && text[*pos+1]=='*' ){
  *pos += 2;		
  while( !(text[*pos] == '*' && text[*pos+1] == '/') ){
   *pos += 1;
   if( !text[*pos] ) error(prog, -1, "missing */");
  }//endwhile
  *pos += 2;
 }//endif
 // line comment
 if( text[*pos]=='#' || (text[*pos]=='R' && wordmatch( pos,"REM", text)) ){	
  while( text[*pos] && text[*pos]!=10 ){
   *pos += 1;
  }//endwhile
 }//endif
}//endproc
token gettoken(program *prog, int test_run, int *pos, unsigned char *text){
 // if it's a test run, the token will be discarded, so strings must not be allocated
 stringslist *progstrings = prog->program_strings;

 token out; out.type = t_nul;

 gettoken_start:
 _gettoken_skippastcomments(prog,pos,text);
 while( isspace(text[*pos]) ){
  *pos+=1;
  _gettoken_skippastcomments(prog,pos,text);
 }
 //fprintf(stderr,"COCK ROCK %c\n",text[*pos]);

 int l;

#if allow_debug_commands
 if( wordmatch_plus_whitespace( pos,"listallids", text) ){	//	
  out = maketoken( t_listallids ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"testbeep", text) ){	//	
  out = maketoken( t_tb ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"printentirestack", text) ){	//	
  out = maketoken( t_printentirestack ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"printstackframe", text) ){	//	
  out = maketoken( t_printstackframe ); goto gettoken_normalout;
 }
#endif

 // ellipsis
 if( wordmatch( pos,"...", text) ){	//	...
  out =  maketoken( t_ellipsis ); goto gettoken_normalout;
 }

 // label
 if( text[*pos]=='.' ){	
  //fprintf(stderr,"here\n");
  l = patternmatch( *pos+1,"_qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890.", text);
  if(l){
   // put here: allocate memory for identifier string and store it wherever
   out.type = t_label;
   if(!test_run) _gettoken_setstring(progstrings, &out,text+*pos,l+1);
   *pos += l+1;
   goto gettoken_normalout;
  }
 }

 // binary number literal/constant
 if( text[*pos] == '0' && text[*pos+1] == 'b' ){
  int pos2 = *pos + 2;
  int bin_value_result = 0;
  while( text[pos2] == '0' || text[pos2] == '1' ){
   bin_value_result = (bin_value_result << 1) | (text[pos2] == '1');
   pos2 += 1;
  }
  out = maketoken_num( (double) bin_value_result );
  *pos = pos2;
  goto gettoken_normalout;
 }

 // hex number literal/constant 
 if( text[*pos] == '0' && text[*pos+1] == 'x' ){ 
  int hexchar(unsigned char c){
   if( c >= '0' && c <= '9') {
    return c-'0';
   } else if( c>='A' && c<='F') {
    return c-'A'+10;
   } else if( c>='a' && c<='f') {
    return c-'a'+10;
   } else return -1;
  }
  int pos2 = *pos + 2;
  int hex_value_result = 0; int new_hex_value;
  while( ( new_hex_value=hexchar(text[pos2]) ) != -1 ){
   hex_value_result = ( hex_value_result << 4 ) | new_hex_value;
   pos2 += 1;
  }
  out = maketoken_num( (double) hex_value_result );
  *pos = pos2;
  goto gettoken_normalout;
 }
 
 // number literal/constant
 if( (text[*pos]>='0' && text[*pos]<='9') || ( text[*pos]=='-' && (text[*pos+1]>='0' && text[*pos+1]<='9') ) ){
  int l=0;
  if( text[*pos]=='-' ){ l+=1; *pos+=1; }
  while( (text[*pos]>='0' && text[*pos]<='9') || (text[*pos] == '.') ){
   *pos += 1;
   l += 1;
  }
  out = maketoken_num( strtod(text + *pos - l, NULL) );
  goto gettoken_normalout;
 }
 /*
 l = patternmatch( *pos,"-123456789.0", text);
 if( l && !(l==1 && text[*pos] == '-') && patterncontains( *pos,"1234567890", text) ){	
  out=maketoken_num( strtod(text+*pos, NULL) );
  *pos += l;
  goto gettoken_normalout;
 }
 */

 if( wordmatch( pos,"&&", text) ){	//	&&
  out =  maketoken( t_land ); goto gettoken_normalout;
 }
 if( wordmatch( pos,"||", text) ){	//	||
  out =  maketoken( t_lor ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"neg", text) ){	//	neg
  out =  maketoken( t_neg ); goto gettoken_normalout;
 }

 // -------------------------------------------------------------------------------------------------------------
 // ----  single character wordmatch  ---------------------------------------------------------------------------
 // -------------------------------------------------------------------------------------------------------------
 if( text[*pos]=='%' ){	//	%
  *pos+=1;
  out = maketoken( t_mod ); goto gettoken_normalout;
 }

 if( text[*pos]=='!' ){	//	!
  *pos+=1;
  out =  maketoken( t_lnot ); goto gettoken_normalout;
 }

 if( text[*pos]=='+' ){	//	+
  *pos+=1;
  out =  maketoken( t_plus ); goto gettoken_normalout;
 }
 if( text[*pos]=='-' ){	//	-
  *pos+=1;
  out =  maketoken( t_minus ); goto gettoken_normalout;
 }
 if( text[*pos]=='/' ){	//	/
  *pos+=1;
  out =  maketoken( t_slash ); goto gettoken_normalout;
 }
 if( text[*pos]=='*' ){	//	*
  *pos+=1;
  out =  maketoken( t_star ); goto gettoken_normalout;
 }
 if( text[*pos]=='&' ){	//	&
  *pos+=1;
  out =  maketoken( t_and ); goto gettoken_normalout;
 }
 if( text[*pos]=='|' ){	//	|
  *pos+=1;
  out =  maketoken( t_or ); goto gettoken_normalout;
 }
 if( text[*pos]=='^' ){	//	^
  *pos+=1;
  out =  maketoken( t_eor ); goto gettoken_normalout;
 }
 if( text[*pos]=='~' ){	//	~
  *pos+=1;
  out =  maketoken( t_not ); goto gettoken_normalout;
 }
 if( text[*pos]=='=' ){	//	=
  *pos+=1;
  out =  maketoken( t_equal ); goto gettoken_normalout;
 }
 if( text[*pos]=='D' ){	//	D ('data', 'dereference' - indirect access)
  *pos+=1;
  out =  maketoken( t_D ); goto gettoken_normalout;
 }
 if( text[*pos]=='A' ){	//	A ('array' - indexed indirect access)
  *pos+=1;
  out =  maketoken( t_A ); goto gettoken_normalout;
 }
 if( text[*pos]=='C' ){	//	C ('character' - byte array access with strings)
  *pos+=1;
  out =  maketoken( t_C ); goto gettoken_normalout;
 }
 if( text[*pos]=='V' ){	//	V ('value' - value array access with strings)
  *pos+=1;
  out =  maketoken( t_V ); goto gettoken_normalout;
 }

 if( text[*pos]=='L' ){	//	L ('local' - indexed local variable access)
  *pos+=1;
  out =  maketoken( t_L ); goto gettoken_normalout;
 }
 if( text[*pos]=='P' ){	//	P ('param' - indexed function parameter access)
  *pos+=1;
  out =  maketoken( t_P ); goto gettoken_normalout;
 }
 if( text[*pos]=='F' ){	//	F (function dereference)
  *pos+=1;
  out =  maketoken( t_F ); goto gettoken_normalout;
 }
 if( text[*pos]=='$' ){	//	$ (stringvariable dereference)
  *pos+=1;
  out =  maketoken( t_S ); goto gettoken_normalout;
 }
 if( text[*pos]=='S' ){	//	S (create temporary stringvariable)
  *pos+=1;
  out =  maketoken( t_SS ); goto gettoken_normalout;
 }

 if( text[*pos]=='@' ){	//  @, get reference
  *pos+=1;
  out = maketoken( t_getref ); goto gettoken_normalout;
 }

 if( text[*pos]==';' ){	//	;
  *pos+=1;
  out =  maketoken( t_endstatement ); goto gettoken_normalout;
 }
 if( text[*pos]=='(' ){	//	(
  *pos+=1;
  out =  maketoken( t_leftb ); goto gettoken_normalout;
 }
 if( text[*pos]==')' ){	//	)
  *pos+=1;
  out =  maketoken( t_rightb ); goto gettoken_normalout;
 }
 // -------------------------------------------------------------------------------------------------------------

 // -------------------------------------------------------------------------------------------------------------
 // -------------------- speednutting ---------------------------------------------------------------------------
 // -------------------------------------------------------------------------------------------------------------

 if( text[*pos]=='<' ){ // == < =================================================================================
  if( wordmatch( pos,"<<", text) ){	//	<<
   out = maketoken( t_shiftleft ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"<=", text) ){	//	<=
   out =  maketoken( t_lesseq ); goto gettoken_normalout;
  }
  *pos += 1;
  out =  maketoken( t_lessthan ); goto gettoken_normalout;
 }

 if( text[*pos]=='>' ){ // == > =================================================================================
  if( wordmatch( pos,">>>", text) ){	//	>>>
   out = maketoken( t_lshiftright ); goto gettoken_normalout;
  }
  if( wordmatch( pos,">>", text) ){	//	>>
   out = maketoken( t_shiftright ); goto gettoken_normalout;
  }
  if( wordmatch( pos,">=", text) ){	//	>=
   out =  maketoken( t_moreeq ); goto gettoken_normalout;
  }
  *pos += 1;
  out =  maketoken( t_morethan ); goto gettoken_normalout;
 } 

 if( text[*pos]=='a' ){ // == A =================================================================================
  if( wordmatch_plus_whitespace( pos,"abs", text) ){	//	abs
   out =  maketoken( t_abs ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"append$", text) ){	//	
   out = maketoken( t_appendS ); goto gettoken_normalout;
  }

  if( wordmatch( pos,"asc$", text) ){	//	asc$
   out = maketoken( t_ascS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"alloc", text) ){	//	
   out = maketoken( t_alloc ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"atan2", text) ){	//	
   out = maketoken( t_atan2 ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"atan", text) ){	//	
   out = maketoken( t_atan ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"acos", text) ){	//	
   out = maketoken( t_acos ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"asin", text) ){	//	
   out = maketoken( t_asin ); goto gettoken_normalout;
  }

 }

 if( text[*pos]=='b' ){ // == B =================================================================================

  if( wordmatch_plus_whitespace( pos,"bget", text) ){	//	
   out = maketoken( t_bget ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"bput", text) ){	//	
   out = maketoken( t_bput ); goto gettoken_normalout;
  }

 }

 if( text[*pos]=='c' ){ // == C =================================================================================
  if( wordmatch_plus_whitespace( pos,"constant", text) ){	//	
   out = maketoken( t_const ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"continue", text) ){ //	continue
   out = maketoken( t_continue ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"chr$", text) ){	//	chr$
   out = maketoken( t_chrS ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"cat$", text) ){	//	cat$
   out = maketoken( t_catS ); goto gettoken_normalout;
  }

  if( wordmatch_plus_whitespace( pos,"close", text) ){	//	
   out = maketoken( t_close ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"cmp$", text) ){	//	cmp$ (strcmp)
   out = maketoken( t_cmpS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"catch", text) ){	//	
   out = maketoken( t_catch ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"caseof", text) ){	//	
   out = maketoken( t_caseof ); goto gettoken_normalout;
  }

  if( wordmatch_plus_whitespace( pos,"cosh", text) ){	//	
   out = maketoken( t_cosh ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"cos", text) ){	//	
   out = maketoken( t_cos ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"ceil", text) ){	//	
   out = maketoken( t_ceil ); goto gettoken_normalout;
  }

 }

 if( text[*pos]=='f' ){ // == F =================================================================================

  if( wordmatch_plus_whitespace( pos,"function", text) ){	//	
   out = maketoken( t_deffn ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"for", text) ){	//	for
   out = maketoken( t_for ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"floor", text) ){	//	
   out = maketoken( t_floor ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"fmod", text) ){	//	
   out = maketoken( t_fmod ); goto gettoken_normalout;
  }

 }

 if( text[*pos]=='r' ){ // == R =================================================================================
  if( wordmatch_plus_whitespace( pos,"return", text) ){	//	
   out = maketoken( t_return ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"right$", text) ){	//	right$
   out = maketoken( t_rightS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"restart", text) ){	//	restart
   out = maketoken( t_restart ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"rnd", text) ){	//	rnd
   out = maketoken( t_rnd ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='i' ){ // == I =================================================================================
  if( wordmatch_plus_whitespace( pos,"if", text) ){	//	
   out = maketoken( t_if ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"increment", text) ){	//	
   out = maketoken( t_increment ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"int", text) ){	//	int
   out =  maketoken( t_int ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"instr$", text) ){	//	instr$
   out = maketoken( t_instrS ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='w' ){ // == W =================================================================================
  if( wordmatch_plus_whitespace( pos,"while", text) ){	//	
   out = maketoken( t_while ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"wait", text) ){	//	
   out = maketoken( t_wait ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"when", text) ){	//	
   out = maketoken( t_when ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='v' ){ // == V =================================================================================

  if( wordmatch_plus_whitespace( pos,"variable", text) ){	//	
   out = maketoken( t_var ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"val$", text) ){	//	val$
   out = maketoken( t_valS ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"vlen$", text) ){	//	vlen$
   out = maketoken( t_vlenS ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"vector$", text) ){	//	vlen$
   out = maketoken( t_vectorS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"vget", text) ){	//	
   out = maketoken( t_vget ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"vput", text) ){	//	
   out = maketoken( t_vput ); goto gettoken_normalout;
  }

 }

 if( text[*pos]=='e' ){ // == E =================================================================================
  if( wordmatch_plus_whitespace( pos,"endwhile", text) ){	//	
   out = maketoken( t_endwhile ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"endif", text) ){	//	
   out = maketoken( t_endif ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"else", text) ){	//	
   out = maketoken( t_else ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"endfunction", text) ){	//	endfunction
   out = maketoken( t_endfn ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"endfor", text) ){	//	endfor
   out = maketoken( t_endfor ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"endloop", text) ){	 //	endloop
   out = maketoken( t_endloop ); goto gettoken_normalout;
  }

  if( wordmatch( pos,"equal$", text) ){	//	cmp$ (strcmp)
   out = maketoken( t_equalS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"endcatch", text) ){	//	
   out = maketoken( t_endcatch ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"eval$", text) ){	//	
   out = maketoken( t_evalS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"evalexpr", text) ){	//	
   out = maketoken( t_evalexpr ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"eval", text) ){	//	
   out = maketoken( t_eval ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"exp", text) ){	//	
   out = maketoken( t_exp ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"eof", text) ){	//	
   out = maketoken( t_eof ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"ext", text) ){	//	
   out = maketoken( t_ext ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"endcase", text) ){	//	
   out = maketoken( t_endcase ); goto gettoken_normalout;
  }

 }

 if( text[*pos]=='s' ){ // == S =================================================================================
  if( wordmatch_plus_whitespace( pos,"sgn", text) ){	//	sgn
   out =  maketoken( t_sgn ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"set", text) ){	//	
   out = maketoken( t_set ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"stringvar", text) ){	//	
   out = maketoken( t_stringvar ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"str$", text) ){	//	str$
   out = maketoken( t_strS ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"string$", text) ){	//	string$
   out = maketoken( t_stringS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"sinh", text) ){	//	
   out = maketoken( t_sinh ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"sin", text) ){	//	
   out = maketoken( t_sin ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"sqr", text) ){	//	
   out = maketoken( t_sqr ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"sget", text) ){	//	
   out = maketoken( t_sget ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"sptr", text) ){	//	
   out = maketoken( t_sptr ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"sput", text) ){	//	
   out = maketoken( t_sput ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='o' ){ // == O =================================================================================
  if( wordmatch( pos,"oscli", text) ){	//	
   out = maketoken( t_oscli ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"option", text) ){	//	
   out = maketoken( t_option ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"openin", text) ){	//	
   out = maketoken( t_openin ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"openout", text) ){	//	
   out = maketoken( t_openout ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"openup", text) ){	//	
   out = maketoken( t_openup ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"otherwise", text) ){	//	
   out = maketoken( t_otherwise ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='l' ){ // == L =================================================================================
  if( wordmatch_plus_whitespace( pos,"local", text) ){	//	
   out = maketoken( t_local ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"left$", text) ){	//	left$
   out = maketoken( t_leftS ); goto gettoken_normalout;
  }
  if( wordmatch( pos,"len$", text) ){	//	len$
   out = maketoken( t_lenS ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"log10", text) ){	//	
   out = maketoken( t_log10 ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"log2", text) ){	//	
   out = maketoken( t_log2 ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"log", text) ){	//	
   out = maketoken( t_log ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='_' ){ // == _ =================================================================================
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
  // '_pi', alias for '3.141592653589793238462643383279502884197169399375105820974944'
  if( wordmatch( pos,"_pi", text) ){
   out = maketoken_num( 3.141592653589793238462643383279502884197169399375105820974944 ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"_rnd_state", text) ){	//	_rnd_state
   out = maketoken( t_rnd_state ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"_prompt", text) ){	//	
   out = maketoken( t_prompt ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"_error_line", text) ){	//	
   out = maketoken( t_error_line ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"_error_column", text) ){	//	
   out = maketoken( t_error_column ); goto gettoken_normalout;
  }
  //if( wordmatch_plus_whitespace( pos,"_error_number", text) ){	//	
  // out = maketoken( t_error_number ); goto gettoken_normalout;
  //}
  if( wordmatch_plus_whitespace( pos,"_error_file", text) ){	//	
   out = maketoken( t_error_file ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"_error_message", text) ){	//	
   out = maketoken( t_error_message ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='t' ){ // == T =================================================================================
  if( wordmatch_plus_whitespace( pos,"throw", text) ){	//	
   out = maketoken( t_throw ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"tanh", text) ){	//	
   out = maketoken( t_tanh ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"tan", text) ){	//	
   out = maketoken( t_tan ); goto gettoken_normalout;
  }
 }

 if( text[*pos]=='p' ){ // == P =================================================================================
  if( wordmatch_plus_whitespace( pos,"print", text) ){		
   out = maketoken( t_print ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"pow", text) ){	//	
   out = maketoken( t_pow ); goto gettoken_normalout;
  }
  if( wordmatch_plus_whitespace( pos,"ptr", text) ){	//	
   out = maketoken( t_ptr ); goto gettoken_normalout;
  }
 }

 // -------------------------------------------------------------------------------------------------------------
 // -------------------------------------------------------------------------------------------------------------
 // -------------------------------------------------------------------------------------------------------------

 if( wordmatch_plus_whitespace( pos,"unclaim", text) ){	//	unclaim [string reference number], release strings. Mainly used when working with temporary strings provided by 'S'
  out = maketoken( t_unclaim ); goto gettoken_normalout;
 }

 if( wordmatch_plus_whitespace( pos,"goto", text) ){	//	
  out = maketoken( t_goto ); goto gettoken_normalout;
 }

 if( wordmatch_plus_whitespace( pos,"quit", text) ){	//	
  out = maketoken( t_quit ); goto gettoken_normalout;
 }

 if( wordmatch_plus_whitespace( pos,"decrement", text) ){	//	
  out = maketoken( t_decrement ); goto gettoken_normalout;
 }

 // string constant specified using QUOTE()
 if( text[*pos]=='Q' && wordmatch(pos, "QUOTE(", text) ){
  int l=0; int level=1;
  while( level ){
   if( !text[*pos] ) error( prog, -1, "QUOTE(): missing )" );
   if( text[*pos] == ')' ){
    level -= 1;
   }
   if( text[*pos] == '(' ){
    level += 1;
   }
   *pos += 1; l += 1;
  }
  if(!test_run){
   _gettoken_setstring(progstrings, &out,text+*pos-l,l-1);
  }
  out.type = t_stringconst;
  goto gettoken_normalout;
 }

 // string constant
 if( text[*pos]=='"' ){
  *pos += 1;
  l=0;
  int escaped=0; int backslash_used=0; 
  while( text[*pos]!='"' || escaped ){
   escaped = 0;
   switch( text[*pos] ){
   case '\\': escaped=1; backslash_used=1; break;
   case 0: error(prog, -1, "missing \"");
   //case 10: error(prog, "missing \" on this line");
   }//endswitch
   l+=1;
   *pos += 1;
  }//endwhile
  *pos += 1;
  out.type = t_stringconst;
  if(!test_run){
   _gettoken_setstring(progstrings, &out,text+*pos-l-1,l);
   if( backslash_used ){
    char *str = out.data.pointer;
    for( int i=0; i<l; i++ ){
     if( str[i]=='\\' ){
      for( int j=i; j<l; j++ ){
       str[j]=str[j+1];
      } // next j
      l -= 1;
     } // endif
    } // next i
   } // endif
  } // endif
  goto gettoken_normalout;
 }//endif

 if( wordmatch( pos,"mid$", text) ){	//	mid$
  out = maketoken( t_midS ); goto gettoken_normalout;
 }

 #ifdef enable_graphics_extension 
 // ============================================================================================
 // ======= GRAPHICS EXTENSION =================================================================
 // ============================================================================================
 
 if( wordmatch_plus_whitespace( pos,"startgraphics", text) ){	//	startgraphics
  out = maketoken( t_startgraphics ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"stopgraphics", text) ){	//	
  out = maketoken( t_stopgraphics ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"winsize", text) ){	//	winsize
  out = maketoken( t_winsize ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"pixel", text) ){	//	
  out = maketoken( t_pixel ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"line", text) ){	//	
  out = maketoken( t_line ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"circlef", text) ){	//	
  out = maketoken( t_circlef ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"circle", text) ){	//	
  out = maketoken( t_circle ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace(pos, "arc", text) ){
  out = maketoken( t_arc ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace(pos, "arcf", text) ){
  out = maketoken( t_arcf ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"rectanglef", text) ){	//	
  out = maketoken( t_rectanglef ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"rectangle", text) ){	//	
  out = maketoken( t_rectangle ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"triangle", text) ){	//	
  out = maketoken( t_triangle ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"drawtext", text) ){	//	
  out = maketoken( t_drawtext ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"drawscaledtext", text) ){	//	
  out = maketoken( t_drawscaledtext ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"refreshmode", text) ){	//	
  out = maketoken( t_refreshmode ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"refresh", text) ){	//	
  out = maketoken( t_refresh ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"gcol", text) ){	//	
  out = maketoken( t_gcol ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"bgcol", text) ){	//	
  out = maketoken( t_bgcol ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"cls", text) ){	//	
  out = maketoken( t_cls ); goto gettoken_normalout;
 }
 // -----
 if( wordmatch_plus_whitespace( pos,"winw", text) ){	//	
  out = maketoken( t_winw ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"winh", text) ){	//	
  out = maketoken( t_winh ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"mousex", text) ){	//	
  out = maketoken( t_mousex ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"mousey", text) ){	//	
  out = maketoken( t_mousey ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"mouseb", text) ){	//	
  out = maketoken( t_mouseb ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"mousez", text) ){	//	
  out = maketoken( t_mousez ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"readkey$", text) ){	//	
  out = maketoken( t_readkeyS ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"readkey", text) ){	//	
  out = maketoken( t_readkey ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"keypressed", text) ){	//	
  out = maketoken( t_keypressed ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"expose", text) ){	//	
  out = maketoken( t_expose ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"wmclose", text) ){	//	
  out = maketoken( t_wmclose ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"keybuffer", text) ){	//	
  out = maketoken( t_keybuffer ); goto gettoken_normalout;
 }
 if( wordmatch_plus_whitespace( pos,"drawmode", text) ){	//	
  out = maketoken( t_drawmode ); goto gettoken_normalout;
 }
 // ============================================================================================
 // ======= END OF GRAPHICS EXTENSION ==========================================================
 // ============================================================================================
 #endif

 // ----------------------------------------------
 // ------- IDENTIFIER ---------------------------
 // ----------------------------------------------

 // identifier
 if( text[*pos]=='_' || (text[*pos]>='a' && text[*pos]<='z') ){	
  l = patternmatch( *pos+1,"_qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890", text);
  //if(l){
   // put here: allocate memory for identifier string and store it wherever
   out.type = t_id;
   // ----
   if(!test_run) _gettoken_setstring(progstrings, &out,text+*pos,l+1);
   // here: we check the list of extensions, and if this id matches one of those, we change the token accordingly
   //fprintf(stderr,"fuck %p\n",prog->extensions);
   // ----
   if(prog->extensions && !test_run){
    id_info *extension_id;
    //fprintf(stderr,"fuckers '%s'\n",(char*)out.data.pointer);
    if( extension_id = find_id( prog->extensions, (char*)out.data.pointer) ){
     out = extension_id->t;
    }
   }
   *pos += l+1;
   goto gettoken_normalout;
  //}
 }

/*
 if( wordmatch( pos,"", text) ){	//	
  out = maketoken( t_ ); goto gettoken_normalout;
 }
*/

 //fprintf(stderr,"FUCK (%c) %d   pos %d\n",text[*pos],text[*pos], *pos);

// fprintf(stderr,"*pos == %d: %d '%c'\n",*pos,text[*pos],text[*pos]);
// if(text[*pos])goto gettoken_start;

 if( out.type != t_nul ) fprintf(stderr,"FUCK OFF!\n"); // this should never FUCKING happen

 if( !text[*pos] ){ out.type = t_nul; goto gettoken_normalout; }

 gettoken_failout:
 if(prog) fprintf(stderr,"WARNING: garbage in input: \"");
 while( text[*pos] && !isspace(text[*pos]) ){
  if(prog) fprintf(stderr,"%c",text[*pos]);
  *pos+=1;
 }
 if(prog) fprintf(stderr,"\"\n");
 while( text[*pos] && isspace(text[*pos]) ){
  *pos+=1;
 }
 return maketoken( t_bad );

 gettoken_normalout:
 #if 0
 if(!test_run) fprintf( stderr, "reached gettoken_normal out with %s\n",tokenstring( out) );
 #endif
 while( text[*pos] && isspace(text[*pos]) ){
  *pos+=1;
  _gettoken_skippastcomments(prog,pos,text);
 }

 // -------- replacements by extensions ----------
 if(prog->extensions && !test_run){
  char searchstring[] = "__REPLACEMENT";
  *searchstring = out.type;
  id_info *check_for_replacement;
  if( check_for_replacement = find_id(prog->extensions,searchstring) ){
   out = check_for_replacement->t;
  }
 }
 // ----------------------------------------------

 return out;
}

int*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
get_line_end_array(unsigned char *text){
 int *out = NULL;
 unsigned char *p = text;
 int nlines=1;
 while(*p){
  if( *p == '\n' ) nlines+=1;
  p++;
 }
 nlines+=1;
 out = calloc(nlines+1,sizeof(int));
 out[0]=-1; 
 out+=1;
 int linep=0;
 p = text;
 while(*p){
  if( *p == '\n' ){
   out[linep]=(int)(p-text);
   linep+=1;
  }
  p++;
 }
 out[linep]=(int)(p-text);
 //fprintf(stderr, "help %d, %d\n",nlines, linep);
 return out; 
}

typedef struct tokentextpos { int line; int column; char *file; int arraysize; int lastindex; } TTP;

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
gettokens(program *prog, token *tokens_return, int len, unsigned char *text, unsigned char *name_of_source_file ){
 stringslist *progstrings = prog->program_strings;
 int *line_end_array=NULL; TTP *TextPosses = NULL; int linep=0;
 if(tokens_return && name_of_source_file && progstrings && progstrings->string){
  //tb();
  line_end_array = get_line_end_array(text);
  TextPosses = (TTP*)progstrings->string;
  if(TextPosses->lastindex){ //rellocate TTP for extra tokens
   int newarraysize = TextPosses->arraysize + 1 + len; // it may be temporarily larger than the actual amount of tokens we will store, because we don't know that yet
   progstrings->string = realloc( progstrings->string, newarraysize * sizeof(TTP) );
   TextPosses = (TTP*)progstrings->string;
   if( TextPosses == NULL ){
    fprintf(stderr,"gettokens: couldn't allocate memory for TTP\n");
    exit(1);
   }
   TextPosses->arraysize = newarraysize;
  }
  //printf("%p\n",TextPosses);
 }
 int count = 0, pos = 0; token t;
 int ttp_arraysize=0, ttp_lastindex=0; if(TextPosses) { ttp_arraysize = TextPosses->arraysize; ttp_lastindex = TextPosses->lastindex; }
 while(pos<len){
  // token text locations for error reports
  if(TextPosses){ 
   while( pos >= line_end_array[linep] ){
    linep += 1;
   }
   TextPosses[ ttp_lastindex + count ] = (TTP){ linep+1 ,pos-(line_end_array[linep-1]+1), name_of_source_file };//tb();
  }
  //if(TextPosses){ tb(); printf("test this shit: line %d, col %d, '%s'\n",TextPosses[count].line,TextPosses[count].column,tokenstring(t)); }
  // the token itself
  t = gettoken(prog,!tokens_return,&pos,text); if(tokens_return)tokens_return[count]=t;
  count +=1;
 }
 if(line_end_array) { free( (line_end_array-1) ); }
 if(TextPosses){ // update and trim the TTP array
  if( ttp_lastindex ){
   TextPosses->lastindex = ttp_lastindex + count;
   //fprintf(stderr,"shit %d\n",TextPosses->lastindex);
   TextPosses->arraysize = TextPosses->lastindex+1;
   TextPosses = realloc( TextPosses, TextPosses->arraysize * sizeof(TTP) );
   TextPosses[TextPosses->lastindex]=(TTP){0,0,0,0,0};
   progstrings->string = (char*) TextPosses;
  } else {
   TextPosses->lastindex = count;
   TextPosses->arraysize = ttp_arraysize;
  }  
 }
 return count;
}

token*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
tokenise(program *prog, char *text, int *length_return, char *name_of_source_file){
 stringslist *progstrings = prog->program_strings;
 int len = strlen(text);
 int count = gettokens(prog,NULL,len,text,NULL);
 // about 'count+1': allocate 1 space for one extra token that goes unused (set to 0) so that there's less likely to be trouble with segmentation faults
 token *out = calloc(count+1,sizeof(token)); 
 if( ! progstrings->string ){
  progstrings->string = (char*) calloc( count+1,sizeof(TTP) );
  ((TTP*)progstrings->string)->arraysize = count+1;
 }
 char *name_of_source_file_permanent_address = NULL;
 if( name_of_source_file ){
  name_of_source_file_permanent_address = calloc( strlen(name_of_source_file)+1, sizeof(char) );
  strcpy( name_of_source_file_permanent_address, name_of_source_file );
  stringslist_addstring( progstrings, name_of_source_file_permanent_address );
 }
 gettokens(prog,out,len,text,name_of_source_file_permanent_address);
 if(length_return)*length_return = count;
 #ifdef enable_debugger
 dbg__add_tokens(prog, text);
 #endif
 return out;
}

token*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
loadtokensfromtext(program *prog, char *path,int *length_return){
 FILE *f = fopen(path,"rb");
 if(f==NULL) return NULL;
 int ext = Ext(f);
 unsigned char *text = calloc( ext+1, sizeof(unsigned char));
 size_t ReturnValue;
 ReturnValue = fread((void*)text, sizeof(char), ext, f);
 token *out = tokenise(prog, text,length_return,path);
 fclose(f); free(text);
 return out;
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
get_sourcetext_location_info( program *prog, int token_p,  int *line, int *column, char **file){
 if( prog->program_strings->string == NULL || ( token_p<0 || token_p>=prog->length )  ){
  get_sourcetext_location_info_failure:
  *line = -1;
  *column = -1;
  *file = "[unspecified]";
  return;
 }
 TTP *ttp = (TTP*)prog->program_strings->string; if( token_p >= ttp->lastindex ){ goto get_sourcetext_location_info_failure; }
 *line = ttp[token_p].line;
 *column = ttp[token_p].column;
 *file = ttp[token_p].file == NULL ? "[unspecified]" : ttp[token_p].file;
 return;
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
print_sourcetext_location( program *prog, int token_p){
 if( prog->program_strings->string == NULL ) return;
 TTP *ttp = (TTP*)prog->program_strings->string;
 //fprintf(stderr,"hmm, ttp==%p\n",ttp); printf("ttp->arraysize==%d, ttp->lastindex==%d\n",ttp->arraysize,ttp->lastindex); //test
 if( token_p<0 || token_p>=prog->length || token_p>=ttp->lastindex ){
  //fprintf(stderr, "print_sourcetext_location: token_p out of range\n");
 }else{
  fprintf(stderr, ":::: At line %d, column %d, in file '%s' ::::\n", ttp[token_p].line, ttp[token_p].column, ttp[token_p].file == NULL ? "null" : ttp[token_p].file );
 }
}

// ----------------------------------------------------------------------------------------------------------------

char tokenstringbuf[256];

char*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
tokenstring(token t){
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
 case t_C:		return "C";
 case t_V:		return "V";
 case t_L:		return "L";
 case t_P:		return "P";
 case t_F:		return "F";
 case t_SS:		return "S";
 case t_Sf:		return "t_Sf";
 case t_deffn:		return "function";
 case t_local:		return "local";
 case t_ellipsis:	return "...";
 case t_number:
  {
   snprintf(tokenstringbuf,sizeof(tokenstringbuf),"%.2f",t.data.number);
   return tokenstringbuf;
  }
 case t_eval:	return "eval"; case t_evalS: return "eval$";
 case t_prompt: return "_prompt";
 case t_extfun: return "t_extfun";
 case t_extsfun: return "t_extsfun";
 case t_extcom: return "t_extcom";
 case t_leftb:		return "(";
 case t_rightb:		return ")";
 case t_endstatement:	return ";";
 case t_goto: return "goto";
 case t_gotof: return "gotoF";

 case t_endloop: return "endloop";
 case t_continue: return "continue";
 case t_restart: return "restart";

 case t_return:		return "return";
 case t_while: case t_whilef: case t_whileff:		return "while";
 case t_endwhile: case t_endwhilef: case t_endwhileff:	return "endwhile";
 case t_if: case t_iff:			return "if";
 case t_else: case t_elsef:		return "else";
 case t_endif: case t_endiff:		return "endif";

 case t_set_stackaccess: case t_set_Df:
 case t_set:		return "set";
 case t_appendS:	return "append$";

 case t_land: case t_landf: return "&&";
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
 case t_stringS:	return "string$";
 case t_ascS:		return "asc$";
 case t_valS:		return "val$";
 case t_lenS:		return "len$";
 case t_vlenS:		return "vlen$";
 case t_cmpS:		return "cmp$";
 case t_instrS:		return "instr$";
 case t_vectorS:	return "vector$";
 case t_equalS:		return "equal$";
 case t_for: case t_forf: return "for";
 case t_endfor: case t_endforf: return "endfor";

 case t_catch: case t_catchf: return "catch";
 case t_endcatch: return "endcatch";
 case t_throw: return "throw";

 case t_error_line:	return "_error_line";
 case t_error_column:	return "_error_column";
 //case t_error_number:	return "_error_number";
 case t_error_file:	return "_error_file";
 case t_error_message:	return "_error_message";

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
 case t_unclaim:	return "unclaim";

 case t_rnd:		return "rnd";
 case t_rnd_state:	return "_rnd_state";
 case t_wait:		return "wait";
 case t_alloc:		return "alloc";
 case t_endfn:		return "endfunction";
 case t_extopt:
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
 case  t_log2:		return "log2";
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
 case t_listallids: return "listallids";
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

 case t_increment: case t_increment_df: case t_increment_stackaccess: return "increment";
 case t_decrement: case t_decrement_df: case t_decrement_stackaccess: return "decrement";

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
 case t_arcf: return "arcf ";
 case t_arc:  return "arc ";
 case t_rectanglef: return "rectanglef";
 case t_rectangle: return "rectangle";
 case t_triangle: return "triangle";
 case t_drawtext: return "drawtext";
 case t_drawscaledtext: return "drawscaledtext";
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
// end of tokenstring

/*
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
detokenise(program *prog, int mark_position){
 int i;
 for( i=0; i<prog->length; i++){
  if(i==mark_position) printf("\nREM: here\n");
  printf( (prog->tokens[i].type==t_endstatement || prog->tokens[i].type==t_label) ? "%s\n":"%s ", tokenstring( prog->tokens[i] ) );
 }
}
*/

// ----------------------------------------------------------------------------------------------------------------
// -------------------- STRING HANDLING ---------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
clean_stringvar( program *prog, stringvar *svr, int preserve ){
 if( svr->unclaimed || !preserve ){
  svr->len = 0;
  if( svr->bufsize == DEFAULT_NEW_STRINGVAR_BUFSIZE ) return;
  svr->bufsize = DEFAULT_NEW_STRINGVAR_BUFSIZE;
  free( svr->string );
  svr->string = malloc( DEFAULT_NEW_STRINGVAR_BUFSIZE );
  if( ! svr->string ){
   error(prog, -1, "clean_stringvar: allocation failed");
  }
 }else if( svr->bufsize != svr->len+1 ){
  svr->bufsize = svr->len+1;
  svr->string = realloc( svr->string, svr->bufsize );
  if( svr->string == NULL ){
   error(prog, -1, "clean_stringvar: reallocation failed");
  }
 }
}

// unclaim [string ref number] 		Release string variables to be reused by 'S'
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
unclaim(program *prog, int *p){
 int starting_p = *p;
 *p += 1;
 int stringvar_num = (int)getvalue(p,prog);
 if( stringvar_num<0 || stringvar_num >= prog->max_stringvars ){
  error(prog, starting_p, "unclaim: bad stringvariable access '%d'",stringvar_num);
 }
 stringvar *svr = prog->stringvars[stringvar_num];
 svr->unclaimed=1;
 if( svr->bufsize > DEFAULT_NEW_STRINGVAR_BUFSIZE ) clean_stringvar( prog, svr, 0 );
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
mystrncpy(char *dest, char *src, size_t n){
 //strncpy(dest,src,n); return;
 size_t i;
 for(i=0; i<n /*&& src[i]*/; i++){
  dest[i]=src[i];
 }
 dest[i]=0;
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
stringvar_adjustsizeifnecessary(program *prog, stringvar *sv, size_t bufsize_required, int preserve){
 //fprintf(stderr,"ADJUST	sv %p		str %p	siz %lu	len %lu	req %lu	preserve %d\n", sv, sv->string, sv->bufsize, sv->len, bufsize_required, preserve); Wait(1);
 if(bufsize_required<sv->bufsize){
  //tb();
  return;
 }
 int new_bufsize = bufsize_required+(256-bufsize_required % 256);
 //if( new_bufsize <= bufsize_required ) error(NULL, -1, "still happening");
 if(preserve){
  sv->string = realloc(sv->string, new_bufsize);
  if(sv->string == NULL) error(prog, -1, "stringvar_adjustsize: realloc failed");
  sv->bufsize = new_bufsize;
  sv->string[sv->len]=0;
 }else{
  free(sv->string);
  sv->bufsize = new_bufsize;
  sv->string = malloc(new_bufsize);
  *sv->string = 0;
 }
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
copy_stringval_to_stringvar(program *prog, stringvar *dest, stringval src){
 if( src.len+1 >= dest->bufsize ) stringvar_adjustsizeifnecessary(prog, dest, src.len+1, 0);
 mystrncpy(dest->string, src.string, src.len);
 dest->len = src.len;
}

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
isstringvalue(TOKENTYPE_TYPE type){
 return (type==t_leftb || type==t_id || (type>=STRINGVALS_START && type<=STRINGVALS_END)) ;
}

// ========================================================
// =========  GETSTRINGVALUE  =============================
// ========================================================

stringval
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
getstringvalue( program *prog, int *pos ){
 int level = prog->getstringvalue_level;
 // check if there's a string accumulator for this level & if not, prepare one 
 if(level >= prog->max_string_accumulator_levels){
  prog->string_accumulator = realloc((void*)prog->string_accumulator, sizeof(void*)*(prog->max_string_accumulator_levels+1));
  prog->string_accumulator[prog->max_string_accumulator_levels] = newstringvar();
  prog->max_string_accumulator_levels += 1;
 }
 stringvar *accumulator = prog->string_accumulator[level];
 token t;
 #ifdef enable_debugger
 dbg__tick_getstringvalue(prog, *pos);
 #endif
 getstringvalue_start:
 t = prog->tokens[ *pos ]; *pos += 1;
 switch( t.type ){
 case t_S:
 {
  int stringvar_num = (int)getvalue(pos,prog);
  if( stringvar_num<0 || stringvar_num >= prog->max_stringvars ){
   error(prog, *pos-1, "getstringvalue: bad stringvariable access '%d'",stringvar_num);
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
  if( sv.len <=0 || midpos < 0 || midpos >= sv.len ){
   sv.len=0;
   return sv;
  }
  if( len < 0 ) len = sv.len;
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
  size_t bufsize_required;
  accumulator->len=0;
  while( isstringvalue( prog->tokens[ *pos ].type ) ){
   in = getstringvalue( prog,pos );
   bufsize_required = accumulator->len + in.len+1;
   if( bufsize_required > accumulator->bufsize ) stringvar_adjustsizeifnecessary(prog, accumulator, bufsize_required, 1);
   mystrncpy( accumulator->string + accumulator->len,  in.string, in.len );
   accumulator->len += in.len;
  }
  stringval out;
  out.len = accumulator->len;
  out.string = accumulator->string;
  prog->getstringvalue_level -= 1;
  return out;
 }
 case t_stringS:
 {
  char *stringS_tempbuf = NULL;
  double n_value = getvalue(pos,prog);
  stringval svl = getstringvalue( prog,pos );
  if( ! svl.len || n_value < 1 ){
   return (stringval){NULL,0};
  }
  size_t n = (size_t)n_value;
  if( svl.string == accumulator->string || ( svl.string > accumulator->string && svl.string <= accumulator->string+accumulator->len ) ){ // Äkta dig för Rövar-Albin
   stringS_tempbuf = malloc(svl.len);
   memcpy(stringS_tempbuf, svl.string, svl.len);
   svl.string = stringS_tempbuf;
  }
  size_t bufsize_required = svl.len * n;
  if( bufsize_required > accumulator->bufsize ){
   stringvar_adjustsizeifnecessary(prog, accumulator, bufsize_required, 1);
  }
  if( svl.len == 1 ){ // if it's one character we can just do a nice memset()
   memset(accumulator->string, svl.string[0], n);
  }else{ // or otherwise
   size_t i;
   char *p = accumulator->string;
   for(i=0; i<n; i++){
    memcpy(p, svl.string, svl.len);
    p += svl.len;
   }//next
  }//endif
  stringval out;
  out.len = bufsize_required;
  out.string = accumulator->string;
  if(stringS_tempbuf){
   free(stringS_tempbuf);
  }
  return out;
 }
 case t_strS:
 {
  double val = getvalue(pos,prog);
  int snpf_return;
  if(!MyIsnan(val) && val == (double)(int)val){
   snpf_return = snprintf(accumulator->string, accumulator->bufsize, "%d",(int)val);
  }else{
   snpf_return = snprintf(accumulator->string, accumulator->bufsize, "%.16f",val);
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
 case t_extsfun:
 {
  return (( stringval(*)(program*,int*) )t.data.pointer)(prog,pos);
 }
 case t_leftb:
 {
  stringval out = getstringvalue( prog,pos );
  if( prog->tokens[ *pos ].type != t_rightb ){
   error(prog, *pos-1, "getstringvalue: expected closing bracket");
  }
  *pos += 1;
  return out;
 }
 case t_vectorS:
 {
  double *vectorp = (double*)accumulator->string;
  accumulator->len = 8;
  *vectorp++ = getvalue(pos, prog);
  while( isvalue( prog->tokens[*pos].type ) ){
   *vectorp++ = getvalue(pos, prog);
   accumulator->len += 8;
   if( accumulator->bufsize - accumulator->len < 32 ){
    size_t ptr_offset = (void*)vectorp - (void*)accumulator->string;
    stringvar_adjustsizeifnecessary(prog, accumulator, accumulator->bufsize+32, 1);
    vectorp = (double*)(accumulator->string + ptr_offset);
   }
  }
  stringval out;
  out.len=accumulator->len;
  out.string=accumulator->string;
  return out;
 }
 case t_evalS:
 {
  stringval v;
  stringval s = getstringvalue(prog,pos);
  int h = s.string[s.len]; s.string[s.len]=0;
  int result = interpreter_eval( prog, eval_getstringvalue, &v, s.string );
  s.string[s.len]=h;
  if( result ) error( prog, *pos, prog->_error_message->string );
  return v;
 }
 case t_sget: // sget [fp] ([optional parameter n])
 {            // If n is greater than 0, it represents the number of bytes to be read. Else, it represents a specific string terminating value. (n <= -256) will read until EOF
  file *f = getfile(prog, getvalue(pos,prog), 1,0);
  int num_bytes_to_read = -10;
  if( isvalue( prog->tokens[*pos].type ) ) num_bytes_to_read = getvalue(pos,prog); // FIXME: size_t fix: this needs fixing to enable loading strings longer than signed 32bit int max
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
     stringvar_adjustsizeifnecessary(prog, accumulator, accumulator->bufsize + 1, 1);
     //printf("fuck2 %d, %d\n",accumulator->len, accumulator->bufsize);
     //if( accumulator->len >= accumulator->bufsize ) error(NULL,-1,"FUCK PENIS FUCK"); //remove later
    }
   }
   // ==========================================================
  }else{ 
   // ======= reading a specific number of bytes ===============
   if( accumulator->bufsize <= num_bytes_to_read ){
    stringvar_adjustsizeifnecessary(prog, accumulator, num_bytes_to_read, 1);
    //if( accumulator->bufsize <= num_bytes_to_read ) error(NULL,-1,"FUCK ###########################################"); //remove later
   }//endif bufsize
   accumulator->len = fread( (void*)accumulator->string, sizeof(char), num_bytes_to_read, f->fp );
   // ==========================================================
  }//endif whether or not we're reading a set number of bytes, or reading until a specific character
  stringval out;
  out.len = accumulator->len;
  out.string = accumulator->string;
  return out;
 }//end case block
 case t_rnd_state: {
  accumulator->len = 16; 
  dhr_random__save_state( accumulator->string );
  return *(stringval*)accumulator;
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
 case t_error_message: return *(stringval*)prog->_error_message;
 case t_error_file:    return *(stringval*)prog->_error_file;
 default:
  error(prog, *pos-1, "getstringvalue: didn't find a stringvalue, instead found '%s'",tokenstring(t));
 }//endswitch
}//endproc

stringvar*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
create_new_stringvar(program *prog){
 int svnum = prog->max_stringvars;
 prog->stringvars = realloc(prog->stringvars, sizeof(void*)*(svnum+1));
 prog->stringvars[svnum] = newstringvar();
 prog->stringvars[svnum]->string_variable_number = svnum; // used by 'getref'
 prog->max_stringvars += 1;
 return prog->stringvars[svnum];
}

stringvar*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
__attribute__((noinline))
obtain_new_stringvar(program *prog){
 for( int i=0; i<prog->max_stringvars; i++ ){
  if(prog->stringvars[i]->unclaimed){
   prog->stringvars[i]->unclaimed = 0;
   prog->stringvars[i]->len=0;
   return prog->stringvars[i];
  }
 }
 return create_new_stringvar(prog);
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

// this will be run whenever the application exits for whatever reason
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
quit_cleanup(program *prog){
 id_info *quit_procs = prog->quit_procs;
 while( quit_procs ){
  if( quit_procs->t.data.pointer != 0 ){
   void (*quit_procedure)(program*) = (void*)quit_procs->t.data.pointer;
   quit_procedure(prog);
  }
  quit_procs = quit_procs->next;
 }
}

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
error(program *prog, int p, char *format, ...){
 va_list args;
 va_start(args, format);
 char s[256];
 vsprintf(s,format,args);
 // ------ if the program is catching an error ---------
 if( prog && prog->error_handler ){
  char *file;
  get_sourcetext_location_info( prog, p,  &prog->_error_line, &prog->_error_column, &file );
  copy_stringval_to_stringvar( prog, prog->_error_message, (stringval){s,strlen(s)} );
  copy_stringval_to_stringvar( prog, prog->_error_file, (stringval){file,strlen(file)} );
  longjmp(*prog->error_handler, 1);
 }
 // ------ default error response --------
 #ifdef enable_debugger
 dbg__quit( prog, p, reason_errored );
 #endif
 if( p > -1 ){
  print_sourcetext_location( prog, p );
 }
 fprintf(stderr, "%s\n",s);
 #ifdef enable_graphics_extension
 if(newbase_is_running){
  if( newbase_allow_fake_vsync ){
   if( ! isatty(STDIN_FILENO) ){
    // if this is a graphical program not running in a terminal (eg. launched from gui file manager)
    // then display the error message in the graphics window so the user knows something happened
    int l = strlen(s);
    graphicsDisplayErrorMessage:
    wmcloseaction = 0;
    SetPlottingMode(3);
    Gcol(0,0,128);
    RectangleFill(1,1,8*(l < 13 ? 13 : l), 25);
    Gcol(255,255,255);
    drawtext_(1,1,0, "Program error");
    drawtext_(1,9,0, s);
    drawtext_(1,18,0,"Click to exit");
    Refresh();
    while( mouse_b ) Wait(1);
    while( !mouse_b ){
     if( exposed ){
      exposed = 0; goto graphicsDisplayErrorMessage;
     }
     Wait(1);
    }
   }
  }
  Wait(1);
  MyCleanup();
 }
 #endif
 if( prog ) quit_cleanup(prog);
 exit(0); 
}

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
isvalue(TOKENTYPE_TYPE type){
 return ( type && (type <= VALUES_END) ) ;
}

char*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
C_CharacterAccess(int *p, program *prog){
 stringval svl; int index;
 svl = getstringvalue(prog,p);
 index = getvalue(p,prog);
 if(index<0 || index >= svl.len ){
  error(prog, *p-1, "C (string character / byte array access): index '%d' out of range",index);
 }//endif
 return svl.string + index;
}//endproc

double*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
V_ValueAccess(int *p, program *prog){
 stringval svl; int index;
 svl = getstringvalue(prog,p);
 index = getvalue(p,prog);
 if(index<0 || index >= (svl.len>>3)){
  error(prog, *p-1, "V (vector access): index '%d' out of range",index);
 }
 return ((double*)svl.string) + index;
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
 #ifdef enable_debugger
 dbg__tick_getvalue(prog, *p);
 #endif
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
  if( !b ) error(prog, *p, "modulo division by zero");
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
 case t_landf:
 {
  int jumpindex = t.data.i;
  while( isvalue( prog->tokens[*p].type ) ){
   if( !(int)getvalue(p,prog) ){
    *p = jumpindex;
    return 0;
   }
  }
  return 1;
 }
 case t_land:
 {
  int oldp = *p - 1;
  int out = (int)getvalue(p, prog); out = (int)getvalue(p, prog) && out; // '&&' takes at least two parameters
  double v;
  while( isvalue( prog->tokens[*p].type ) ){
   v=getvalue(p, prog);
   out = out && (int)v;
  }
  prog->tokens[oldp].type = t_landf;
  prog->tokens[oldp].data.i = *p;
  return (double)out;
 }
 case t_lorf:
 {
  int jumpindex = t.data.i;
  double v;
  while( isvalue( prog->tokens[*p].type ) ){
   if( (int)getvalue(p,prog) ){
    *p = jumpindex;
    return 1;
   }
  }
  return 0;
 }
 case t_lor:
 {
  int oldp = *p - 1;
  int out = (int)getvalue(p, prog); out = (int)getvalue(p, prog) || out; // '||' takes at least two parameters
  double v;
  while( isvalue( prog->tokens[*p].type ) ){
   v=getvalue(p, prog);
   out = out || (int)v;
  }
  prog->tokens[oldp].type = t_lorf;
  prog->tokens[oldp].data.i = *p;
  return (double)out;
 }
 case t_lnot:
 {
  int out=getvalue(p, prog);
  return (double)(!out);
 }

 case t_rnd:
 {
  int n = (int)getvalue(p,prog);
  unsigned int r = dhr_random_u32(_rnd_v++);
  switch(n){
   case 0:  return (double)(int)r;
   case 1:  return (double)r / 4294967296.0;
   default: return (double)((int)(r>>1) % n);
  }
 }

 case t_leftb:
 {
  double out = getvalue(p, prog);
  if( prog->tokens[*p].type != t_rightb ){
   error(prog, *p-1, "expected closing bracket");
  }
  *p += 1;
  return out;
 }

 case t_D:
 {
  int index = getvalue(p, prog);
  if( index>=0 && index<prog->vsize) return prog->vars[ index ];
  error(prog, *p-1, "getvalue: bad variable access '%d'",index);
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
  error(prog, *p-1, "getvalue: bad array access '%d'",index);
  break;
 }
/*
 // This is unused...
 case t_Af:
 { 
  int index = t.data.i + (int)getvalue(p,prog);
  if( index>=0 && index<prog->vsize) return prog->vars[ index ];
  error(prog, *p-1, "getvalue: bad array access");
  break; 
 }
*/
 case t_C:
 {
  return (double)(unsigned char)*C_CharacterAccess(p, prog);
  break;
 }
 case t_V:
 {
  return *V_ValueAccess(p, prog);
  break;
 }
 case t_P:
 {
  int index = prog->sp + prog->current_function->num_locals + (int)getvalue(p,prog);
  if(index<0 || index>=prog->ssize) error(prog, *p-1, "getvalue: bad parameter access");
  return prog->stack[index];
 }
 case t_L:
 {
  int index = prog->sp + (int)getvalue(p,prog);
  if(index<0 || index>=prog->ssize) error(prog, *p-1, "getvalue: bad local variable access");
  return prog->stack[index];
 }
 case t_stackaccess:
 {
  int index = prog->sp + t.data.i;
  //if(index<0 || index>=prog->ssize) error(prog, *p-1, "getvalue: bad stack access");
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
 case t_vlenS:
 {
  stringval sv = getstringvalue(prog,p);
  return (double)(sv.len>>3);
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
 case t_equalS:
 {
  stringval sv1 = getstringvalue(prog,p);
  prog->getstringvalue_level += 1;
  stringval sv2 = getstringvalue(prog,p);
  prog->getstringvalue_level -= 1;
  return ( (sv1.len==sv2.len)
            &&
           (sv1.string[sv1.len-1] == sv2.string[sv2.len-1])
            &&
           (*sv1.string == *sv2.string)
            &&
           (!strncmp( sv1.string+1, sv2.string+1, sv1.len-2)) );
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
  stringvar *svr = obtain_new_stringvar(prog);
  if( determine_valueorstringvalue(prog, *p, 0) == determined_stringvalue ){
   copy_stringval_to_stringvar(prog, svr, getstringvalue(prog, p) );
  }
  return svr->string_variable_number;
 }
 case t_extfun:
 {
  return (( double(*)(int*,program*) )t.data.pointer)(p,prog);
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
 case  t_log2:	
  return log2(getvalue(p,prog));
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
    stringvar *referred_string_constant = obtain_new_stringvar(prog);
    copy_stringval_to_stringvar(prog, referred_string_constant, getstringvalue(prog, p) );
    // replace the getref token with a referredstringconst
    prog->tokens[*p-2].type = t_referredstringconst;
    prog->tokens[*p-2].data.i = referred_string_constant->string_variable_number;
    // return the reference number of the new string variable
    return referred_string_constant->string_variable_number;
   }
  default: error(prog, *p, "getvalue: bad use of @");
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
   fnum = getvalue(p,prog);
   if( (fnum<0 || fnum>(MAX_FUNCS-1)) || !prog->functions[fnum] ){
    error(prog, *p, "getvalue: F: no such function numbered '%d'",fnum);
   }
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
   if( newsp + minimum_stack_required > prog->ssize ) error(prog, *p, "ran out of stack");
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
     if( spp >= prog->ssize ) error(prog, *p, "ran out of stack");
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
 case t_error_line:	return prog->_error_line;
 case t_error_column:	return prog->_error_column;
 //case t_error_number:	return prog->_error_number;
 // =======================================================================
 // ======= EVAL ==========================================================
 // =======================================================================
 case t_eval: case t_evalexpr: {
  double v;
  stringval s = getstringvalue(prog,p);
  int h = s.string[s.len]; s.string[s.len]=0;
  int result = interpreter_eval( prog, t.type==t_evalexpr ? eval_getvalue : eval_interpreter, &v, s.string );
  s.string[s.len]=h;
  if( result ) error( prog, *p, prog->_error_message->string );
  return v;
 }
 // ==================================================================================
 // ======= 'pseudo values' ==========================================================
 // ==================================================================================
 // Although isvalue() will return 0 for these tokens, getvalue() will still operate on them. These are known as 'pseudo values'.
 // This allows for things like checking the return value of the external command called by 'oscli',
 // or checking if catch caught an error
 case t_oscli: // although t_oscli is a command, and not a value according to isvalue(), enable getvalue to operate on it so we can get the retun value of system()
  return oscli(prog,p);
 case t_catch: // process an unprocessed 'catch'
  process_catch(prog,*p-1);
  t = prog->tokens[ *p - 1 ];
  // deliberately run through
 case t_catchf: { // catch returns 1 if an error was caught, 0 otherwise
  int catchretval = catch( prog, p, 0 /*action: call interpreter*/ , NULL, NULL);
  *p = t.data.i;
  return catchretval;
 }
 default:
  error(prog, *p-1, "getvalue: expected a value, instead found '%s'",tokenstring(t));
 }//endswitch t.type
}//endproc getvalue

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

// ===================================
// == CATCH, AND EXCEPTION HANDLING ==
// ===================================

#define catch_magicnumber 11673314021918612911030223021049252244204987641215237006073991731396480243030832901810606149921138073986225596601835736011927752855882887282164819871339110857468686545366091027521562266553425728565376521514336124928.000000
#define catch__call_interpreter		0
#define catch__call_getvalue		1
#define catch__call_getstringvalue	2

void
__attribute__((noinline))
process_catch(program *prog, int p){
 int pp=p;
 int level=1;
 while(level && pp<prog->length){
  pp += 1;
  switch( prog->tokens[pp].type ){
   case t_catch:    level += 1; break;
   case t_endcatch: level -= 1; break;
   case t_deffn:    level = 0;
  }
 }
 if(prog->tokens[pp].type != t_endcatch){
  error(prog, p, "no 'endcatch' for this 'catch'");
 }
 prog->tokens[p].type = t_catchf;
 prog->tokens[p].data.i = pp+1;
}


typedef
struct {
 jmp_buf *hold_error_handler;
 func_info *hold_current_function;
 int hold_getstringvalue_level;
 int hold_sp;
 int hold_spo;
}
saved_interpreter_state;

void
__attribute__((noinline))
interpreter_save_state( program *prog, saved_interpreter_state *state ){
 state->hold_error_handler = prog->error_handler;
 state->hold_current_function = prog->current_function;
 state->hold_getstringvalue_level = prog->getstringvalue_level;
 state->hold_sp = prog->sp;
 state->hold_spo = prog->spo;
}

void
__attribute__((noinline))
interpreter_load_state( program *prog, saved_interpreter_state *state ){
 prog->error_handler = state->hold_error_handler;
 prog->current_function = state->hold_current_function;
 prog->getstringvalue_level = state->hold_getstringvalue_level;
 prog->sp = state->hold_sp;
 prog->spo = state->hold_spo;
}

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
__attribute__((noinline))
catch( program *prog, int *p, int action, double *returnValue, stringval *returnStringValue ){

 // save current interpreter state information
 saved_interpreter_state state;
 interpreter_save_state( prog, &state );
 
 // prepare setjmp
 prog->error_handler = malloc(sizeof(jmp_buf));
 int setjmp_retval = setjmp( *prog->error_handler );

 // perform the requested action, saving the return value if successful
 double catch_returnValue;
 stringval catch_returnString = (stringval){"PENIS",0};
 if( ! setjmp_retval ){
  switch( action ){
   case catch__call_interpreter:
    catch_returnValue = interpreter(*p,prog );
    break;
   case catch__call_getvalue:
    catch_returnValue = getvalue(p,prog);
    break;
   case catch__call_getstringvalue:
    catch_returnString = getstringvalue(prog,p);
    if( returnStringValue ) *returnStringValue = catch_returnString;
    break;
  }
  if( returnValue ) *returnValue = catch_returnValue;
 }

 // tidy away the setjmp
 free( prog->error_handler );
 // and restore the previous interpreter state information
 interpreter_load_state( prog, &state );

 // return value is 0 if no error was caught, and 1 if an error was caught
 return setjmp_retval;
}

// ===========
// == OSCLI ==
// ===========

int oscli(program *prog, int *p){
 stringval sv = getstringvalue( prog, p );
 char holdthis = sv.string[sv.len]; sv.string[sv.len]=0;
 //fprintf(stderr,"oscli: '%s'\n",sv.string);
 int SystemReturnValue;
 SystemReturnValue = system(sv.string);
 sv.string[sv.len]=holdthis;
 return SystemReturnValue;
}

// ==========
// == EVAL ==
// ==========

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
__attribute__ ((noinline))
interpreter_eval( program *prog, int action, void *return_value, unsigned char *text ){ //fprintf(stderr, "prog->maxlen	%d	%d\n",prog->maxlen,prog->length);
 int setjmp_retval; int tokens_length; int starting_p;
 // . save some existing data to be restored later
 stringslist *tail = stringslist_gettail( prog->program_strings );
 //TTP hold_ttp = *(TTP*)prog->program_strings->string;
 saved_interpreter_state state; interpreter_save_state( prog, &state );
 int hold_length = prog->length;
 // prepare error handler
 prog->error_handler = malloc(sizeof(jmp_buf));
 // catch errors
 setjmp_retval = setjmp( *prog->error_handler );
 // here: ---------------------------------------------------------------------------------
 //  . get program text string
 //  . process it
 //  . append tokens
 if( ! setjmp_retval ){
  size_t len = strlen( text );
  tokens_length = gettokens(prog,NULL,len,text,NULL);
  //fprintf(stderr, "s.... %d, %d		%d\n",prog->length,tokens_length,prog->maxlen);
  // handle the scenario where no tokens are generated
  if( !tokens_length ){
   goto text_processing_error_exit;
  }
  // make space for new tokens if necessary
  if( prog->maxlen < prog->length + tokens_length + 2  ){
   prog->maxlen = prog->length + tokens_length + 2;
   prog->tokens = realloc( prog->tokens, prog->maxlen*sizeof(token) );
   if( ! prog->tokens ){ // if realloc failed, fatal error
    fprintf(stderr, "eval: fatal error: realloc failed when making space for new tokens\n");
    exit(1);
   }
  }
  // append tokens
  starting_p = prog->length+1;
  gettokens(prog,prog->tokens + (prog->length+1),len,text,NULL);
  prog->length += tokens_length+1;
  prog->tokens[prog->length].type = t_nul;
  // cement
  #ifdef enable_debugger
  dbg__add_tokens(prog, text);
  #endif
 }else{
  // there was a problem when processing the text...
  goto text_processing_error_exit;
 }
 // here: ---------------------------------------------------------------------------------
 //  . execute interpreter
 // catch errors
 setjmp_retval = setjmp( *prog->error_handler );
 if( ! setjmp_retval ){
  switch( action ){
   case eval_interpreter: {
    double v = interpreter( starting_p, prog );
    if( return_value ) *(double*)return_value = v;
   } break;
   case eval_getvalue: {
    int p = starting_p;
    double v = getvalue( &p, prog );
    if( return_value ) *(double*)return_value = v;
   } break;
   case eval_getstringvalue: {
    int p = starting_p;
    stringval v = getstringvalue( prog, &p );
    if( return_value ) *(stringval*)return_value = v;
   } break;
  }
 }
 // here: ---------------------------------------------------------------------------------
 //  . clean up and return
 // restore program original length
 prog->length = hold_length;
 #ifdef enable_debugger
 dbg__remove_tokens(prog);
 #endif
 // exit from here if there was a text processing error...
 text_processing_error_exit:
 // free error handler
 free( prog->error_handler );
 // restore state
 interpreter_load_state( prog, &state );
 // free program strings created for this eval
 free_stringslist( tail->next );
 tail->next = NULL;
 // restore TTP state
 //*(TTP*)prog->program_strings->string = hold_ttp;
 // the return value is whether or not there was an error
 return setjmp_retval;
}

// ===================================
// == INTERACTIVE PROMPT =============
// ===================================

#ifdef ENABLE_HELP
 #include "help.c"
#endif


// regrettably, I can't find a nice way of avoiding the use of global variables for this part of the interpreter
int prompt__len;
program *prompt__prog;
void prompt_sigint_handler(int signum){
 fprintf(stderr, "Ctrl+C pressed (SIGINT)\n");
 for(int i=prompt__len; i<=prompt__prog->length; i++){
  prompt__prog->tokens[i].type=t_nul;
 }
}

void
__attribute__ ((noinline))
interactive_prompt(program *prog){
 prompt__prog = prog;
 fprintf(stderr, "Johnsonscript prompt\n");
 fprintf(stderr, " Type 'calc mode' for expression evaluator mode,\n");
 fprintf(stderr, " 'string mode' for string expression evaluator mode,\n");
 fprintf(stderr, " or 'normal mode' for command/script evaluator mode. (default)\n");
 #ifdef ENABLE_HELP
 fprintf(stderr, " Type 'help' or 'help [keyword]' for help information.\n");
 #endif
 fprintf(stderr, " Type exit to leave the prompt\n");
 const int bufsize = 1024*1024;
 char *buf = calloc(1,bufsize+1);
 int mode = eval_interpreter;
 int len = prog->length; prompt__len = len;
 while(1){
  signal(SIGINT, SIG_DFL); // restore default behaviour so that Ctrl+C quits
  printf("> ");
  char *retval = fgets(buf, bufsize, stdin);
  if( feof(stdin) ){
   fprintf( stderr, "\n" );
   return;
  }
  if( !strcmp( "normal mode\n", buf ) ){
   fprintf(stderr, "Entering command/script evaluator mode\n");
   mode = eval_interpreter;
  }else if( !strcmp( "calc mode\n", buf ) ){
   fprintf(stderr, "Entering numeric expression evaluator mode\n");
   mode = eval_getvalue;
  }else if( !strcmp( "string mode\n", buf ) ){
   fprintf(stderr, "Entering string expression evaluator mode\n");
   mode = eval_getstringvalue;
  }else if( !strcmp( "exit\n", buf ) ){
   fprintf(stderr, "Exiting prompt\n");
   return;
#ifdef ENABLE_HELP
  }else if( !strcmp( "help\n", buf ) || !strncmp( "help ", buf, 5 ) ){
   interactive_help( buf );
   //fprintf( stderr, "do help stuff here\n" );
#endif
  }else {
   signal(SIGINT, prompt_sigint_handler); // set our handler so that Ctrl+C can break out of loops and stuff
   switch( mode ){
    case eval_interpreter: {
     if( interpreter_eval( prog, mode, NULL, buf ) ){
      fprintf( stderr, "%s\n", prog->_error_message->string );
     }
    } break;
    case eval_getvalue: {
     double v;
     if( interpreter_eval( prog, mode, &v, buf ) )
      fprintf( stderr, "%s\n", prog->_error_message->string );
     else{
      if( !MyIsnan(v) && (long long int)v == v )
       fprintf( stderr, "%lld\n", (long long int)v );
      else
       fprintf( stderr, "%.16f\n", v );
     }
    } break;
    case eval_getstringvalue: {
     stringval v;
     if( interpreter_eval( prog, mode, &v, buf ) )
      fprintf( stderr, "%s\n", prog->_error_message->string );
     else {
      for( int i=0; i<v.len; i++ ) fprintf(stderr, "%c", v.string[i]);
      fprintf( stderr, "\n" );
     }
    } break;
   }
  }
 }
}

// ===================================
// == IF SEARCH ======================
// ===================================

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
_interpreter_ifsearch(int p, program *prog){
 int starting_p = p;
 int levelcount=1;
 while(levelcount){
  if(p>=prog->length || prog->tokens[p].type == t_deffn) error(prog, starting_p, "missing endif or otherwise bad ifs");
  switch(prog->tokens[p].type){
  case t_else:
   { // catch extraneous 'else'
    int pp = p+1;
    while( pp < prog->length ){
     if( prog->tokens[pp].type == t_else ){
      error( prog, pp, "extraneous 'else'" );
     }else if( prog->tokens[pp].type == t_if || prog->tokens[pp].type == t_endif || prog->tokens[pp].type == t_deffn ){
      break;
     }
     pp += 1;
    }
   }
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

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
_interpreter_labelsearch(program *prog,char *labelstring, int labelstringlen){
 int i;
 for(i=0; i<prog->length; i++){
  if( prog->tokens[i].type == t_label && !strncmp((char*)prog->tokens[i].data.pointer, labelstring, labelstringlen)
       && !( ((char*)prog->tokens[i].data.pointer)[labelstringlen] ) // Äkta dig för Rövar-Albin
    ){
   return i+1;
  }//endif
 }//next
 error(prog, -1, "goto: couldn't find label '%s'",labelstring);
 return 0;
}//endproc

struct caseof {
 int num_whens;
 int *whens; // position of 'whens'
 int *whens_; // position of first ';' after each 'when'
 int otherwise; //position of otherwise (this will point to the 'endcase' if there was no otherwise'
};

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
caseof_skippast(program *prog, int p){ // p must point to the starting 'caseof'
 int i = p;
 int level = 1; // 'level'
 while( (i < prog->length) && level){
  i+=1;
  switch( prog->tokens[i].type ){
  case t_caseof: level += 1; break;
  case t_endcase: level -= 1; break;
  }
 }
 if(i == prog->length) error(prog, p, "caseof_skippast: missing endcase");
 return i;
}
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
caseof_numwhens(program *prog, int p,struct caseof *co){ // p must point to the starting 'caseof'
 int out=0;
 int i = p+1;
 while( i < prog->length ){
  switch( prog->tokens[i].type ){
  case 0:
   error(prog, p, "caseof_numwhens: missing endcase");
   break;
  case t_caseof:
   i = caseof_skippast( prog, i);
   break;
  case t_when:
   if(co){
    co->whens[out] = i;
    if( out>0 && !co->whens_[out-1] ){
     error(prog, i, "caseof_numwhens:1: 'when' list not terminated by ';'"); // this can't always be caught but at least in some cases it'll be nice to be notified by this error message
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
     error(prog, i, "caseof_numwhens:2: 'when' list not terminated by ';'");
    }
   }
   break;
  case t_endcase:
   return out;
  }
  i+=1;
 }
 error(prog, -1, "caseof_numwhens: this should never happen");
}
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
determine_valueorstringvalue(program *prog, int p, int errorIfNeither){// this returns 0 for a value, 1 for a stringvalue, -1 for neither OR causes an error if neither is found
 int i; i=p; while( prog->tokens[i].type == t_leftb ){ i+=1; } // skip past any left brackets
 if( prog->tokens[i].type == t_id ) process_id(prog, &prog->tokens[i]); // process it if it's an id
 if( isstringvalue(prog->tokens[i].type) ) return 1; // return 1 if it's a stringvalue
 if( isvalue(prog->tokens[i].type) ) return 0; // return 0 if it's a value
 if( errorIfNeither ) error(prog, p, "expected a value or a stringvalue"); // cause an error because we expected a value or a stringvalue
 return -1;
}
#define PROCESSCASEOF_RUBBISH 0
void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
_interpreter_processcaseof(program *prog, int p){ int i;
//  create caseof struct.
 struct caseof *co = calloc(1, sizeof(struct caseof));
 int caseofpos = p;
 int endcasepos = caseof_skippast(prog,p);
 #if PROCESSCASEOF_RUBBISH
 fprintf(stderr,"fuckk %s\n",tokenstring(prog->tokens[ endcasepos ]));
 #endif
//  determine if this is a caseofV or caseofS, or cause an error if there's an inappropriate token after the caseof.
//  replace the caseof with caseofV_f or caseofS_f appropriately.
 prog->tokens[caseofpos].type = determine_valueorstringvalue(prog,p+1,1) ? t_caseofS : t_caseofV;
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
  fprintf(stderr,"ffuck %0d: %d(%s), %d(%s)\n", i, co->whens[i],tokenstring(prog->tokens[co->whens[i]]), co->whens_[i],tokenstring(prog->tokens[co->whens_[i]])  );
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
  fprintf(stderr,"ffuck %0d: %d(%s), %d(%s)\n", i, co->whens[i],tokenstring(prog->tokens[co->whens[i]]), co->whens_[i],tokenstring(prog->tokens[co->whens_[i]])  );
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

// FOR

int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
expressionIsSimple(program *prog, int p){
 if( prog->tokens[ p ].type == t_number ){ 
  return 1;
 }
 int parenLevel = 0;
 while( isvalue( prog->tokens[p].type ) || (prog->tokens[p].type==t_rightb && (parenLevel>0))  ){ 
  switch( prog->tokens[ p ].type ){
  case t_leftb:  parenLevel++; break;
  case t_rightb: parenLevel--; break;
  case t_D: case t_A: case t_L: case t_P: case t_F: case t_Ff: case t_Df: case t_SS: case t_Af: case t_stackaccess: case t_getref: case t_S: case t_Sf: case t_C: case t_V:
  case t_ascS: case t_valS: case t_lenS: case t_cmpS: case t_instrS: case t_rnd: case t_alloc:
  case t_openin: case t_openout: case t_openup: case t_eof: case t_bget: case t_vget: case t_ptr: case t_ext:
  case t_extfun:
  case t_evalexpr: case t_eval: case t_equalS:
  #ifdef enable_graphics_extension
  case t_winw: case t_winh: case t_mousex: case t_mousey: case t_mousez: case t_readkey: case t_keypressed: case t_expose: case t_wmclose: case t_keybuffer:
  #endif
   return 0;
  case t_id:
   // peek the id, if it's just a number constant then that's okay, otherwise return 0
   {
    id_info *thisIdInfo = peek_id(prog, (char*)prog->tokens[p].data.pointer );
    if( ! thisIdInfo ) return 0;
    token t = thisIdInfo->t;
    if( t.type != t_number ){
     return 0;
    }
   }
   break;
  }
  p += 1;
 }
 return 1;
}

typedef struct {
 int type; // 0: t_Df   1: t_stackaccess
 double *v; int stackOffset; // either the pointer to the variable (for t_Df) or offset for the variable on the stack (for t_stackaccess)
 int fromValIsConstant; double fromVal; int fromValExpressionP; // from value
 int toValIsConstant;   double toVal;   int toValExpressionP;   // to value
 int stepValIsConstant; double stepVal; int stepValExpressionP; // step value
 int start_p, end_p; // start and end positions of the loop body
} forinfo;

#if 0
void debugforinfo( program *prog, forinfo *thisfor ){
 fprintf(stderr,"thisfor:\n type:%d\n v:%p\n stackOffset:%d\n fromValIsConstant:%d\n fromVal:%f\n fromValExpressionP:%d\n toValIsConstant:%d\n toVal:%f\n toValExpressionP:%d\n stepValIsConstant:%d\n stepVal:%f\n stepValExpressionP:%d\n start_p:%d\n end_p:%d\n",
thisfor->type,
thisfor->v,
thisfor->stackOffset,
thisfor->fromValIsConstant,
thisfor->fromVal,
thisfor->fromValExpressionP,
thisfor->toValIsConstant,
thisfor->toVal,
thisfor->toValExpressionP,
thisfor->stepValIsConstant,
thisfor->stepVal,
thisfor->stepValExpressionP,
thisfor->start_p,
thisfor->end_p
);
}
#endif

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
_interpreter_processfor(program *prog, int p,  double *fromVal, double *toVal, double *stepVal ){
 // johnsonscript implementation of the classic 'for' loop.
 /*
  variable i; # the loop variable must already exist, either as a global variable made with the 'variable' command or as a local variable in a function
  # example of a 'for' without explicit step (in which case it defaults to 1)
  # this will print the numbers 1 2 3 4 
  for i 1 4;
   print "test one: " i;
  endfor
  print
  # example of a 'for' with step specified
  # this will print the numbers 1 3 5 7
  for i 1 7 2;
   print "test two: " i;
  endfor
  quit
 */
 
 forinfo *thisfor = calloc(1,sizeof(forinfo));
 stringslist_addstring(prog->program_strings, (char *) thisfor );
 
 int for_p = p; // location of the 'for' token itself
 int endfor_p=0; // location of the 'endfor' corresponding to this 'for'

 if( prog->tokens[p+1].type != t_id ){
  error(prog, for_p, "for: not given an ID for a loop variable");
 }
 p += 1;
 process_id(prog, &prog->tokens[p] );
 switch( prog->tokens[p].type ){
  case t_Df: {
   thisfor->type = 0;
   thisfor->v = (double*)prog->tokens[p].data.pointer;
  }
  break;
  case t_stackaccess: {
   thisfor->type = 1;
   thisfor->stackOffset = prog->tokens[p].data.i;
  }
  break;
  default: error(prog, for_p, "for: not given a loop variable");
 }
 p += 1;
 
 int start_p=0,end_p=0;

 int forlevel=0;
 while(!endfor_p){
  p+=1;
  switch(prog->tokens[p].type){
   case 0: {
    error(prog, for_p, "no matching 'endfor' for this 'for'");
   } break;
   case t_endfor: {
    if( !forlevel ){
     endfor_p = p; 
    }else{
     forlevel -= 1;
    }
   } break;
   case t_for: {
    forlevel += 1;
   } break;
  }
 }

 end_p = endfor_p+1;

 p = for_p + 2;

 double for_processParam( int *constflag, int *exp_p, double *val ){
  double result;
  if( expressionIsSimple(prog,p) ){
   *constflag = 1;
   result = getvalue(&p,prog); 
   *val = result;
  }else{
   *constflag = 0;
   *exp_p = p;
   result = getvalue(&p,prog);
  }
  return result;
 } 

 // ---- FROM -------
 *fromVal = for_processParam( &thisfor->fromValIsConstant, &thisfor->fromValExpressionP, &thisfor->fromVal );
 // ---- TO ---------
 *toVal = for_processParam( &thisfor->toValIsConstant, &thisfor->toValExpressionP, &thisfor->toVal );
 // ---- STEP -------
 if( determine_valueorstringvalue( prog, p, 0 ) == determined_neither &&  prog->tokens[p].type != t_endstatement ){
  error( prog, p, "for parameters list must be terminated with ';'" );
 }
 if( prog->tokens[p].type == t_endstatement ){
  thisfor->stepValIsConstant = 1;
  thisfor->stepVal = 1;
  *stepVal = 1;
  start_p = p+1;
 }else{
 
  *stepVal = for_processParam( &thisfor->stepValIsConstant, &thisfor->stepValExpressionP, &thisfor->stepVal );

  if( prog->tokens[p].type != t_endstatement ){
   error(prog, p, "for parameters list must be terminated with ';'");
  }

  start_p = p+1;

 }
 
 thisfor->start_p = start_p;
 thisfor->end_p = end_p;

 prog->tokens[for_p].type = t_forf;
 prog->tokens[for_p].data.pointer = (void*)thisfor;
 prog->tokens[endfor_p].type = t_endforf;
 prog->tokens[endfor_p].data.pointer = (void*)thisfor;
 
 p = for_p;

 //debugforinfo(prog, thisfor);
}

double*
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
interpreter_for_getLoopVarPtr(program *prog, forinfo *thisfor){
 // get a pointer to the loop variable
 double *v; 
 if( thisfor->type ){
  v = &prog->stack[prog->sp + thisfor->stackOffset];
 }else{
  v = thisfor->v;
 }
 return v;
}

double
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
interpreter_for_getToVal(program *prog, forinfo *thisfor){
 // check the "to value".
 double toVal;
 if( thisfor->toValIsConstant ){
  toVal = thisfor->toVal;
 }else{
  int pp = thisfor->toValExpressionP;
  toVal = getvalue(&pp, prog);
 }
 return toVal;
}

double
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
interpreter_for_getStepVal(program *prog, forinfo *thisfor){
 // check the "step value"
 double stepVal;
 if( thisfor->stepValIsConstant ){
  stepVal = thisfor->stepVal;
 }else{
  int pp = thisfor->stepValExpressionP;
  //debugforinfo(prog, thisfor);
  stepVal = getvalue(&pp, prog);
  if( stepVal == 0 ){
   error(prog, pp, "for: step can't be 0");
  }
 }
 return stepVal;
}

// direction >= 0 will find the end of the loop, direction < 0 will find the start.
// returns -1 if no start or end of a loop was found
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
__attribute__ ((noinline))
findStartOrEndOfCurrentLoop(program *prog, int p, int direction){
 direction = direction<0 ? -1 : 1;
 int level = 1;
 while( level ){
  p += direction;
  if( p<0 || p>=prog->length ) return -1;
  switch( prog->tokens[p].type ){
   case t_deffn:
    return -1;
   case t_while: case t_whilef: case t_whileff: case t_for: case t_forf: {
    level += direction;
   } break;
   case t_endwhile: case t_endwhilef: case t_endwhileff: case t_endfor: case t_endforf: {
    level -= direction;
   } break;
  }
 }
 return p;
}

// ========================================================
// =========  INTERPRETER  ================================
// ========================================================

double
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
interpreter(int p, program *prog){
 token t;
 interpreter_start:
 #ifdef enable_debugger
 dbg__tick_interpreter(prog, p);
 #endif
 t = prog->tokens[p];
 switch( t.type ){
 case t_return:
 {
  p+=1;
  return getvalue(&p, prog);
 }
 case t_endwhilef:
 {
  int pp = t.data.i;
  if( getvalue(&pp,prog) ){
   p = pp; goto interpreter_start;
  }
  p+=1;
  break;
 } 
 case t_whilef:
 { // fast while/endwhile
  p+=1;
  if( !getvalue(&p, prog) ){
   p=t.data.i;
  }
  break;
 }
 case t_gotof: case t_whileff: case t_endwhileff:
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
   if(p>=prog->length) error(prog, s_pos, "missing 'endwhile'");
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
  prog->tokens[e_pos].data.i = s_pos+1;
  //go back to the position of the while so it can be executed as normal
  p=s_pos;
  // if this is while loop is something similar to 'while 1' or 'while 0',
  // then we can just turn it into a jump that doesn't waste time checking a constant value very loop
  if( prog->tokens[s_pos+1].type == t_id ) process_id(prog,&prog->tokens[s_pos+1]);
  if( prog->tokens[s_pos+1].type == t_number ){
   if( prog->tokens[s_pos+1].data.number ){ // true
    int loopBodyStart = s_pos+2+(prog->tokens[s_pos+2].type==t_endstatement);
    prog->tokens[s_pos].type = t_whileff;   // fix 'while'
    prog->tokens[s_pos].data.i = loopBodyStart; // make it jump straight to the start of the loop body
    //prog->tokens[s_pos+1].type = t_endstatement; // blank out the value (unnecessary)
    prog->tokens[e_pos].type = t_endwhileff; // fix 'endwhile'
    prog->tokens[e_pos].data.i = loopBodyStart; // make it jump straight to the start of the loop body too
   }else{ // false
    prog->tokens[s_pos].type = t_whileff;  // fix 'while'
    prog->tokens[s_pos].data.i = e_pos+1; // make it jump past the loop
    prog->tokens[e_pos].type = t_endwhileff;   // fix 'endwhile'
    prog->tokens[e_pos].data.i = e_pos+1; // make it jump past the loop too
   }
  }
  break;
 }
 case t_endwhile:
 {
  int starting_p = p;
  int levelcount=1;
  while(levelcount){
   p-=1;
   if(p<0) error(prog, starting_p, "can't find 'while' for this 'endwhile'");
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
  //if( prog->tokens[ifpos].type != t_if ) error(NULL,-1,"blah1\n"); 
  //if( elsepos>=0 && prog->tokens[elsepos].type != t_else ) error(NULL,-1,"blah2\n"); 
  //if( prog->tokens[endifpos].type != t_endif ) error(NULL,-1,"blah3\n"); 
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
 // ========================================================
 // =========  FOR  ========================================
 // ========================================================
 // t_for: encountering a new for
 case t_for: {
  double fromVal, toVal, stepVal, *v;
  _interpreter_processfor(prog, p, &fromVal, &toVal, &stepVal );
  forinfo *thisfor = (forinfo*)prog->tokens[p].data.pointer;
  v = interpreter_for_getLoopVarPtr(prog, thisfor);
  //fprintf(stderr,"%p\n",v);
  *v = fromVal;
  if( stepVal>0 ? (*v>toVal) : (*v<toVal) ){
   // condition already met, skip past the loop body
   p = thisfor->end_p;
  }else{
   p = thisfor->start_p;
  }
 } break;
 // t_forf: working on a fully processed for
 case t_forf: { // entering a for loop.
  forinfo *thisfor = (forinfo*)t.data.pointer;
  // get a pointer to the loop variable
  double *v = interpreter_for_getLoopVarPtr(prog, thisfor);
  // initialise the loop variable
  if( thisfor->fromValIsConstant ){
   *v = thisfor->fromVal;
  }else{
   int pp = thisfor->fromValExpressionP;
   *v = getvalue(&pp, prog);
  }
  // check the "to value".
  double toVal = interpreter_for_getToVal(prog, thisfor);
  // check the "step value"
  double stepVal = interpreter_for_getStepVal(prog, thisfor);
  // Check the condition. We won't execute the loop body at all if the stop condition is already met
  if( stepVal>0 ? (*v>toVal) : (*v<toVal) ){
   // condition already met, skip past the loop body
   p = thisfor->end_p;
  }else{
   p = thisfor->start_p;
  }
 } break;
 // t_endforf: working on a fully processed for
 case t_endforf: { // encountering the end of the for loop, advancing the counter variable, checking the condition, and either continuing or exiting
  forinfo *thisfor = (forinfo*)t.data.pointer;
  // get a pointer to the loop variable
  double *v = interpreter_for_getLoopVarPtr(prog, thisfor);
  // check the "to value".
  double toVal = interpreter_for_getToVal(prog, thisfor);
  // check the "step value"
  double stepVal = interpreter_for_getStepVal(prog, thisfor);
  // Advance
  *v += stepVal;
  // Check the condition
  if( stepVal>0 ? (*v>toVal) : (*v<toVal) ){
   // condition met, skip past the 'endfor'
   p+=1;
  }else{
   p = thisfor->start_p;
  }
 } break;
 // t_endfor: encountering an unprocessed 'endfor'
 case t_endfor: {
  int pp = findStartOrEndOfCurrentLoop(prog, p, -1);
  if(pp==-1) error(prog,p,"no 'for' for this 'endfor'");
  double blah1,blah2,blah3;
  _interpreter_processfor(prog, pp, &blah1, &blah2, &blah3);
 }
 break;
 // ========================================================
 // =========  ENDLOOP, CONTINUE, RESTART  =================
 // ========================================================
 // Commands managing loop control flow, similar in concept to 'break' and 'continue' in C
 //findStartOrEndOfCurrentLoop(program *prog, int p, int direction)
 case t_endloop: case t_continue: case t_restart: {
  int op = p;
  int direction = t.type==t_restart ? -1 : 1;
  p+=1;
  int levelIsGiven = isvalue( prog->tokens[p].type );
  int levelIsConstant = expressionIsSimple( prog, p );
  int level = levelIsGiven ? getvalue(&p,prog) : 1;
  int i;
  p = op;
  for(i=0; i<level; i++){
   p = findStartOrEndOfCurrentLoop(prog, p, direction);
  }
  if(p==-1){
   char *msg=NULL;
   switch( t.type ){
    case t_endloop:
     msg = (level==1 ? "endloop: not in a loop\n" : "endloop: couldn't find loop level specified"); break;
    case t_continue:
     msg = (level==1 ? "continue: not in a loop\n" : "continue: couldn't find loop level specified"); break;
    case t_restart:
     msg = (level==1 ? "restart: not in a loop\n" : "restart: couldn't find loop level specified"); break;
   }
   error(prog, op, msg);
  }
  if(t.type == t_endloop) p+=1;
  if(levelIsConstant){
   prog->tokens[op].type = t_gotof;
   prog->tokens[op].data.i = p;
  }
 } break;
 // ========================================================
 // =========  SET  ========================================
 // ========================================================
 // fast 'set'
 case t_set_Df: p+=2; *(double*)t.data.pointer = getvalue(&p,prog); break;
 case t_set_stackaccess: p+=2; prog->stack[prog->sp+t.data.i] = getvalue(&p,prog); break;
 // normal 'set'
 case t_set: t_set_start:
 {
  t=prog->tokens[p+1]; p+=2;
  int index;
  switch( t.type ){
  case t_D:
  {
   index = (int)getvalue(&p, prog);
   //fprintf(stderr,"  set index: %d\n",index);
   if(index<0 || index>=prog->vsize) error(prog, p-1, "set: bad variable access '%d'",index);
   prog->vars[index] = getvalue(&p, prog);
   break;
  }
  case t_Df:
  {
   // fix
   prog->tokens[p-2].type = t_set_Df;
   prog->tokens[p-2].data.pointer = (void*) t.data.pointer;
   // perform set
   *(double*)t.data.pointer = getvalue(&p, prog);
   break;
  }
  case t_A:
   index = (int)getvalue(&p,prog) + (int)getvalue(&p,prog);
   if(index<0 || index>=prog->vsize) error(prog, p-1, "set: bad array access '%d'",index);
   prog->vars[index] = getvalue(&p, prog);
   break;
  case t_C:
   {
    char *characterpointer = C_CharacterAccess( &p, prog );
    *characterpointer = (char) getvalue(&p,prog);
   }
   break;
  case t_V:
   {
    double *valuepointer = V_ValueAccess( &p, prog);
    *valuepointer = getvalue(&p,prog);
   }
   break;
  case t_P:
   index = prog->sp + prog->current_function->num_locals + (int)getvalue(&p,prog);
   if(index<0 || index>=prog->ssize) error(prog, p-1, "set: bad parameter access");
   prog->stack[index] = getvalue(&p, prog);
   break;
  case t_L:
   index = prog->sp + (int)getvalue(&p,prog);
   if(index<0 || index>=prog->ssize) error(prog, p-1, "set: bad parameter access");
   prog->stack[index] = getvalue(&p, prog);
   break;
  case t_stackaccess:
   // fix
   prog->tokens[p-2].type = t_set_stackaccess;
   prog->tokens[p-2].data.i = t.data.i;
   // perform set
   index = prog->sp + t.data.i;
   //if(index<0 || index>=prog->ssize) error(prog, "interpreter: set: bad stack access");
   prog->stack[index] = getvalue(&p, prog);
   break;
  case t_id:
   process_id(prog, &prog->tokens[p - 1]);
   p -= 2;
   goto t_set_start;
  case t_S:
  {
   int string_number = getvalue(&p,prog);
   if(string_number<0 || string_number>=prog->max_stringvars) error(prog, p-1, "set: bad stringvar access '%d'",string_number);
   stringvar *svr = prog->stringvars[string_number];
   copy_stringval_to_stringvar(prog, svr, getstringvalue(prog, &p) );
   break;
  }
  case t_Sf:
  {
   stringvar *svr = (stringvar*)t.data.pointer;
   copy_stringval_to_stringvar(prog, svr, getstringvalue(prog, &p) );
   break;
  }
  default:
   error(prog, p-2, "bad 'set' command");
  }
  break; // Sat 14 Sep 12:35 - is this break necessary?
 }
 // ========================================================
 // =========  INCREMENT & DECREMENT  ======================
 // ========================================================
 case t_increment: case t_decrement: {
  int type = (t.type == t_increment);
  double inc = type ? 1.0 : -1.0;
  int starting_p = p;
  p+=1;
  increment_start:
  switch( prog->tokens[p].type ){
   case t_id:
    process_id(prog, &prog->tokens[p]);
    goto increment_start;
   case t_Df:
    prog->tokens[starting_p].type = type ? t_increment_df : t_decrement_df;
    prog->tokens[starting_p].data.pointer = prog->tokens[p].data.pointer;
    (*(double*)prog->tokens[p].data.pointer) += inc;
    p+=1;
    break;
   case t_stackaccess:
    prog->tokens[starting_p].type = type ? t_increment_stackaccess : t_decrement_stackaccess;
    prog->tokens[starting_p].data.i = prog->tokens[p].data.i;
    prog->stack[prog->sp + prog->tokens[p].data.i] += inc;
    p+=1;
    break;
   case t_L:
    error(prog, p, "increment: 'L' not supported");
   case t_P:
    {
     p+=1;
     int index = prog->sp + prog->current_function->num_locals + (int)getvalue(&p,prog);
     if(index<0 || index>=prog->ssize) error(prog, starting_p+1, "increment: bad parameter access");
     prog->stack[index] += inc;
    }
    break;
   case t_V:
    p+=1;
    (*V_ValueAccess( &p, prog)) += inc;
    break;
   case t_C:
    p+=1;
    (*C_CharacterAccess( &p, prog )) += (char)inc;
    break;
   case t_D:
    {
     p+=1;
     int index = (int)getvalue(&p,prog);
     if(index<0 || index>=prog->vsize) error(prog, starting_p+1, "increment: bad variable access '%d'",index);
     prog->vars[index] += inc;
    }
    break;
   case t_A:
    {
     p+=1;
     int index = (int)getvalue(&p,prog) + (int)getvalue(&p,prog);
     if(index<0 || index>=prog->vsize) error(prog, starting_p+1, "increment: bad array access '%d'",index);
     prog->vars[index] += inc;
    }
    break;
   default:
    error(prog, p-1, "increment: bad argument '%s'",tokenstring(prog->tokens[p]));
  }
 } break;
 case t_increment_df: {
  (*(double*)t.data.pointer)+=1.0;
  p+=2;
 } break;
 case t_increment_stackaccess: {
  prog->stack[prog->sp+t.data.i]+=1.0;
  p+=2;
 } break;
 case t_decrement_df: {
  (*(double*)t.data.pointer)-=1.0;
  p+=2;
 } break;
 case t_decrement_stackaccess: {
  prog->stack[prog->sp+t.data.i]-=1.0;
  p+=2;
 } break;
 // ========================================================
 // =========  CATCH  ======================================
 // ========================================================
 case t_catch: {
  process_catch(prog,p);
  break;
 }
 case t_catchf: {
  p+=1;
  double retval;
  int errorcaught = catch( prog, &p, catch__call_interpreter, &retval, NULL );
  if( !errorcaught && retval != catch_magicnumber ){
   return retval;
  }
  p=t.data.i;
  break;
 }
 case t_endcatch: {
  return catch_magicnumber;
 }
 // ========================================================
 // =========  THROW  ======================================
 // ========================================================
 case t_throw: {
  p+=1;
  int pp = p;
  stringval sv = getstringvalue(prog, &p);
  int l = sv.len > 128 ? 128 : sv.len;
  char msg[l+1];
  strncpy(msg,sv.string,l);
  msg[l]=0;
  error(prog, pp, msg);
 }
 // ========================================================
 // ======= EVAL ===========================================
 // ========================================================
 case t_eval: {
  int starting_p = p;
  p+=1;
  double v;
  stringval s = getstringvalue(prog,&p);
  int h = s.string[s.len]; s.string[s.len]=0;
  int result = interpreter_eval( prog, eval_interpreter, &v, s.string );
  s.string[s.len]=h;
  if( result ) error( prog, starting_p, prog->_error_message->string );
 } break;
 // ========================================================
 // ======= INTERACTIVE PROMPT =============================
 // ========================================================
 case t_prompt: {
  p+=1;
  interactive_prompt(prog);
 } break;
 // ========================================================
 // =========  CREATING VARIABLES ETC  =====================
 // ========================================================
 case t_var: // create a variable
 {
  p+=1;
  while( prog->tokens[p].type != t_endstatement ){
   if( prog->tokens[p].type != t_id ){ 
    error(prog, p, "bad 'variable' command or missing ';'");
   }
   //if( find_id(prog->ids,     (char*)prog->tokens[p].data.pointer) ) error(prog, -1, "interpreter: variable: this identifier is already used");
   add_id__error_if_fail( prog, p, "the id for this variable is already used",
                          prog->ids, make_id( (char*)prog->tokens[p].data.pointer, maketoken_Df( prog->vars + allocate_variable_data(prog, 1) ) ) );
   p+=1;
  }
  p+=1;
  break;
 }
 case t_const: // create a constant
 {
  p+=1;
  if( prog->tokens[p].type != t_id ){
   error(prog, p, "bad 'constant' command");
  }
  char *idstring = (char*)prog->tokens[p].data.pointer;
  //if( find_id(prog->ids, idstring) ) error(prog, -1, "interpreter: constant: this identifier is already used");
  p+=1;
  add_id__error_if_fail( prog, p, "the id for this constant is already used",
          prog->ids, make_id( idstring, maketoken_num( getvalue(&p, prog) ) ) );
  break;
 }
 case t_stringvar: // create a string variable
 {
  size_t bufsize; token idt;
  p+=1;
  t_stringvar_start:
  // get id for new string variable
  idt = prog->tokens[p]; 
  if( idt.type != t_id ){
   error(prog, p, "bad 'stringvar' command or missing ';'");
  }
  p+=1;
  // create id for new string variable
  add_id__error_if_fail( prog, p, "the id for this stringvar is already used",
          prog->ids, make_id( (char*)idt.data.pointer, maketoken_Sf( obtain_new_stringvar(prog) ) ) );
  if( prog->tokens[p].type != t_endstatement ) goto t_stringvar_start;
  p+=1;
  break;
 }
 // ========================================================
 // =========  UNCLAIM  ====================================
 // ========================================================
 case t_unclaim:
 {
  unclaim(prog, &p);
  break;
 }
 // ========================================================
 // =========  APPEND$  ====================================
 // ========================================================
 case t_appendS:
 {
  p+=1;
  stringvar *appendTo; stringval appendString;
  TOKENTYPE_TYPE type = prog->tokens[p].type;
  if(type==t_id){
   process_id(prog, &prog->tokens[p]);
   type = prog->tokens[p].type;
  }
  if(type==t_Sf){
   appendTo = prog->tokens[p].data.pointer;
   p+=1;
  }else if(type==t_S){
   p+=1;
   int svrNum = getvalue(&p,prog);
   if(svrNum<0 || svrNum>= prog->max_stringvars){
    error(prog, p, "append$: bad stringvariable access");
   }
   appendTo = prog->stringvars[svrNum];
  }else{
   error(prog, p, "append$: not given a string variable");
  }
  do{
   appendString = getstringvalue(prog,&p);
   size_t len = appendString.len;
   if( appendTo->len + len + 1 >= appendTo->bufsize ){
    if( appendString.string >= appendTo->string && appendString.string < (appendTo->string + appendTo->len) ){ // Äkta dig för Rövar-Albin
     size_t ptr_offset = appendString.string - appendTo->string;
     stringvar_adjustsizeifnecessary(prog, appendTo, appendTo->len + len + 1, 1);
     appendString.string = appendTo->string + ptr_offset;
    }else{
     stringvar_adjustsizeifnecessary(prog, appendTo, appendTo->len + len + 1, 1);
    }
   }
   memcpy( appendTo->string + appendTo->len, appendString.string, len);
   appendTo->len += len;
  }while( determine_valueorstringvalue(prog, p, 0) == determined_stringvalue );
  break;
 }
 // ========================================================
 // =========  PRINT  ======================================
 // ========================================================
 case t_print: // print
 {
  double value; stringval stringvalue; int i;
  p+=1;
  t_print_start: // =========================================================================
  if( ! (isvalue(prog->tokens[p].type) || isstringvalue(prog->tokens[p].type)) ){
   p += (prog->tokens[p].type == t_endstatement);
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
   printf("%.16f",value);
  }
  goto t_print_start;
  t_print_stringval: //====================================================================
  stringvalue = getstringvalue( prog, &p );
  for(i=0; i<stringvalue.len; i++){
   putchar(stringvalue.string[i]);
  }
  goto t_print_start;
 }
 // ========================================================
 // =========  WAIT  =======================================
 // ========================================================
 case t_wait:
 {
  p+=1;
  #ifdef enable_graphics_extension
  double waitv = getvalue(&p,prog);
  if( ((int)waitv==16) && newbase_is_running && newbase_allow_fake_vsync ){
   newbase_fake_vsync_request=1;
   while( newbase_fake_vsync_request ) usleep(4);
  }else{
   usleep((int)( waitv*1000 ));
  }
  #else
  usleep((int)( getvalue(&p,prog)*1000 ));
  #endif
  break;
 }
 case t_endfn:
 case t_deffn: // upon meeting a function definition, we know that the 'scope' of the current function (or of the main program body) has reached its end.
  return 0.0;
 case t_id: // ids can represent functions, so they must be processed if encountered by 'interpreter'
  process_id(prog,&prog->tokens[p]);
  goto interpreter_start;
 case t_extcom:
  p+=1;
  ( (void(*)(int*,program*)) t.data.pointer )(&p,prog);
  break;
 case t_F: case t_Ff: case t_extfun: // procedure call (a function is called and the return value is discarded)
  getvalue(&p,prog);
  break;
 case t_option:
 {
  option( prog, &p );
  break;
 }
 case t_extopt:
 {
  void *fn = t.data.pointer;
  p+=2;
  ( (void(*)(program*,int*)) fn )(prog,&p);
  break;
 }
 case t_oscli:
 {
  p+=1;
  oscli( prog, &p );
  break;
 }
 // ========================================================
 // =========  FILE RELATED COMMANDS  ======================
 // ========================================================
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
 // ========================================================
 // =========  CASEOF  =====================================
 // ========================================================
 case t_caseof:
  _interpreter_processcaseof(prog,p);
  break;
 case t_caseofV:
  { 
   struct caseof *co = t.data.pointer;
   p+=1;
   double v = getvalue(&p,prog);
   #if PROCESSCASEOF_RUBBISH
   fprintf(stderr,"v: %f\n",v);
   #endif
   int i;
   for(i=0; i < co->num_whens; i++){
    p=co->whens[i];
    #if PROCESSCASEOF_RUBBISH
    fprintf(stderr,"shit %s\n",tokenstring(prog->tokens[p]));
    #endif
    double v2;
    while( isvalue( prog->tokens[p].type ) ){
     v2 = getvalue(&p,prog);
     #if PROCESSCASEOF_RUBBISH
     fprintf(stderr,"v2: %f\n",v2);
     #endif
     if( v == v2 ){
      #if PROCESSCASEOF_RUBBISH
      tb(); fprintf(stderr,"match %f, %f\n",v,v2);
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
   prog->getstringvalue_level += 1;
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
  prog->getstringvalue_level -= 1;
  break;
 // ========================================================
 // =========  QUIT  =======================================
 // ========================================================
 case t_quit:
  {
   #ifdef enable_debugger
   dbg__quit(prog, p, reason_quitted);
   #endif
   p+=1;
   #ifdef enable_graphics_extension
   if(newbase_is_running){
    Wait(1);
    MyCleanup();
   }
   #endif
   double ret_val = 0;
   if( isvalue( prog->tokens[p].type ) ){
    //fprintf(stderr,"this is happening\n"); tb(); // test
    ret_val = getvalue(&p,prog);
   }
   quit_cleanup(prog);
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
 case t_arcf: case t_arc: {
  p+=1;
  int x,y,rx,ry; double start_angle,extent_angle;
  x  = getvalue(&p,prog);
  y  = getvalue(&p,prog);
  rx = getvalue(&p,prog);
  ry = getvalue(&p,prog);
  start_angle = getvalue(&p,prog);
  extent_angle = getvalue(&p,prog);
  Arc( x, y, rx, ry, start_angle, extent_angle, t.type == t_arcf );
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
 case t_drawscaledtext:
 {
  p+=1;
  int x,y,xs,ys; stringval sv;
  x=getvalue(&p,prog); y=getvalue(&p,prog); xs=getvalue(&p,prog); ys=getvalue(&p,prog);
  sv = getstringvalue( prog, &p );
  char holdthis = sv.string[sv.len]; sv.string[sv.len]=0;
  drawscaledtext(x,y,xs,ys, sv.string);
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
 case t_listallids:
 {
  p+=1;
  list_all_ids(prog);
  break;
 }
 case t_printentirestack:
 {
  fprintf(stderr,"---- STACK ----\n");
  int i;
  for(i=0; i<prog->sp; i++){ // print everything until the current stack frame.
   fprintf(stderr," %f\n",prog->stack[ i ]);
  }  
 }// continue into t_printstackframe and let it print the current stack frame (as well as advance p past the printentirestack command)
 case t_printstackframe:
 {
  p+=1;
  int i;
  fprintf(stderr,"STACK FRAME\n----- locals -----\n");
  for(i=0; i < prog->current_function->num_locals + (prog->current_function->params_type==p_atleast ? prog->stack[ prog->sp ] : prog->current_function->num_params ); i++){
   if(i==prog->current_function->num_locals) fprintf(stderr,"----- params -----\n");
   fprintf(stderr," %f\n",prog->stack[ prog->sp + i ]);
  }
  fprintf(stderr,"------------------\n\n");
 } break;
 case t_tb: tb(); p+=1; break;
#endif
 case t_nul: {
  #ifdef enable_debugger
  if( prog->current_function == &prog->initial_function && ! prog->error_handler ){
   dbg__quit(prog, p, reason_ended);
  }
  #endif
  /*
  if( prog->current_function != &prog->initial_function ){
   error(prog,-1,"unexpected end of program");
  }
  */
  return 0;
 }
 case t_label: case t_endif: case t_endiff: case t_endstatement:
  p+=1;
  goto interpreter_start;
 default:
  error(prog, p, "expected a command, instead found '%s'",tokenstring(t));
 }
 goto interpreter_start;
}

// ----------------------------------------------------------------------------------------------------------------
// ---- MAIN FUNCTION ---------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

void
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
sl_c__parse_argc_argv(program *prg, int argc, char **argv){
 add_id__error_if_fail( prg, -1, "main: failed to create _argc constant, this should never happen\n",
                        prg->ids, make_id( "_argc", maketoken_num( argc-2 ) ) );
 int i;
 for(i=2; i<argc; i++){
  copy_stringval_to_stringvar( prg,  create_new_stringvar(prg), (stringval) { argv[i], strlen(argv[i]) } );
 }

 char errormessage[] = "main: failed to create _argv0 constant, this should never happen\n";
 char varname[] = "_argv0";
 for(i=0; i<2; i++){
  varname[5]='0'+i;
  errormessage[28]='0'+i;
  char *varname_permanent_address = calloc(8,sizeof(char));
  strcpy(varname_permanent_address, varname);
  stringvar *svr = create_new_stringvar(prg);
  copy_stringval_to_stringvar( prg, svr, (stringval) { argv[i], strlen(argv[i])} );
  add_id__error_if_fail( prg, -1, errormessage,
                         prg->ids, make_id( varname_permanent_address, maketoken_Sf( svr ) ) );
  stringslist_addstring(prg->program_strings, varname_permanent_address);
 }
}

#define main_printstuff 0
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
sl_c__main(int argc, char **argv, id_info *extensions, char *extra_version_info){
 SeedRng();
 //printf("sz %d\n",sizeof(token)); return 0;
 program *prg = NULL;

 if(argc>1){
  prg = init_program( argv[1], Exists(argv[1]), extensions ); 
 }else{
  fprintf(stderr,"Johnsonscript interpreter, built on %s, %s\nhttps://github.com/dusthillresident/JohnsonScript/\n", __DATE__, __TIME__);
  if( extra_version_info ) fprintf(stderr,"%s",extra_version_info);
  fprintf(stderr,"Usage:\n%s [program text or path to a file containing program text]\n",argv[0]);
  fprintf(stderr,"To enter the interactive prompt:\n%s _prompt\n",argv[0]);
  return 0;
 }

#if 1
 sl_c__parse_argc_argv(prg, argc, argv);
#endif

#if main_printstuff
 fprintf(stderr,"ok\n\n");
 detokenise(prg,-1);
 fprintf(stderr,"\n");
#endif

#if main_printstuff
 fprintf(stderr,"------- program output -------\n");
#endif
 double result = interpreter(0,prg);
#if main_printstuff
 fprintf(stderr,"------------------------------\n");
 fprintf(stderr,"\n");
#endif

#if main_printstuff
 list_all_ids(prg); printf("\n");
#endif

#if main_printstuff
 fprintf(stderr,"result: %f\n",result);
#endif

#if main_printstuff
 printf("----\n");
 detokenise(prg,-1);
 printf("----\n");
#endif

#if main_printstuff
 printf("ended\n");
#endif
#ifdef enable_debugger
 interactive_prompt(prg);
#endif
 quit_cleanup(prg);
 unloadprog(prg);
 return (int)result;
}

#ifndef disable_sl_c_main
int
#ifndef DISABLE_ALIGN_STUFF
__attribute__((aligned(ALIGN_ATTRIB_CONSTANT)))
#endif
main(int argc, char **argv){
 char extra_version_info[100] = {0};
 #ifdef enable_graphics_extension
 strcat(extra_version_info, "Xlib graphics extension\n");
 #endif
 return sl_c__main(argc,argv,NULL, *extra_version_info ? extra_version_info : NULL );
}
#endif

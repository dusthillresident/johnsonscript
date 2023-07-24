#define disable_sl_c_main
#include "sl.c"

void trans_add_id(id_info *ids, id_info *new_id){
 // check if this id name is already used in this id list
 if( ids->name && !strcmp(new_id->name, ids->name) ){
  printf("add_id: id was '%s'\n",new_id->name);
  fprintf(stderr,"add_id: this id is already used in this id list\n");
  exit(0);
 }
 // ----------
 if( ids->next == NULL){
  ids->next = new_id;
 }else{
  add_id( ids->next, new_id);
 }
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

#define TRANS_CRASHDEBUG 0

void translate_value(program *prog, int *p);
void translate_stringvalue(program *prog, int *p);
void translate_command(program *prog, int *p);
char* trans__transval_into_tempbuf( program *prog, int *p );
char* trans__transstringval_into_tempbuf( program *prog, int *p );

// ----

char *trans_file_input_path = NULL;

char *trans_protos = NULL; int trans_protos_p=0;
char *trans_main = NULL; int trans_main_p=0;
char *trans_varp = NULL; int trans_varp_p=0;
char *trans_vars = NULL; int trans_vars_p=0;

int trans_fn=0;
int trans_num_funcs=0;
int trans_var_n=0;
int trans_program_contains_F=0; // if the program makes use of function references, every function must be of the sort that takes its parameters as a single 'double[]'
int trans_vsize = 2048; // size of the variables array
int trans_unnamed_stringvars=0;
id_info *trans_global_ids = NULL; // keep track of declarations
typedef struct { int gpos,lpos; char *desc; } Declaration;
int *trans_function_gposses = NULL;
int trans_functions_start_p=0;
int trans_program_uses_maths = 0;
int trans_program_uses_gfx = 0;
int trans_reverse_function_params = 0;

#define PrintProt(...) (trans_protos_p  +=  sprintf(trans_protos  +  trans_protos_p,__VA_ARGS__))
#define PrintMain(...) (trans_main_p    +=  sprintf(trans_main    +  trans_main_p,  __VA_ARGS__))
#define PrintVarp(...) (trans_varp_p    +=  sprintf(trans_varp    +  trans_varp_p,  __VA_ARGS__))
#define PrintVars(...) (trans_vars_p    +=  sprintf(trans_vars    +  trans_vars_p,  __VA_ARGS__))

#define CurTok prog->tokens[ *p ]

#define PrintErr(...) fprintf(stderr,__VA_ARGS__);

#define ErrorOut(...) {trans_print_sourcetext_location(prog,*p);PrintErr(__VA_ARGS__);exit(0);}

void trans_print_sourcetext_location( program *prog, int token_p){
 if( prog->program_strings->string == NULL ) return;
 TTP *ttp = (TTP*)prog->program_strings->string;
 if( token_p<0 || token_p>=prog->length ){
  PrintErr("trans_print_sourcetext_location: token_p out of range\n");
 }else{
  PrintErr(":::: At line %d, column %d, file '%s' ::::\n", ttp[token_p].line, ttp[token_p].column, ttp[token_p].file );
 }
}

Declaration* MakeDecl(int gpos, int lpos, char *desc){ // desc means 'description'
 Declaration *d = calloc(1,sizeof(Declaration));
 *d = (Declaration){ gpos,lpos,desc };
 return d;
}

void AddDecl(Declaration *d, char *idstring){
 trans_add_id(trans_global_ids, make_id( idstring, maketoken_Df( (double*)d ) ) );
}

Declaration* FindDecl(char *idstring){
 id_info *foundid = find_id( trans_global_ids, idstring );
 if(foundid){
  return (Declaration*)foundid->t.data.pointer;
 }
 return NULL;
}

void SetGLPos(Declaration *d, program *prog, int p){
 d->lpos = p;
 if( p < trans_functions_start_p ){
  d->gpos = p;
 }else{
  d->gpos = trans_function_gposses[ prog->current_function->function_number ];  
 }//endif
}//endproc

void TestThisShit(){
 id_info *ids = trans_global_ids;
 while(ids){
  if(ids->name){
   printf("---- %s ----\n",ids->name);
   Declaration *dc = (Declaration*)ids->t.data.pointer;
   printf("gpos %d\n", dc->gpos );
   printf("lpos %d\n", dc->lpos );
   printf("desc '%s'\n\n", dc->desc );
  }
  ids = ids->next;
 }//endwhile
}//endproc


typedef struct { char *s; int p; } TransBuf;
TransBuf SaveTM(){ return (TransBuf){trans_main,trans_main_p}; }
void LoadTM(TransBuf buf){
 trans_main = buf.s;
 trans_main_p = buf.p; 
}

void SetTM(char *s, int p){trans_main=s;trans_main_p=p;}

char* trans__double_into_tempbuf( double v ){
 char *out = malloc(32768);
 sprintf(out,"%a",v);
 return out;
}

char* trans__transval_into_tempbuf( program *prog, int *p ){
 char *out = calloc(1*1024*32,sizeof(char)); 
 TransBuf tmhold = SaveTM();
 SetTM(out,0);
 translate_value(prog, p);
 LoadTM(tmhold);
 return out;
}

char* trans__transstringval_into_tempbuf( program *prog, int *p ){
 char *out = calloc(1*1024*32,sizeof(char)); 
 TransBuf tmhold = SaveTM();
 SetTM(out,0);
 translate_stringvalue(prog, p);
 LoadTM(tmhold);
 return out;
}

token trans_peek_id(program *prog, token *t){
 token out = (token){0,0};
 id_info *foundid=NULL;
 // first, check current function's id list
 foundid = find_id( prog->current_function->ids, (char*)t->data.pointer );
 // if nothing was found, check the global id list
 if(foundid==NULL) foundid = find_id( prog->ids, (char*)t->data.pointer );
 // if nothing was still found, PENIS
 if(foundid==NULL){
  out.type=255;
 }else{
  out = foundid->t;
 }
 // --- something was found ---
 return out;
}

void trans_build_fgpos_array(program *prog){
 int i=0,j; 
 while( prog->functions[i] ){
  trans_function_gposses[i] = prog->length;
  for(j=0; j<prog->length; j++){
   if( prog->tokens[j].type == t_id ){
    token peek = trans_peek_id(prog, &prog->tokens[j]);
    if( peek.type == t_Ff ){ 
     if( ! strcmp( getfuncname(prog,(func_info*)peek.data.pointer), getfuncname(prog, prog->functions[i] ) ) ){
      trans_function_gposses[i] = j;
      goto trans_build_fgpos_array_next;
     }//endif we found the first reference to this function in the program
    }//endif we found a reference to a function in the program
   }//endif found an id
   if( prog->tokens[j].type == t_deffn ){
    while( prog->tokens[j].type != t_endstatement && j < prog->length ){
     j++;
    }//endwhile
    if( j >= prog->length ){ fprintf(stderr,"error: program contains a broken function definition\n"); exit(0); }
    j--;
   }//endif need to skip past a function definition
  }//next j going through the program
  trans_build_fgpos_array_next:
  i++;
 }//endwhile i going through functions
}//endproc


// table for translating IDs
typedef
struct {
 // there's a flag variable and string for how to represent this as a value, how getref should process it, and how set should process it 
 // the flag variable is true if this is possible and allowed. if not true, then the corresponding string is an error message about why this is not allowed
 int value; char *value_s; 
 int getref; char *getref_s; 
 int set; char *set_s;

 void *extra; // pointer for extra data
}
TransTable;
//(TransTable){ 0,NULL,  0,NULL,  0,NULL,  NULL}
TransTable* MakeTransTable(int value, char *value_s,  int getref,char *getref_s,  int set,char *set_s, void *extra){
 TransTable *out = calloc(1,sizeof(TransTable));
 *out = (TransTable){value,value_s, getref,getref_s, set,set_s, extra};
 return out;
}


#define DEBUG_GLPOS 0
void translate__process_id(program *prog, token *t){
 id_info *foundid=NULL;
 // first, check current function's id list
 foundid = find_id( prog->current_function->ids, (char*)t->data.pointer ); if(foundid) goto translate_process_id_out;
 // if nothing was found, check the global id list
 if(foundid==NULL) foundid = find_id( prog->ids, (char*)t->data.pointer );
 // if nothing was still found, error
 if(foundid==NULL){
  trans_print_sourcetext_location( prog, (int) (t - prog->tokens) );
  fprintf(stderr,"translate__process_id: unknown id: '%s'\n",(char*)t->data.pointer);
  exit(0);
 }
 // now do this check that tries to make sure the program is not referencing something before it's supposed to exist
 Declaration *findd = FindDecl((char*)t->data.pointer);
 if(findd){
  int lpos = ((long long int)t - (long long int)prog->tokens)/(sizeof(token));
  // ----
  #if DEBUG_GLPOS
  if( (prog->tokens+lpos) != (t) ){
   fprintf(stderr,"FUCK OFF\n");
   exit(0);
  }
  #endif
  // ----
  int gpos = (prog->current_function == &prog->initial_function) ? lpos : trans_function_gposses[ prog->current_function->function_number ] ;
  if( findd->gpos > gpos || (findd->gpos == gpos && findd->lpos > lpos ) ){
   trans_print_sourcetext_location( prog, (int) (t - prog->tokens) );
   fprintf(stderr,"it looks like this id '%s' of type '%s' is being referenced before its declaration\n", (char*)t->data.pointer, findd->desc);
   #if DEBUG_GLPOS
   fprintf(stderr," gpos: %d\n lpos %d\n\n",findd->gpos,findd->lpos);
   fprintf(stderr,"_gpos: %d\n_lpos %d\n",gpos,lpos);
   #endif
   exit(0);
  }//endif pos check
  #if DEBUG_GLPOS
  else{
   fprintf(stderr,"\n\n===== %s =====\n",(char*)t->data.pointer);
   fprintf(stderr," gpos: %d\n lpos %d\n\n",findd->gpos,findd->lpos);
   fprintf(stderr,"_gpos: %d\n_lpos %d\n",gpos,lpos);
   fprintf(stderr,"==========\n\n");
  }
  #endif
 }//endif findd
 // --- something was found ---
 // overwrite the token with the kind of token in the found id_info
 translate_process_id_out:
 *t = foundid->t;
}

void translate_processid(program *prog, int *p){
 char *idstring = (char*)prog->tokens[ *p ].data.pointer;
 translate__process_id(prog, prog->tokens + *p);
}


int translate_determine_valueorstringvalue_(program *prog, int p, int error_if_neither){// this returns 0 for a value, 1 for a stringvalue, returns -1 OR causes an error if neither is found
 int i; i=p; while( prog->tokens[i].type == t_leftb ){ i+=1; } // skip past any left brackets
 if( prog->tokens[i].type == t_id ) translate_processid(prog, &i); // process it if it's an id
 if( isstringvalue(prog->tokens[i].type) ) return 1; // return 1 if it's a stringvalue
 if( isvalue(prog->tokens[i].type) ) return 0; // return 0 if it's a value
 if( error_if_neither){
  trans_print_sourcetext_location( prog, p );
  fprintf(stderr,"translate_determine_blah : expected a value or a stringvalue ('%s')\n",tokenstring(prog->tokens[i])); // cause an error because we expected a value or a stringvalue
  exit(0);
 }
 return -1;
}

int translate_determine_valueorstringvalue(program *prog, int p){
 return translate_determine_valueorstringvalue_(prog, p, 1);
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------


void translate_stringvalue(program *prog, int *p){
 PrintMain(" ");
 trans_stringval_start:
 #if TRANS_CRASHDEBUG
 fprintf(stderr,"translate_stringvalue: %s\n",tokenstring(prog->tokens[ *p ]));
 #endif
 switch( prog->tokens[ *p ].type ){
  case t_sget:
   if( trans_reverse_function_params ){
    *p += 1;
    PrintMain("Johnson_Sget(");
    translate_value(prog, p);
    PrintMain(",");
    translate_value(prog, p);
    PrintMain(",++SAL)");
   }else{
    *p += 1;
    PrintMain("Johnson_Sget(++SAL,");
    translate_value(prog, p);
    PrintMain(",");
    translate_value(prog, p);
    PrintMain(")");
   }
   break;
  case t_S:
   { 
    *p += 1;
    PrintMain("SVRtoSVL( Johnson_stringvar_deref(");
    translate_value(prog, p);
    PrintMain(") )");
   } break;
  case t_Sf:
   {
    TransTable *ttab = (TransTable*)prog->tokens[ *p ].data.pointer;
    if( ! ttab->value ){
     fprintf(stderr,"translate_value: t_Sf: error: '%s'\n",ttab->value_s);
     exit(0);
    }
    PrintMain("%s",ttab->value_s);
    *p += 1;
   } break;
  case t_catS: 
   if( trans_reverse_function_params ){
    *p += 1;
    if( translate_determine_valueorstringvalue_(prog, *p, 0) != 1 ){ // the weird behaviour of cat$, you don't have to give it anything at all
     PrintMain("(SVL){0,\"PENIS\"}"); 
     break;
    }
    /*
    PrintMain(" ");
    */
    int count=0;
    while( translate_determine_valueorstringvalue_(prog, *p, 0) == 1 ){
     PrintMain("CatS("); translate_stringvalue(prog, p); PrintMain(",");
     count += 1;
    }
    PrintMain("(SVL){0,\"PENIS\"} ");
    while(count){
     PrintMain(",++SAL) ");
     count--;
    }
   }else{
    *p += 1;
    if( translate_determine_valueorstringvalue_(prog, *p, 0) != 1 ){
     PrintMain("(SVL){0,\"PENIS\"}"); 
     break;
    }
    /*
    PrintMain(" ");
    */

    int count=0;
    while( translate_determine_valueorstringvalue_(prog, *p, 0) == 1 ){
     PrintMain("CatS(++SAL,"); translate_stringvalue(prog, p); PrintMain(",");
     count += 1;
    }
    PrintMain("(SVL){0,\"PENIS\"} ");
    while(count){
     PrintMain(") ");
     count--;
    }
   }
   break;
  case t_stringS:
   {
    *p += 1;
    char *n = trans__transval_into_tempbuf( prog,p );
    char *s = trans__transstringval_into_tempbuf( prog,p );
    if( trans_reverse_function_params ){
     PrintMain( "StringS( %s, %s , %s )", s, n, "++SAL" );
    }else{
     PrintMain( "StringS( %s, %s , %s )", "++SAL", n, s );
    }
    free(n); free(s);
   } break;
  case t_id: 
   {
    translate_processid(prog, p);
    goto trans_stringval_start;
   } break;
  case t_stringconst:
   {
    //PrintMain("ToSVL(\"");
    int i,l; char *s = (char*)prog->tokens[ *p ].data.pointer;
    l = strlen(s);
    PrintMain("(SVL){ %d, (char[]){\"",l);
    for(i=0; i<l; i++){
     switch( s[i] ){
     case '\r':
     case '\n':
      PrintMain("\\n");
      break;
     case '"':
      PrintMain("%c%c",'\\',s[i]);
      break;
     case '\\': 
      PrintMain("%c%c",s[i],s[i]);
     default:
      PrintMain("%c",s[i]);
     }
    }
    *p += 1;
    PrintMain("\"}}");
    //PrintMain("\")");
   } break;
  case t_midS:
   {
    PrintMain("MidS(");
    *p += 1;
    translate_stringvalue(prog, p);

    PrintMain(",");
    translate_value(prog, p);

    PrintMain(",");
    translate_value(prog, p);

    PrintMain(")");
   } break;
  case t_rightS:
   {
    PrintMain("RightS(");
    *p += 1;
    translate_stringvalue(prog, p);
    PrintMain(",");
    translate_value(prog, p);
    PrintMain(")");
   } break;
  case t_leftS:
   {
    PrintMain("LeftS(");
    *p += 1;
    translate_stringvalue(prog, p);
    PrintMain(",");
    translate_value(prog, p);
    PrintMain(")");
   } break;
  case t_chrS:
   if( trans_reverse_function_params ){
    PrintMain("ChrS(");
    *p += 1;
    translate_value(prog, p);
    PrintMain(",++SAL)");
   }else{
    PrintMain("ChrS(++SAL,");
    *p += 1;
    translate_value(prog, p);
    PrintMain(")");
   }
   break;
  case t_vectorS:
   {
    if(trans_reverse_function_params){
     PrintMain("VectorS( (double[]){ ");
    }else{
     PrintMain("VectorS( ++SAL, (double[]){ ");
    }
    int n = 1;
    *p += 1;
    translate_value(prog, p);
    while( isvalue( CurTok.type) ){
     PrintMain(", ");
     translate_value(prog, p);
     n += 1;
    }
    if(trans_reverse_function_params){
     PrintMain("}, %d, ++SAL)", n);
    }else{
     PrintMain("}, %d )", n);
    }
   }
   break;
  case t_strS:
   if( trans_reverse_function_params ){
    PrintMain("StrS(");
    *p += 1;
    translate_value(prog, p);
    PrintMain(",++SAL)");
   }else{ 
    PrintMain("StrS(++SAL,");
    *p += 1;
    translate_value(prog, p);
    PrintMain(")");
   }
   break;
  case t_leftb:
   {
    PrintMain("(");
    *p += 1;
    translate_stringvalue(prog, p);
    if( prog->tokens[ *p ].type != t_rightb ){
     ErrorOut("translate_stringvalue: bad brackets\n");
    }
    PrintMain(")");
    *p += 1;
   } break;
/*
  case t_:
   {
    PrintMain(" ");
    *p += 1;
    translate_value(prog, p);
   } break;
*/
  default:
   ErrorOut("translate_stringvalue: FAILURE: Not supported or error: '%s'\n",tokenstring(prog->tokens[ *p ]));
 }
 PrintMain(" ");
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

void translate_value(program *prog, int *p){
 PrintMain(" ");
 trans_val_start:
 #if TRANS_CRASHDEBUG
 if( CurTok.type == t_Ff ){
  fprintf(stderr,"translate_value: t_Ff '%s'\n",  getfuncname(prog,CurTok.data.pointer) );
 }else{
  fprintf(stderr,"translate_value: %s\n",  tokenstring(prog->tokens[ *p ])  );
 }
 #endif
 switch( prog->tokens[ *p ].type ){
 case t_endstatement:
  ErrorOut("translate_value: expected to read a value, but found endstatement\n");
  break; 
 #ifdef enable_graphics_extension 
  case t_winw: case t_winh: case t_mousex: case t_mousey: case t_mousez:
  case t_mouseb: case t_readkey: case t_keypressed: case t_expose: case t_wmclose: case t_keybuffer:
  trans_program_uses_gfx = 1;
  switch(CurTok.type){
  case t_winw:
   {
    *p += 1;
    PrintMain("(double)");
    PrintMain("WinW");
   } break;
  case t_winh:
   {
    *p += 1;
    PrintMain("(double)");
    PrintMain("WinH");
   } break;
  case t_mousex:
   {
    *p += 1;
    PrintMain("(double)");
    PrintMain("mouse_x");
   } break;
  case t_mousey:
   {
    *p += 1;
    PrintMain("(double)");
    PrintMain("mouse_y");
   } break;
  case t_mousez:
   {
    *p += 1;
    PrintMain("(double)");
    PrintMain("mouse_z");
   } break;
  case t_mouseb:
   {
    *p += 1;
    PrintMain("(double)");
    PrintMain("mouse_b");
   } break;
  case t_readkey:
   *p += 1;
   PrintMain("(double)");
   PrintMain("GET()");
   break;
  case t_keypressed:
   *p += 1;
   PrintMain("(double)");
   PrintMain("(iskeypressed==NULL ? 0 : iskeypressed[ 0xff & ");
   PrintMain("(int)");
   translate_value(prog,p);
   PrintMain("])");
   break;
  case t_expose: 
   *p += 1;
   PrintMain("Johnson_Expose()");
   break;
  case t_wmclose:
   *p += 1;
   PrintMain("Johnson_WMClose()");
   break;
  case t_keybuffer:
   *p += 1;
   PrintMain("(double)");
   PrintMain("KeyBufferUsedSpace()");
   break;
  default:
   ErrorOut("This is not implemented yet: '%s'\n",tokenstring(CurTok));
  } break;
 #endif
 // ----- maths ------
 case t_tan: case t_tanh: case t_atan: case t_atan2: case t_acos: case t_cos: case t_asin: case t_sin: case t_sinh: case t_exp: case t_log: case t_log10: case t_pow: case t_sqr: case t_ceil: case t_floor: case t_fmod:
 trans_program_uses_maths=1;
 switch( CurTok.type ){
  case t_tan:
  case t_tanh:
  case t_atan:
  case t_acos:
  case t_cos:
  case t_asin:
  case t_sin:
  case t_sinh:
  case t_exp:
  case t_log:
  case t_log10:
  case t_sqr:
  case t_ceil:
  case t_floor:
   {
    PrintMain("%s(", CurTok.type==t_sqr ? "sqrt" : tokenstring(prog->tokens[ *p ])  );
    *p += 1;
    translate_value(prog, p); 
    PrintMain(")");
   } break;
  case t_pow:
  case t_fmod:
  case t_atan2:
   {
    PrintMain("%s(",tokenstring(prog->tokens[ *p ]));
    *p += 1;
    translate_value(prog, p); 
    PrintMain(",");
    translate_value(prog, p); 
    PrintMain(")");
   } break;
 } break;
 // ----- files ------
 case t_ext:
  {
   *p += 1; 
   PrintMain("(double)");
   PrintMain("Johnson_Ext(");
   translate_value(prog, p); 
   PrintMain(")");
  } break;
 case t_ptr:
  {
   *p += 1; 
   PrintMain("(double)");
   PrintMain("Johnson_Ptr(");
   translate_value(prog, p); 
   PrintMain(")");
  } break;
 case t_vget:
  {
   *p += 1; 
   PrintMain("Johnson_Vget(");
   translate_value(prog, p); 
   PrintMain(")");
  } break;
 case t_bget:
  {
   *p += 1; 
   PrintMain("(double)");
   PrintMain("Johnson_Bget(");
   translate_value(prog, p); 
   PrintMain(")");
  } break;
 case t_eof:
  {
   *p += 1; 
   PrintMain("(double)");
   PrintMain("Johnson_Eof(");
   translate_value(prog, p); 
   PrintMain(")");
  } break;
 case t_openin: case t_openout: case t_openup:
  {
   PrintMain("(double)");
   PrintMain("Johnson_OpenFile( ");
   char *accessmode;
   switch( CurTok.type ){
    case t_openin:  accessmode = "J_OPENIN";  break;
    case t_openout: accessmode = "J_OPENOUT"; break;
    case t_openup:  accessmode = "J_OPENUP";  break;
   }
   *p += 1; 
   translate_stringvalue(prog, p); 
   PrintMain(", %s)",accessmode);
  } break;
 // ----- end of files ------
 case t_SS: 
  {
   *p += 1;
   PrintMain("(double)");
   switch( translate_determine_valueorstringvalue_(prog, *p, 0) ){
   case 1: // initialiser string given
    PrintMain("(SetSVR( NewStringvar_svrp(), ");
    translate_stringvalue(prog, p); 
    PrintMain(")->string_variable_number)");
    break;
   case -1: case 0: // no initialiser given
    PrintMain("(NewStringvar_svrp()->string_variable_number)");
    break;
   }
  } break;
 case t_instrS:
  if( trans_reverse_function_params ){
   *p += 1;
   PrintMain("(double)");
   PrintMain("InstrS(");
   translate_stringvalue(prog, p);    PrintMain(",");
   translate_stringvalue(prog, p); 
   PrintMain(",++SAL)");
  }else{
   *p += 1;
   PrintMain("(double)");
   PrintMain("InstrS(++SAL,");
   translate_stringvalue(prog, p);    PrintMain(",");
   translate_stringvalue(prog, p); 
   PrintMain(")");
  }
  break;
 case t_cmpS:
  if( trans_reverse_function_params ){
   *p += 1;
   PrintMain("(double)");
   PrintMain("CmpS(");
   translate_stringvalue(prog, p);    PrintMain(",");
   translate_stringvalue(prog, p); 
   PrintMain(",++SAL)");
  }else{
   *p += 1;
   PrintMain("(double)");
   PrintMain("CmpS(++SAL,");
   translate_stringvalue(prog, p);    PrintMain(",");
   translate_stringvalue(prog, p); 
   PrintMain(")");
  }
  break;
 case t_lenS:
  {
   *p += 1;
   PrintMain("(double)");
   PrintMain("(");
   PrintMain("(");
   translate_stringvalue(prog, p);
   PrintMain(").len");
   PrintMain(")");
  } break;
 case t_vlenS:
  {
   *p += 1;
   PrintMain("(double)");
   PrintMain("(");
   PrintMain("(");
   translate_stringvalue(prog, p);
   PrintMain(").len>>3");
   PrintMain(")");
  } break;
 case t_valS:
  {
   // if val$ has a constant as input, we can just evaluate that right now and make the translated program simpler
   if( prog->tokens[ *p + 1 ].type == t_stringconst  ){
    double value = getvalue(p,prog);
    if( !MyIsnan(value) && value == (int)value ){
     PrintMain("(double)");
     PrintMain("%d",(int)value);
    }else{
     PrintMain("%a",value);
    }
   }else{ // otherwise, we must translate it as a call to ValS() from johnsonlib
    *p += 1;
    PrintMain("ValS(");
    translate_stringvalue(prog, p);
    PrintMain(")");
   }
  } break;
 case t_ascS:
  {
   *p += 1;
    if( CurTok.type == t_stringconst ){
     PrintMain("(double)");
     PrintMain("%d",*(char*)CurTok.data.pointer);
     *p += 1; 
    }else{
     PrintMain("AscS(");
     translate_stringvalue(prog, p);
     PrintMain(")");
    }
  } break;
 case t_getref:
  {
   *p += 1;
   transval_getref_start:
   switch( prog->tokens[ *p ].type ){
   // ------ strings -------
   case t_stringconst:
    {
     // declare global int for it
     PrintVarp("int Unnamed_string_variable_%d;\n",trans_unnamed_stringvars);
     // assign string variable to it
     PrintVars("Unnamed_string_variable_%d = (SetSVR( NewStringvar_svrp(), ",trans_unnamed_stringvars);   

     TransBuf tmhold = SaveTM();
     SetTM(trans_vars,trans_vars_p);
     translate_stringvalue(prog, p);
     trans_vars_p = trans_main_p;
     LoadTM(tmhold);

     PrintVars(")->string_variable_number);\n");            
     // give the reference number
     PrintMain("(double)Unnamed_string_variable_%d",trans_unnamed_stringvars);
     //done
     trans_unnamed_stringvars += 1;
    } break;
   case t_Sf:
    {
     TransTable *ttab = ((TransTable*)prog->tokens[ *p ].data.pointer);
     if(!ttab->getref){
      fprintf(stderr,"translate value: getref t_Sf: error: '%s'\n", ttab->getref_s );
      exit(0);
     }
     PrintMain("%s",ttab->getref_s);
     *p += 1;
    } break;
   case t_S:
    { 
     *p += 1;
     PrintMain("(");
     translate_value(prog, p); 
     PrintMain(")");
    } break;
   // ----------------------
   case t_P: case t_A: case t_D: {
    PrintMain("Johnson_getref(&");
    translate_value(prog, p); 
    PrintMain(")");
   } break;
   case t_Ff: {
    PrintMain("(double)%d", ((func_info*)prog->tokens[*p].data.pointer)->function_number );
    *p += 1;
   } break; 
   case t_Df: {
     TransTable *ttab = ((TransTable*)prog->tokens[ *p ].data.pointer);
     if(!ttab->getref){
      fprintf(stderr,"translate value: getref t_Df: error: '%s'\n", ttab->getref_s );
      exit(0);
     }
     PrintMain("%s",ttab->getref_s);
     *p += 1;
    }
    break;
   case t_id: translate_processid(prog, p); goto transval_getref_start;
   default: 
    fprintf(stderr,"translate value: getref: error: not supported or bad '%s'\n",tokenstring( prog->tokens[ *p ] ) );
    exit(0);
   }
  } break;
 case t_D:
  {
   *p += 1;
   PrintMain("FirstVarP[(int)(");
   translate_value(prog, p);
   PrintMain(")]");
  } break;
 case t_A:
  {
   *p += 1;
   PrintMain("FirstVarP[(int)(");
   translate_value(prog, p);
   PrintMain(")+(int)(");
   translate_value(prog, p);
   PrintMain(")]");
  } break;
 case t_C:
  {
   *p += 1;
   char *s     = trans__transstringval_into_tempbuf( prog, p );
   char *index =       trans__transval_into_tempbuf( prog, p );
   if( trans_reverse_function_params ){
    PrintMain( "*Johnson_C_CharacterAccess( %s, %s )", index, s );
   }else{
    PrintMain( "*Johnson_C_CharacterAccess( %s, %s )", s, index );
   }
   free(s); free(index);
  } break;
 case t_V:
  {
   *p += 1;
   char *s     = trans__transstringval_into_tempbuf( prog, p );
   char *index =       trans__transval_into_tempbuf( prog, p );
   if( trans_reverse_function_params ){
    PrintMain( "*Johnson_V_ValueAccess( %s, %s )", index, s );
   }else{
    PrintMain( "*Johnson_V_ValueAccess( %s, %s )", s, index );
   }
   free(s); free(index);
  } break;
 case t_number:
  {
   PrintMain("%a",prog->tokens[ *p ].data.number);
   *p += 1;
  } break;
 case t_lessthan: case t_morethan: case t_lesseq: case t_moreeq: case t_equal:// two params
  {
   PrintMain("(double)");
   PrintMain("(");
   char *s = tokenstring( prog->tokens[ *p ] );
   *p += 1;
   translate_value(prog, p);
   PrintMain("%s",s);
   if( *s == '=' ) PrintMain("="); // C requires '=='
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_lshiftright:
  { 
   *p += 1;
   PrintMain("(double)(");
   PrintMain("(unsigned int)(int)");
   translate_value(prog, p);
   PrintMain(">> ");
   PrintMain("(unsigned int)(int)");
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_mod: case t_shiftleft: case t_shiftright:// two params, which C requires to be integers
  {
   PrintMain("(double)(");
   char *s = tokenstring( prog->tokens[ *p ] );
   *p += 1;
   PrintMain("(int)");
   translate_value(prog, p);
   PrintMain("%s",s);
   PrintMain(" (int)");
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_plus: case t_minus: case t_star: case t_slash: case t_land: case t_lor: // at least two params
  {
   if( prog->tokens[ *p ].type == t_land || prog->tokens[ *p ].type == t_lor ){
    PrintMain("(double)");
   }
   PrintMain("(");
   char *s = tokenstring( prog->tokens[ *p ] );
   *p += 1;
   translate_value(prog, p);
   PrintMain("%s",s);
   translate_value(prog, p);
   while( isvalue( prog->tokens[ *p ].type ) ){
    PrintMain("%s",s);
    translate_value(prog, p);
   }
   PrintMain(")");
  } break;
 case t_and: case t_or: case t_eor: // at least two params which C requires to be integers
  {
   PrintMain("(double)(");
   char *s = tokenstring( prog->tokens[ *p ] );
   *p += 1;
   PrintMain("(int)");
   translate_value(prog, p);
   PrintMain("%s",s);
   PrintMain(" (int)");
   translate_value(prog, p);
   while( isvalue( prog->tokens[ *p ].type ) ){
    PrintMain("%s",s);
    PrintMain("(int)");
    translate_value(prog, p);
   }
   PrintMain(")");
  } break;
 case t_not:
  {  
   PrintMain("(double)");
   PrintMain("~");
   PrintMain("(int)");
   *p += 1;
   translate_value(prog, p);
  } break;
 case t_lnot:
  {
   PrintMain("(double)!(");
   *p += 1;
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_id:
  {
   translate_processid(prog, p);
   goto trans_val_start;
  } break;
 case t_Df: 
  {
   TransTable *ttab = (TransTable*)prog->tokens[ *p ].data.pointer;
   if( ! ttab->value ){
    fprintf(stderr,"translate_value: t_Df: error: '%s'\n",ttab->value_s);
    exit(0);
   }
   PrintMain("%s",ttab->value_s);
   *p += 1;
  } break;

 case t_stackaccess:
  {
   if( prog->tokens[ *p ].data.i != 0 ){
    fprintf(stderr,"translate_value: t_stackaccess: unhandled (%d)\n",prog->tokens[ *p ].data.i);
    exit(0);
   }
   if( prog->current_function->params_type == p_atleast){
    PrintMain("Params[0]");
   }else{
    PrintMain("(double)%d",prog->current_function->num_params);
   }
   *p += 1;
  } break;
 case t_P:
  {
   if( prog->current_function->params_type == p_exact){
    fprintf(stderr,"translate_value: P: not supported in functions with fixed number of params\n");
    exit(0);
   }else{
    *p += 1;
    PrintMain("Params[(int)(1+(");
    translate_value(prog, p);
    PrintMain("))]");
   }
  } break;
 case t_F: 
/*
 function references can be supported but with these limitations:
 . you can't know in advance how many parameters the function will take, so you have to read everything until there are no more values, and programs have to be written to account for that
 . every function must take parameters as a double[] array
*/
  {
   *p += 1;
   PrintMain("Johnson_function_deref( (double[]){");

   // process function reference value
   TransBuf tmhold = SaveTM(); 
   char *func_ref = calloc(2048,1);
   SetTM(func_ref,0);
   translate_value(prog, p);

   char *param_list = calloc(1024*1024,1);
   SetTM(param_list, 0);
 
   // process params list
   
   int pcount = 0; 
   while( translate_determine_valueorstringvalue_(prog, *p, 0) == 0 ){
    translate_value(prog, p);
    if( translate_determine_valueorstringvalue_(prog, *p, 0) == 0 ) PrintMain(",");
    pcount+=1;
   }

   LoadTM(tmhold);
   PrintMain(" %d,%s},%s )",pcount,param_list,func_ref);

   free(func_ref);
   free(param_list);

  } break;
 case t_Ff:
  {
   func_info *func = (func_info*)prog->tokens[ *p ].data.pointer;
   char *func_name = getfuncname( prog, func );
   PrintMain("FN_%s(",func_name);
   *p += 1;

   if( func->params_type == p_atleast || trans_program_contains_F ){

    PrintMain(" (double[]){ ");

    TransBuf tmhold = SaveTM();
    char *tempbuf = calloc(1024*1024,1);
    SetTM(tempbuf,0);
    int i, pcount = 0;
    for(i=0; i<func->num_params; i++){
     translate_value(prog, p);
     PrintMain(", ");
     pcount+=1;
    }
    if( func->params_type == p_atleast ){
     while( isvalue( prog->tokens[ *p ].type ) ){
      translate_value(prog, p);
      PrintMain(", ");
      pcount+=1;
     }
    }
    LoadTM(tmhold);

    if( func->params_type == p_exact && pcount == 0 ){
     // no parameters at all
     trans_main_p -= 11;
     PrintMain("NULL)");
    }else{ // p_atleast (or there were parameters at all and we are just doing all functions this funny way because the program contains function references)
     PrintMain("%d, ",pcount);
     PrintMain("%s",tempbuf);
    }

    free(tempbuf);  
    if(pcount){
     trans_main_p -= 2;
     PrintMain(" } )");
    }else{
     PrintMain(" )");
    }

   }else if(func->num_params){

    int i;
    char **value_strings = calloc(func->num_params+1, sizeof(char**));
    TransBuf tmhold = SaveTM();

   
    for(i=0; i<func->num_params; i++){
     value_strings[i] = calloc(1024,sizeof(char));
     SetTM(value_strings[i], 0);
     translate_value(prog, p);
    }

    LoadTM(tmhold);
    for( i = (trans_reverse_function_params ? func->num_params-1 : 0);
         (trans_reverse_function_params ? i>=0 : i<func->num_params );
         i += (trans_reverse_function_params ? -1 : 1)
    ){
     PrintMain("%s",value_strings[i]);
     if( trans_reverse_function_params ? i : i < func->num_params-1 ){
      PrintMain(",");
     }
     free(value_strings[i]);
    }
   
    free(value_strings);    

    PrintMain(")");
   }else{
    PrintMain(")");
   }

  } break; // end of t_Ff
 case t_leftb:
  { 
   *p += 1;
   translate_value(prog, p);
   if( prog->tokens[ *p ].type != t_rightb ){
    fprintf(stderr,"translate_value: bad brackets\n");
    exit(0);
   }
   *p += 1;
  } break;
 case t_abs:
  {
   PrintMain("Johnson_abs(");
   *p += 1;
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_int:
  {
   PrintMain("(double)(int)(");
   *p += 1;
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_neg:
  {
   PrintMain("-(");
   *p += 1;
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_sgn:
  {
   PrintMain("Johnson_sgn(");
   *p += 1;
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_alloc:
  {
   PrintMain("Johnson_alloc(");
   *p += 1;
   translate_value(prog, p);
   PrintMain(")");
  } break;
 case t_rnd:
  {
   PrintMain("(double)");
   PrintMain("Rnd(");
   *p += 1;
   translate_value(prog, p);
   PrintMain(")");
  } break;
/*
 case t_:
  {
   PrintMain("");
   *p += 1;
   translate_value(prog, p);
   PrintMain("");
  } break;
*/
  default:
  ErrorOut("translate_value: FAILURE: Not supported or error: '%s'\n",tokenstring(prog->tokens[ *p ]));
 }
 PrintMain(" ");
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

/*
// obsolete
int expression_is_simple(program *prog, int p){
 if( prog->tokens[ p ].type == t_number ){ 
  return 1;
 }
 while( isvalue( prog->tokens[ p ].type ) ){ 
  switch( prog->tokens[ p ].type ){
  case t_D: case t_A: case t_L: case t_P: case t_F: case t_Ff: case t_Df: case t_SS: case t_Af: case t_stackaccess: case t_getref: 
  case t_ascS: case t_valS: case t_lenS: case t_cmpS: case t_instrS: case t_rnd: case t_alloc:
  case t_openin: case t_openout: case t_openup: case t_eof: case t_bget: case t_vget: case t_ptr: case t_ext:
  case t_winw: case t_winh: case t_mousex: case t_mousey: case t_mousez: case t_readkey: case t_keypressed: case t_expose: case t_wmclose: case t_keybuffer:
   return 0;
  case t_id:
   // peek the id, if it's just a number constant then that's okay, otherwise return 0
   {
    token t = trans_peek_id(prog,&prog->tokens[p]);
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
*/

int trans_caseof_num = 0;

void translate_command_forcaseof(program *prog, int *p){
 switch( CurTok.type ){
  case t_nul:
   trans_print_sourcetext_location( prog, *p);
   PrintErr("translate__caseof: error: broken caseof structure: endcase is missing");
   exit(0);
   break;
  case t_deffn:
   trans_print_sourcetext_location( prog, *p);
   PrintErr("translate__caseof: error: Broken/untranslatable caseof structure: function definitions inside caseof blocks can not be supported\n");
   exit(0);
   break;
  default: translate_command(prog, p);
 }
}

void translate__caseof(program *prog, int *p){

 int casenum = trans_caseof_num;
 trans_caseof_num += 1;
 *p += 1;
 
 int CaseofType = translate_determine_valueorstringvalue(prog, *p); // 0 is value, 1 is stringvalue;
 
 if(CaseofType){
  if(CurTok.type != t_stringconst){
   PrintVarp("SVR *J_caseof_%d_val_svr=NULL;\n",casenum);
   PrintVars("J_caseof_%d_val_svr = calloc(1,sizeof(SVR));\n",casenum);

   PrintMain("{ // caseof block (stringvalue type, non-constant)\nSetSVR(J_caseof_%d_val_svr,",casenum);
   translate_stringvalue(prog, p); 
   PrintMain(");\nSVL J_caseof_%d_val_svl = SVRtoSVL(J_caseof_%d_val_svr);\n",casenum,casenum);
  }else{
   PrintMain("{ // caseof block (stringvalue type, constant)\nSVL J_caseof_%d_val_svl = \n",casenum);
   translate_stringvalue(prog, p); 
   PrintMain(";\n");
  }
 }else{
  PrintMain("{ // caseof block (value type)\ndouble J_caseof_%d_val = ",casenum);
  translate_value(prog, p);
  PrintMain(";\n");
 }
 int thislevel=0;
 while( *p < prog->length && (thislevel || (CurTok.type != t_when && CurTok.type != t_otherwise && CurTok.type != t_endcase)) ){
  if( CurTok.type == t_caseof  ) thislevel += 1; 
  if( CurTok.type == t_endcase ) thislevel -= 1;
  if( CurTok.type == t_deffn || CurTok.type == t_var || CurTok.type == t_stringvar || CurTok.type == t_const || CurTok.type == t_label ){
   ErrorOut("translate__caseof: error: Broken caseof structure: unsupportable garbage in void area after caseof value\n");
  }
  *p += 1;
 }
 if( *p >= prog->length ) ErrorOut("translate__caseof: error: went to end of program skipping void area after caseof value\n");

 int otherwise=-1;
 int whencount=0;
 while( *p < prog->length ){ 
  switch( CurTok.type ){
  case t_otherwise:
   {
    if( otherwise != -1 ){ 
     trans_print_sourcetext_location( prog, *p);
     PrintErr("warning: caseof structure with multiple 'otherwise's\n");
    }
    otherwise = *p;
    *p += 1;
    while( CurTok.type != t_when && CurTok.type != t_endcase && CurTok.type != t_otherwise ){
     *p += 1;
    }
   } break;
  case t_endcase:
   {
    *p += 1;
    if( otherwise != -1 ){
     int holdp = *p;
     *p = otherwise+1;
     while( CurTok.type != t_endcase && CurTok.type != t_when ){
      translate_command_forcaseof(prog, p);
     }//endwhile 
     *p = holdp;
    }
    PrintMain("J_caseof_%d_endcase:;\n} // end caseof block\n",casenum);
    return;
   } break;
  case t_when:
   {
    int c=0;
    whencount += 1;
    PrintMain("if( ");
    *p += 1;
    while( translate_determine_valueorstringvalue_(prog, *p,0) == CaseofType ){
     c += 1;  
     if(CaseofType){
      if(trans_reverse_function_params){
       PrintMain("!CmpS( J_caseof_%d_val_svl,",casenum);
       translate_stringvalue(prog,p);
       PrintMain(",++SAL)");
      }else{
       PrintMain("!CmpS( ++SAL, J_caseof_%d_val_svl,",casenum);
       translate_stringvalue(prog,p);
       PrintMain(")");
      }
      if( CurTok.type != t_endstatement ){
       PrintMain("|| ");
      }//endif
     }else{
      PrintMain("J_caseof_%d_val ==",casenum);
      translate_value(prog,p);
      if( CurTok.type != t_endstatement ){
       PrintMain("|| ");
      }//endif
     }//endif caseoftype
    }//endwhile
    if( CurTok.type != t_endstatement ){
     trans_print_sourcetext_location( prog, *p);
     PrintErr("translate__caseof: error: 'when' list not terminated with ';'\n");
     exit(0);
    }
    if( c == 0 ){
     trans_print_sourcetext_location( prog, *p);
     PrintErr("translate__caseof: error: no values after 'when'\n");
     exit(0);
    }
    PrintMain("){\n");

    while( CurTok.type != t_endcase && CurTok.type != t_when && CurTok.type != t_otherwise ){
     translate_command_forcaseof(prog, p);
    }//endwhile 

    PrintMain("goto J_caseof_%d_endcase;\n}\n",casenum);

   } break;
  default:
   trans_print_sourcetext_location( prog, *p);
   PrintErr("translate__caseof: error: this should never happen\n");
   exit(0);
  }//endswitch
 }//endwhile

 trans_print_sourcetext_location( prog, *p); 
 PrintErr("translate__caseof: error: it should not be possible for this to happen");
 exit(0);

}//endproc


int TheseStringsMatch(char *a, char *b){
 while( *a && *b ){
  if( *a != *b ) return 0;
  a++;b++;
 }
 return (*a == *b);
}

void translate_command(program *prog, int *p){
 #if TRANS_CRASHDEBUG
 fprintf(stderr,"translate_command: %s\n",tokenstring(prog->tokens[ *p ]));
 #endif
 trans_com_start:
 switch( prog->tokens[ *p ].type ){
 case t_for:
  {
   *p += 1;
   if( CurTok.type != t_id ){
    ErrorOut("for: not given an id for a loop variable\n");
   }
   translate_processid(prog, p);
   switch( CurTok.type ){
    case t_Df: case t_stackaccess: break;
    default: {
     ErrorOut("for: not given a loop variable\n");
    }
   }
   char *loopvar = trans__transval_into_tempbuf( prog, p );
   char *fromval = NULL;
   char *toval = NULL;
   char *stepval = NULL;
   // from val
   if( expressionIsSimple( prog, *p ) ){
    fromval = trans__double_into_tempbuf(getvalue(p,prog));
   }else{
    fromval = trans__transval_into_tempbuf( prog, p );
   }
   // to val
   int to_is_constant = 0;
   if( expressionIsSimple( prog, *p ) ){
    to_is_constant=1;
    toval = trans__double_into_tempbuf(getvalue(p,prog));
   }else{
    toval = trans__transval_into_tempbuf( prog, p );
   }
   // step val
   int step_is_constant = 0;
   if( CurTok.type != t_endstatement ){
    if( expressionIsSimple( prog, *p ) ){ 
     step_is_constant=1;
     stepval = trans__double_into_tempbuf(getvalue(p,prog));
    }else{
     stepval = trans__transval_into_tempbuf( prog, p );
    }
   }else{
    step_is_constant = 1;
    stepval = trans__double_into_tempbuf( 1.0 );
   }
   // ok
   PrintMain( "{\ndouble fromval = %s; double toval = %s; double stepval = %s; %s = fromval;\n", fromval, toval, stepval, loopvar );
   PrintMain( "if(stepval==0){ fprintf(stderr,\"for step can't be 0\\n\"); exit(1); }\n" );
   PrintMain( "while( stepval>0 ? %s<=toval : %s>=toval ){\n",loopvar,loopvar  );
   while( CurTok.type != t_endfor ){
    translate_command(prog,p);
   }
   {
    PrintMain("{\n%s += %s;\n",loopvar,stepval);
    if(!step_is_constant) PrintMain("stepval = %s;\n",stepval);
    if(!to_is_constant)   PrintMain("toval = %s;\n",toval);
    PrintMain("}\n");
   }
   PrintMain("} // endfor\n}\n");
   *p += 1;
   free(loopvar);
   free(fromval);
   free(toval);
   free(stepval);
  } break;
 case t_endfor:
  {
   ErrorOut("'endfor' encountered, this is not supposed to happen. likely a broken 'for' loop structure\n");
  } break;
 case t_oscli:
  {
   *p += 1;
   PrintMain("{\nSVL J_oscli_svl =");
   translate_stringvalue(prog,p);
   PrintMain(";\nchar J_oscli_svl_hold=J_oscli_svl.buf[J_oscli_svl.len]; J_oscli_svl.buf[J_oscli_svl.len]=0;\n");
   PrintMain("int gcc_please_dont_complain=system(J_oscli_svl.buf);\n");
   PrintMain("J_oscli_svl.buf[J_oscli_svl.len]=J_oscli_svl_hold;\n}\n");
  } break;
 case t_wait:
  {
   *p += 1;
   #ifdef enable_graphics_extension
   if( trans_program_uses_gfx ){
    PrintMain("{ // 'wait' command\n");
    PrintMain("double waitv = "); translate_value(prog,p); PrintMain(";\n");
    PrintMain("if( newbase_is_running && newbase_allow_fake_vsync && ((int)waitv==16) ){\n");
    PrintMain("newbase_fake_vsync_request=1;\nwhile( newbase_fake_vsync_request ) usleep(4);\n");
    PrintMain("} else usleep((int)( waitv*1000 ));\n");
    PrintMain("}\n");
   }else{
    PrintMain("usleep( 1000 * (");
    translate_value(prog,p);
    PrintMain(") );\n");
   }
   #else
   PrintMain("usleep( 1000 * (");
   translate_value(prog,p);
   PrintMain(") );\n");
   #endif
  } break;
 #ifdef enable_graphics_extension 
 case t_startgraphics: case t_stopgraphics: case t_winsize: case t_pixel: case t_line: case t_circlef: case t_circle: case t_arcf: case t_arc: case t_rectanglef: case t_rectangle: case t_triangle: case t_drawtext: case t_drawscaledtext:
 case t_refreshmode: case t_refresh: case t_gcol: case t_bgcol: case t_cls: case t_drawmode:
  {
   trans_program_uses_gfx = 1;
   switch( CurTok.type ){
   case t_startgraphics: case t_winsize:
    {
     int type = CurTok.type;
     *p += 1;
     char *width  = trans__transval_into_tempbuf( prog, p );
     char *height = trans__transval_into_tempbuf( prog, p );
     if( trans_reverse_function_params ){
      PrintMain("%s(%s,%s);\n",( type==t_startgraphics?"Johnson_StartGraphics":"Johnson_WinSize" ),height,width);
     }else{
      PrintMain("%s(%s,%s);\n%s",( type==t_startgraphics?"start_newbase_thread":"SetWindowSize" ),width,height,type==t_startgraphics?"":"Wait(1);\n");
     }
     free(width);
     free(height);
    }
    break;
   case t_stopgraphics: 
    *p += 1;
    PrintMain("MyCleanup();\n");
    break;
   case t_pixel: 
    {
     *p += 1;
     int count = 0;
     PrintMain("Johnson_Pixel( (int[]){");
     do {
      translate_value(prog,p);
      PrintMain(",");
      count += 1;
     } while( isvalue( CurTok.type ) );
     trans_main_p -= 1;
     PrintMain("}, %d);\n",count);
     if( ! count ){
      ErrorOut("translate_command: no arguments given to pixel\n");
     }
     if(count & 1){
      ErrorOut("translate_command: bad arguments to 'pixel': uneven number of arguments\n");
     }
    }
    break;
   case t_line: 
    {
     *p += 1;
     int count = 0;
     PrintMain("Johnson_Line( (int[]){");
     do {
      translate_value(prog,p);
      PrintMain(",");
      count += 1;
     } while( isvalue( CurTok.type ) );
     trans_main_p -= 1;
     PrintMain("}, %d);\n",count);
     if( count < 4 ){
      ErrorOut("translate_command: not enough arguments given to line\n");
     }
     if(count & 1){
      ErrorOut("translate_command: bad arguments to 'line': uneven number of arguments\n");
     }
    }
    break;
   case t_circlef: case t_circle: 
    {
     int fill = (CurTok.type == t_circlef);
     *p += 1;
     char *x  = trans__transval_into_tempbuf( prog, p );
     char *y  = trans__transval_into_tempbuf( prog, p );
     char *r  = trans__transval_into_tempbuf( prog, p );
     if( trans_reverse_function_params ){
      PrintMain("Johnson_Circle(%d,%s,%s,%s);\n",fill,r,y,x);
     }else{
      PrintMain("Circle%s(%s,%s,%s);\n",fill?"Fill":"",x,y,r);
     }
     free(x); free(y); free(r);
    }
    break;
   case t_arcf: case t_arc:
    {
     int fill = (CurTok.type == t_arcf);
     *p += 1;
     char *x, *y, *rx, *ry, *start_angle, *extent_angle;
     x = trans__transval_into_tempbuf( prog, p );
     y = trans__transval_into_tempbuf( prog, p );
     rx = trans__transval_into_tempbuf( prog, p );
     ry = trans__transval_into_tempbuf( prog, p );
     start_angle = trans__transval_into_tempbuf( prog, p );
     extent_angle = trans__transval_into_tempbuf( prog, p );
     if( trans_reverse_function_params ){
      PrintMain("{\nint arc_x,arc_y,arc_rx,arc_ry; double arc_start,arc_extent;\narc_x=%s;\narc_y=%s;\narc_rx=%s;\narc_ry=%s;\narc_start=%s;\narc_extent=%s;\n",
                x, y, rx, ry, start_angle, extent_angle );
      PrintMain("Arc(arc_x, arc_y, arc_rx, arc_ry, arc_start, arc_extent, %d);\n}\n",fill);
     }else{
      PrintMain("Arc(%s,%s,%s,%s,%s,%s,%d);\n",x,y,rx,ry,start_angle,extent_angle,fill);
     }
     free(x); free(y); free(rx); free(ry); free(start_angle); free(extent_angle); 
    }
    break;
   case t_rectanglef: case t_rectangle: 
    {
     int fill = (CurTok.type == t_rectanglef);
     *p += 1;
     char *x  = trans__transval_into_tempbuf( prog, p );
     char *y  = trans__transval_into_tempbuf( prog, p );
     char *w  = trans__transval_into_tempbuf( prog, p );
     char *h  = NULL;
     if( isvalue(CurTok.type) ){
      h  = trans__transval_into_tempbuf( prog, p );
     }
     if( h ){ 
      if( trans_reverse_function_params ){
       PrintMain("Johnson_Rectangle(%d,%s,%s,%s,%s);\n",fill,h,w,y,x);
      }else{
       PrintMain("Rectangle%s(%s,%s,%s,%s);\n",fill?"Fill":"",x,y,w,h);
      }
      free(h);
     }else{
      if( trans_reverse_function_params ){
       PrintMain("Johnson_RectangleWH(%d,%s,%s,%s);\n",fill,w,y,x);
      }else{
       PrintMain("Johnson_RectangleWH(%s,%s,%s,%d);\n",x,y,w,fill);
      }
     }
     free(x); free(y); free(w);
    }
    break;
   case t_triangle: 
    *p += 1;
    PrintMain("Johnson_Triangle( (int[]){");
    translate_value(prog, p); PrintMain(","); translate_value(prog, p); PrintMain(",");
    translate_value(prog, p); PrintMain(","); translate_value(prog, p); PrintMain(",");
    translate_value(prog, p); PrintMain(","); translate_value(prog, p);
    PrintMain("} );\n");
    break;
   case t_drawtext:
    {
     *p += 1;
     char *x  = trans__transval_into_tempbuf( prog, p );
     char *y  = trans__transval_into_tempbuf( prog, p );
     char *s  = trans__transval_into_tempbuf( prog, p );
     if( trans_reverse_function_params ){
      PrintMain("Johnson_DrawText(");
      translate_stringvalue(prog,p);
      PrintMain(",%s,%s,%s,++SAL);\n",s,y,x);
     }else{
      PrintMain("Johnson_DrawText(++SAL,%s,%s,%s,",x,y,s);
      translate_stringvalue(prog,p);
      PrintMain(");\n");
     }
     free(x); free(y); free(s);
    }
    break;
   case t_drawscaledtext:
    {
     *p += 1;
     char *x,*y,*xs,*ys;
     x  = trans__transval_into_tempbuf( prog, p );
     y  = trans__transval_into_tempbuf( prog, p );
     xs = trans__transval_into_tempbuf( prog, p );
     ys = trans__transval_into_tempbuf( prog, p );
     PrintMain("{\n int x = %s;\nint y = %s;\nint xs = %s;\nint ys = %s;\n",x,y,xs,ys);
     PrintMain("SVL str ="); translate_stringvalue(prog,p); PrintMain("; int holdthis = str.buf[str.len]; str.buf[str.len]=0;\n");
     PrintMain("drawscaledtext(x,y,xs,ys,str.buf);\n");
     PrintMain("str.buf[str.len] = holdthis;\n");
     PrintMain("}\n");
     free(x); free(y); free(xs); free(ys);
    }
    break;
   case t_refreshmode:
    *p += 1;
    PrintMain("Johnson_RefreshMode(");
    translate_value(prog,p);
    PrintMain(");\n");
    break;
   case t_refresh:
    *p+=1;
    PrintMain("Refresh();\n");
    break;
   case t_gcol: case t_bgcol:
    {
     int bgtype = (CurTok.type == t_bgcol);
     *p+=1;
     char *r  = trans__transval_into_tempbuf( prog, p );
     char *g = NULL;
     char *b = NULL;
     if( isvalue(CurTok.type) ){
      g  = trans__transval_into_tempbuf( prog, p );
      b  = trans__transval_into_tempbuf( prog, p );
     }
     if(g){
      if( trans_reverse_function_params ){
       PrintMain( "Johnson_%s(%s,%s,%s);\n", ( bgtype?"BGCol":"GCol" ),b,g,r );
      }else{ 
       PrintMain( "%s(%s,%s,%s);\n", ( bgtype?"GcolBG":"Gcol" ),r,g,b );
      }
      free(g); free(b);
     }else{
      PrintMain( "%s( MyColour2( %s ) );\n", (bgtype?"GcolBGDirect":"GcolDirect"), r );
     }
     free(r);
    }
    break;
   case t_cls:
    *p += 1;
    PrintMain("Cls();\n");
    break;
   case t_drawmode:
    *p += 1;
    PrintMain("SetPlottingMode(");
    translate_value(prog,p);
    PrintMain(");\n");
    break;
   }
  } break;
 #endif
 case t_close:
  {
   *p += 1;
   PrintMain("Johnson_Close(");
   translate_value(prog, p);
   PrintMain(");");
  } break;
 case t_sput:
  {
   *p += 1;
   PrintMain("{\nint file_handle =");
   translate_value(prog, p);
   PrintMain(";\n");
  
   do {
    PrintMain("Johnson_Sput( file_handle,");
    translate_stringvalue(prog, p);
    PrintMain(");\n");
   }while( isstringvalue( prog->tokens[*p].type ) );

   PrintMain("}\n");
  } break;
 case t_vput:
  {
   *p += 1;
   PrintMain("Johnson_Vput(");
   translate_value(prog, p);
   PrintMain(",");
   translate_value(prog, p);
   PrintMain(");");
  } break;
 case t_bput:
  {
   *p += 1;
   char *fp = trans__transval_into_tempbuf( prog, p );
   char *v  = trans__transval_into_tempbuf( prog, p );
   if( isvalue(CurTok.type) ){
    PrintMain("{\ndouble fp = %s;\n",fp);
    PrintMain("Johnson_Bput(fp,%s);",v);
    do{
     PrintMain("Johnson_Bput(fp,"); translate_value(prog, p); PrintMain(");\n");
    }while( isvalue(CurTok.type) );
    PrintMain("}\n");
   }else{
    PrintMain("Johnson_Bput(%s,%s);",fp,v);
   }
   free(fp); free(v);   
  } break;
 case t_sptr:
  {
   *p += 1;
   PrintMain("Johnson_Sptr(");
   translate_value(prog, p);
   PrintMain(",");
   translate_value(prog, p);
   PrintMain(");");
  } break;
 case t_option:
  {
   *p += 1;
   if( CurTok.type != t_stringconst ){
    trans_print_sourcetext_location( prog, *p);
    PrintErr("translate_command: option: not supported or broken option command\n");
    exit(0);
   }
   char *opstr = (char*)CurTok.data.pointer;
   *p += 1;
   if( TheseStringsMatch(opstr, "unclaim") ){
    PrintMain("StringVars[(int)");
    translate_value(prog, p);
    PrintMain("]->claimed = 0;\n");
   }else if( TheseStringsMatch(opstr, "vsize") ){
    if( expressionIsSimple( prog, *p ) ){
     trans_vsize = 1+(int)getvalue(p,prog);
    }else{
     trans_print_sourcetext_location( prog, *p);
     PrintErr("translate_command: option: 'vsize': only a static/simple expression can be accepted\n");
     exit(0);
    }//endif valid vsize
   }else if( TheseStringsMatch(opstr, "ssize") ){
    trans_print_sourcetext_location( prog, *p);
    PrintErr("translate_command: option: warning: 'ssize' cannot be supported\n");
    PrintMain("/*\n there was an 'option ssize' here: \n");
    translate_value(prog, p);
    PrintMain("\n*/\n");
   }else if( TheseStringsMatch(opstr, "wintitle") ){
    #ifdef enable_graphics_extension 
    PrintMain("{\nSVL J_wintitle_svl = ");
    translate_stringvalue(prog,p);
    PrintMain(";\nchar J_wintitle_svl_hold = J_wintitle_svl.buf[J_wintitle_svl.len]; J_wintitle_svl.buf[J_wintitle_svl.len]=0;\n");
    PrintMain("SetWindowTitle(J_wintitle_svl.buf);\n");
    PrintMain("J_wintitle_svl.buf[J_wintitle_svl.len] = J_wintitle_svl_hold;\n}\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "wmclose") ){
    #ifdef enable_graphics_extension 
    PrintMain("wmcloseaction = ");
    translate_value(prog,p);
    PrintMain(";\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "seedrnd") ){
    PrintMain("Johnson_Seedrnd( (int[3]){");
    int i;
    for(i=0; i<3; i++){
     if( translate_determine_valueorstringvalue_(prog, *p, 0) == 0 ){
      translate_value(prog,p);
     }else{
      PrintMain("0");
     }
     if(i<2) PrintMain(",");
    }
    PrintMain("} );\n");
   }else if( TheseStringsMatch(opstr, "copytext") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ SVL s = ");
    translate_stringvalue(prog,p);
    PrintMain("; NB_CopyTextN( s.buf, s.len ); } \n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "pastetext") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ int n = (int)");
    translate_value(prog,p);
    PrintMain("; if( NB_PasteText() ){ if( StringVars[n]->bufsize < PasteBufferContentsSize ) StringVars[n]->buf = realloc(StringVars[n]->buf, PasteBufferContentsSize); memcpy(StringVars[n]->buf,(void*)PasteBuffer, PasteBufferContentsSize); StringVars[n]->len = PasteBufferContentsSize; } else StringVars[n]->len = 0; }\n ");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "setcliprect") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ int x,y,w,h; ");
    PrintMain("x = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("y = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("w = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("h = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("SetClipRect(x,y,w,h); }\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "clearcliprect") ){
    #ifdef enable_graphics_extension 
    PrintMain("ClearClipRect();\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "drawbmp") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ SVL bmpstring; int x,y; ");
    PrintMain("bmpstring = "); translate_stringvalue(prog,p); PrintMain("; ");
    PrintMain("x = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("y = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("if( Johnson_BmpValidityCheck(bmpstring) ) NB_DrawBmp(x,y,0,0,-1,-1,(Bmp*)bmpstring.buf); }\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "drawbmpadv") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ SVL bmpstring; int x,y,sx,sy,w,h; ");
    PrintMain("bmpstring = "); translate_stringvalue(prog,p); PrintMain("; ");
    PrintMain("x  = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("y  = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("sx = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("sy = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("w  = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("h  = "); translate_value(prog,p); PrintMain("; ");
    PrintMain("if( Johnson_BmpValidityCheck(bmpstring) ) NB_DrawBmp(x,y,sx,sy,w,h,(Bmp*)bmpstring.buf); }\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "copybmp") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ SVL s = ");
    translate_stringvalue(prog,p);
    PrintMain("; if( s.len > 54 ) NB_CopyBmpN( s.buf, s.len ); } \n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "pastebmp") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ int n = (int)");
    translate_value(prog,p);
    PrintMain("; if( NB_PasteBmp() ){ if( StringVars[n]->bufsize < PasteBufferContentsSize ) StringVars[n]->buf = realloc(StringVars[n]->buf, PasteBufferContentsSize); memcpy(StringVars[n]->buf,(void*)PasteBuffer, PasteBufferContentsSize); StringVars[n]->len = PasteBufferContentsSize; } else StringVars[n]->len = 0; }\n ");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "hascliptext") ){
    #ifdef enable_graphics_extension 
    PrintMain("FirstVarP[(int)("); translate_value(prog,p); PrintMain(")] = NB_HasClipText();\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "hasclipbmp") ){
    #ifdef enable_graphics_extension 
    PrintMain("FirstVarP[(int)("); translate_value(prog,p); PrintMain(")] = NB_HasClipBmp();\n");
    #else
    ErrorOut("FUCK OFF!\n");
    #endif
   }else if( TheseStringsMatch(opstr, "customchar") ){
    #ifdef enable_graphics_extension 
    PrintMain("{ // option \"customchar\"\nint character_number; unsigned char glyph_bytes[8];\ncharacter_number =");
    translate_value(prog,p); PrintMain(";\n");
    int i;
    for(i=0; i<8; i++){
     PrintMain("glyph_bytes[%d] = ",i); translate_value(prog,p); PrintMain(";\n");
    }
    PrintMain("if( !( character_number & ~0xff ) ){\n");
    PrintMain("CustomChar( character_number, glyph_bytes[0],glyph_bytes[1],glyph_bytes[2],glyph_bytes[3],glyph_bytes[4],glyph_bytes[5],glyph_bytes[6],glyph_bytes[7] );\n");
    PrintMain("}\n}\n");
    #else
    ErrorOut("PENIS\n");
    #endif
   }else if( TheseStringsMatch(opstr, "startgraphics") ){
    #ifdef enable_graphics_extension 
    PrintMain("/* option \"startgraphics\" */\n{\nint startgraphics_w = "); translate_value(prog,p);
    PrintMain(";\nint startgraphics_h = "); translate_value(prog,p); 
    PrintMain(";\nNewBase_MyInit(startgraphics_w,startgraphics_h,0);usleep(1000);\n}\n");
    #else
    ErrorOut("PENIS\n");
    #endif
   }else if( TheseStringsMatch(opstr, "xupdate") ){
    #ifdef enable_graphics_extension
    if( translate_determine_valueorstringvalue_(prog, *p, 0) == 0 ){
     PrintMain("/* option \"xupdate\" */NewBase_HandleEvents("); 
     translate_value(prog,p);
     PrintMain(");\n");
    }else{
     PrintMain("NewBase_HandleEvents(0);\n");
    }
    PrintMain("XFlush(Mydisplay);\n");
    #else
    ErrorOut("PENIS\n");
    #endif
   }else if( TheseStringsMatch(opstr, "xresource") ){
    #ifdef enable_graphics_extension
    PrintMain("{\n/* option \"xresource\" */\nSVR *dest = StringVars[(int)");
    translate_value(prog, p);
    PrintMain("];\nSVL a = ");
    translate_stringvalue(prog,p);
    PrintMain(";\nSVL b = ");
    translate_stringvalue(prog,p);
    PrintMain(";\nunsigned char h_a = a.buf[a.len], h_b=b.buf[b.len]; a.buf[a.len]=0; b.buf[b.len]=0;\n");
    PrintMain("char *xresource_result = NewBase_GetXResourceString(a.buf, b.buf); if(xresource_result) {\nint l = strlen(xresource_result); ExpandSVR( dest, l+1 );\n");
    PrintMain("strcpy(dest->buf, xresource_result); dest->len=l; free(xresource_result);\n}else{\ndest->len=0;\n}\na.buf[a.len]=h_a; b.buf[b.len]=h_b;\n}\n");
    #else
    ErrorOut("PENIS\n");
    #endif
   }else{
    trans_print_sourcetext_location( prog, *p);
    PrintErr("translate_command: option: unrecognised option '%s'\n",opstr);
    exit(0);
   }
  } break;
 case t_id:
  translate_processid(prog, p);
  goto trans_com_start;
  break;
 case t_caseof:
  translate__caseof(prog, p);
  break;
 case t_quit:
  {
   *p += 1;
   PrintMain("exit(");
   if( isvalue( prog->tokens[ *p ].type ) ){
    translate_value(prog, p);
   }else{
    PrintMain("0");
   }
   PrintMain(");\n");
  } break;
 case t_label:
  {
   PrintMain("Label_%s:\n",(char*)(1+prog->tokens[ *p ].data.pointer));
   *p += 1;
  } break;
 case t_goto:
 {
  *p += 1;
  if( prog->tokens[ *p ].type != t_stringconst ){
   trans_print_sourcetext_location( prog, *p);
   fprintf(stderr,"translate_command: goto with non constant string can not be supported\n");
   exit(0);
  }
  PrintMain("goto Label_%s;\n",(char*)(1+prog->tokens[ *p ].data.pointer));
  *p += 1;
 } break;
 case t_Ff:
  translate_value(prog, p);
  PrintMain(";\n");
  break;
 case t_endfn:
  *p += 1;
  PrintMain("return 0.0;\n");
  break;
 case t_while:
  {
   PrintMain("while(");
   *p += 1;
   translate_value(prog, p);
   PrintMain("){\n");
  } break;
 case t_endwhile: case t_endif:
  {
   PrintMain("}\n");
   *p += 1;
  } break;
 case t_if:
  {
   PrintMain("if(");
   *p += 1;
   translate_value(prog, p);
   PrintMain("){\n");
  } break;
 case t_else:
  {
   PrintMain("}else{\n");
   *p += 1;
  } break;
 case t_appendS:
  {
   *p += 1;
   if( CurTok.type == t_id ) translate_processid(prog, p);
   PrintMain("{ // append$\nSVR *appendTo = ");
   if( CurTok.type == t_Sf ){
    TransTable *ttab = (TransTable*)prog->tokens[ *p ].data.pointer;
    if(!ttab->set){
     ErrorOut("append$: error: '%s'\n", ttab->set_s);
    }
    PrintMain("%s ;\n",ttab->set_s);
    *p += 1;
   }else if(CurTok.type == t_S){
    *p += 1;
    PrintMain("Johnson_stringvar_deref( ");   
    translate_value(prog, p);
    PrintMain(");\n");   
   }else{
    ErrorOut( "bad use of append$" );
   }
   do{
    PrintMain("AppendS( appendTo, ");
    translate_stringvalue(prog, p);
    PrintMain(");\n");
   }while( isstringvalue(CurTok.type) );
   PrintMain("}\n");
  } break;
 case t_set: 
  {
   // ==================================
   *p += 1;
   trans_com_setstart:
   #if TRANS_CRASHDEBUG
   fprintf(stderr," set: %s\n",tokenstring(prog->tokens[ *p ]));
   #endif 
   switch( prog->tokens[ *p ].type ){
   case t_id:
    {
     translate_processid(prog, p);
     goto trans_com_setstart;
    } break;
   // ----- set strings -------
   case t_Sf:
    {
     TransTable *ttab = (TransTable*)prog->tokens[ *p ].data.pointer;

     if(!ttab->set){
      trans_print_sourcetext_location( prog, *p);
      fprintf(stderr,"translate command: set: t_Sf: error: '%s'\n", ttab->set_s );
      exit(0);
     }
  
     PrintMain("SetSVR(%s,",ttab->set_s);   
     *p += 1;
     translate_stringvalue(prog, p);
     PrintMain(");\n");

    } break;
   case t_S:
    {
     *p += 1;
     PrintMain("SetSVR( Johnson_stringvar_deref( ");   
     translate_value(prog, p);
     PrintMain("),");   
     translate_stringvalue(prog, p);
     PrintMain(") ;\n");   
    } break;
   // -------------------------
   case t_P:
    {
     translate_value(prog, p);
     PrintMain("=");
     translate_value(prog, p);
     PrintMain(";\n");
    } break;
   case t_A:
    {
     translate_value(prog, p);
     PrintMain(" = ");
     translate_value(prog, p);
     PrintMain(";\n");
    } break;
   case t_C:
    {
     PrintMain(" *(&");
     translate_value(prog, p);
     PrintMain(") = (char)(int) ");
     translate_value(prog, p);
     PrintMain(";\n");
    } break;
   case t_V:
    {
     PrintMain(" *(&");
     translate_value(prog, p);
     PrintMain(") = ");
     translate_value(prog, p);
     PrintMain(";\n");
    } break;
   case t_D:
    {
     *p += 1;
     PrintMain("FirstVarP[(int)(");
     translate_value(prog, p);
     PrintMain(")] = ");
     translate_value(prog, p);
     PrintMain(";\n");
    } break;
   case t_Df:
    {
     TransTable *ttab = ((TransTable*)prog->tokens[ *p ].data.pointer);

     if(!ttab->set){
      trans_print_sourcetext_location( prog, *p);
      fprintf(stderr,"translate command: set: t_Df: error: '%s'\n", ttab->set_s );
      exit(0);
     }

     PrintMain("%s ",ttab->set_s);
     *p += 1;

     PrintMain("=");
     translate_value(prog, p);
     PrintMain(";\n");
    } break;
   default:
    trans_print_sourcetext_location( prog, *p);
    fprintf(stderr,"translate_command: set: error or not supported, '%s'\n",tokenstring(prog->tokens[ *p ]));
    exit(0);
   }
   // ==================================
  } break;
 case t_stringvar:
  {
   int var_token_p = *p;
   *p += 1;
   while( prog->tokens[ *p ].type != t_endstatement ){
    // ---- get id and process ----
    if( prog->tokens[ *p ].type != t_id ){
     trans_print_sourcetext_location( prog, *p);
     fprintf(stderr,"translate_command_ stringvar: bad stringvar declaration or unsupported feature\n");
     exit(0);
    }
    char *idstring = (char*)prog->tokens[ *p ].data.pointer;
    Declaration *findd = FindDecl(idstring); //
    if(findd){
     trans_print_sourcetext_location( prog, *p);
     fprintf(stderr,"translate_command: stringvar: this id '%s' is already in use as a '%s'\n",idstring,findd->desc);
     exit(0);
    }
    Declaration *maked = MakeDecl(0,0,"StringVariable");
    SetGLPos(maked, prog, *p);
    AddDecl( maked, idstring );

    // prepare declaration in the translated c program
    PrintVarp("SVR *SVAR_%s = NULL;\n",idstring);            
    PrintVars("SVAR_%s = NewStringvar_svrp();\n",idstring );

    // prepare translation table and add this to the program id list
    char *value_s = calloc(strlen(idstring)+15,1);
    char *getref_s = calloc(strlen(idstring)+30,1);
    char *set_s = calloc(strlen(idstring)+10,1);
    sprintf(value_s,"SVRtoSVL(SVAR_%s)",idstring);
    sprintf(getref_s,"(SVAR_%s->string_variable_number)",idstring);
    sprintf(set_s,"SVAR_%s",idstring);

    TransTable *ttab = MakeTransTable(1,value_s, 1,getref_s, 1,set_s, NULL);

    trans_add_id(prog->ids, make_id( idstring, maketoken_Sf( (stringvar*)ttab ) ) );
    
    *p += 1;
   }
   //tb();
  } break;
 case t_var:
  {
   int var_token_p = *p;
   *p += 1;
   while( prog->tokens[ *p ].type == t_id ){
    char *idstring = (char*)prog->tokens[ *p ].data.pointer;

    Declaration *findd = FindDecl(idstring);
    if(findd){
     trans_print_sourcetext_location( prog, *p);
     fprintf(stderr,"translate_command: variable: this id '%s' is already in use as a '%s'\n",idstring,findd->desc);
     exit(0);
    }
    Declaration *maked = MakeDecl(0,0,"Variable");
    SetGLPos(maked, prog, *p);
    AddDecl( maked, idstring );

    PrintVarp("double *VAR_%s = NULL;\n",idstring);
    PrintVars("VAR_%s = &FirstVarP[ (int)Johnson_alloc(1) ] ;\n",idstring );
    // put here: add to global id list 

    char *value_s = calloc(strlen(idstring)+7,1);
    char *getref_s = calloc(strlen(idstring)+30,1);
    sprintf(value_s,"*VAR_%s",idstring);
    sprintf(getref_s,"Johnson_getref(VAR_%s)",idstring);

    TransTable *ttab = MakeTransTable(1,value_s, 1,getref_s, 1,value_s, NULL);

    trans_add_id(prog->ids, make_id( idstring, maketoken_Df( (double*)ttab ) ) );
    *p += 1;
    trans_var_n += 1;
   }
   if( prog->tokens[ *p ].type != t_endstatement ){
    trans_print_sourcetext_location( prog, *p);
    fprintf(stderr,"translate_command: bad variable declaration\n");
    exit(0);
   }
   *p += 1;
   prog->tokens[ var_token_p ].type = 199;
  } break;
 case 199:
  {
   trans_print_sourcetext_location( prog, *p);
   fprintf(stderr,"it should not be possible for this to happen again because the declaration preprocess pass should clear this away with ';'s\n");
   exit(0);
  } break;
 case t_const:
  {
   int const_token_p = *p;
   *p += 1;
   int const_id_p = *p;
   if( CurTok.type != t_id ){
    ErrorOut("translate_command: t_const: bad constant declaration, wanted an id but got '%s'\n",tokenstring(CurTok));
   }
   char *idstring = (char*)prog->tokens[ *p ].data.pointer;
   *p += 1;

   Declaration *findd = FindDecl(idstring);
   if(findd){
    trans_print_sourcetext_location( prog, *p);
    fprintf(stderr,"translate_command: constant: this id '%s' is already in use as a '%s'\n",idstring,findd->desc);
    exit(0);
   }
   Declaration *maked = MakeDecl(0,0,"Constant");
   SetGLPos(maked, prog, *p);
   AddDecl( maked, idstring );

   if( prog->tokens[ *p ].type == t_number ){ // if it's just a simple constant
    trans_add_id(prog->ids, make_id( idstring, prog->tokens[ *p ] ));
    *p += 1;
    break;
   }
   if( expressionIsSimple( prog, *p ) ){ // if it looks like a very simple expression
    double value = getvalue(p,prog);
    trans_add_id(prog->ids, make_id( idstring, maketoken_num(value) ) );
    break;
   }
   // otherwise we have to do this: 
   PrintVarp("double CON_%s; ",idstring);

   prog->tokens[const_token_p].type = 200; // we will set the value of this constant later during the main translation pass.
 
   TransTable *ttab = calloc(1,sizeof(TransTable));
   *ttab = (TransTable){
			1, calloc(strlen(idstring)+5,1),
                        0, "Trying to ref a constant",
			0, "Trying to set a constant",
			NULL };
   sprintf(ttab->value_s,"CON_%s",idstring);

   trans_add_id(prog->ids, make_id( idstring, maketoken_Df( (double*)ttab ) ) );

  } break;
 case 200:
  {
   *p += 1;
   char *idstring = (char*)prog->tokens[ *p ].data.pointer;
   *p += 1;
   // now we will do it.
   PrintMain("CON_%s = ",idstring);
   translate_value(prog, p);
   PrintMain(";\n"); // Äkta dig för Rövar-Albin
  } break;
 case t_return:
  {
   PrintMain("return ");
   *p += 1;
   translate_value(prog, p);
   PrintMain(";\n");
  } break;
 case t_endstatement:
  {
   while( prog->tokens[ *p ].type == t_endstatement){
    PrintMain(";");
    *p += 1;
   }
   PrintMain("\n");
  } break;
 case t_print:
  {
   *p += 1;
   while( isvalue(CurTok.type) || isstringvalue(CurTok.type) ){
    //tb(); printf("printing: %s\n",tokenstring(CurTok));
    if( translate_determine_valueorstringvalue(prog, *p) ){ //stringvalue
     PrintMain("PrintSVL(");
     translate_stringvalue(prog, p);
     PrintMain(");");
    }else{ //value
     PrintMain("{ double value =");
     translate_value(prog, p);
     PrintMain(";\n");
     PrintMain("if( !Johnson_isnan(value) && value == (double)(int)value ){ printf(\"%%d\",(int)value); }else{ printf(\"%%f\",value); }\n");
     PrintMain("}\n");
    }//endif value or string value
   }//endwhile values and stringvalues
   *p += (CurTok.type == t_endstatement);
   PrintMain("printf(\"\\n\");\n");
  } break;
 case t_deffn:
  {

   *p += 1; // move past 'function'

   if(CurTok.type != t_id){
    ErrorOut("This type of function definition is not supported (1)\n");
   }

   { // let's do this now instead
    prog->current_function = &prog->initial_function;
    token t = trans_peek_id(prog,&prog->tokens[*p]);
    if( t.type != t_Ff ){
     trans_print_sourcetext_location( prog, *p);
     fprintf(stderr,"not good '%s'\n",tokenstring(t));
     exit(0);
    }
    prog->current_function = (func_info*)t.data.pointer; trans_fn = prog->current_function->function_number;
   } 

   if(trans_fn == 0){
    PrintMain("printf(\"\\nJOHNSONSCRIPT WARNING: Reached end of main(). This should not normally happen\\n\");\n}\n\n");
   }else{
    PrintMain("}\n\n");
   }
   PrintMain("double FN_%s(", (char*)prog->tokens[ *p ].data.pointer );
   *p += 1; // move past fn's ID
   if(CurTok.type == t_P || CurTok.type == t_L){
    ErrorOut("This type of function definition is not supported (2)\n");
   }
   // ------- process param list -------
   if( (prog->current_function->params_type == p_exact) && ! trans_program_contains_F ){ // the function takes normal parameter list

    id_info *fn_ids = NULL;
    int i,j;
    for(j=0; j<prog->current_function->num_params; j++){ // get each parameter ID from the function info struct 

     if( trans_reverse_function_params ){ // if we must reverse the parameter list because C on this arch evaluates parameters in reverse order vs johnsonscript
      fn_ids = prog->current_function->ids;
      for(i=prog->current_function->num_params-1-j; i>=0; i--){
       fn_ids = fn_ids->next;
      }
     }else{ // or if we don't have to reverse it after all
      if(fn_ids == NULL) fn_ids = prog->current_function->ids;
      fn_ids = fn_ids->next;
     }

     //PrintErr(" FUCKING SHIT '%s'\n",fn_ids->name); // REM test

     TransTable *ttab = calloc(1,sizeof(TransTable)); // create a translation table for each parameter ID
     *ttab = (TransTable){
		1, calloc( strlen(fn_ids->name)+5, 1),
		1, calloc( strlen(fn_ids->name)+30, 1),
		1, NULL,
		NULL };
     ttab->set_s = ttab->value_s;
     sprintf(ttab->value_s,"PR_%s",fn_ids->name);
     sprintf(ttab->getref_s,"Johnson_getref(&%s)",ttab->value_s);

     token tok; tok.type = t_Df; tok.data.pointer = (void*)ttab; // associate it
     fn_ids->t = tok;

     PrintMain("double %s, ",ttab->value_s);

    }

    if(prog->current_function->num_params)trans_main_p -= 2;
    PrintMain("){\n");

   }else{
    PrintMain(" double Params[] ){\n");

    id_info *fn_ids = NULL;
    int i; // process params list
    fn_ids = prog->current_function->ids->next;
    for(i=0; i<prog->current_function->num_params; i++){
     TransTable *ttab = calloc(1,sizeof(TransTable)); // create a translation table for each parameter ID
     *ttab = (TransTable){
		1, calloc( strlen(fn_ids->name)+5, 1),
		1, calloc( strlen(fn_ids->name)+30, 1),
		1, NULL,
		NULL };
     ttab->set_s = ttab->value_s;
     sprintf(ttab->value_s,"Params[%d]",i+1);
     sprintf(ttab->getref_s,"Johnson_getref(Params + %d)",i+1);
     token tok; tok.type = t_Df; tok.data.pointer = (void*)ttab; // associate it
     fn_ids->t = tok;

     //printf("%d : %s\n",i+1,fn_ids->name); //test
     fn_ids = fn_ids->next;
    }

   }//endif params type
   while( (prog->tokens[ *p ].type != t_local) && (prog->tokens[ *p ].type != t_endstatement) ){
    *p += 1;
   }//endwhile

   //  ------- process local list -------
   if( prog->tokens[ *p ].type == t_local){
    *p += 1; 
    if( prog->tokens[ *p ].type != t_id ){
     trans_print_sourcetext_location( prog, *p);
     fprintf(stderr,"translate_command: bad function def for '%s'\n",getfuncname( prog, prog->functions[trans_fn] ) );
     exit(0);
    }
    while( prog->tokens[ *p ].type == t_id ){

     //------ create translation tables for the local variables ------

     id_info *fn_ids = find_id(prog->current_function->ids, (char*)prog->tokens[ *p ].data.pointer);
     TransTable *ttab = calloc(1,sizeof(TransTable)); // create a translation table for each parameter ID
     *ttab = (TransTable){
		1, calloc( strlen(fn_ids->name)+5, 1),
		1, calloc( strlen(fn_ids->name)+30, 1),
		1, NULL,
		NULL };
     ttab->set_s = ttab->value_s;
     sprintf(ttab->value_s,"LC_%s",fn_ids->name);
     sprintf(ttab->getref_s,"Johnson_getref(&%s)",ttab->value_s);

     token tok; tok.type = t_Df; tok.data.pointer = (void*)ttab; // associate it
     fn_ids->t = tok;
     //---------------------------------------------------------------
     PrintMain("double %s; ", ttab->value_s );

     *p += 1;
    }
    PrintMain("\n");
   }
   if( prog->tokens[ *p ].type != t_endstatement ){
    trans_print_sourcetext_location( prog, *p);
    fprintf(stderr,"translate_command: bad function def for '%s', missing final ';'\n",getfuncname( prog, prog->functions[trans_fn] ) );
    exit(0);
   }
   *p += 1;

  } break;
 case t_F:
  {
   translate_value(prog, p);
   PrintMain(";\n");
  } break;
/*
 case t_:
  {
   PrintMain(";\n");
   *p += 1;
  } break;
*/
  #if allow_debug_commands
  case t_tb:
  {
   PrintMain("tb();\n");
   *p += 1;
  } break;
  #endif
  default:
   ErrorOut("translate_command: FAILURE: Not supported or error, '%s'\n",tokenstring(prog->tokens[ *p ]));
 }
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

int chdir_to_the_dir_in_this_path( char *path ){
 char editpath[4096] = { 0 };
 strcpy(editpath,path);
 int l = strlen(editpath);
 while( editpath[l] != '/' ){
  editpath[l] = 0;
  l--;
 }//endwhile
 return chdir(editpath);
}//endproc

void translate_preprocess_OptionImport(program *prog){
 int i=0;
 int *p = &i; // for ErrorOut 
 int j,startp;
 char currdir[4096]={0};
 if( trans_file_input_path ){ // save current working directory
  getcwd(currdir,4096);
  if( chdir_to_the_dir_in_this_path( trans_file_input_path ) ) ErrorOut("translate_preprocess_OptionImport: shouldn't happen\n"); // change to the directory the source file is from
 }//endif
 while( i < prog->length ){
  if( prog->tokens[i].type == t_option ){
   startp=i;
   i++;
   if( prog->tokens[i].type == t_stringconst && TheseStringsMatch((char*)prog->tokens[i].data.pointer, "import") ){
    i++;
    if( prog->tokens[i].type == t_stringconst){
     // -----
     token *tokens; int tokens_length; char *filename;
     // get filename
     filename = (char*)prog->tokens[i].data.pointer;
     // load text file and process it, getting the tokens (program code data) and program_strings
     tokens = loadtokensfromtext(prog, filename, &tokens_length);
     if( tokens == NULL ){
      ErrorOut("translate_preprocess_OptionImport: couldn't open file '%s'\n",filename);
     }
     // remove everything before the function definitions
     for(j=0; j<tokens_length; j++){
      if( tokens[j].type == t_deffn ){ break; }
      tokens[j].type = t_endstatement;
     }
     // resize array and append new code
     prog->tokens = realloc(prog->tokens, (prog->maxlen + tokens_length)*sizeof(token));
     memcpy(prog->tokens + prog->maxlen, tokens, sizeof(token) * tokens_length);
     prog->length += tokens_length; prog->maxlen+=tokens_length;
     // process function definitions for imported code
     process_function_definitions(prog,prog->maxlen - tokens_length);
     // tidy up and erase the option command so it doensn't get seen by the main translation pass
     free(tokens);
     for(j=startp; j<=i; j++){
      prog->tokens[j].type = t_endstatement;
     }//next
     // -----
    }else{
     ErrorOut("option \"import\": 'import' requires string constant\n");
    }
   }//endif
  }//endif
  i++;
 }
 if( trans_file_input_path ){ // save current working directory
  if( chdir(currdir) ){
   ErrorOut("translate_preprocess_OptionImport: this absolutely shouldn't happen\n");
  }
 }//endif
}//endproc

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

void translate_function_proto(program *prog, int n){
 func_info *func = prog->functions[n];
 if( (func->params_type == p_exact) && ! trans_program_contains_F ){ // exact number of params (and program doesn't use function references) 
  PrintProt("double FN_%s(",getfuncname( prog, prog->functions[n] ) );
  int i;
  if(func->num_params){
   for(i=0; i < func->num_params-1; i++){
    PrintProt("double, ");
   }//next
   PrintProt("double");
  }//endif
  PrintProt(");\n");
 }else{ // variable number of params, or the program uses function references so every function must take a 'double[]'
  PrintProt("double FN_%s( double[] );\n",getfuncname( prog, prog->functions[n] ) );
 }
}//endproc

void translate_clear_disabled_areas(program *prog){ 
 int ifpos,elsepos,endifpos;
 int p = 0;
 while( p < prog->length ){ 
  if( prog->tokens[ p ].type == t_if && prog->tokens[ p+1 ].type == t_number ){

   ifpos = p;
   elsepos = _interpreter_ifsearch(p+1, prog);
   if( prog->tokens[elsepos].type == t_endif ){
    endifpos = elsepos; elsepos=-1;
   }else{
    endifpos = _interpreter_ifsearch(elsepos+1, prog);
   }
   if(elsepos == -1) elsepos = endifpos;

   int i, ifblock_contains_a_label=0;
   for(i=ifpos; i<endifpos; i++){
    if( prog->tokens[i].type == t_label ){
     ifblock_contains_a_label = 1; i=endifpos;
    }
   }
   if(!ifblock_contains_a_label){
    int clrstart, clrend;
    if( prog->tokens[ p+1 ].data.number != 0.0 ){
     clrstart = elsepos;
     clrend = endifpos;
    }else{
     clrstart = ifpos;
     clrend = elsepos;
    }
    token cleartoken = (token){0}; cleartoken.type = t_endstatement;
 
    for(i=clrstart; i<=clrend; i++){
     prog->tokens[ i ] = cleartoken;
    }
    prog->tokens[ ifpos ] = cleartoken;
    prog->tokens[ ifpos+1 ] = cleartoken;
    prog->tokens[ elsepos ] = cleartoken;
    prog->tokens[ endifpos ] = cleartoken;
   }

  }//endif found an if with a num constant
  p+=1;
 }
}

void translate_MarkFunctionAsFound(int *ar,program *prog, func_info *f){
 if(ar[f->function_number]) return;
 ar[f->function_number] = 1;
 int p = f->start_pos; 
 func_info *hold = prog->current_function; prog->current_function = f;
 while( p < prog->length ){
  switch( prog->tokens[p].type ){
  case t_id: {
    token t = trans_peek_id(prog, &prog->tokens[p]);
    if( t.type == t_Ff ) translate_MarkFunctionAsFound(ar,prog,(func_info*)t.data.pointer);
   } break;
  case t_deffn:
   p = prog->length;
   break;
  }
  p++;
 }
 prog->current_function = hold;
}
void translate_remove_unused_functions(program *prog){
 int found[MAX_FUNCS];
 int i,j;
 for(i=0; i<MAX_FUNCS; i++){ found[i] = 0; }
 i=0;
 while(i<trans_functions_start_p){
  if( prog->tokens[i].type == t_id ){
   token t = trans_peek_id(prog, &prog->tokens[i]);
   if( t.type == t_Ff ) translate_MarkFunctionAsFound(found,prog,(func_info*)t.data.pointer);
  }
  i++;
 }

 token cleartoken = (token){t_endstatement,0};
 i=0;
 while(prog->functions[i]){
  if( !found[i] ){
   j=prog->functions[i]->start_pos;
  
   while( prog->tokens[j].type != t_deffn ){
    j--;
   }

   prog->tokens[j] = cleartoken;
   while(j < prog->length){
    switch(prog->tokens[j].type){
     case t_deffn:
      j=prog->length;
      break;
     default:
      prog->tokens[j] = cleartoken;
    }//endswitch
    j++;
   }//endwhile j
  }//endif notfound
  i++;
 }//endwhile i

}

void translate_blankarea(program *prog, int p1, int p2 ){
 token cleartoken = (token){t_endstatement,0};
 while(p1<p2){
  prog->tokens[ p1 ] = cleartoken;
  p1++;
 }
}

#define DISABLE_THIS_GPOS_THING 0

#if DISABLE_THIS_GPOS_THING
int _translate_preprocess_declarations_gposval=0;
#endif

int translate_preprocess_declarations_max_descent_level = 25;

int _translate_preprocess_declarations(program *prog,func_info *f, int *already, int descent_level){
 if(descent_level >= translate_preprocess_declarations_max_descent_level ) return 0;
 func_info *hold = prog->current_function;
 prog->current_function = f;
 if(f->function_number >= 0){
  if( already[f->function_number] & 2 ){
   fprintf(stderr,"error: function containing declarations is called twice\n");
   exit(0);
  }
 }
 int p=prog->current_function->start_pos;
 int pp;
 int found_declarations = 0;
 int loop_level=0;
 while(p<prog->length){
#if DISABLE_THIS_GPOS_THING
  if( f == &prog->initial_function ){ _translate_preprocess_declarations_gposval = p; }
#endif
  switch(prog->tokens[p].type){
  case t_while:
   loop_level += 1;
   break;
  case t_endwhile:
   loop_level -= 1;
   break;
  case t_deffn:
   p = prog->length;
   break;
  case t_id:
   {
    token peek = trans_peek_id(prog, &prog->tokens[p]);
    if( peek.type == t_Ff && peek.data.pointer != prog->current_function ){
#if DISABLE_THIS_GPOS_THING
     trans_function_gposses[ ((func_info*)peek.data.pointer)->function_number ] = _translate_preprocess_declarations_gposval;
#endif
     int this_function_has_decls = _translate_preprocess_declarations(prog, (func_info*)peek.data.pointer, already, descent_level + 1);
     if( loop_level && this_function_has_decls){ 
      trans_print_sourcetext_location( prog, p);
      fprintf(stderr,"error: function '%s' containing declaration(s) is called inside a loop\n",getfuncname(prog,(func_info*)peek.data.pointer));
      exit(0);
     }
     found_declarations |= this_function_has_decls;
    }
   } break;
  case t_var: case t_const: case t_stringvar:
   {
    if( loop_level ){
     trans_print_sourcetext_location( prog, p);
     fprintf(stderr,"error: declaration inside a loop\n");
     exit(0);
    }
    found_declarations=1;
    pp=p;
    translate_command(prog, &p);
    if( prog->tokens[pp].type < 200 ){ // unless something required work later during the main translation pass, we need to clear away the declaration 
     translate_blankarea(prog, pp, p);
    }
    p-=1; // put p back a position, because translate_command changed the value of p, and the increment at the end of the loop will skip past an extra position
   } break;
  }//endswitch
  p+=1;
 }//endwhile p
 if(f->function_number >= 0){
  already[f->function_number] = (found_declarations<<1) | 1;
 }
 prog->current_function = hold;
 return found_declarations;
}//endproc

void translate_preprocess_declarations(program *prog){
 int *already; already = calloc(MAX_FUNCS,sizeof(int));
 _translate_preprocess_declarations(prog,&prog->initial_function, already, 0);
 free(already);
}

void check_for_oldstyle_functions(program *prog){
 int i,j;
 int inside_function_definition = 0;
 int *p = &i; // for ErrorOut
 for(i=0; i<prog->length; i++){
  switch( prog->tokens[i].type ){
  case t_deffn: inside_function_definition = 1; break;
  case t_endstatement: inside_function_definition = 0; break;
  case t_F: case t_P: case t_L:
   if(inside_function_definition){
    ErrorOut("This program contains old-style function definitions and can not be supported\n");
   }//endif
  }//endswitch
 }//next
}//endproc

void BlockValidityCheck(program *prog, TOKENTYPE_TYPE opener, TOKENTYPE_TYPE closer){
 int i;
 int *p = &i; // this is for ErrorOut
 int level=0;
 for(i=0; i<prog->length; i++){
  if( prog->tokens[ i ].type == opener ) level += 1;
  if( prog->tokens[ i ].type == closer ) level -= 1;
  if( opener==t_if && !level && prog->tokens[ i ].type == t_else ){
   ErrorOut("'else' outside an 'if' block\n");
  }
 }//next
 if( level ){
  while( level ){
   if( prog->tokens[ i ].type == opener ) level -= 1;
   if( prog->tokens[ i ].type == closer ) level += 1;
   i--;
   if(i<0) ErrorOut("BlockValidityCheck: this should never happen\n");
  }//endwhile
  token t; t.type = opener;
  ErrorOut("Unterminated '%s' block\n", tokenstring( t ) );
 }//endif
}//endproc

// ===================================
int param_order_test_val = 0;

int param_order_test_fn(){
 param_order_test_val = param_order_test_val + 1;
 return param_order_test_val;
}

int param_order_test(int a, int b, int c){
 if( b != 2 ) goto paramordertest_complain;
 if( a == 1 && c == 3 ){ 
  return 0;
 }
 if( a == 3 && c == 1){
  return 1;
 }
 // complain about something unexpected happening here
 paramordertest_complain:
 PrintErr("param_order_test: unexpected result:\n a == %d\n b == %d\n c == %d\n",a,b,c);
 exit(0);
}

int does_this_arch_use_reverse_order_evaluation_of_function_params(){
 return param_order_test(param_order_test_fn(),param_order_test_fn(),param_order_test_fn());
}
// ===================================


void translate_program(program *prog){

 translate_preprocess_OptionImport(prog);

 check_for_oldstyle_functions(prog);
 BlockValidityCheck(prog, t_if, t_endif);
 BlockValidityCheck(prog, t_while, t_endwhile);
 BlockValidityCheck(prog, t_caseof, t_endcase);
 BlockValidityCheck(prog, t_for, t_endfor);
 trans_reverse_function_params = does_this_arch_use_reverse_order_evaluation_of_function_params();
 //trans_reverse_function_params=1;
 #if TRANS_CRASHDEBUG 
 PrintErr("Detected function parameter evaluation order for this arch: %s\n", trans_reverse_function_params ? "Right to left (reversed)" : "Left to right");
 #endif

 int i=0; int p=0;
 
 trans_protos = calloc(1,1024*1024*1);
 trans_main = calloc(1,1024*1024*1);
 trans_vars = calloc(1,1024*1024*1);
 trans_varp = calloc(1,1024*1024*1);
 trans_global_ids = calloc(1,sizeof(id_info));
 
 { // prepare Argc id
  TransTable *ttab = MakeTransTable(1,"(double)Johnson_Argc", 0,"Can't ref _argc", 0,"_argc cannot be set", NULL);
  trans_add_id(prog->ids, make_id( "_argc", maketoken_Df( (double*)ttab ) ) );
  // prepare _argv0 and _argv1
  TransTable *tt_argv0
   =
   MakeTransTable(
    1, "SVRtoSVL(Johnson_Argv0)",
    1, "(Johnson_Argv0->string_variable_number)",
    1, "Johnson_Argv0",
    NULL);
  trans_add_id(prog->ids, make_id( "_argv0", maketoken_Sf( (stringvar*)tt_argv0 ) ) );
  // argv 1
  TransTable *tt_argv1
   =
   MakeTransTable(
    1, "SVRtoSVL(Johnson_Argv1)",
    1, "(Johnson_Argv1->string_variable_number)",
    1, "Johnson_Argv1",
    NULL);
  trans_add_id(prog->ids, make_id( "_argv1", maketoken_Sf( (stringvar*)tt_argv1 ) ) );
 }

 p=0; trans_functions_start_p = -1;
 while( p < prog->length ){ 
  if( prog->tokens[p].type == t_deffn ){
   trans_functions_start_p = p;
   break;
  }
  p+=1;
 }
 if( trans_functions_start_p == -1 ) trans_functions_start_p = prog->length;

 // does this program contain function references?
 p=0;
 while( p < prog->length ){ 
  if( prog->tokens[p].type == t_F ){
   trans_program_contains_F = 1;
   break;
  }
  p+=1;
 }

 trans_function_gposses = calloc(256,sizeof(int));
 trans_build_fgpos_array(prog); 
#if 1
 if( ! trans_program_contains_F ){
  translate_remove_unused_functions(prog);
 }
 translate_clear_disabled_areas(prog);
#endif

 i=0; // do function prototypes
 while( prog->functions[i] ){
  translate_function_proto(prog,i);
  i++;
 }
 PrintProt("\n");

 if(trans_program_contains_F){
  i=0;  // build function reference table
  while( prog->functions[i] ){
   char *funcname = getfuncname( prog, prog->functions[i] ); 
   PrintMain("Johnson_function_table[%d] = &FN_%s;\n",i,funcname);
   i += 1;
  }
 }

 // process all variable/constant/etc declarations
 translate_preprocess_declarations(prog);
//TestThisShit();exit(0);

 // count number of functions
 for(p=0; p<prog->length; p++){
  if( prog->tokens[p].type == t_deffn ) trans_num_funcs += 1;
 }

 // ---------------

 p=0;
 while( p < prog->length ){ // this is the main translation pass
  translate_command(prog, &p);
 }

 // ---------------
 
 char VarsArrayDeclaration[384];
 sprintf(VarsArrayDeclaration,"double VarsArray[%d]; Johnson_vsize=%d; FirstVarP = VarsArray;\nJohnson_num_funcs = %d;\n",trans_vsize,trans_vsize,trans_num_funcs);

 printf("%s%s%s%s%s%s%s%s%s%s%s%s%s",
  trans_program_uses_maths ? "\n#include <math.h>\n" : "\n",
  "#include <stdio.h>\n#include <stdlib.h>\n#include <unistd.h>\n#define using_johnsonlib\n",
  (trans_program_uses_gfx ? "#define using_johnsonlib_graphics\n":""),
  (trans_reverse_function_params ? "#define JOHNSON_PARAMETERS_REVERSED\n":""),
  "#include \"johnsonlib.c\"\n\n",
  trans_protos,
  trans_varp,
  "\n\n",
  "int main(int argc, char **argv){\nJohnsonlib_init(argc,argv);\n",
  VarsArrayDeclaration,
  trans_vars,
  "/* translated body of program */\n",
  trans_main
 );
 printf("}\n");
}

// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------


int main(int argc, char **argv){

 program *prg = NULL;

 if(argc>1){
  prg = init_program( argv[1], Exists(argv[1]), NULL ); 
  if( Exists(argv[1]) && strstr(argv[1],"/") ) trans_file_input_path = argv[1];
 }else{
  printf("%s [program text or path to a file containing program text]\nThis will write the translated C program to stdout.\n",argv[0]);
  return 0;
 }

 translate_program(prg);
 exit(0);

}

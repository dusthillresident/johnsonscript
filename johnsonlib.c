// This is used by the Johnsonscript to C source-to-source compiler

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mylib.c"

// memory problem debugging stuff
#if 0
 void* mymallocfordebug(size_t size    ,int n, char *f){
  void *out = malloc(size);
  printf("line %d,	%s:	MALLOC	%p\n",n,f,out);
  return out;
 }
 void* mycallocfordebug(size_t nmemb, size_t size    ,int n, char *f){
  void *out = calloc(nmemb,size);
  printf("line %d,	%s:	CALLOC	%p\n",n,f,out); 
  return out;
 }
 void* myreallocfordebug(void *ptr, size_t size    ,int n, char *f){
  void *out = realloc(ptr,size);
  printf("line %d,	%s:	REALLOC	%p (input %p)\n",n,f,out,ptr);
  return out;
 }
 void myfreefordebug(void *ptr,      int n, char *f){
  printf("line %d,	%s:	FREEING	%p\n",n,f,ptr);
  free(ptr);
 }
 #define malloc(a) mymallocfordebug(a,__LINE__,__FILE__)
 #define calloc(a,b) mycallocfordebug(a,b,__LINE__,__FILE__)
 #define realloc(a,b) myreallocfordebug(a,b,__LINE__,__FILE__)
 #define free(p) myfreefordebug(p,__LINE__,__FILE__)
#endif

void Johnsonlib_MemoryAllocFailCheck(void *ptr, char *id){
 if( ! ptr ){
  printf("Johnsonlib error: memory allocation failed (%s)\n",id);
  exit(0);
 }
}

double *FirstVarP = NULL;
int Johnson_alloc_p=0;
int Johnson_vsize=2048;
double Johnson_alloc(int n){
 int out = Johnson_alloc_p;
 if( out + n >= Johnson_vsize ){
  printf("Johnsonscript error: ran out of variable memory\n");
  exit(0);
 }
 int i;
 for(i=0; i<n; i++){ // initialise it to 0 
  FirstVarP[ Johnson_alloc_p + i ] = 0;
 }
 Johnson_alloc_p += n;
 return out;
}

double Johnson_getref(double *p){
 long long int pp = ((long long int)p - (long long int)FirstVarP);
 if(pp&7){
  printf("Johnsonscript error: getref: alignment problem\n");
  exit(0);
 }
 return (double)(pp >> 3);
}

typedef double (*JohnsonFunctionP)(double[]);

JohnsonFunctionP *Johnson_function_table = NULL;

int Johnson_num_funcs = 0;
double Johnson_function_deref( double Params[], int n){
 if( n < 0 || n >= Johnson_num_funcs ){
  printf("Johnsonscript error: getref: bad function dereference\n");
  exit(0);
 }
// double (*penismagic)(double[]) = Johnson_function_table[n];
 return Johnson_function_table[n](Params);
}

double Johnson_abs(double v){
 if(v<0) return -v;
 return v;
}

double Johnson_sgn(double v){
 return((v>0)-(v<0));
}

typedef
struct StringVariable {
 int len;
 int bufsize;
 char *buf;
 // ----
 int claimed;
 int string_variable_number;
}
SVR;

void ExpandSVR( SVR *svr, int newsize ){
 if( newsize < 0 ){
  printf("ExpandSVR: bad request\n");
  exit(0);
 }
 if( svr == NULL){
  fprintf(stderr,"Johnsonlib: ExpandSVR: svr == NULL\n");
  exit(0);
 }
 if( svr->bufsize >= newsize ){ 
  return;
 }
 if( svr->len && (svr->len >= svr->bufsize) ){
  fprintf(stderr,"Johnsonlib: ExpandSVR: len => bufsize\n");
  exit(0);
 }
 newsize = newsize+(512-(newsize % 512));
 //printf("ExpandSVR: %p : bufsize %d, newsize %d\n",svr->buf,svr->bufsize,newsize);
 if( svr->buf == NULL ){
  svr->buf = calloc(1, newsize);
  Johnsonlib_MemoryAllocFailCheck(svr->buf,"SVR buffer allocation");
  svr->bufsize = newsize;
 }else{
  if( svr->bufsize < newsize ){
   svr->buf = realloc(svr->buf, newsize);
   Johnsonlib_MemoryAllocFailCheck(svr->buf,"SVR buffer reallocation");
   svr->bufsize = newsize;
   svr->buf[svr->len]=0;
  }//endif bufsize < newsize
 }//endif buf == NULL
}//endproc

typedef
struct StringValue {
 int len;
 char *buf;
}
SVL;

unsigned char*
#ifdef JOHNSON_PARAMETERS_REVERSED
Johnson_C_CharacterAccess( int index, SVL svl )
#else
Johnson_C_CharacterAccess( SVL svl, int index )
#endif
{
 if( index<0 || index>=svl.len ){
  printf("Johnsonscript error: C (characteraccess): index out of range\n");
  exit(0);
 }
 return svl.buf + index;
}

SVR **StringAcc;
int max_stringacc = 32;

SVR* NewStrAccLevel( int n ){
 if( n == max_stringacc ){
  max_stringacc += 1;
  StringAcc = realloc(StringAcc, max_stringacc * sizeof(void*) );
  Johnsonlib_MemoryAllocFailCheck( StringAcc, "StringAcc array resize");
  StringAcc[n] = calloc(1,sizeof(SVR));
  Johnsonlib_MemoryAllocFailCheck( StringAcc[n], "StringAcc SVR" );
 }
 ExpandSVR( StringAcc[n], 256);
 return StringAcc[n];
}

int SAL=-1; // string accumulator level

SVR **StringVars;
int max_stringvars = 6;

SVR* Johnson_stringvar_deref(int n){
 if( n<0 || n>=max_stringvars ){
  fprintf(stderr,"Johnsonscript error: string dereference out of range\nn == %d\nmax_stringvars == %d\n",n,max_stringvars);
  exit(0);
 }
 return StringVars[n];
}

int NewStringvar(){
 int i;
 for(i=0; i<max_stringvars; i++){
  if( ! StringVars[i]->claimed ){ 
   StringVars[i]->claimed = 1;
   StringVars[i]->len = 0;
   return i;
  }//endif
 }//next
 i = max_stringvars; 
 max_stringvars += 1;
//printf("%p\n",StringVars);
 StringVars = realloc(StringVars, max_stringvars * sizeof(void*) ); 
 Johnsonlib_MemoryAllocFailCheck( StringVars, "StringVars array resize");
//printf("%p\n",StringVars);
 StringVars[i]=calloc(1,sizeof(SVR));
 Johnsonlib_MemoryAllocFailCheck( StringVars[i], "new stringvar SVR");
 ExpandSVR( StringVars[i], 256 ); 
 StringVars[i]->claimed = 1; 
 StringVars[i]->string_variable_number = i;
 return i;
}//endproc
SVR* NewStringvar_svrp(){
 int svn = NewStringvar();
 return StringVars[svn];
}

int SF_isFirst=1; // this string function is the first being evaluated

void AppendSVLtoSVR( SVR *svr, SVL a){
 int newlen = svr->len + a.len;
 if(newlen >= svr->bufsize){
  ExpandSVR(svr, newlen+1);
 }
 memcpy( svr->buf + svr->len, a.buf, a.len);
 svr->len += a.len;
}

void ClearSVR( SVR *svr ){
 svr->len=0;
}


SVR* SetSVR( SVR *svr, SVL a){
 ClearSVR( svr );
 AppendSVLtoSVR( svr, a );
 return svr;
}

SVL SVRtoSVL( SVR *svr ){
 return (SVL){ svr->len, svr->buf };
}

void PrintSVL( SVL svl ){
 int i;
 for(i=0; i<svl.len; i++){
  putchar(svl.buf[i]);
 }
 //printf("\n");
}

#ifdef JOHNSON_PARAMETERS_REVERSED
SVL CatS( SVL a, SVL b, int sa_l )
#else
SVL CatS( int sa_l, SVL a, SVL b )
#endif
{
 SVR *Acc = NewStrAccLevel( sa_l );
 #if 0
 printf("---- %d ----\n",sa_l);
 PrintSVL( a ); printf("\n");
 PrintSVL( b ); printf("\n");
 printf("---------\n\n");
 #endif
 // ----------
 ClearSVR( Acc );
 AppendSVLtoSVR( Acc, a );
 AppendSVLtoSVR( Acc, b );
 // ----------
 if( sa_l == 0 ){
  SAL=-1;
 }
 return SVRtoSVL( Acc );
}

SVL ToSVL( char *string ){
 return (SVL){ strlen(string), string};
}

#ifdef JOHNSON_PARAMETERS_REVERSED
SVL StringS( SVL svl, int n, int sa_l )
#else
SVL StringS( int sa_l, int n, SVL svl  )
#endif
{
 char *stringS_tempbuf = NULL;
 SVR *accumulator = NewStrAccLevel( sa_l );
 SVL out = (SVL){0,NULL};
 // ----------
 if( svl.len && n > 0 ){
  if( svl.buf == accumulator->buf || ( svl.buf > accumulator->buf && svl.buf <= accumulator->buf+accumulator->len ) ){ // Äkta dig för Rövar-Albin
   stringS_tempbuf = malloc(svl.len);
   memcpy(stringS_tempbuf, svl.buf, svl.len);
   svl.buf = stringS_tempbuf;
  }
  int bufsize_required = svl.len * n + 1;
  if( bufsize_required < 0 ){ 
   printf("Johnsonlib StringS: string too long\n");
   exit(0);
  }
  if( bufsize_required > accumulator->bufsize ) ExpandSVR( accumulator, bufsize_required );
  if( svl.len == 1 ){ // if it's one character we can just do a nice memset()
   memset(accumulator->buf, svl.buf[0], n);
  }else{ // or otherwise
   int i;
   char *p = accumulator->buf;
   for(i=0; i<n; i++){
    memcpy( p, svl.buf, svl.len );
    p+=svl.len;
   }//endif
  }//endif
  accumulator->len = bufsize_required;
  out = SVRtoSVL( accumulator );
  if(stringS_tempbuf){
   free(stringS_tempbuf);
  }
 }//endif 
 // ----------
 if( sa_l == 0 ){
  SAL=-1;
 }//endif
 return out;
}//endproc

// --------------------------------

SVL MidS( SVL sv, int midpos, int len){
 if( sv.len<=0 || midpos < 0 || midpos >= sv.len ){
  sv.len=0;
  return sv;
 }
 if( len < 0 ) len = sv.len;
 if( midpos + len >= sv.len ){
  len -= ((midpos+len) - sv.len);
 }
 sv.buf += midpos;
 sv.len = len;
 return sv;
}

SVL RightS( SVL in, int len){
 if(len>in.len)len=in.len; if(len<0)len=0;
 in.buf += (in.len-len);
 in.len = len;
 return in;
}

SVL LeftS( SVL in, int len){
 if(len>in.len)len=in.len; if(len<0)len=0;
 in.len = len;
 return in;
}

int Johnson_isnan( double val ){
 return ((int)val && val==0.0);
}

#ifdef JOHNSON_PARAMETERS_REVERSED
SVL StrS( double val, int sa_l )
#else
SVL StrS( int sa_l, double val )
#endif
{
 SVR *accumulator = NewStrAccLevel( sa_l );
 // ----------
 int snpf_return;
 if(!Johnson_isnan(val) && val == (double)(int)val){
  snpf_return = snprintf(accumulator->buf, accumulator->bufsize, "%d",(int)val);
 }else{
  snpf_return = snprintf(accumulator->buf, accumulator->bufsize, "%f",val);
 }
 SVL out;
 out.len = snpf_return>=accumulator->bufsize ? accumulator->bufsize-1 : snpf_return;
 out.buf = accumulator->buf;
 // ----------
 if( sa_l == 0 ){
  SAL=-1;
 }
 return out;
}

#ifdef JOHNSON_PARAMETERS_REVERSED
SVL ChrS( char c, int sa_l )
#else
SVL ChrS( int sa_l, char c )
#endif
{
 SVR *Acc = NewStrAccLevel( sa_l );
 // ----------
 SetSVR( Acc, (SVL){ 1, &c } );
 // ----------
 if( sa_l == 0 ){
  SAL=-1;
 }
 return SVRtoSVL( Acc );
}

// --------- johnsonscript string-related functions that return a numerical value ---------------

double ValS( SVL s ){
 char hold = s.buf[s.len]; s.buf[s.len] = 0;
 double out = strtod( s.buf, NULL );
 s.buf[s.len] = hold;
 return out;
}

int AscS( SVL s ){
 if(s.len == 0) return -1; 
 return (unsigned char) s.buf[0];
}

#ifdef JOHNSON_PARAMETERS_REVERSED
int CmpS( SVL sv1, SVL sv2, int sa_l )
#else
int CmpS( int sa_l, SVL sv1, SVL sv2 )
#endif
{
 int result = 0;
 // ----------
  if( sv1.len == sv2.len ){
   result = strncmp( sv1.buf, sv2.buf, sv1.len );
   goto CmpS_out;
  }
 result = strncmp( sv1.buf, sv2.buf, sv1.len < sv2.len ? sv1.len : sv2.len );
  if( result == 0 ){
   result = sv1.len > sv2.len ? 1 : -1; /*sv1.len > sv2.len ? sv1.string[sv1.len-1] : -sv2.string[sv2.len-1];*/
  }
 // ----------
 CmpS_out:
 if( sa_l == 0 ){
  SAL=-1;
 }
 return result;
}

#ifdef JOHNSON_PARAMETERS_REVERSED
int InstrS( SVL sv1, SVL sv2, int sa_l )
#else
int InstrS( int sa_l, SVL sv1, SVL sv2 )
#endif
{
 int result = -1;
 // ----------
 if( sv1.len < sv2.len ) goto InstrS_out;
 int i;
 for(i=0; i <= sv1.len - sv2.len; i++){
  if( (sv1.buf[i] == sv2.buf[0])
        &&
       (sv1.buf[i+sv2.len-1] == sv2.buf[sv2.len-1])
        &&
       (!strncmp( sv1.buf + i, sv2.buf, sv2.len))
  ){
   result = i;
   break;
  }//endif
 }//next
 // ----------
 InstrS_out:
 if( sa_l == 0 ){
  SAL=-1;
 }
 return result;
}

// ----------------------------------------------------------------------------------------------
// files

typedef
struct JohnsonFile {
 int open;
 int read_access;
 int write_access;
 FILE *fp;
}
FIL;

FIL **Johnson_files;
int max_files = 8;

int Johnson_free_fileslot(){
 int i;
 for(i=0; i<max_files; i++){
  if( ! Johnson_files[i]->open ) return i;
 }
 i = max_files; max_files += 1;
 Johnson_files = realloc(Johnson_files, max_files * sizeof(void*) ); 
 Johnsonlib_MemoryAllocFailCheck( Johnson_files, "file info array resize");
 Johnson_files[i] = calloc(1,sizeof(FIL));
 Johnsonlib_MemoryAllocFailCheck( Johnson_files[i], "new file into struct");
 return i;
}

// --- johnsonscript file functions returning number

#define J_OPENIN 0
#define J_OPENOUT 1
#define J_OPENUP 2

int Johnson_OpenFile(SVL filename, int access_mode){
 int ret=0;
 char hold = filename.buf[filename.len]; filename.buf[filename.len]=0;
 // ----
 int fileslot = Johnson_free_fileslot();
 FILE *f = NULL;
 int write_access=0,read_access=0;
 switch( access_mode ){
  case J_OPENIN: // openin
   f = fopen(filename.buf,"rb");
   read_access = 1;
   write_access = 0;
   break;
  case J_OPENOUT: // openout
   f = fopen(filename.buf,"wb");
   read_access = 0;
   write_access = 1;
   break;
  case J_OPENUP: // openup
   f = fopen(filename.buf,"r+b");
   read_access = 1;
   write_access = 1;
   break;
 }
 if( f ){
  Johnson_files[fileslot]->open = 1;
  Johnson_files[fileslot]->fp = f;
  Johnson_files[fileslot]->read_access = read_access;
  Johnson_files[fileslot]->write_access = write_access;
  ret = fileslot + 1;
 }
 // ----
 filename.buf[filename.len]=hold;
 return ret;
}


FIL* JohnsonFiles_getFileStructPtr(int filenum, int read, int write){
 if(filenum<=0 || filenum>=max_files){
  if( filenum == 0 )
   fprintf(stderr,"Johnsonscript error: bad file access: file number is 0\n");
  else
   fprintf(stderr,"Johnsonscript error: bad file access: file number out of range\n");
  exit(0);
 }
 FIL *f = Johnson_files[filenum-1];
 if( ! f->open ){
  fprintf(stderr,"Johnsonscript error: bad file access: not an open file\n");
  exit(0);
 }
 if( read && ! f->read_access  ){
  fprintf(stderr,"Johnsonscript error: bad file access: read from non-readable file\n");
  exit(0);
 }
 if( write && ! f->write_access ){
  fprintf(stderr,"Johnsonscript error: bad file access: write to non-writeable file\n");
  exit(0);
 }
 return f;
}

int Johnson_Eof(int filenum){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 0);
 return feof(f->fp);
}

int Johnson_Bget(int filenum){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 1, 0);
 return fgetc(f->fp);
}

double Johnson_Vget(int filenum){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 1, 0);
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

int Johnson_Ptr(int filenum){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 0);
 return ftell(f->fp);
}

int Johnson_Ext(int filenum){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 0);
 return Ext(f->fp);
}

// --- johnsonscript file commands

void Johnson_Sptr(int filenum, int val){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 0);
 SetPtr(f->fp, val);
}

void Johnson_Bput(int filenum, int byte){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 1);
 fputc(byte, f->fp);
}

void Johnson_Vput(int filenum,double val){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 1);
 fwrite((void*)&val, sizeof(double), 1, f->fp);
}

void Johnson_Sput(int filenum, SVL sv){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 1);
 fwrite( (void*)sv.buf, sizeof(char), sv.len, f->fp );
}

void Johnson_Close(int filenum){
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 0, 0);
 fclose( f->fp );
 f->open = 0;
 f->fp = NULL;
}

// --- johnsonscript file functions returning a stringvalue

#ifdef JOHNSON_PARAMETERS_REVERSED
SVL Johnson_Sget(int filenum, int num_bytes_to_read, int sa_l)
#else
SVL Johnson_Sget(int sa_l, int filenum, int num_bytes_to_read )
#endif
{ 
 FIL *f = JohnsonFiles_getFileStructPtr(filenum, 1, 0);
 SVR *Acc = NewStrAccLevel( sa_l );
 // ----------
 ClearSVR( Acc );
 if( num_bytes_to_read <= 0 ){
  int read_until = -num_bytes_to_read;
  int ch;
  while( 1 ){
   ch = fgetc(f->fp);
   if( ch == read_until || ch == -1 ) break;
   Acc->buf[ Acc->len ] = ch;
   Acc->len += 1;
   if( Acc->bufsize - Acc->len < 10 ){

    ExpandSVR( Acc, Acc->bufsize + 256 );

   }
  }
 }else{
  if( Acc->bufsize <= num_bytes_to_read ){
   ExpandSVR( Acc, num_bytes_to_read + 1 );
  }//endif
  Acc->len = fread( (void*)Acc->buf, sizeof(char), num_bytes_to_read, f->fp );
 } 
 // ----------
 if( sa_l == 0 ){
  SAL=-1;
 }
 return SVRtoSVL( Acc );
}

// ----------------------------------------------------------------------------------------------

#ifdef using_johnsonlib_graphics

#define myxlib_notstandalonetest
#include "NewBase.c"

// because of the weird reverse order of evaluation of function parameters in GNU C on some architectures, this is necessary... why am I writing this shit, nobody will read it

// ---- johnsonscript graphics commands ----
#ifdef JOHNSON_PARAMETERS_REVERSED
void Johnson_StartGraphics(int h,int w){ 
 start_newbase_thread(w,h);
}
#endif

#ifdef JOHNSON_PARAMETERS_REVERSED
void Johnson_WinSize(int h, int w){
 SetWindowSize(w,h);
 Wait(1);
}
#endif

void Johnson_Pixel(int *pixels, int n_pixels){
 int i;
 for(i=0; i<n_pixels; i+=2){
  Plot69(pixels[i],pixels[i+1]);
 }
}

void Johnson_Line(int *lines, int n_points){
 int i;
 Line(lines[0],lines[1],lines[2],lines[3]);
 for(i=2; i<n_points-2; i+=2){
  Line(
       lines[i  ], lines[i+1],
       lines[i+2], lines[i+3]
  );
 }
}

#ifdef JOHNSON_PARAMETERS_REVERSED
void Johnson_Circle(int fill,int r, int y, int x){
 if(fill)
  CircleFill(x,y,r);
 else
  Circle(x,y,r);
}
#endif

#ifdef JOHNSON_PARAMETERS_REVERSED
// if parameters not reversed, in the case of rectangle X Y W H just call Rectangle/RectangleFill directly
void Johnson_Rectangle(int fill, int h, int w, int y, int x){
 if(fill)
  RectangleFill(x,y,w,h);
 else
  Rectangle(x,y,w,h);
}
#endif

// but in the case of rectangle X Y WH this is necessary
#ifdef JOHNSON_PARAMETERS_REVERSED
void Johnson_RectangleWH(int fill, int wh, int y, int x)
#else
void Johnson_RectangleWH(int x, int y, int wh, int fill)
#endif
{
 if(fill)
  RectangleFill(x,y,wh,wh);
 else
  Rectangle(x,y,wh,wh);
}

void Johnson_Triangle( int points[6] ){
 Triangle(points[0],points[1], points[2],points[3], points[4],points[5] );
}

#ifdef JOHNSON_PARAMETERS_REVERSED
void Johnson_DrawText( SVL sv, int s, int y, int x, int sa_l )
#else
void Johnson_DrawText( int sa_l, int x, int y, int s, SVL sv )
#endif
{
 char holdthis = sv.buf[sv.len]; sv.buf[sv.len]=0;
 drawtext_(x,y,s, sv.buf);
 sv.buf[sv.len]=holdthis;
 // ----------
 if( sa_l == 0 ){
  SAL=-1;
 }
}

void Johnson_RefreshMode(int m){
 switch(m){
 case 0:
  RefreshOn();
  break;
 case 1:
  RefreshOff();
  break;
 }
}

#ifdef JOHNSON_PARAMETERS_REVERSED
void Johnson_GCol(int b,int g,int r){
 Gcol(r,g,b);
}
#endif

#ifdef JOHNSON_PARAMETERS_REVERSED
int Johnson_BGCol(int b,int g,int r){
 GcolBG(r,g,b);
}
#endif

// ---- johnsonscript graphics functions ---- 

int Johnson_Expose(){
 int expo = exposed;
 exposed = 0;
 return expo;
}

int Johnson_WMClose(){
 int wmc = wmclosed;
 wmclosed = 0;
 return wmc;
}

int Johnson_BmpValidityCheck( SVL bmpstring ){
 if( bmpstring.len <= 54 ) return 0;
 unsigned int offset     = *(unsigned int*)(bmpstring.buf + 2+4+2+2);
 unsigned int image_size = *(unsigned int*)(bmpstring.buf + 2+4+2+2+ 4+4+4+4+2+2+4);
 if( offset + image_size > (unsigned int)bmpstring.len ){
  printf("Johnsonscript error: drawbmp: invalid bitmap\n");
  return 0;
 }
 return 1;
}

#endif

// ----------------------------------------------------------------------------------------------

void Johnson_Seedrnd(int v[3]){
 XRANDrand = v[0];
 XRANDranb = v[1];
 XRANDranc = v[2];
}

// ----------------------------------------------------------------------------------------------

int Johnson_Argc = 0;

void Johnsonlib_init(int argc, char **argv){
 int i;
 // --- string accumulator ---
 StringAcc = calloc(max_stringacc,sizeof(void*));
 Johnsonlib_MemoryAllocFailCheck( StringAcc, "StringAcc init");
 for(i=0;i<max_stringacc;i++){
  StringAcc[i]=calloc(1,sizeof(SVR));
  ExpandSVR( StringAcc[i], 256 );
 }
 // --- string variables ---
 StringVars = calloc(max_stringvars,sizeof(void*));
 Johnsonlib_MemoryAllocFailCheck( StringVars, "StringVars init");
 for(i=0;i<max_stringvars;i++){
  StringVars[i]=calloc(1,sizeof(SVR));
  ExpandSVR( StringVars[i], 256 );
  StringVars[i]->string_variable_number=i;
 }
 // --- files ---
 Johnson_files = calloc(max_files,sizeof(void*));
 Johnsonlib_MemoryAllocFailCheck( Johnson_files, "Johnson_files init");
 for(i=0;i<max_files;i++){
  Johnson_files[i] = calloc(1,sizeof(FIL));
 }
 Johnson_files[0]->open=1; Johnson_files[0]->read_access=1;  Johnson_files[0]->fp = stdin;
 Johnson_files[1]->open=1; Johnson_files[1]->write_access=1; Johnson_files[1]->fp = stdout;
 Johnson_files[2]->open=1; Johnson_files[2]->write_access=1; Johnson_files[2]->fp = stderr;
 // --- function table ---
 Johnson_function_table = calloc(256,sizeof(void*));
 Johnsonlib_MemoryAllocFailCheck( Johnson_function_table, "Johnson function table");
 // arguments
 for(i=1; i<argc; i++){
  SetSVR( NewStringvar_svrp(), ToSVL( argv[i] ) );
 }
 Johnson_Argc = argc-1;
 // seed random number generator
 SeedRng();
}


#ifndef using_johnsonlib
int main(int argc,char **argv){
 // ----- init ------------
 Johnsonlib_init(argc,argv);
 // -----------------------
 #if 0
 int filehandle = Johnson_OpenFile( LeftS( ToSVL((char[]){"filewritetest.txtFUCK"}), 17 ) , J_OPENOUT);
 if( filehandle ){
  Johnson_Sput(filehandle, ToSVL("test string\n"));
 }else{
  printf("couldn't open!\n");
 }
 //printf("%s\n", (char[]){"what happens if I do this?"} );
 #else
 int filehandle = Johnson_OpenFile( LeftS( ToSVL((char[]){"filewritetest.txtFUCK"}), 17 ) , J_OPENIN);
 PrintSVL( Johnson_Sget( filehandle, -1, ++SAL ) );
 #endif
 Johnson_Close(filehandle);
 //Johnson_Sput(2, ToSVL("should be stdout\n"));
 return 0;
}
#endif

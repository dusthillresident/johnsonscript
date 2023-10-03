#include <stdio.h>
#include <stdlib.h>
// ============== testbeep =======================

#define tb() TB_(__LINE__,__FILE__)
void TB_(int n,char *f){
 fprintf(stderr,"tb %d, %s\n",n,f);
 if( system("beep -f 100 -l 2 &") );
}

// =============== file routines in the style of BBC BASIC ===============

int Ext(FILE *fp){
 int result,currentposition;
 currentposition=ftell(fp);
 fseek(fp, 0, SEEK_END); result=ftell(fp);
 fseek(fp, currentposition, SEEK_SET);
 return result;
}

/*
int Ptr(FILE *fp){
 return ftell(fp);
}*/

int SetPtr(FILE *fp, int pos){
 //int ex = Ext(fp);
 if(pos < 0){
  fprintf( stderr, "SetPr negative: %d\n",pos);
  pos=0;
 }
 return fseek(fp, pos, SEEK_SET);
}

/*
FILE* Openin(char *filename){
 return fopen(filename,"rb");
}

FILE* Openout(char *filename){
 return fopen(filename,"wb");
}

FILE* Openup(char *filename){
 return fopen(filename,"r+b");
}
*/

// =============== RNG ===================

unsigned long long int _rnd_v1, _rnd_v2;
#define RND_V1M (unsigned long long int)0x91abe813a357d
#define RND_V1L (unsigned long long int)0x1db61b5bde843616
unsigned int dhr_random_u32(){
 unsigned long long int m1 = _rnd_v1 & 0xffffffffffff;
 _rnd_v1 = (_rnd_v1 + m1 + 1)*(RND_V1M+m1); if(_rnd_v1>RND_V1L){ _rnd_v1=(RND_V1L*m1)+_rnd_v2; _rnd_v2=m1; }
 return (unsigned int)m1 ^ (unsigned int)(_rnd_v1 >> 48);
}

// state_return must be a char buffer of length 29 at least, to contain 28 hex characters and a 0 null terminator 
void dhr_random__save_state( char *state ){
 // convert to hex charagers
 sprintf(state,"%016llx%012llx", _rnd_v1, _rnd_v2);
}

int dhr_random__load_state( char *state ){
 char buf[29];
 strncpy( buf,state, 28 );
 size_t l = strlen(state);
 // if we're given an invalid string that's too short,
 // then let's at least pad it with '1's so we (hopefully) 
 // have less of a chance of ending up with a state that's really broken...
 if( l<28 ){
  for(int i=l; i<28; i++)
   buf[i]='1';
 }
 buf[28]=0;
 _rnd_v1 = 0; _rnd_v2 = 0;
 int ret = sscanf( buf, "%016llx%012llx", &_rnd_v1, &_rnd_v2 ) == 2;
 _rnd_v2 = _rnd_v2 & 0xffffffffffff;
 return ret;
}

int Rnd(int n){
 unsigned long int r = dhr_random_u32();
 if( !n )
  return r;
 else
  return (int)(r & 0x7fffffff) % n;
}

#ifndef TimeConflictBullshit
#include <time.h>
void SeedRng(){
 unsigned long long int t = time(NULL);
 unsigned long long int a=0, b=0;
 for(int i=0; i<32; i++){
  a = (a<<1) | !!(t & (1 << i));
  b = (b<<1) | !!(t & (2 << i));
 }
 _rnd_v1 = (a * 0xf3259af + t) % RND_V1L;
 _rnd_v2 = (b * 0xa35d391 + ~t) & 0xffffffffffff;
 for(int i=0; i<8; i++){
  dhr_random_u32();
 }
}
#endif

// =============== check if a file already exists ===================

int Exists(char *s){
 FILE *T = fopen(s,"rb");
 if( T==NULL )
  return 0;
 else {
  fclose(T); 
  return 1;
 }
}

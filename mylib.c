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


unsigned long long int _rnd_v; // provides a state seed for Rnd()
// dhr_random, version "special4"
// This returns a Special Number given a seed 64 bit seed integer 'n'.
//  ------- WARNING ---------------------------------------------------------------------------------
//  Special Numbers are NOT the same as pseudo-random numbers.
//  You must NOT use this function for serious scientific, statistical, or mathematical applications.
unsigned int dhr_random_u32(unsigned long long int n){
 unsigned long long int m,v,v2;
 v  = n*0xdbc7f5af2198a8d1;
 v2 = (n>>15)^0x79f9bae8bbde;
 const unsigned long long int vm = 0x91abe813a357d;
 m = 0x27763eac5a46 *(v>>14);
 v = (v+m+1)*(vm-m)^(v2>>1);
 v = (v2*v)^(v*(m-1));
 return (unsigned int)(v ^ (v>>32));
}

// 'state' must be a buffer that's at least 65 (64 hex characters and '0' null terminator)
void dhr_random__save_state( char *state ){
 sprintf(state,"%016llx", _rnd_v);
}

void dhr_random__load_state( char *state ){
 sscanf(state,"%016llx",&_rnd_v);
}

int Rnd(int n){
 unsigned int r = dhr_random_u32(_rnd_v++);
 if( !n )
  return r;
 else
  return (int)(r & 0x7fffffff) % n;
}

#ifndef TimeConflictBullshit
#include <time.h>
void SeedRng(){
 unsigned long long int t = time(NULL);
 _rnd_v = ((unsigned long long int)dhr_random_u32(t) << 32) | (unsigned long long int)dhr_random_u32(~t);
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

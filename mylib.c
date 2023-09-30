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

double _rnd_v;
// Warning: output sequence changes when gcc's -Ofast flag is enabled
// Statistical testing was done with -Ofast on
double dhr_random(){
 double mviv = _rnd_v - (int)_rnd_v;				
 _rnd_v=(_rnd_v+34.19249667096534) * 82.41991188303562 * (93.7185494935692-mviv);	
 if( _rnd_v > 8321.787723770267 ){					
  _rnd_v = mviv+(8321.787723770267*(_rnd_v-(int)_rnd_v));			
 }							
 return mviv;					
}

void dhr_random__seed(double seed){
 int isNanOrInf(double x){
  x=x*0.0; // (inf * 0) becomes -nan
  if( x==0.0 && (int)x ){ // all comparisons between nan and any other number seem to evaluate to 1, and (int)nan becomes -2147483648
   return 1;
  }
  return 0;
 }
 double constrain(double x, double max){
  if(isNanOrInf(x)) return 0.0;
  if( x < 0.0 ) x = -x;
  if( x > max ){
   x = (x/max);
   x = max*(x-(int)x);
  }
  return x;
 }
 _rnd_v  = constrain( seed,  8321.787723770267  );
}

double dhr_random__current_seed(){
 return _rnd_v;
}

int Rnd(int n){
 double r = dhr_random();
 if( n == 0 )
  return (int)(unsigned int)(r * (double)0x100000000);
 else
  return n * r;
}

#ifndef TimeConflictBullshit
#include <time.h>
void SeedRng(){
 unsigned int t = time(NULL);
 t = ((t<<24) | (t>>8)) ^ 0xaaaaaaaa;
 _rnd_v = ((double)t / (double)0x100000000) * 8321.787723770267 ;
 //fprintf(stderr, "_rnd_v is now %f\n",_rnd_v);
 for(int i=0; i<16; i++){
  dhr_random();
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

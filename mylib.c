// ============== testbeep =======================

#define tb() TB_(__LINE__,__FILE__)
void TB_(int n,char *f){
 printf("tb %d, %s\n",n,f);
 system("beep -f 100 -l 2 &");
}

// =============== load and save ints to files ===============

int getint(FILE *fp){
 int i,r=0;
 for(i=0;i<4;i+=1){
  r=r | (fgetc(fp)<<(8*i));
 }
 return r;
}

void putint(int in, FILE *fp){
 int i;
 for(i=1;i<=4;i++){
  fputc(in & 255,fp);
  in=in>>8;
 }
}

// ============== short ===============

short int getshortint(FILE *fp){
 short int i,r=0;
 for(i=0;i<2;i+=1){
  r=r | (fgetc(fp)<<(8*i));
 }
 return r;
}

void putshortint(short int in, FILE *fp){
 fputc(in & 255,fp);
 fputc((in>>8) & 255,fp);
}


// =============== file routines in the style of BBC BASIC ===============

int Ext(FILE *fp){
 int result,currentposition;
 currentposition=ftell(fp);
 fseek(fp, 0, SEEK_END); result=ftell(fp);
 fseek(fp, currentposition, SEEK_SET);
 return result;
}

int Ptr(FILE *fp){
 return ftell(fp);
}

int SetPtr(FILE *fp, int pos){
 //int ex = Ext(fp);
 if(pos < 0){
  printf("SetPr negative: %d\n",pos);
  pos=0;
 }
 return fseek(fp, pos, SEEK_SET);
}

FILE* Openin(char *filename){
 return fopen(filename,"rb");
}

FILE* Openout(char *filename){
 return fopen(filename,"wb");
}

FILE* Openup(char *filename){
 return fopen(filename,"r+b");
}

// =============== RNG ===================

unsigned long int XRANDrand=0xA43C58DF;
unsigned long int XRANDranb=0xE629FDC4;
unsigned long int XRANDranc=0x18A7FC21;

void XRANDswap(unsigned long int *a,unsigned long int *b){
 unsigned long int c=*a;
 *a=*b;
 *b=c;
}

int Rnd(int in){
 XRANDswap(&XRANDrand,&XRANDranb);

 XRANDrand=(XRANDrand<<15) | ((XRANDrand>>16)&0b00000000000000001111111111111111);
 XRANDrand=(~XRANDrand)<<1 | ((~XRANDrand)>>31 & 1);
 XRANDrand=XRANDrand ^ ((XRANDranb>>3)&0b00011111111111111111111111111111);
 XRANDrand=(XRANDrand<<8) | ((XRANDrand>>23)&0b00000000000000000000000111111111);
 XRANDranc=(XRANDranc<<1) | (XRANDrand&1);
 XRANDrand=XRANDrand ^ (XRANDranc & 256);

 if( in == 0) return XRANDrand;
 return (XRANDrand & 0b01111111111111111111111111111111) % in;
}

#ifndef TimeConflictBullshit
#include <time.h>
void SeedRng(){
 XRANDrand=(unsigned long int) time(NULL);
 Rnd(2); Rnd(2);
 XRANDranb=(unsigned long int) ~time(NULL);
 Rnd(2); Rnd(2);
 XRANDranc=(unsigned long int) time(NULL)<<3;
 Rnd(2); Rnd(2);
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


#ifndef _MYLIB_H

// ============== testbeep =======================

#define tb() TB_(__LINE__,__FILE__)
void TB_(int n,char *f);

// =============== file routines in the style of BBC BASIC ===============

int Ext(FILE *fp);

int SetPtr(FILE *fp, int pos);

// =============== RNG ===================

//static double _rnd_v;					

double dhr_random_u32(unsigned long long int n);

void dhr_random__save_state( char * );
void dhr_random__load_state( char * );

int Rnd(int n);

void SeedRng();

// =============== check if a file already exists ===================

int Exists(char *s);

#define _MYLIB_H
#endif
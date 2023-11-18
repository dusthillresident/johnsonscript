
#define pf(...) fprintf(stderr, __VA_ARGS__)
void interactive_help(char *buf){
 if( ! strcmp( "help\n", buf ) ){
  pf(
"\
---- Help topics ----\n\
 introduction	language\n\
 expressions	variables	functions\n\
 referencing	arrays		vectors\n\
 keywords	controlflow\n\
 commands	numerics	strings\n\
 consoleio	fileio		graphics\n\
");
 }else{
 }
}
#undef pf
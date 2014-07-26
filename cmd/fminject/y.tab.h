#ifndef YYERRCODE
#define YYERRCODE 256
#endif

#define COMMA 257
#define COUNT 258
#define ECLASS 259
typedef union {
	char *strval;
	int intval;
} YYSTYPE;
extern YYSTYPE yylval;

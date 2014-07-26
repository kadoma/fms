%{
/**
 * fminject.y
 *
 *  Created on: Mar 1, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *  	fminject.y
 */

#include <stdio.h>
%}

%union {
	char *strval;
	int intval;
}

%token COMMA
%token COUNT ECLASS
%type <strval> ECLASS
%type <intval> repeat COUNT

%%
stmtlist: stmtlist stmt
	| stmt
	;

stmt: ECLASS repeat
	{
		fm_inject($1, $2);
	}
	;

repeat: COMMA COUNT
	{
		$$ = $2;
	}
	|
	{
		$$ = 1;
	}
	;

%%
void
yyerror(char *msg)
{
	fprintf(stderr, "%s\n", msg);
}

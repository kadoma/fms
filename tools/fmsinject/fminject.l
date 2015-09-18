%{
/**
 * fminject.l
 *
 *  Created on: Mar 1, 2010
 *      Author: nilei
 *
 *  Description:
 *  	fminject.l
 */

#include <stdio.h>
#include <string.h>
#include "y.tab.h"
%}

ws			[ \t]+
comment		(\/\/[^\n]*)|(\/\*[^\/]*\*\/)
evtclass	^ereport(\.[a-zA-Z0-9\-_]+)+
repeat		N=[0-9]+

%%
{ws}	;

{comment}	{
		printf("comment:%s\n", yytext);
	}

{evtclass}	{
		printf("evtclass %s\n", yytext);
		yylval.strval = strdup(yytext);
		return ECLASS;
	}

{repeat}	{
		printf("repeat %s\n", yytext);
		yylval.intval = (int)repval(yytext);
		return COUNT;
	}

,	{return COMMA;}

.	;

%%
int
yywrap(void)
{
	return 1;
}

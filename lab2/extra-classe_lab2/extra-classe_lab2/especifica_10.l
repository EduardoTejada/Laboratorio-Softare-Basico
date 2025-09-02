%{
#include <stdio.h>
%}

%%
#!.*\n		{ for(int i = 0; yytext[i] != '\n'; i++) printf("%c", yytext[i]); }
\$#		{ ECHO; }
#.*\n		{ ; }
.		{ ECHO; }
%%

int main() {
    yylex();
    return 0;
}

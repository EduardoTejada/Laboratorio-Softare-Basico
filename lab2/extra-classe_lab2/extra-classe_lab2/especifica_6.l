%{
		 int InverteInt();
%}

%%

[.]             ;
var             ;
[{]             printf("/*");
[}]             printf("*/");
mod             printf("%%");
or              printf("||");
and             printf("&&");
begin           ;
end             printf("}");
if              printf("if (");
then            printf(")");
program.*[(].*$ printf("int main()\n{");
[^:><][=]       printf("==");
[:][=]          printf("=");
[<][>]          printf("!=");
^.*integer;     InverteInt();
.               ECHO;

%%

int InverteInt()
{ int i;
  printf("int ");  for(i=0; yytext[i]!=':'; i++) printf("%c", yytext[i]);
  printf(";");
}


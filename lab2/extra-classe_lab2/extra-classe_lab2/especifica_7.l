%{
int linhas=0,simbolos=0,palavras=0,paginas=1;
%}
 
%START		NovaPalavra
NovaLinha	[\n]
Espaco		[\t ]
NovaPagina	[\f]
   
%%

{Espaco}	{ BEGIN 0; simbolos++; }
{NovaLinha}	{ BEGIN 0; simbolos++; linhas++; }
{NovaPagina}	{ BEGIN 0; simbolos++; paginas++; }
<NovaPalavra>.	{ simbolos++; }
.		{ BEGIN NovaPalavra; simbolos++; palavras++; }

%%

int main()
{
  while (yylex());
  printf("\nResultados: \n%d pagina(s)\n%d linha(s)\n%d palavra(s)\n%d caracter(es)\n", paginas,linhas,palavras,simbolos);
}

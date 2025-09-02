%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *unidades[]= {"primeiro","um","dois","tres","quatro","cinco",
                   "seis", "sete","oito","nove"};
char *interm[]  = {"dez","onze","doze","treze",
                   "quatorze","quinze","dezesseis",
                   "dezessete","dezoito","dezenove"};
char *dezenas[] = {"vinte","trinta","quarenta","cinquenta",
                   "sessenta","setenta","oitenta","noventa"};
char *meses[]   = {"janeiro","fevereiro","marco","abril","maio",
                   "junho","julho","agosto","setembro",
                   "outubro","novembro","dezembro"};

int dia, mes, ano; // Variaveis para armazenar os equivalentes inteiros da entrada
char stringDia[20] = "", stringMes[10], stringAno[30]; // Variaveis para salvar as strings correspondents de cada campo

void mostrarData(char * sd, char * sm, char * sa); // Cabecalho de funcao para mostrar a data
void dezenaPorExtenso(int num, char * strMsg); // Cabecalho de funcao para converter uma dezena numerica em um texto por extenso

%}

PrimeiroCampo  ((0[1-9])|(1[0-9])|(2[0-9])|(3[0-1]))[/]
SegundoCampo   ((0[1-9])|(1[0-2]))[/]
TerceiroCampo  [0-9][0-9]
DATA           {PrimeiroCampo}{SegundoCampo}{TerceiroCampo}

%%


{DATA}	                {
			  dia = atoi(yytext);	  // Recebe o dado somente se estiver na formatacao correta e entao salva cada numero dos campos na respectiva variavel convertido pra inteiro
			  mes = atoi(yytext + 3); // Passa os dois primeiros digitos e a barra
			  ano = atoi(yytext + 6); // Passa os dois digitos do segundo campo e a barra
			  
			  if(dia == 1) strcpy(stringDia, unidades[0]); // Dia 1 - Primeiro
			  else if(dia < 10) strcpy(stringDia, unidades[dia]); // Dia quatro, cinco, seis
			  else if(dia < 20) strcpy(stringDia, interm[dia-10]);// Dia onze, doze, treze
			  else dezenaPorExtenso(dia, stringDia); // Chama a funcao responsavel por tratar quando mistura dezena e unidade
			  
			  strcpy(stringMes, meses[mes-1]); // O mes pode ser convertido facilmente

			  strcpy(stringAno, "dois mil"); // Todos os anos vao comecar com dois mil
			  if(ano > 0) { strcat(stringAno, " e "); // Caso seja apenas dois mil nao precisa do " e ", senao precisa
                                if(ano < 10) strcat(stringAno, unidades[ano]); // dois mil e quatro, cinco, seis
                                else if(ano < 20) strcat(stringAno, interm[ano-10]); // dois mil e onde, doze, treze
                                else dezenaPorExtenso(ano, stringAno);} // Chama a funcao responsavel por tratar quando mistura dezena e unidade


			  mostrarData(stringDia, stringMes, stringAno); // Mostra a data no formato correto
			  strcpy(stringDia, ""); // Reinicia o dia (necessario pois em alguns casos o dia e somente concatenado e nao iniciado)
			}
.			; { /* Caso recebe qualquer outra coisa ignora */ }


%%

void mostrarData(char * sd, char * sm, char * sa){
	printf("%s de %s de %s", sd, sm, sa);
}


void dezenaPorExtenso(int num, char * strMsg){
	int dez  = (int) num / 10; // Separa o digito da dezena
	int uni = num - dez*10; // Separa o digito da centena
	strcat(strMsg, dezenas[dez-2]); // A dezena converte pra vinte, trinta, quarenta (esta concatenando e nao copiando pois no caso do ano a string chega nessa etapa ja com "dois mil e ..."
	if(uni == 0) return; // Caso a unidade seja zero fica somente a dezena
	strcat(strMsg, " e "); // Caso contrario coloca o " e "
	strcat(strMsg, unidades[uni]); // e mostra a unidade "vinte e cinco"
}

int main() {
    yylex();
    return 0;
}

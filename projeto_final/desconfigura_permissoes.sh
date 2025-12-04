#!/bin/bash

# restaura teste.txt
chmod a+rw webspace_para_testes/teste.txt

# restaura permissão de diretório dir
chmod a+rwx webspace_para_testes/dir

# restaura os arquivos que perderam leitura
chmod a+rw webspace_para_testes/dir3/index.html
chmod a+rw webspace_para_testes/dir4/index.html
chmod a+rw webspace_para_testes/dir4/welcome.html

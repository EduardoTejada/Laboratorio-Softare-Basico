#!/bin/bash

# Limpa o arquivo de registro para a nova execução
> registro.txt

echo "=== INICIANDO TESTES ==="

# --- Requisições GET ---
echo -e "\n--- Testando GET ---"
./servidor meu-webspace reqs/req_GET_dir1.txt resp/resp_GET_dir1.txt registro.txt
head -20 resp/resp_GET_dir1.txt

./servidor meu-webspace reqs/req_GET_dir2.txt resp/resp_GET_dir2.txt registro.txt
head -20 resp/resp_GET_dir2.txt

./servidor meu-webspace reqs/req_GET_dir11.txt resp/resp_GET_dir11.txt registro.txt
head -20 resp/resp_GET_dir11.txt

./servidor meu-webspace reqs/req_GET_index.txt resp/resp_GET_index.txt registro.txt
head -20 resp/resp_GET_index.txt

./servidor meu-webspace reqs/req_GET_texto1.txt resp/resp_GET_texto1.txt registro.txt
head -20 resp/resp_GET_texto1.txt

./servidor meu-webspace reqs/req_GET_texto2.txt resp/resp_GET_texto2.txt registro.txt
head -20 resp/resp_GET_texto2.txt


# --- Requisições HEAD ---
echo -e "\n--- Testando HEAD ---"
./servidor meu-webspace reqs/req_HEAD_dir1.txt resp/resp_HEAD_dir1.txt registro.txt
cat resp/resp_HEAD_dir1.txt

./servidor meu-webspace reqs/req_HEAD_dir2.txt resp/resp_HEAD_dir2.txt registro.txt
cat resp/resp_HEAD_dir2.txt

./servidor meu-webspace reqs/req_HEAD_dir11.txt resp/resp_HEAD_dir11.txt registro.txt
cat resp/resp_HEAD_dir11.txt

./servidor meu-webspace reqs/req_HEAD_test.txt resp/resp_HEAD_test.txt registro.txt
cat resp/resp_HEAD_test.txt

./servidor meu-webspace reqs/req_HEAD_texto1.txt resp/resp_HEAD_texto1.txt registro.txt
cat resp/resp_HEAD_texto1.txt

./servidor meu-webspace reqs/req_HEAD_texto2.txt resp/resp_HEAD_texto2.txt registro.txt
cat resp/resp_HEAD_texto2.txt


# --- Requisições OPTIONS ---
echo -e "\n--- Testando OPTIONS ---"
./servidor meu-webspace reqs/req_OPTIONS.txt resp/resp_OPTIONS.txt registro.txt
cat resp_OPTIONS.txt


# --- Requisições TRACE ---
echo -e "\n--- Testando TRACE ---"
./servidor meu-webspace reqs/req_TRACE.txt resp/resp_TRACE.txt registro.txt
cat resp/resp_TRACE.txt


# --- Fim dos testes ---
echo -e "\n=== CONTEÚDO DO REGISTRO ==="
cat registro.txt

echo -e "\n=== TESTES CONCLUÍDOS ==="
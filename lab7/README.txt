# Servidor HTTP Monoprocesso

## Compilação
make mono

## Execução
./servidor_mono [endereco_IP] [porta]
./servidor_mono 0.0.0.0 3500



# Servidor Multiprocesso

## Compilação
make poll

## Execução
./servidor_multi_processo [endereco_IP] [porta] [max_processos]
./servidor_multi_processo 0.0.0.0 3500 8



# Servidor Multithread

## Compilação
make thread

## Execução
./servidor_multi_thread [endereco_IP] [porta] [max_threads]
./servidor_multi_thread 0.0.0.0 3500 8
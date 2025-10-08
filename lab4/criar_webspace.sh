# Criar o diretório principal
mkdir meu-webspace

# Criar os subdiretórios
mkdir meu-webspace/dir1
mkdir meu-webspace/dir1/dir11
mkdir meu-webspace/dir2

# Criar os arquivos
touch meu-webspace/index.html
touch meu-webspace/dir1/texto1.html
touch meu-webspace/dir1/texto2.html

# Diretório principal - leitura, escrita e execução para owner
chmod 700 meu-webspace

# Arquivo index.html - leitura para owner
chmod 400 meu-webspace/index.html

# Diretório dir1 - leitura, escrita e execução para owner
chmod 700 meu-webspace/dir1

# Diretório dir11 - leitura, escrita e execução para owner
chmod 700 meu-webspace/dir1/dir11

# Arquivo texto1.html - leitura para owner
chmod 400 meu-webspace/dir1/texto1.html

# Arquivo texto2.html - sem permissão de leitura para owner
chmod 000 meu-webspace/dir1/texto2.html

# Diretório dir2 - leitura e escrita, sem execução para owner
chmod 600 meu-webspace/dir2

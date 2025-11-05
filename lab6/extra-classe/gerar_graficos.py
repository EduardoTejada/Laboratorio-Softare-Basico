import matplotlib.pyplot as plt
import numpy as np

# Dados exemplo - substitua com seus resultados reais
threads_processes = [1, 2, 4, 8, 16, 32, 64]
multithread_perf = [561.8, 283.1, 472, 465.5, 121.8, 120.9, 126.9]  # fetches/sec
multiprocess_perf = [39354.8, 120.3, 198, 205.4, 185.9, 195.2, 190.1]  # fetches/sec
monoprocess_perf = [92.4, 77.3, 83.1, 86.7, 56, 62.4762, 72.9996]  # fetches/sec

plt.figure(figsize=(12, 8))

# Gráfico comparativo
plt.subplot(2, 1, 1)
plt.ylim(top=600)
plt.plot(threads_processes, multithread_perf, 'bo-', label='Multithread', linewidth=2)
plt.plot(threads_processes, multiprocess_perf, 'ro-', label='Multiprocess', linewidth=2)
plt.plot(threads_processes, monoprocess_perf, 'go-', label='Monoprocess', linewidth=2)
plt.xlabel('Número de Threads/Processos')
plt.ylabel('Requisições por Segundo')
plt.title('Comparação de Desempenho - Servidores HTTP')
plt.legend()
plt.grid(True)

# Gráfico foco multithread vs multiprocess
plt.subplot(2, 1, 2)
plt.ylim(top=600)
plt.plot(threads_processes, multithread_perf, 'bo-', label='Multithread', linewidth=2)
plt.plot(threads_processes, multiprocess_perf, 'ro-', label='Multiprocess', linewidth=2)
plt.xlabel('Número de Threads/Processos')
plt.ylabel('Requisições por Segundo')
plt.title('Multithread vs Multiprocess')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.savefig('desempenho_servidores.png', dpi=300)
plt.show()
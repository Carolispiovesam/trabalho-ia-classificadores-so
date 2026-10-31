import pandas as pd
import matplotlib.pyplot as plt
import os

print("A carregar os dados CSV...")
# 1. Carregar os dados gerados pelo seu programa em C
resumo = pd.read_csv('resumo.csv')
# Usamos a Repetição 1 da configuração de Referência (C=4, N=32) para os gráficos detalhados (G3 e G4)
eventos = pd.read_csv('eventos_C4_N32_rep1.csv')

# ---------------------------------------------------------
# G1: Capacidade x Vazão (Fixo N=32)
# ---------------------------------------------------------
print("A gerar o Gráfico 1...")
# Agrupa por Capacidade e calcula média, mínimo e máximo da vazão
g1_data = resumo[resumo['N'] == 32].groupby('C')['vazao_V'].agg(['mean', 'min', 'max']).reset_index()
g1_data['erro_min'] = g1_data['mean'] - g1_data['min']
g1_data['erro_max'] = g1_data['max'] - g1_data['mean']

plt.figure(figsize=(8, 5))
plt.errorbar(g1_data['C'], g1_data['mean'], yerr=[g1_data['erro_min'], g1_data['erro_max']], 
             fmt='-o', capsize=5, color='blue', linewidth=2, markersize=8)
plt.title('G1: Vazão por Capacidade (N=32)')
plt.xlabel('Capacidade do Carrinho (C)')
plt.ylabel('Vazão (passageiros/s)')
plt.xticks([2, 4, 8])
plt.grid(True, linestyle='--', alpha=0.7)
plt.savefig('G1_Capacidade_x_Vazao.png', bbox_inches='tight')
plt.close()

# ---------------------------------------------------------
# G2: Total de Passageiros x p95 (Fixo C=4)
# ---------------------------------------------------------
print("A gerar o Gráfico 2...")
# Agrupa por Quantidade de Passageiros e calcula as métricas do p95
g2_data = resumo[resumo['C'] == 4].groupby('N')['espera_p95'].agg(['mean', 'min', 'max']).reset_index()
g2_data['erro_min'] = g2_data['mean'] - g2_data['min']
g2_data['erro_max'] = g2_data['max'] - g2_data['mean']

plt.figure(figsize=(8, 5))
plt.errorbar(g2_data['N'], g2_data['mean'], yerr=[g2_data['erro_min'], g2_data['erro_max']], 
             fmt='-s', capsize=5, color='red', linewidth=2, markersize=8)
plt.title('G2: p95 da Espera por Total de Passageiros (C=4)')
plt.xlabel('Quantidade de Passageiros (N)')
plt.ylabel('Espera p95 (segundos)')
plt.xticks([16, 32, 64])
plt.grid(True, linestyle='--', alpha=0.7)
plt.savefig('G2_Passageiros_x_p95.png', bbox_inches='tight')
plt.close()

# ---------------------------------------------------------
# G3: Viagem x p95 da Espera (da Referência)
# ---------------------------------------------------------
print("A gerar o Gráfico 3...")
# Filtra os eventos para encontrar a chegada e o embarque de cada passageiro
embarques = eventos[(eventos['tipo'] == 'Passageiro') & (eventos['evento'] == 'Embarcou')].copy()
chegadas = eventos[(eventos['tipo'] == 'Passageiro') & (eventos['evento'] == 'Chegada na Fila')].copy()

# Cruza os dados de chegada e embarque usando o ID do participante
dados_viagem = pd.merge(chegadas[['participante', 'tempo']], 
                        embarques[['participante', 'tempo', 'id_operacao']], 
                        on='participante', suffixes=('_chegada', '_embarque'))

# Calcula o tempo real de espera (t_embarque - t_chegada)
dados_viagem['espera'] = dados_viagem['tempo_embarque'] - dados_viagem['tempo_chegada']

# Calcula o p95 para cada viagem individual (id_operacao)
g3_data = dados_viagem.groupby('id_operacao')['espera'].quantile(0.95).reset_index()

plt.figure(figsize=(8, 5))
plt.bar(g3_data['id_operacao'], g3_data['espera'], color='orange', edgecolor='black')
plt.title('G3: p95 da Espera por Viagem (C=4, N=32)')
plt.xlabel('Número da Viagem (Viagem Atribuída)')
plt.ylabel('Espera p95 dos passageiros (segundos)')
plt.xticks(g3_data['id_operacao'])
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.savefig('G3_Viagem_x_p95.png', bbox_inches='tight')
plt.close()

# ---------------------------------------------------------
# G4: Tempo x Ocupação do Carro (Linha do Tempo)
# ---------------------------------------------------------
print("A gerar o Gráfico 4...")
# Filtra apenas os eventos globais e de carrinho (ignora a fila)
eventos_ocupacao = eventos.sort_values(by='tempo')
eventos_carro = eventos[eventos['tipo'] == 'Carro'].copy()

plt.figure(figsize=(10, 5))
# Cria um gráfico em degraus (step) que sobe/desce conforme a lotação do carrinho muda
plt.step(eventos_ocupacao['tempo'], eventos_ocupacao['estado_ocupacao'], where='post', color='green', linewidth=2)

# Adiciona linhas verticais coloridas para marcar o início de cada fase do carrinho
for _, row in eventos_carro.iterrows():
    if row['evento'] == 'Fase: Passeando':
        plt.axvline(x=row['tempo'], color='purple', linestyle='--', alpha=0.5, label='Início do Passeio')
    elif row['evento'] == 'Fase: Desembarcando':
        plt.axvline(x=row['tempo'], color='red', linestyle='--', alpha=0.5, label='Início do Desembarque')
    elif row['evento'] == 'Fase: Embarcando':
        plt.axvline(x=row['tempo'], color='blue', linestyle='--', alpha=0.5, label='Início do Embarque')

# Remove legendas duplicadas no gráfico
handles, labels = plt.gca().get_legend_handles_labels()
by_label = dict(zip(labels, handles))
plt.legend(by_label.values(), by_label.keys(), loc='upper left')

plt.title('G4: Ocupação do Carrinho ao Longo do Tempo (C=4, N=32)')
plt.xlabel('Tempo da Execução (segundos)')
plt.ylabel('Passageiros Dentro do Carrinho')
plt.yticks(range(5)) # Eixo Y fixo entre 0 e 4
plt.grid(True, linestyle='--', alpha=0.5)
plt.savefig('G4_Tempo_x_Ocupacao.png', bbox_inches='tight')
plt.close()

print("\nSucesso! As 4 imagens (G1, G2, G3 e G4) foram geradas e guardadas na sua pasta.")
# 🚗🛜Protótipo de um Estacionamento Inteligente - Smart Parking

Sistema embarcado para gestão inteligente de vagas em estacionamento, com detecção de veículos via sensores, controle automático de cancelas, interface web em tempo real e painel informativo para motoristas.

---

## 📌Objetivo do Projeto 

Este projeto tem como objetivo desenvolver um sistema inteligente de automação para estacionamentos, com foco na **gestão eficiente de vagas** e no **controle automatizado de acesso veicular**, eliminando o controle manual de entrada e saída, disponibilizando em tempo real a ocupação das vagas e agilizando o fluxo de veículos.

O sistema busca resolver problemas comuns em estacionamentos públicos e privados, como:

- Falta de informações em tempo real sobre vagas disponíveis;
- Processos manuais de controle de entrada e saída;
- Dificuldades na gestão do fluxo de veículos.

---

#### Principais soluções propostas:

- Detecção automática de presença de veículos nas vagas;
- Controle automático de entrada e saída via cancela;
- Interface web responsiva;
- Painel físico mostrando disponibilidade de vagas;
- Registros de entrada, saída e pagamento.

---

## 👥Público-alvo

- **Proprietários e gestores** de estacionamentos que desejam reduzir custos operacionais e eliminar controle manual;
- **Usuários finais (motoristas)** que buscam agilidade e informações em tempo real sobre vagas disponíveis;
- **Técnicos e administradores** responsáveis pela operação do sistema, com acesso a ferramentas de monitoramento, relatórios e controle remoto de sensores;
- **Empresas e instituições** que desejam melhorar o controle de acesso veicular, como prédios comerciais, universidades, hospitais e condomínios.

---

## ⚙️Funcionalidades

- Detecção em tempo real de vagas ocupadas/livres com sensores;
- Controle automatizado das cancelas (entrada e saída);
- Interface web em tempo real mostrando situação atual das vagas;
- Painel informativo de status na entrada do estacionamento;
- Registro de histórico de movimentações;
- Cálculo de pagamento automático na saída;

---

## 🚀Tecnologias Utilizadas

- 🖥️**Microcontrolador:** BeagleBone Black
- 🔌**Sensoriamento:** Sensor de Obstáculo Infravermelho
- 🦾**Atuadores:** Servos Motores
- 🔗**Backend/Servidor:** Linguagem C (Hardware) com sockets TCP/IP
- 🖥️**Interface Web:**	HTML, CSS, JavaScript
- 🖼️**Painel físico:** Display LCD via pinos GPIO

---

## 📑Requisitos

### ➡️Requisitos Funcionais (RF):
- RF01: O sistema deve detectar a presença de veículos nas vagas por meio de sensores.
- RF02: O sistema deve abrir automaticamente a cancela quando um veículo se aproxima da entrada e há vaga disponível.
- RF03: O sistema deve impedir a entrada de veículos se todas as vagas estiverem ocupadas.
- RF04: O sistema deve registrar a entrada e a saída de cada veículo.
- RF05: O sistema deve atualizar em tempo real a quantidade de vagas disponíveis.
- RF06: O sistema deve disponibilizar uma interface web acessível para visualização das vagas ocupadas e disponíveis.
- RF07: O sistema deve exibir em tempo real na interface web o status de cada vaga (livre/ocupada).
- RF08: O sistema deve exibir uma mensagem na interface web alertando que o estacionamento está cheio quando todas as vagas estiverem ocupadas.
- RF09: O sistema deve possuir um painel local (display LCD ou LED) informando o número de vagas disponíveis na entrada do estacionamento.
- RF10: O sistema deve controlar automaticamente a cancela de saída.

### ➡️Requisitos Não Funcionais (RNF):
- RNF01: O sistema deve ter resposta em tempo real para atualizar o status das vagas na interface web.
- RNF02: O sistema deve ser acessível via dispositivos móveis e desktops, com interface responsiva.
- RNF03: O sistema deve manter a disponibilidade durante o horário de funcionamento do estacionamento.
- RNF04: A comunicação entre sensores e o sistema central deve ser segura e imune a interferências.
- RNF05: O código fonte do sistema deve ser modular para facilitar futuras atualizações.
- RNF06: Deve haver controle de posição para garantir que a cancela nunca fique entreaberta por falha.

---

## 🧪Testes
- Foram realizados testes para detecção correta de veículos nas vagas, utilizando sensores conectados à BeagleBone Black.
- Testes de atualização em tempo real da interface web para visualização de vagas livres e ocupadas.
- Validação do tempo de resposta da abertura automática da cancela (inferior a 3 segundos após detecção).
- Simulações de cenário com estacionamento cheio, exibindo corretamente a mensagem de lotação no painel e bloqueando entrada.
- Testes de responsividade da interface web em dispositivos móveis e computadores.
- Testes de detecção correta da saída do veículo, incluindo registro de horário e liberação da vaga.
- Testes físicos de funcionamento com 2 vagas, exibindo corretamente o status livre/ocupada tanto no display quanto na interface web.

---

## 📊Modelos e Diagramas

- 20 Histórias de Usuário no formato 3C;
- Diagrama de Classes;
- Diagrama de Atividades;
- Arquitetura do Sistema.

Acesse todos os detalhes no arquivo [documents/Protótipo de um Estacionamento Inteligente.pdf](https://github.com/nathalia0902/Prototipo-de-um-Estacionamento-Inteligente/blob/main/documents/Prot%C3%B3tipo%20de%20um%20Estacionamento%20Inteligente.pdf).

---

## 🗂️ Estrutura do Repositório

Prototipo-de-um-Estacionamento-Inteligente/
│
├── documents/                  # Documentação (PDF, diagramas, listas)
│   └── Protótipo de um Estacionamento Inteligente.pdf
│
├── projeto/                    #  Backend e Frontend do projeto
│   ├── bin/                     # Compilados do projeto (executáveis)
│   │   └── projeto
│   │
│   ├── inc/                     # Arquivos de cabeçalho (.h)
│   │   ├── gpio.h
│   │   ├── lcd1602.h
│   │   ├── main.h
│   │   ├── pwm.h
│   │   └── sensor.h
│   │
│   ├── obj/                     # Objetos compilados (.o)
│   │   ├── gpio.o
│   │   ├── lcd1602.o
│   │   ├── main.o
│   │   ├── pwm.o
│   │   └── sensor.o
│   │
│   ├── smart-parking/
│   │   ├── src/
│   │   │   ├── main.c
│   │   │   ├── server.c
│   │   │   ├── server.h
│   │   │   ├── vaga.c
│   │   │   └── vaga.h
│   │   │ 
│   │   ├── www/
│   │   │   ├── index.html
│   │   │   ├── script.js
│   │   │   └── style.css
│   │   │ 
│   │   ├── Makefile
│   │   └── smart-parking
│   │
│   ├── src/                     # Código-fonte principal (.c)
│   │   ├── Makefile
│   │   ├── gpio.c
│   │   ├── lcd1602.c
│   │   ├── main.c
│   │   ├── pwm.c
│   │   └── sensor.c
│   │
│   ├── Config.in                
│   ├── README.md                
│   ├── projeto.mk               # Makefile (projeto.mk) para compilação
│   ├── server                   # Executável do servidor
│   ├── server.c                 # Código principal do servidor
│   ├── setenv bootargs console.txt # Configuração de inicialização
│   └── videoProjeto.mp4         # Demonstração em vídeo do projeto
│
└── README.md                  # Descrição completa do projeto


## 🧑🏻‍💻Autores

- Beatriz de Sousa Alves – Engenharia da Computação – UFC Quixadá  
- Maria Eduarda Almeida Rodrigues – Engenharia da Computação – UFC Quixadá  
- Nathalia de Oliveira Lima – Engenharia da Computação – UFC Quixadá  
- Pablo Brandão Passos – Engenharia da Computação – UFC Quixadá
- Vitória das Graças Silva – Engenharia da Computação – UFC Quixadá

---


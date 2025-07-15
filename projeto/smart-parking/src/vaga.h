#ifndef VAGA_H
#define VAGA_H

#include <stdbool.h>
#include <time.h>

#define NUM_VAGAS 2
#define TAXA_POR_SEGUNDO 1.0

typedef struct {
    bool ocupada;
    time_t entrada;
    double valor_devido;
    char mensagem_pagamento[256];
} VagaInfo;

extern VagaInfo vagas[NUM_VAGAS];

int read_sensor(const char *path);
void atualizar_status_vagas(void);
void calcular_pagamento(int vaga_num);

#endif
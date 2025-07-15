#include "vaga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GPIO12 "/sys/class/gpio/gpio12/value"
#define GPIO13 "/sys/class/gpio/gpio13/value"

VagaInfo vagas[NUM_VAGAS] = {0};

int read_sensor(const char *path) {
FILE *f = fopen(path, "r");
if (!f) return -1;
    char buf[2];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    return atoi(buf) == 0 ? 1 : 0;
}

void calcular_pagamento(int vaga_num) {
if (vagas[vaga_num].ocupada) {
    time_t saida = time(NULL);
    double segundos = difftime(saida, vagas[vaga_num].entrada);
    vagas[vaga_num].valor_devido = segundos * TAXA_POR_SEGUNDO;


    char entrada_str[20], saida_str[20];
    strftime(entrada_str, sizeof(entrada_str), "%H:%M:%S", localtime(&vagas[vaga_num].entrada));
    strftime(saida_str, sizeof(saida_str), "%H:%M:%S", localtime(&saida));

    snprintf(vagas[vaga_num].mensagem_pagamento, sizeof(vagas[vaga_num].mensagem_pagamento),
             "Pagamento devido na Vaga %d:\nEntrada: %s\nSaída: %s\nTempo: %.0f segundos\nValor: R$ %.4f",
             vaga_num + 1, entrada_str, saida_str, segundos, vagas[vaga_num].valor_devido);
}
}

void atualizar_status_vagas(void) {
static int last_vaga1 = -1, last_vaga2 = -1;
int status1 = read_sensor(GPIO12);
int status2 = read_sensor(GPIO13);


if (status1 != last_vaga1) {
    if (status1 == 1 && !vagas[0].ocupada) {
        vagas[0].ocupada = true;
        vagas[0].entrada = time(NULL);
        memset(vagas[0].mensagem_pagamento, 0, sizeof(vagas[0].mensagem_pagamento));
    } else if (status1 == 0 && vagas[0].ocupada) {
        calcular_pagamento(0);
        vagas[0].ocupada = false;
    }
    last_vaga1 = status1;
}

if (status2 != last_vaga2) {
    if (status2 == 1 && !vagas[1].ocupada) {
        vagas[1].ocupada = true;
        vagas[1].entrada = time(NULL);
        memset(vagas[1].mensagem_pagamento, 0, sizeof(vagas[1].mensagem_pagamento));
    } else if (status2 == 0 && vagas[1].ocupada) {
        calcular_pagamento(1);
        vagas[1].ocupada = false;
    }
    last_vaga2 = status2;
}
}
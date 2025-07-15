#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 8080
#define TAXA_POR_SEGUNDO 1.0 // R$/hora
#define GPIO12 "/sys/class/gpio/gpio12/value"
#define GPIO13 "/sys/class/gpio/gpio13/value"
#define MAX_CLIENTS 10

typedef struct {
    bool ocupada;
    time_t entrada;
    double valor_devido;
    char mensagem_pagamento[256];
    char qr_code_data[512];  // Para armazenar dados do QR code
    bool pago;               // Novo campo para controlar se foi pago
} VagaInfo;

VagaInfo vagas[2] = {0};
int client_sockets[MAX_CLIENTS] = {0};
int num_clients = 0;

int read_sensor(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) return -1;

    char buffer[2];
    fgets(buffer, sizeof(buffer), file);
    fclose(file);
    return atoi(buffer) == 0 ? 1 : 0;
}

void calcular_pagamento(int vaga_num) {
    if (vagas[vaga_num].ocupada) {
        time_t saida = time(NULL);
        double segundos = difftime(saida, vagas[vaga_num].entrada);
        vagas[vaga_num].valor_devido = segundos * TAXA_POR_SEGUNDO;
        vagas[vaga_num].pago = false;  // Inicializa como não pago

        char entrada_str[20], saida_str[20];
        strftime(entrada_str, sizeof(entrada_str), "%H:%M:%S", localtime(&vagas[vaga_num].entrada));
        strftime(saida_str, sizeof(saida_str), "%H:%M:%S", localtime(&saida));

        snprintf(vagas[vaga_num].mensagem_pagamento, sizeof(vagas[vaga_num].mensagem_pagamento),
            "Pagamento devido na Vaga %d:\n"
            "Entrada: %s\n"
            "Saída:   %s\n"
            "Tempo: %.0f segundos\n"
            "Valor: R$ %.2f",
            vaga_num+1, entrada_str, saida_str, segundos, vagas[vaga_num].valor_devido);
        
        // Gerar dados para QR code PIX
        snprintf(vagas[vaga_num].qr_code_data, sizeof(vagas[vaga_num].qr_code_data),
            "https://api.qrserver.com/v1/create-qr-code/?size=150x150&data=PIX%%3A%%2F%%2Fsmartparking%%40email.com%%3Famount%%3D%.2f%%26message%%3DVaga%d",
            vagas[vaga_num].valor_devido, vaga_num+1);
    }
}

void cleanup_clients() {
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] > 0) {
            char buf[1];
            if (recv(client_sockets[i], buf, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
                close(client_sockets[i]);
                client_sockets[i] = 0;
            }
        }
    }
    
    int j = 0;
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] > 0) {
            client_sockets[j++] = client_sockets[i];
        }
    }
    num_clients = j;
}

void atualizar_status_vagas() {
    static int last_vaga1 = -1;
    static int last_vaga2 = -1;

    int vaga1_status = read_sensor(GPIO12);
    int vaga2_status = read_sensor(GPIO13);

    if (vaga1_status != last_vaga1 || vaga2_status != last_vaga2) {
        if (vaga1_status == 1 && !vagas[0].ocupada) {
            vagas[0].ocupada = true;
            vagas[0].entrada = time(NULL);
            memset(vagas[0].mensagem_pagamento, 0, sizeof(vagas[0].mensagem_pagamento));
            memset(vagas[0].qr_code_data, 0, sizeof(vagas[0].qr_code_data));
            vagas[0].pago = false;
        } else if (vaga1_status == 0 && vagas[0].ocupada) {
            calcular_pagamento(0);
            vagas[0].ocupada = false;
        }

        if (vaga2_status == 1 && !vagas[1].ocupada) {
            vagas[1].ocupada = true;
            vagas[1].entrada = time(NULL);
            memset(vagas[1].mensagem_pagamento, 0, sizeof(vagas[1].mensagem_pagamento));
            memset(vagas[1].qr_code_data, 0, sizeof(vagas[1].qr_code_data));
            vagas[1].pago = false;
        } else if (vaga2_status == 0 && vagas[1].ocupada) {
            calcular_pagamento(1);
            vagas[1].ocupada = false;
        }

        last_vaga1 = vaga1_status;
        last_vaga2 = vaga2_status;

        char update_msg[256];
        snprintf(update_msg, sizeof(update_msg),
            "{\"vaga1\":%d,\"vaga2\":%d,\"timer1\":%.0f,\"timer2\":%.0f,\"valor1\":%.4f,\"valor2\":%.2f}",
            vagas[0].ocupada ? 1 : 0,
            vagas[1].ocupada ? 1 : 0,
            vagas[0].ocupada ? difftime(time(NULL), vagas[0].entrada) : 0,
            vagas[1].ocupada ? difftime(time(NULL), vagas[1].entrada) : 0,
            vagas[0].ocupada ? difftime(time(NULL), vagas[0].entrada) * TAXA_POR_SEGUNDO : 0,
            vagas[1].ocupada ? difftime(time(NULL), vagas[1].entrada) * TAXA_POR_SEGUNDO : 0);

        for (int i = 0; i < num_clients; i++) {
            if (client_sockets[i] > 0) {
                char ws_msg[512];
                snprintf(ws_msg, sizeof(ws_msg), "data: %s\n\n", update_msg);
                errno = 0;
                int res = send(client_sockets[i], ws_msg, strlen(ws_msg), MSG_NOSIGNAL);
                if (res <= 0) {
                    if (errno == EPIPE || errno == ECONNRESET) {
                        close(client_sockets[i]);
                        client_sockets[i] = 0;
                    }
                }
            }
        }
    }
}

void enviar_atualizacao() {
    char update_msg[256];
    snprintf(update_msg, sizeof(update_msg),
        "{\"vaga1\":%d,\"vaga2\":%d,\"timer1\":%.0f,\"timer2\":%.0f,\"valor1\":%.4f,\"valor2\":%.2f}",
        vagas[0].ocupada ? 1 : 0,
        vagas[1].ocupada ? 1 : 0,
        vagas[0].ocupada ? difftime(time(NULL), vagas[0].entrada) : 0,
        vagas[1].ocupada ? difftime(time(NULL), vagas[1].entrada) : 0,
        vagas[0].ocupada ? difftime(time(NULL), vagas[0].entrada) * TAXA_POR_SEGUNDO : 0,
        vagas[1].ocupada ? difftime(time(NULL), vagas[1].entrada) * TAXA_POR_SEGUNDO : 0);

    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] > 0) {
            char ws_msg[512];
            snprintf(ws_msg, sizeof(ws_msg), "data: %s\n\n", update_msg);
            errno = 0;
            int res = send(client_sockets[i], ws_msg, strlen(ws_msg), MSG_NOSIGNAL);
            if (res <= 0) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                }
            }
        }
    }
}

void handle_updates(int client_sock) {
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] == client_sock) {
            return;
        }
    }

    if (num_clients < MAX_CLIENTS) {
        client_sockets[num_clients++] = client_sock;
    } else {
        close(client_sock);
        return;
    }

    const char *header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n";
    send(client_sock, header, strlen(header), 0);

    char initial_msg[256];
    snprintf(initial_msg, sizeof(initial_msg),
        "data: {\"vaga1\":%d,\"vaga2\":%d,\"timer1\":%.0f,\"timer2\":%.0f,\"valor1\":%.4f,\"valor2\":%.2f}\n\n",
        vagas[0].ocupada ? 1 : 0,
        vagas[1].ocupada ? 1 : 0,
        vagas[0].ocupada ? difftime(time(NULL), vagas[0].entrada) : 0,
        vagas[1].ocupada ? difftime(time(NULL), vagas[1].entrada) : 0,
        vagas[0].ocupada ? difftime(time(NULL), vagas[0].entrada) * TAXA_POR_SEGUNDO : 0,
        vagas[1].ocupada ? difftime(time(NULL), vagas[1].entrada) * TAXA_POR_SEGUNDO : 0);
    send(client_sock, initial_msg, strlen(initial_msg), 0);
}

void send_html(int client_sock) {
    char html[8000];
    const char *template_part1 = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html lang=\"pt-br\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>Smart Parking</title>\n"
        "    <link href=\"https://fonts.googleapis.com/css?family=Poppins\" rel=\"stylesheet\">\n"
        "    <link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css\">\n"
        "    <style>\n"
        "        body{background:#4F4F4F;font-family:'Poppins'}\n"
        "        .container{border-radius:8px;width:90%%;margin:2rem auto;background:#696969;display:flex;flex-direction:column}\n"
        "        .main-text{text-align:center;margin:2em auto;flex-grow:0}\n"
        "        .main-text h1{padding-top:1em;margin:0}\n"
        "        .main-text p{margin:0}\n"
        "        .lot-id{color:#DCDCDC}\n"
        "        main{display:grid;grid-template-columns:300px 1fr;flex-grow:1}\n"
        "        .data-box{padding-top:1rem;background:#1E90FF;color:#fff;display:grid;grid-template-columns:1fr 1fr;grid-template-rows:3rem 2.5rem 2.5rem auto;justify-items:center;align-items:center}\n"
        "        .m-para{text-align:center;font-size:20px;grid-column:1/span 2;grid-row:1/2}\n"
        "        .total-para{grid-column:1/3;grid-row:2/3;margin-left:-25px;}\n"
        "        .total-data{grid-column:2/3;grid-row:2/3}\n"
        "        .avail-para{grid-column:1/3;grid-row:3/4;margin-left:-25px;}\n"
        "        .avail-data{grid-column:2/3;grid-row:3/4}\n"
        "        .location{grid-column:1/3;text-align:center}\n"
        "        .location i{display:block;font-size:3rem;text-align:center}\n"
        "        .display-box{padding:2em;background:#2c3e50;display:grid;grid-template-columns:repeat(auto-fit,minmax(6em,1fr));grid-gap:1.5rem;align-items:center;justify-items:center}\n"
        "        .lot-box{border-radius:6px;background:#3CB371;width:5em;height:7rem;display:flex;align-items:center;justify-content:center;color:#fff}\n"
        "        .inactive{background:#c0392b!important}\n"
        "        .payment-info {background: #f39c12; padding: 1rem; margin: 1rem; border-radius: 8px; color: white;}\n"
        "        .payment-info h3 {margin-top: 0;}\n"
        "        .payment-message {background: #27ae60; padding: 1rem; margin: 1rem; border-radius: 8px; color: white; position: relative;}\n"
        "        .timer {font-size: 0.8em; margin-top: 5px;}\n"
        "        .qr-code-container {text-align: center; margin: 1rem 0;}\n"
        "        .qr-code {max-width: 200px; margin: 0 auto;}\n"
        "        .payment-button {\n"
        "            background-color: #4CAF50;\n"
        "            border: none;\n"
        "            color: white;\n"
        "            padding: 10px 20px;\n"
        "            text-align: center;\n"
        "            text-decoration: none;\n"
        "            display: inline-block;\n"
        "            font-size: 16px;\n"
        "            margin: 10px 2px;\n"
        "            cursor: pointer;\n"
        "            border-radius: 5px;\n"
        "        }\n"
        "        .payment-button:hover {background-color: #45a049;}\n"
        "        .mark-paid-button {\n"
        "            background-color: #3498db;\n"
        "            border: none;\n"
        "            color: white;\n"
        "            padding: 8px 16px;\n"
        "            text-align: center;\n"
        "            text-decoration: none;\n"
        "            display: inline-block;\n"
        "            font-size: 14px;\n"
        "            cursor: pointer;\n"
        "            border-radius: 4px;\n"
        "            position: absolute;\n"
        "            bottom: 10px;\n"
        "            right: 10px;\n"
        "        }\n"
        "        .mark-paid-button:hover {background-color: #2980b9;}\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"main-text\">\n"
        "            <h1>SmartParking</h1>\n"
        "            <p>Seja Bem - Vindo(a) <span class=\"lot-id\">(Username)</span></p>\n"
        "        </div>\n"
        "        <main>\n"
        "            <div class=\"data-box\">\n"
        "                <p class=\"m-para\">Status do Estacionamento</p>\n"
        "                <p class=\"avail-para\">Vagas Disponíveis:</p><span class=\"avail-data\" id=\"available-count\">%d</span>\n"
        "                <p class=\"total-para\">Vagas Ocupadas:</p><span class=\"total-data\" id=\"occupied-count\">%d</span>\n"
        "                <div class=\"location\">\n"
        "                    <p>Localização:</p><i class=\"fas fa-map-marker-alt\"></i><p>Universidade Federal do Ceará - Campus Quixadá</p>\n"
        "                </div>\n"
        "            </div>\n"
        "            <div class=\"display-box\">\n"
        "                <div id=\"spot1\" class=\"lot-box %s\">1 %s<div class=\"timer\" id=\"timer1\">%s</div></div>\n"
        "                <div id=\"spot2\" class=\"lot-box %s\">2 %s<div class=\"timer\" id=\"timer2\">%s</div></div>\n"
        "            </div>\n"
        "        </main>\n"
        "        %s\n"
        "        <div class=\"payment-info\">\n"
        "            <h3><i class=\"fas fa-money-bill-wave\"></i> Informações de Pagamento</h3>\n"
        "            <div id=\"payment-details\">%s</div>\n"
        "            %s\n"
        "        </div>\n"
        "    </div>\n"
        "    <script>\n"
        "        const eventSource = new EventSource('/updates');\n"
        "        \n"
        "        eventSource.onmessage = function(e) {\n"
        "            const data = JSON.parse(e.data);\n"
        "            updateInterface(data);\n"
        "        };\n"
        "        \n"
        "        function updateInterface(data) {\n"
        "            const vaga1 = document.getElementById('spot1');\n"
        "            const vaga2 = document.getElementById('spot2');\n"
        "            \n"
        "            if (data.vaga1) {\n"
        "                vaga1.classList.add('inactive');\n"
        "                vaga1.innerHTML = '1 (ocupada)<div class=\"timer\" id=\"timer1\">' + data.timer1 + ' seg (R$' + data.valor1.toFixed(2) + ')</div>';\n"
        "            } else {\n"
        "                vaga1.classList.remove('inactive');\n"
        "                vaga1.innerHTML = '1<div class=\"timer\" id=\"timer1\"></div>';\n"
        "            }\n"
        "            \n"
        "            if (data.vaga2) {\n"
        "                vaga2.classList.add('inactive');\n"
        "                vaga2.innerHTML = '2 (ocupada)<div class=\"timer\" id=\"timer2\">' + data.timer2 + ' seg (R$' + data.valor2.toFixed(2) + ')</div>';\n"
        "            } else {\n"
        "                vaga2.classList.remove('inactive');\n"
        "                vaga2.innerHTML = '2<div class=\"timer\" id=\"timer2\"></div>';\n"
        "            }\n"
        "            \n"
        "            const ocupadas = (data.vaga1 ? 1 : 0) + (data.vaga2 ? 1 : 0);\n"
        "            document.getElementById('occupied-count').innerText = ocupadas;\n"
        "            document.getElementById('available-count').innerText = 2 - ocupadas;\n"
        "        }\n"
        "        \n"
        "        function generatePayment(vagaNum) {\n"
        "            fetch('/generate_payment?vaga=' + vagaNum)\n"
        "                .then(response => response.text())\n"
        "                .then(data => {\n"
        "                    location.reload();\n"
        "                });\n"
        "        }\n"
        "        \n"
        "        function markAsPaid(vagaNum) {\n"
        "            fetch('/mark_paid?vaga=' + vagaNum)\n"
        "                .then(response => response.text())\n"
        "                .then(data => {\n"
        "                    location.reload();\n"
        "                });\n"
        "        }\n"
        "    </script>\n"
        "</body>\n"
        "</html>";

    int ocupadas = (vagas[0].ocupada ? 1 : 0) + (vagas[1].ocupada ? 1 : 0);
    int disponiveis = 2 - ocupadas;
    
    char vaga1_class[20], vaga2_class[20], vaga1_text[20], vaga2_text[20];
    char vaga1_timer[50] = "", vaga2_timer[50] = "";
    
    strcpy(vaga1_class, vagas[0].ocupada ? "inactive" : "");
    strcpy(vaga2_class, vagas[1].ocupada ? "inactive" : "");
    strcpy(vaga1_text, vagas[0].ocupada ? "(ocupada)" : "");
    strcpy(vaga2_text, vagas[1].ocupada ? "(ocupada)" : "");
    
    if (vagas[0].ocupada) {
        double segundos = difftime(time(NULL), vagas[0].entrada);
        sprintf(vaga1_timer, "%.0f seg (R$%.4f)", segundos, segundos * TAXA_POR_SEGUNDO);
    }
    if (vagas[1].ocupada) {
        double segundos = difftime(time(NULL), vagas[1].entrada);
        sprintf(vaga2_timer, "%.0f seg (R$%.4f)", segundos, segundos * TAXA_POR_SEGUNDO);
    }
    
    char payment_message[1024] = "";
    for (int i = 0; i < 2; i++) {
        if (!vagas[i].ocupada && strlen(vagas[i].mensagem_pagamento) > 0 && !vagas[i].pago) {
            char temp[700];
            snprintf(temp, sizeof(temp),
                    "<div class=\"payment-message\" id=\"payment-message-%d\">\n"
                    "<h3><i class=\"fas fa-receipt\"></i> Recibo da Vaga %d</h3>\n"
                    "<pre>%s</pre>\n"
                    "<div class=\"qr-code-container\">\n"
                    "    <img class=\"qr-code\" src=\"%s\" alt=\"QR Code para pagamento\">\n"
                    "</div>\n"
                    "<button class=\"mark-paid-button\" onclick=\"markAsPaid(%d)\">\n"
                    "    <i class=\"fas fa-check-circle\"></i> Marcar como Pago\n"
                    "</button>\n"
                    "</div>",
                    i+1, i+1, vagas[i].mensagem_pagamento, vagas[i].qr_code_data, i+1);
            strcat(payment_message, temp);
        }
    }
    
    char payment_details[500] = "";
    char payment_button[500] = "";
    
    if (vagas[0].ocupada) {
        char entryTimeStr[20];
        strftime(entryTimeStr, sizeof(entryTimeStr), "%H:%M:%S", localtime(&vagas[0].entrada));
        double segundos = difftime(time(NULL), vagas[0].entrada);
        sprintf(payment_details + strlen(payment_details), 
                "<p>Vaga 1: Entrada às %s - Tempo: %.0f seg - Valor: R$ %.2f</p>", 
                entryTimeStr, segundos, segundos * TAXA_POR_SEGUNDO);
        
        strcat(payment_button, 
               "<button class=\"payment-button\" onclick=\"generatePayment(1)\">\n"
               "    <i class=\"fas fa-qrcode\"></i> Gerar Pagamento Vaga 1\n"
               "</button>");
    }
    
    if (vagas[1].ocupada) {
        char entryTimeStr[20];
        strftime(entryTimeStr, sizeof(entryTimeStr), "%H:%M:%S", localtime(&vagas[1].entrada));
        double segundos = difftime(time(NULL), vagas[1].entrada);
        sprintf(payment_details + strlen(payment_details), 
                "<p>Vaga 2: Entrada às %s - Tempo: %.0f seg - Valor: R$ %.2f</p>", 
                entryTimeStr, segundos, segundos * TAXA_POR_SEGUNDO);
        
        strcat(payment_button, 
               "<button class=\"payment-button\" onclick=\"generatePayment(2)\">\n"
               "    <i class=\"fas fa-qrcode\"></i> Gerar Pagamento Vaga 2\n"
               "</button>");
    }
    
    if (!vagas[0].ocupada && !vagas[1].ocupada && strlen(payment_message) == 0) {
        strcpy(payment_details, "<p>Nenhuma vaga ocupada no momento</p>");
    }

    snprintf(html, sizeof(html), template_part1, 
             disponiveis, ocupadas,
             vaga1_class, vaga1_text, vaga1_timer,
             vaga2_class, vaga2_text, vaga2_timer,
             payment_message,
             payment_details,
             payment_button);
    
    send(client_sock, html, strlen(html), 0);
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    printf("Servidor iniciado em http://localhost:%d\n", PORT);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Erro socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro bind");
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Erro listen");
        exit(1);
    }

    int iteration_count = 0;
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        int activity = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_sock < 0) {
                perror("accept");
                continue;
            }

            char buffer[1024] = {0};
            int bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                close(client_sock);
                continue;
            }

            if (strstr(buffer, "GET /updates") != NULL) {
                handle_updates(client_sock);
            } else if (strstr(buffer, "GET /generate_payment") != NULL) {
                char *query = strstr(buffer, "vaga=");
                if (query) {
                    int vaga_num = atoi(query + 5) - 1;
                    if (vaga_num >= 0 && vaga_num < 2 && vagas[vaga_num].ocupada) {
                        calcular_pagamento(vaga_num);
                        vagas[vaga_num].ocupada = false;
                    }
                }
                
                const char *response = 
                    "HTTP/1.1 303 See Other\r\n"
                    "Location: /\r\n\r\n";
                send(client_sock, response, strlen(response), 0);
                close(client_sock);
            } else if (strstr(buffer, "GET /mark_paid") != NULL) {
                char *query = strstr(buffer, "vaga=");
                if (query) {
                    int vaga_num = atoi(query + 5) - 1;
                    if (vaga_num >= 0 && vaga_num < 2) {
                        // Limpa os dados de pagamento quando marcado como pago
                        memset(vagas[vaga_num].mensagem_pagamento, 0, sizeof(vagas[vaga_num].mensagem_pagamento));
                        memset(vagas[vaga_num].qr_code_data, 0, sizeof(vagas[vaga_num].qr_code_data));
                        vagas[vaga_num].pago = true;
                    }
                }
                
                const char *response = 
                    "HTTP/1.1 303 See Other\r\n"
                    "Location: /\r\n\r\n";
                send(client_sock, response, strlen(response), 0);
                close(client_sock);
            } else if (strstr(buffer, "GET / ") != NULL || strstr(buffer, "GET /index.html") != NULL) {
                atualizar_status_vagas();
                send_html(client_sock);
                close(client_sock);
            } else {
                close(client_sock);
            }
        }

        if (++iteration_count % 10 == 0) {
            cleanup_clients();
        }

        atualizar_status_vagas();
        enviar_atualizacao();
    }

    close(server_fd);
    return 0;
}
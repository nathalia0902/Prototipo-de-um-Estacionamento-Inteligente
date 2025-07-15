#include "server.h"
#include "vaga.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10

static int client_sockets[MAX_CLIENTS] = {0};
static int num_clients = 0;

static void cleanup_clients() {
    for (int i = 0; i < num_clients; ++i) {
        if (client_sockets[i] > 0) {
            char buf[1];
            if (recv(client_sockets[i], buf, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
                close(client_sockets[i]);
                client_sockets[i] = 0;
            }
        }
    }

    int j = 0;
    for (int i = 0; i < num_clients; ++i) {
        if (client_sockets[i] > 0) {
            client_sockets[j++] = client_sockets[i];
        }
    }
    num_clients = j;
}

static void send_sse_update() {
    char msg[256];
    snprintf(msg, sizeof(msg),
             "data: {\"vaga1\":%d,\"vaga2\":%d}\n\n",
             vagas[0].ocupada ? 1 : 0,
             vagas[1].ocupada ? 1 : 0);

    for (int i = 0; i < num_clients; ++i) {
        if (client_sockets[i] > 0) {
            send(client_sockets[i], msg, strlen(msg), MSG_NOSIGNAL);
        }
    }
}

static void handle_updates(int sock) {
    if (num_clients < MAX_CLIENTS) {
        client_sockets[num_clients++] = sock;

        const char *header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n\r\n";
        send(sock, header, strlen(header), 0);
    } else {
        close(sock);
    }
}

static void serve_file(int sock, const char *path, const char *content_type) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        const char *error404 = "HTTP/1.1 404 Not Found\r\n\r\nArquivo não encontrado";
        send(sock, error404, strlen(error404), 0);
        close(sock);
        return;
    }

    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
    send(sock, header, strlen(header), 0);

    char buffer[1024];
    ssize_t bytes;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        send(sock, buffer, bytes, 0);
    }

    close(fd);
    close(sock);
}

void start_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Erro ao fazer bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Erro ao escutar");
        exit(EXIT_FAILURE);
    }

    printf("Servidor rodando em http://localhost:%d\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client < 0) continue;

        char req[1024] = {0};
        read(client, req, sizeof(req) - 1);

        atualizar_status_vagas();

        if (strstr(req, "GET /updates") != NULL) {
            handle_updates(client);
        } else if (strstr(req, "GET /style.css") != NULL) {
            serve_file(client, "www/style.css", "text/css");
        } else if (strstr(req, "GET /script.js") != NULL) {
            serve_file(client, "www/script.js", "application/javascript");
        } else {
            serve_file(client, "www/index.html", "text/html");
        }

        cleanup_clients();
        send_sse_update();
        usleep(500000);
    }

    close(server_fd);
}
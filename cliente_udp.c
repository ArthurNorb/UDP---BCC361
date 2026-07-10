// cliente
// gcc cliente_udp.c -o cliente_udp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTA 8080
#define TAM_BUFFER 1024

// le um arquivo da pasta upload e envia em pedacos de 1024 bytes, esperando um ack do servidor a cada pedaco (stop-and-wait)
void enviar_arquivo(int socket_cliente, struct sockaddr_in *endereco_servidor, socklen_t tamanho_servidor, char *nome) {
    char caminho[300]; // caminho do arquivo dentro da pasta upload

    snprintf(caminho, sizeof(caminho), "upload/%s", nome);

    FILE *arquivo = fopen(caminho, "rb"); // abre em modo binario para ler os bytes crus

    if (arquivo == NULL) {
        printf("Nao foi possivel abrir o arquivo '%s'. Ele esta na pasta upload?\n", caminho);
        return;
    }

    // descobre o tamanho do arquivo em bytes indo ate o fim e voltando pro inicio
    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo); // total de bytes do arquivo
    rewind(arquivo);

    char controle[400]; // mensagem que avisa o servidor o nome e o tamanho do arquivo

    snprintf(controle, sizeof(controle), "UPLOAD|%s|%ld", nome, tamanho_arquivo);

    // manda o controle e espera o servidor confirmar que esta pronto
    sendto(socket_cliente, controle, strlen(controle), 0,
           (struct sockaddr *)endereco_servidor, tamanho_servidor);

    char ack[TAM_BUFFER]; // buffer para a confirmacao vinda do servidor

    recvfrom(socket_cliente, ack, TAM_BUFFER - 1, 0,
             (struct sockaddr *)endereco_servidor, &tamanho_servidor);

    printf("Enviando '%s' (%ld bytes)...\n", nome, tamanho_arquivo);

    char pedaco[TAM_BUFFER]; // buffer com um pedaco do arquivo
    long total_enviado = 0; // quantos bytes ja foram enviados

    // le e envia o arquivo pedaco por pedaco, esperando um ack antes de mandar o proximo
    while (total_enviado < tamanho_arquivo) {
        int bytes_lidos = fread(pedaco, 1, TAM_BUFFER, arquivo); // le um pedaco do arquivo

        sendto(socket_cliente, pedaco, bytes_lidos, 0,
               (struct sockaddr *)endereco_servidor, tamanho_servidor);

        // espera o ack do servidor antes de continuar
        recvfrom(socket_cliente, ack, TAM_BUFFER - 1, 0,
                 (struct sockaddr *)endereco_servidor, &tamanho_servidor);

        total_enviado += bytes_lidos;
    }

    fclose(arquivo);
    printf("Arquivo enviado com sucesso!\n");
}

int main() {
    int socket_cliente; // id para o s.o.
    char mensagem[TAM_BUFFER]; // texto digitado pelo usuario
    char buffer[TAM_BUFFER]; // resposta recebida do servidor

    struct sockaddr_in endereco_servidor;
    socklen_t tamanho_servidor = sizeof(endereco_servidor);

    // cria socket UDP
    socket_cliente = socket(AF_INET, SOCK_DGRAM, 0); // dgram = datagrama (UDP) | stream = TCP

    if (socket_cliente < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // configura o endereco do servidor
    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_port = htons(PORTA);

    // ip do servidor, para testar na mesma maquina usamos 127.0.0.1
    if (inet_pton(AF_INET, "127.0.0.1", &endereco_servidor.sin_addr) <= 0) {
        perror("Endereco IP invalido");
        close(socket_cliente);
        exit(EXIT_FAILURE);
    }

    printf("Cliente UDP iniciado.\n");
    printf("Digite uma mensagem, ou /upload <arquivo> para enviar um arquivo, ou /sair para encerrar.\n");

    // loop de conversa: fica lendo o que o usuario digita ate ele decidir sair
    while (1) {
        printf("> ");

        // se der EOF (ctrl+d) encerra o cliente
        if (fgets(mensagem, TAM_BUFFER, stdin) == NULL) {
            break;
        }

        // remove o \n do final, se existir
        mensagem[strcspn(mensagem, "\n")] = '\0';

        // comando para encerrar o cliente
        if (strcmp(mensagem, "/sair") == 0) {
            break;
        }

        // se o usuario nao digitou nada, volta a pedir
        if (strlen(mensagem) == 0) {
            continue;
        }

        // comando de upload no formato /upload <nome_do_arquivo>
        if (strncmp(mensagem, "/upload ", 8) == 0) {
            enviar_arquivo(socket_cliente, &endereco_servidor, tamanho_servidor, mensagem + 8);
            continue;
        }

        // caso normal: envia a mensagem de texto e mostra a resposta do servidor
        sendto(socket_cliente, mensagem, strlen(mensagem), 0,
               (struct sockaddr *)&endereco_servidor, tamanho_servidor);

        memset(buffer, 0, TAM_BUFFER);

        int bytes_recebidos = recvfrom(socket_cliente, buffer, TAM_BUFFER - 1, 0,
                                       (struct sockaddr *)&endereco_servidor, &tamanho_servidor);

        if (bytes_recebidos < 0) {
            perror("Erro ao receber resposta");
            continue;
        }

        buffer[bytes_recebidos] = '\0';
        printf("Resposta do servidor: %s\n", buffer);
    }

    printf("Encerrando cliente.\n");
    close(socket_cliente);

    return 0;
}

// servidor
// gcc servidor_udp.c -o servidor_udp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PORTA 8080
#define TAM_BUFFER 1024

// recebe um arquivo pedaco por pedaco e grava na pasta recebidos, confirmando cada pedaco com um ack (stop-and-wait)
void receber_arquivo(int socket_servidor, struct sockaddr_in *endereco_cliente, socklen_t tamanho_cliente, char *nome, long tamanho) {
    char caminho[300]; // caminho onde o arquivo sera salvo

    snprintf(caminho, sizeof(caminho), "recebidos/%s", nome);

    FILE *arquivo = fopen(caminho, "wb"); // abre em modo binario para gravar os bytes crus

    if (arquivo == NULL) {
        printf("Erro ao criar o arquivo '%s'.\n", caminho);
        return;
    }

    char buffer[TAM_BUFFER]; // buffer com o pedaco recebido
    long total_recebido = 0; // quantos bytes ja foram gravados

    // recebe pedacos ate juntar o tamanho total que foi informado no controle
    while (total_recebido < tamanho) {
        int bytes_recebidos = recvfrom(socket_servidor, buffer, TAM_BUFFER, 0,
                                       (struct sockaddr *)endereco_cliente, &tamanho_cliente);

        if (bytes_recebidos <= 0) {
            continue;
        }

        fwrite(buffer, 1, bytes_recebidos, arquivo); // grava o pedaco no arquivo
        total_recebido += bytes_recebidos;

        // responde ack para o cliente liberar o proximo pedaco
        sendto(socket_servidor, "OK", 2, 0,
               (struct sockaddr *)endereco_cliente, tamanho_cliente);

        printf("Recebido %ld de %ld bytes\n", total_recebido, tamanho);
    }

    fclose(arquivo);
    printf("Arquivo salvo em '%s'.\n", caminho);
}

int main() {
    int socket_servidor; // id para o s.o.
    char buffer[TAM_BUFFER]; // mensagem recebida do cliente
    char resposta[TAM_BUFFER + 50]; // resposta enviada de volta ao cliente

    struct sockaddr_in endereco_servidor;
    struct sockaddr_in endereco_cliente;

    socklen_t tamanho_cliente = sizeof(endereco_cliente);

    // cria a pasta onde os arquivos recebidos serao salvos (nao faz nada se ela ja existir)
    mkdir("recebidos", 0777);

    // cria socket UDP
    socket_servidor = socket(AF_INET, SOCK_DGRAM, 0); // dgram = datagrama (UDP) | stream = TCP

    if (socket_servidor < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // configura o endereco do servidor
    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_addr.s_addr = INADDR_ANY;
    endereco_servidor.sin_port = htons(PORTA);

    // associa o socket a porta
    if (bind(socket_servidor,
             (struct sockaddr *)&endereco_servidor,
             sizeof(endereco_servidor)) < 0) {
        perror("Erro no bind");
        close(socket_servidor); // fecha o socket antes de sair
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP aguardando mensagens na porta %d...\n", PORTA);

    while (1) {
        memset(buffer, 0, TAM_BUFFER);
        memset(resposta, 0, sizeof(resposta));

        // recvfrom e bloqueante, o programa fica esperando ate receber algo
        int bytes_recebidos = recvfrom(
            socket_servidor,
            buffer,
            TAM_BUFFER - 1,
            0,
            (struct sockaddr *)&endereco_cliente,
            &tamanho_cliente
        );

        if (bytes_recebidos < 0) {
            perror("Erro ao receber mensagem");
            continue;
        }

        buffer[bytes_recebidos] = '\0'; // garante que a string termine, sem lixo

        // se a mensagem for um pedido de upload, entra no modo de receber arquivo
        if (strncmp(buffer, "UPLOAD|", 7) == 0) {
            char nome[256]; // nome do arquivo que vai chegar
            long tamanho = 0; // tamanho total do arquivo em bytes

            // le do controle o nome e o tamanho do arquivo
            sscanf(buffer, "UPLOAD|%255[^|]|%ld", nome, &tamanho);

            printf("Recebendo arquivo '%s' de %s:%d (%ld bytes)...\n",
                   nome,
                   inet_ntoa(endereco_cliente.sin_addr),
                   ntohs(endereco_cliente.sin_port),
                   tamanho);

            // confirma para o cliente que esta pronto para receber os pedacos
            sendto(socket_servidor, "OK", 2, 0,
                   (struct sockaddr *)&endereco_cliente, tamanho_cliente);

            receber_arquivo(socket_servidor, &endereco_cliente, tamanho_cliente, nome, tamanho);
            continue;
        }

        printf("Mensagem recebida de %s:%d -> %s\n",
               inet_ntoa(endereco_cliente.sin_addr),
               ntohs(endereco_cliente.sin_port),
               buffer);

        // monta a resposta de eco
        snprintf(resposta, sizeof(resposta),
                 "OK. Mensagem recebida: %s", buffer);

        // envia a resposta ao cliente
        int bytes_enviados = sendto(
            socket_servidor,
            resposta,
            strlen(resposta),
            0,
            (struct sockaddr *)&endereco_cliente,
            tamanho_cliente
        );

        if (bytes_enviados < 0) {
            perror("Erro ao enviar resposta");
        }
    }

    close(socket_servidor);

    return 0;
}

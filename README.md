# Comunicação Cliente/Servidor com UDP

- **Nome:** Arthur Norberto da Silveira
- **Matrícula:** 21.2.4100
- **Disciplina:** BCC361 - Redes de Computadores

## O que é isso?

Um cliente e um servidor bem simples que conversam por **UDP**. Dá pra trocar mensagens de
texto e também **enviar qualquer arquivo** (texto, imagem, pdf, o que for) do cliente para o
servidor. Tudo em C, rodando na mesma máquina (`127.0.0.1`, porta `8080`).

## Como compilar

Só rodar `make` na raiz do projeto:

```bash
make
```

Isso gera dois programas: `cliente_udp` e `servidor_udp`. Se quiser limpar os binários depois,
é `make clean`.

## Como rodar

Você vai precisar de **dois terminais**.

No primeiro, liga o servidor:

```bash
./servidor_udp
```

No segundo, liga o cliente:

```bash
./cliente_udp
```

Agora é só digitar no cliente. Uma mensagem qualquer é enviada pro servidor, que imprime o que
recebeu e devolve um "OK" de volta.

## Enviando um arquivo

1. Coloque o arquivo que você quer mandar dentro da pasta **`upload/`** (já tem um
   `exemplo.txt` lá pra testar).
2. No cliente, digite:

   ```
   /upload exemplo.txt
   ```

3. O servidor recebe o arquivo e salva ele dentro da pasta **`recebidos/`** (essa pasta é
   criada sozinha na primeira vez), com o mesmo nome e o mesmo conteúdo do original.

O servidor vai descrever tudo no terminal dele: que arquivo está chegando, de quem, o tamanho,
o progresso dos bytes recebidos e onde ele foi salvo.

Pra encerrar o cliente é só digitar `/sair` (ou apertar `Ctrl+D`).

## Como o envio funciona por dentro

O arquivo é lido em **modo binário** (os bytes crus, do jeito que ele é no disco), então
funciona pra qualquer tipo de arquivo, não só texto. Como um pacote UDP não cabe um arquivo
grande inteiro, ele é quebrado em **pedaços de até 1024 bytes**.

Pra não perder pedaço no caminho, usei a ideia de **stop-and-wait**: o cliente manda um pedaço
e só manda o próximo depois que o servidor confirma que recebeu (um "OK", o famoso **ACK**).
O passo a passo é mais ou menos assim:

1. O cliente avisa o servidor: "vou mandar o arquivo tal, com tantos bytes" (`UPLOAD|nome|tamanho`).
2. O servidor abre o arquivo de destino e responde que está pronto.
3. O cliente manda um pedaço, espera o ACK, manda o próximo, e assim por diante até acabar.
4. O servidor sabe o tamanho total, então ele sabe exatamente a hora de parar e fechar o arquivo.

No fim, o arquivo em `recebidos/` fica **idêntico** ao que estava em `upload/`.

## Detalhe importante (limitação)

Pra manter o código simples, **não tem retransmissão nem timeout**. Rodando na mesma máquina
(localhost) com o stop-and-wait isso não dá problema na prática, mas numa rede de verdade, se
um pacote se perdesse, o programa ficaria esperando o ACK que não vem. Deixei assim de
propósito pra focar na ideia principal do trabalho.

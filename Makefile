# compila o cliente e o servidor digitando apenas "make"

all: cliente_udp servidor_udp

cliente_udp: cliente_udp.c
	gcc cliente_udp.c -o cliente_udp

servidor_udp: servidor_udp.c
	gcc servidor_udp.c -o servidor_udp

# remove os binarios gerados
clean:
	rm -f cliente_udp servidor_udp

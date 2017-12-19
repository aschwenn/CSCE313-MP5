# makefile

all: dataserver client

netreqchannel.o: netreqchannel.H netreqchannel.C
	g++ -c -g netreqchannel.C

dataserver: server.C netreqchannel.o
	g++ -g -o dataserver server.C netreqchannel.o -lpthread -lnsl

client: client.C semaphore.H bounded_buffer.H netreqchannel.o
	g++ -g -o client client.C semaphore.H bounded_buffer.H netreqchannel.o -lpthread -lnsl

clean:
	rm -f *.o

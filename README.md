# CSCE313-MP5
Data server program and client which makes requests over a network

This program involves a dataserver program, which generates information for three users when the client requests their information, and the client program, which connects to the data server and sends a series of requests. These programs are connected via the NetworkRequestChannel class, which utilizes sockets to connect over a network.

Type "make" to compile the client and dataserver programs.

The dataserver can be called by using:
```
  dataserver -p <port number for data server>
             -b <backlog of the server socket>
```
The client can be called by using:
```
  client -n <number of data requests per person>
         -b <size of the bounded buffer in requests>
         -w <number of request channels>
         -h <name of server host>
         -p <port number of server host>
```
If these command-line arguments are not passed in, the defaults will be used.

PLEASE NOTE: This work was completed by myself in Fall 2017. I am uploading this for educational purposes only. Remember, an Aggie does not lie, cheat, or steal, or tolerate those that do. Do not attempt to copy my code.

# Secure Terminal Chat Room

This project is a secure, multi-client terminal chat room built in C and deployed using Docker.  
All chat messages are protected using authenticated encryption (AEAD), and the system includes replay protection, message validation, and isolation inside a private Docker network.

## Features
- Multi client chat server (broadcast model)
- Authenticated encryption 
- Nonces for replay protection
- Safe C coding practices (bounds checking, no unsafe functions)
- Dockerized client and server containers
- Reproducible build with `make bootstrap`

### Protocol & Network Setup
The chat network uses a X25519 Diffie-Hellman key exchange + ChaCha20-Poly1305 AEAD encryption via libsodium. Essentially, each client uses a unique key for their session, which are decrypted by the server, and re-encrypted with each of the recipients' keys.

The network model consists of a single server with multiple clients, within the subnet `172.16.238.0/24`. The following is the IP mapping for our example server and three client network:
- server: `172.16.238.10`
- client1: `172.16.238.11`
- client2: `172.16.238.12`
- client3: `172.16.238.13`


## How to Run
Before anything, make sure you have Docker installed and running on your system.

### Automatic Script
To quickly run and test out the program, use the `make up && make demo` commands. These will build the docker containers, and then run the script that simulates a conversation in the network.
```bash
make up && make demo
```

### Manual Setup Instructions
1. Clone the repo, change to its directory, and then build the Docker containers:
```bash
cd src/
docker compose up -d --build
```

2. Enter the chat rooms for the server and each of the clients (this example uses `51262` for the server port):

```bash
docker exec -it chat_server server 51262
docker exec -it client1 client 172.16.238.10 51262
docker exec -it client2 client 172.16.238.10 51262
docker exec -it client3 client 172.16.238.10 51262
```
The command line arguments follow this structure:
- `docker exec -it chat_server server <port>`
- `docker exec -it client<i> client <ip> <port>`

If the Docker network is set up, running these commands should let you directly start the server and enter the chatroom on each of the clients. From there, you should be able to enter a username, and then send messages through the chat room.

3. Perform a tcpdump on the chat server on the target port to see encrypted traffic. (example using `51262` as the port.)
```bash
docker exec -it chat_server tcpdump -i eth0 port 51262 -A 
```
The command line arguments follow this structure:
- `docker exec -it chat_server tcpdump -i eth0 port <port> -A`

#Trigger CI

All traffic seen through the tcpdump should be fully encrypted, and no plaintext should be exposed.

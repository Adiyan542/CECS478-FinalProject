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

## How to Run

### Setup Instructions
1. Build the Docker container
- Run the following commands:
```bash
cd src/
docker compose up -d --build
```

2. Compile the `server.c` and `client.c` code.

```bash
gcc server.c -o server -pthread
gcc client.c -o client -pthread 

# FOR ENCRYPTION
gcc server.c -o server -pthread -lssl -lcrypto
gcc client.c -o client -pthread -lssl -lcrypto
```

3. Enter the terminal for the server and each of the clients.
```bash
docker exec -it chat_server bash
docker exec -it client1 bash
docker exec -it client2 bash
docker exec -it client3 bash
```

4. Run the compiled server/client code on their respective devices.
```bash
./server 51262          # run this on the server

./client 172.16.238.10 51262  # run the following on each client
```

For troubleshooting:

```bash
hostname -I # get ip address

# extra installs
apt update
apt install libssl-dev
```

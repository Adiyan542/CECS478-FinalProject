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

### Protocol
Uses a X25519 Diffie-Hellman (Curve25519) key exchange + ChaCha20-Poly1305 AEAD encryption via libsodium. Essentially, each client uses a unique key for their session, which are decrypted by the server, and re-encrypted with each of the recipients' keys.

## How to Run

### Setup Instructions
1. Build the Docker container
- Run the following commands:
```bash
cd src/
docker compose up -d --build
```

2. Enter the chat rooms for the server and each of the clients. (example, using 51262 as the port.)
```bash
docker exec -it chat_server server 51262
docker exec -it client1 client 172.16.238.10 51262
docker exec -it client2 client 172.16.238.10 51262
docker exec -it client3 client 172.16.238.10 51262
```

- Considering that the docker network is set up, running these commands should let you start the server and enter the chatroom on each of the clients. From there, you should be able to enter a username, and then send messages through the chat room.

3. Perform a tcpdump on the chat server on the target port to see encrypted traffic. (example, using 51262 as the port.)
```bash
docker exec -it chat_server tcpdump -i eth0 port 51262 -A 
```

## Dev Notes
For troubleshooting:

```bash
hostname -I # get ip address
```

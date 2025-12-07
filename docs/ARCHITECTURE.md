# Architecture Overview — Secure Terminal Chat Room
CECS 478 – Alpha/Beta Integrated Release

## 1. System Summary
This project is a secure chat room that runs in the terminal. It uses Docker to create a private virtual network where a server and several clients can communicate. Every message sent over the network is encrypted, so no one can read it from the outside.

The program uses:
- **X25519 Diffie–Hellman** for creating shared secret keys  
- **ChaCha20-Poly1305 AEAD** for encrypting and protecting each message  
- **Nonces** (message counters) to stop replay attacks  

The system has:
- 1 server container  
- 3 separate client containers  
All containers live inside the same private Docker network.

---

## 2. High-Level Architecture

The server is the “center” of the chat. All clients connect to it, and it forwards encrypted messages to everyone.


                    +---------------------+
                    |     chat_server     |
                    |   (172.16.238.10)   |
                    +----------+----------+
                               ^
                               |
         -------------------------------------------------
         |                        |                       |
         v                        v                       v
+----------------+      +----------------+      +----------------+
|    client1     |      |    client2     |      |    client3     |
|  Bob           |      |  Alice         |      |  David         |
|  172.16.238.11 |      |  172.16.238.12 |      |  172.16.238.13 |
+----------------+      +----------------+      +----------------+



How the system works:
1. A client connects to the server using TCP.
2. The server and client exchange public keys.
3. Both calculate the same shared secret key.
4. The client encrypts messages → sends them → server decrypts → server reencrypts for each client.
5. Every message includes an increasing **nonce** to prevent replay attacks.

---

## 3. Docker Network Layout

All containers run only inside the private network:

Docker subnet: 172.16.238.0/24

- Server:   172.16.238.10  
- Client 1: 172.16.238.11  
- Client 2: 172.16.238.12  
- Client 3: 172.16.238.13  

This stops outside computers from accessing the chat.

---

## 4. Cryptography Flow

### When a client joins:
1. The server sends its public key.
2. The client sends its public key.
3. Both use X25519 to compute the same shared secret.
4. This shared secret becomes the encryption key for that session.

### Encrypting a message:
- Client uses ChaCha20-Poly1305 AEAD  
- Adds a nonce (message counter)  
- Sends encrypted bytes to the server  

If a nonce repeats or is out of order, the message is rejected.

---

## 5. Responsibilities

### Server
- Accepts new clients  
- Performs key exchange  
- Stores session keys  
- Stores nonce counters  
- Broadcasts messages  
- Logs joins and leaves  
- Shuts down cleanly  

### Client
- Connects to server  
- Performs key exchange  
- Encrypts outgoing messages  
- Decrypts incoming messages  
- Keeps track of nonces  
- Deletes keys from memory on exit  

---

## 6. Security Rules (Invariants)
- No plaintext is ever sent over the network  
- Each client has its own session key  
- Nonces prevent replay attacks  
- Keys are wiped from memory after use  
- All containers communicate only inside Docker’s private network  

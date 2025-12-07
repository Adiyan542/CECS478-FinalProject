# Security Invariants — Secure Chat System

This document explains the main security rulesthat the system always follows.  
These rules make sure the chatroom stays private, safe, and protected from basic attacks.

## 1. What This System Protects Against

The system is designed to protect against:

- People trying to read messages by watching network traffic  
- Attackers changing messages in transit  
- Attackers replaying old messages  
- Fake clients sending invalid or strange data  
- Anyone trying to read plaintext from outside the containers  

The goal is to keep every message private and tamper proof.

## 2. Encryption Guarantees

### All messages are encrypted
Every message sent between a client and the server uses **ChaCha20 Poly1305 AEAD**.  
This means:

- Messages cannot be read without the key  
- Messages cannot be changed without being detected  
- The system rejects corrupted messages automatically  

When we look at the traffic in Wireshark or tcpdump, we only see random bytes, not readable text.

### Strong key exchange (X25519)
When a client connects, the server and client exchange public keys and create a shared secret using **X25519 Diffie Hellman**.

This gives:

- A strong shared session key  
- Forward secrecy so even if a key leaks later, past messages stay protected  

Each client gets its **own unique session key**, which is not shared with others.

---

## 3. Replay Protection

Every encrypted packet includes an 8 byte **nonce**, which is a number that goes up with every message.

## 4. Memory Safety and Key Wiping

The system uses `sodium_memzero()` to wipe secret keys from memory when the client or server shuts down.

This prevents:

- Keys staying in RAM  
- Memory scraping attacks  
- Leaked secrets after exit  

## 5. Docker Network Isolation

All containers run inside a private Docker subnet: **172.16.238.0/24**

Only the server and clients inside this subnet can communicate.  
No outside computer or program can access the traffic.

This protects:

- Network confidentiality  
- Isolation from the host machine  
- Replay testing without outside interference  

---

## 6. Input Validation

The system checks inputs to avoid crashes or unsafe behavior.

### On the client:
- Username length is checked  
- Empty messages are ignored  
- Very long messages are handled safely  

### On the server:
- Decryption failures are logged  
- Invalid public keys are rejected  
- Corrupted packets are dropped  
- Disconnects are handled cleanly  

This helps prevent malformed input attacks

## 7. Summary of Security Protocols

- No plaintext ever travels over the network  
- Each client gets a unique session key  
- Nonces prevent replay attacks  
- Encryption ensures confidentiality and integrity  
- Keys are wiped from memory on exit  
- The Docker network keeps traffic isolated and private  

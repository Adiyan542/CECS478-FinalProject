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

# What Works / What’s Next  
CECS 478 – Alpha/Beta Integrated Release

## 1. What Works

### ✔ End to End Encrypted Chat System  
The full system works from start to finish.  
Clients can join, chat, and leave.  
Messages are encrypted the entire time.

### ✔ Secure Key Exchange  
The server and each client use X25519 to create a shared secret.  
Each client has its own session key, so keys are never reused.

### ✔ Encrypted Messaging  
All chat messages use ChaCha20 Poly1305 AEAD encryption.  
No plaintext appears on the network.

### ✔ Replay Protection  
Each message has a unique nonce that increases every time.  
If the nonce is reused or out of order, the message is rejected.

### ✔ Dockerized Environment  
The whole system runs inside a private Docker network.  
IP addresses are fixed, which makes testing easier and safer.

### ✔ Automated Demo System  
Running `make demo` shows:
- A full “happy path” chat conversation  
- A negative test with unusual input  
- Clean shutdown of server and clients  

### ✔ Evidence Collected  
The project already includes:
- pcaps of encrypted traffic  
- server logs  
- client logs  
- early evaluation notes  

These will be used in the final evaluation

## 2. What’s Next

### ➤ Complete Data Analysis  
More packet captures will be collected to study:
- message sizes  
- encryption overhead  
- frequency of messages  
- timing behavior  

### ➤ Optional Future Improvements  
These features are not required but could be added later:
- key rotation  
- user authentication  
- support for multiple chat rooms  
- better logging or UI improvements


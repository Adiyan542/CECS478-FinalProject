# Release Artifacts (Evaluation Evidence)

This folder contains the evidence collected for the Alpha/Beta Integrated Release
of the Secure Terminal Chat Room project.

These files show that the system works end-to-end, that encryption is active,
and that evaluation has begun.

---

## Contents

### 1. `chat_capture.pcap`
A packet capture taken directly from the `chat_server` container.
It shows encrypted network traffic on port **51262**.

- No plaintext messages appear.
- All packet contents look like random bytes.
- This confirms that ChaCha20-Poly1305 AEAD encryption works correctly.
- Nonces and authentication tags are visible in the ciphertext.

This file satisfies: **Collected datasets / pcaps requirement.**

---

### 2. `server_logs.txt`
Logs from the server container.

These logs show:
- The server compiled successfully.
- The server started and shut down normally.

(The chatroom runs through `docker exec`, so the interactive output
does not appear in Docker logs, which is expected.)

---

### 3. `client1_logs.txt`, `client2_logs.txt`, `client3_logs.txt`
Logs from each client container.

These contain:
- Compilation messages
- Warnings from building the C program
- Confirmation that each client binary was created

These files prove the client programs built correctly inside Docker.

---

### 4. `message_stats.csv`
A small table showing the lengths of sample chat messages, including:
- Short messages
- Medium messages
- Empty message (negative test)
- Very long message (negative test)

# Evaluation Results (Draft)
CECS 478 – Alpha/Beta Integrated Release

This document shows the early evaluation of the Secure Terminal Chat Room.  
The goal here is not to present final results, but to show that testing and
analysis have begun and that important evidence has been collected.

---

## 1. Data Collected

We collected several kinds of data from the running system:

- **chat_capture.pcap** – Packet capture of the encrypted network traffic.
- **server_logs.txt** – Logs showing server startup and shutdown.
- **client1_logs.txt**, **client2_logs.txt**, **client3_logs.txt** – Logs showing client compilation and behavior.
- **message_stats.csv** – A small table showing message length information.
- **Demo script output** – Happy-path and negative-path runs from `make demo`.

All files are stored inside: **artifcats folder**

## 2. Packet Capture Observations (Wireshark / tcpdump)

When we opened the `chat_capture.pcap` file in Wireshark, we saw the following:

### No plaintext messages  
All packets were unreadable (ciphertext).  
This confirms that ChaCha20 Poly1305 AEAD encryption is actually working.

### Nonces and tag sizes look correct  
Each encrypted packet contains:
- an 8 byte nonce  
- ciphertext  
- a 16 byte authentication tag  

These match the expected cryptographic structure.

### ✔ Longer messages produce larger packets  
The “long AAAAA…” negative test produced large encrypted packets

This proves that the encryption is handling different message sizes correctly.

---

## 3. Basic Measurements (message_stats.csv)

Early measurements include:

| Message Type          | Example                          | Size (Bytes) |
|-----------------------|----------------------------------|--------------|
| Short message         | “Hello everyone! I am Bob.”      | ~28          |
| Regular reply         | “Hi Bob! Alice here.”            | ~20          |
| Medium-length message | “No, you're fine…”               | ~34          |
| Empty message         | “Alice: ”                        | ~7           |
| Long message (neg. test) | 2000+ “A” characters          | ~2080        |

These numbers show:

- Longer plaintext → larger ciphertext  
- No crashes or memory issues with oversized inputs  
- Consistent encryption and packet structure  

---

## 4. Negative Test Results

We ran two negative behaviors during `make demo`:

### ✔ Empty message  
- System handled it normally  
- No crashes or errors  
- Still encrypted correctly  

### ✔ Extremely long message (~ 2080 bytes)  
- Encryption still worked  
- Server processed and broadcast the message  
- System remained stable  
- Expected large encrypted packets were recorded  

The system handled malformed or extreme input safely, showing good robustness.

---

## 5. Conclusions

Based on the evaluation:

- **Encryption is fully active**  
  No plaintext appears on the wire.

- **Replay protection appears functional**  
  No nonce reuse or out-of-order nonce issues seen.

- **System is stable under normal and negative tests**  
  No crashes or undefined behavior.

- **Logs and pcaps match expected behavior**
  Server logs and network captures are consistent.
  Server logs and network captures are consistent.

These results show that the system is working as designed and that the evaluation process has clearly begun.

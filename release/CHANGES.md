# Tagged Release Changes

### v1.0.0

This is the stable version of our chat server, with the following core features:
- Server and multi-client network running on a private Docker network.
- Secure key exchange using Diffie Hellman and encrypts traffic using ChaCha20 Poly1305 AEAD encryption.
- Reproducibility tests through commands such as `make up && make demo`.
- Code polished to fix any lingering warnings and errors.

Source code is located under `src/`, documents are located under `docs/`, and logs/pcap files are located under `artifacts/release/`.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sodium.h>

#define LENGTH 2048
#define ENCRYPTED_LENGTH (LENGTH + crypto_aead_chacha20poly1305_IETF_ABYTES)
#define CLEAR_LINE_ANSI "\033[1A\033[2K\r"
#define RULES_TXT "Welcome to the chatroom! Enter `/exit` to safely exit.\n"

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];
unsigned char session_key[crypto_aead_chacha20poly1305_IETF_KEYBYTES];
unsigned long long send_nonce_counter = 0;
unsigned long long recv_nonce_counter = 0;
pthread_mutex_t nonce_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Perform key exchange with server */
int perform_key_exchange(int sockfd, unsigned char *session_key) {
	unsigned char client_public_key[crypto_kx_PUBLICKEYBYTES];
	unsigned char client_secret_key[crypto_kx_SECRETKEYBYTES];
	unsigned char server_public_key[crypto_kx_PUBLICKEYBYTES];
	unsigned char rx_key[crypto_kx_SESSIONKEYBYTES];
	unsigned char tx_key[crypto_kx_SESSIONKEYBYTES];
	
	// Generate ephemeral keypair
	if (crypto_kx_keypair(client_public_key, client_secret_key) != 0) {
		fprintf(stderr, "ERROR: Failed to generate keypair\n");
		return -1;
	}
	
	// Receive server's public key
	ssize_t received = recv(sockfd, server_public_key, crypto_kx_PUBLICKEYBYTES, MSG_WAITALL);
	if (received != crypto_kx_PUBLICKEYBYTES) {
		fprintf(stderr, "ERROR: Failed to receive server public key\n");
		return -1;
	}
	
	// Send client's public key
	if (send(sockfd, client_public_key, crypto_kx_PUBLICKEYBYTES, 0) != crypto_kx_PUBLICKEYBYTES) {
		perror("ERROR: Failed to send client public key");
		sodium_memzero(client_secret_key, sizeof(client_secret_key));
		return -1;
	}
	
	// Compute shared secret (client side)
	if (crypto_kx_client_session_keys(rx_key, tx_key,
	                                   client_public_key, client_secret_key,
	                                   server_public_key) != 0) {
		fprintf(stderr, "ERROR: Key exchange failed (suspicious server public key)\n");
		sodium_memzero(client_secret_key, sizeof(client_secret_key));
		return -1;
	}
	
	// Use tx_key (what client sends to server) as the session key
	// This matches with server's rx_key
	memcpy(session_key, tx_key, crypto_aead_chacha20poly1305_IETF_KEYBYTES);
	
	// Zero out the secret key immediately
	sodium_memzero(client_secret_key, sizeof(client_secret_key));
	
	return 0;
}

/* Encrypt message */
int encrypt_message(const unsigned char *plaintext, size_t plaintext_len,
                    unsigned char *ciphertext, unsigned long long nonce_counter) {
	unsigned char nonce[crypto_aead_chacha20poly1305_IETF_NPUBBYTES] = {0};
	memcpy(nonce, &nonce_counter, sizeof(nonce_counter));
	
	unsigned long long ciphertext_len;
	int result = crypto_aead_chacha20poly1305_ietf_encrypt(
		ciphertext, &ciphertext_len,
		plaintext, plaintext_len,
		NULL, 0,
		NULL,
		nonce,
		session_key
	);
	
	return (result == 0) ? (int)ciphertext_len : -1;
}

/* Decrypt message */
int decrypt_message(const unsigned char *ciphertext, size_t ciphertext_len,
                    unsigned char *plaintext, unsigned long long nonce_counter) {
	unsigned char nonce[crypto_aead_chacha20poly1305_IETF_NPUBBYTES] = {0};
	memcpy(nonce, &nonce_counter, sizeof(nonce_counter));
	
	unsigned long long plaintext_len;
	int result = crypto_aead_chacha20poly1305_ietf_decrypt(
		plaintext, &plaintext_len,
		NULL,
		ciphertext, ciphertext_len,
		NULL, 0,
		nonce,
		session_key
	);
	
	return (result == 0) ? (int)plaintext_len : -1;
}

void str_trim_lf (char* arr, int length) {
	int i;
	for (i = 0; i < length; i++) {
		if (arr[i] == '\n' || i == length - 1) {
			arr[i] = '\0';
			break;
		}
	}
}

void catch_ctrl_c_and_exit(int sig) {
	flag = 1;
}

void str_overwrite_stdout() {
	printf("%s", "> ");
	fflush(stdout);
}

void send_msg_handler() {
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};
	unsigned char encrypted[ENCRYPTED_LENGTH + sizeof(unsigned long long)];

	while(1) {
		str_overwrite_stdout();
		if (fgets(message, LENGTH, stdin) == NULL) {
			break; // Handle Ctrl+D (EOF) gracefully
		}

		// CHECK FOR OVERFLOW
		size_t len = strlen(message);
		if (len == LENGTH - 1 && message[len - 1] != '\n') {
				// Flush the standard input buffer
				int ch;
				while ((ch = getchar()) != '\n' && ch != EOF);
		}

		printf("%s", CLEAR_LINE_ANSI);
		str_trim_lf(message, LENGTH);

		if (strncmp(message, "/exit", 5) == 0) {
			break;
		} else {
			snprintf(buffer, sizeof(buffer), "%s: %s\n", name, message);
			
			// Encrypt message
			pthread_mutex_lock(&nonce_mutex);
			int encrypted_len = encrypt_message(
				(unsigned char*)buffer, strlen(buffer),
				encrypted + sizeof(unsigned long long),
				send_nonce_counter
			);
			
			if(encrypted_len < 0){
				fprintf(stderr, "ERROR: Encryption failed\n");
				pthread_mutex_unlock(&nonce_mutex);
				continue;
			}
			
			// Prepend nonce counter
			memcpy(encrypted, &send_nonce_counter, sizeof(unsigned long long));
			send_nonce_counter++;
			pthread_mutex_unlock(&nonce_mutex);
			
			// Send encrypted message
			int total_len = sizeof(unsigned long long) + encrypted_len;
			send(sockfd, encrypted, total_len, 0);
		}

		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}
	catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() {
	unsigned char encrypted[ENCRYPTED_LENGTH + sizeof(unsigned long long)];
	unsigned char decrypted[LENGTH];
	
	while (1) {
		int receive = recv(sockfd, encrypted, ENCRYPTED_LENGTH + sizeof(unsigned long long), 0);
		
		if (receive > sizeof(unsigned long long)) {
			unsigned long long received_nonce;
			memcpy(&received_nonce, encrypted, sizeof(unsigned long long));
			
			int decrypted_len = decrypt_message(
				encrypted + sizeof(unsigned long long),
				receive - sizeof(unsigned long long),
				decrypted,
				received_nonce
			);
			
			if(decrypted_len > 0){
				decrypted[decrypted_len] = '\0';
				printf("%s", (char*)decrypted);
				str_overwrite_stdout();
			} else {
				fprintf(stderr, "ERROR: Decryption failed\n");
			}
		} else if (receive == 0) {
			break;
		}
		
		memset(encrypted, 0, sizeof(encrypted));
		memset(decrypted, 0, sizeof(decrypted));
	}
}

int main(int argc, char **argv){
	if(argc != 3){
		printf("Usage: %s <ip> <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Initialize libsodium
	if (sodium_init() < 0) {
		fprintf(stderr, "ERROR: libsodium initialization failed\n");
		return EXIT_FAILURE;
	}

	char *ip = argv[1];
	int port = atoi(argv[2]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("Please enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	if (strlen(name) > 32 || strlen(name) < 2){
		printf("Name must be less than 30 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	// Perform Diffie-Hellman key exchange
	printf("Performing key exchange with server...\n");
	if (perform_key_exchange(sockfd, session_key) != 0) {
		fprintf(stderr, "ERROR: Key exchange failed\n");
		close(sockfd);
		return EXIT_FAILURE;
	}
	printf("Key exchange successful! Secure session established.\n");

	// Send encrypted name
	unsigned char encrypted_name[64];
	int encrypted_len = encrypt_message(
		(unsigned char*)name, strlen(name),
		encrypted_name + sizeof(unsigned long long),
		send_nonce_counter
	);
	
	if(encrypted_len < 0){
		fprintf(stderr, "ERROR: Name encryption failed\n");
		sodium_memzero(session_key, sizeof(session_key));
		close(sockfd);
		return EXIT_FAILURE;
	}
	
	memcpy(encrypted_name, &send_nonce_counter, sizeof(unsigned long long));
	send_nonce_counter++;
	
	send(sockfd, encrypted_name, sizeof(unsigned long long) + encrypted_len, 0);

	printf("\n=== FINAL PROJECT: CHAT SERVER ===\n");
	printf(RULES_TXT);

	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		sodium_memzero(session_key, sizeof(session_key));
		close(sockfd);
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		sodium_memzero(session_key, sizeof(session_key));
		close(sockfd);
		return EXIT_FAILURE;
	}

	pthread_join(send_msg_thread, NULL);

	if(flag){
		printf("\nExiting the chatroom...\n");
	}

	// Zero out session key before exit
	sodium_memzero(session_key, sizeof(session_key));
	close(sockfd);

	return EXIT_SUCCESS;
}

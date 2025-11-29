#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sodium.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define ENCRYPTED_BUFFER_SZ (BUFFER_SZ + crypto_aead_chacha20poly1305_IETF_ABYTES)
#define SERVER_KEYPAIR_FILE "server_keypair.key"

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

// Server's long-term keypair
static unsigned char server_public_key[crypto_kx_PUBLICKEYBYTES];
static unsigned char server_secret_key[crypto_kx_SECRETKEYBYTES];

/* Client structure */
typedef struct {
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	unsigned long long rx_nonce_counter;  // Nonces for receiving from client
	unsigned long long tx_nonce_counter;  // Nonces for sending to client
	unsigned char session_key[crypto_aead_chacha20poly1305_IETF_KEYBYTES];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Load or generate server keypair */
int load_or_generate_server_keypair() {
	FILE *fp = fopen(SERVER_KEYPAIR_FILE, "rb");
	if (fp) {
		size_t read_pub = fread(server_public_key, 1, crypto_kx_PUBLICKEYBYTES, fp);
		size_t read_sec = fread(server_secret_key, 1, crypto_kx_SECRETKEYBYTES, fp);
		fclose(fp);
		
		if (read_pub == crypto_kx_PUBLICKEYBYTES && read_sec == crypto_kx_SECRETKEYBYTES) {
			printf("Loaded server keypair from %s\n", SERVER_KEYPAIR_FILE);
			return 0;
		}
	}
	
	// Generate new keypair
	if (crypto_kx_keypair(server_public_key, server_secret_key) != 0) {
		fprintf(stderr, "ERROR: Failed to generate keypair\n");
		return -1;
	}
	
	fp = fopen(SERVER_KEYPAIR_FILE, "wb");
	if (!fp) {
		perror("ERROR: Cannot create keypair file");
		return -1;
	}
	fwrite(server_public_key, 1, crypto_kx_PUBLICKEYBYTES, fp);
	fwrite(server_secret_key, 1, crypto_kx_SECRETKEYBYTES, fp);
	fclose(fp);
	
	printf("Generated new server keypair and saved to %s\n", SERVER_KEYPAIR_FILE);
	return 0;
}

/* Perform key exchange with client */
int perform_key_exchange(int sockfd, unsigned char *session_key) {
	unsigned char client_public_key[crypto_kx_PUBLICKEYBYTES];
	unsigned char rx_key[crypto_kx_SESSIONKEYBYTES];
	unsigned char tx_key[crypto_kx_SESSIONKEYBYTES];
	
	// Send server's public key
	if (send(sockfd, server_public_key, crypto_kx_PUBLICKEYBYTES, 0) != crypto_kx_PUBLICKEYBYTES) {
		perror("ERROR: Failed to send server public key");
		return -1;
	}
	
	// Receive client's public key
	ssize_t received = recv(sockfd, client_public_key, crypto_kx_PUBLICKEYBYTES, MSG_WAITALL);
	if (received != crypto_kx_PUBLICKEYBYTES) {
		fprintf(stderr, "ERROR: Failed to receive client public key\n");
		return -1;
	}
	
	// Compute shared secret (server side)
	if (crypto_kx_server_session_keys(rx_key, tx_key,
	                                   server_public_key, server_secret_key,
	                                   client_public_key) != 0) {
		fprintf(stderr, "ERROR: Key exchange failed (suspicious client public key)\n");
		return -1;
	}
	
	// Use rx_key (what server receives from client) as the session key
	// We'll derive the actual encryption key from it
	memcpy(session_key, rx_key, crypto_aead_chacha20poly1305_IETF_KEYBYTES);
	
	return 0;
}

/* Encrypt message */
int encrypt_message(const unsigned char *plaintext, size_t plaintext_len,
                    unsigned char *ciphertext, unsigned long long nonce_counter,
                    const unsigned char *key) {
	unsigned char nonce[crypto_aead_chacha20poly1305_IETF_NPUBBYTES] = {0};
	memcpy(nonce, &nonce_counter, sizeof(nonce_counter));
	
	unsigned long long ciphertext_len;
	int result = crypto_aead_chacha20poly1305_ietf_encrypt(
		ciphertext, &ciphertext_len,
		plaintext, plaintext_len,
		NULL, 0,
		NULL,
		nonce,
		key
	);
	
	return (result == 0) ? (int)ciphertext_len : -1;
}

/* Decrypt message */
int decrypt_message(const unsigned char *ciphertext, size_t ciphertext_len,
                    unsigned char *plaintext, unsigned long long nonce_counter,
                    const unsigned char *key) {
	unsigned char nonce[crypto_aead_chacha20poly1305_IETF_NPUBBYTES] = {0};
	memcpy(nonce, &nonce_counter, sizeof(nonce_counter));
	
	unsigned long long plaintext_len;
	int result = crypto_aead_chacha20poly1305_ietf_decrypt(
		plaintext, &plaintext_len,
		NULL,
		ciphertext, ciphertext_len,
		NULL, 0,
		nonce,
		key
	);
	
	return (result == 0) ? (int)plaintext_len : -1;
}

void str_trim_lf (char* arr, int length) {
	int i;
	for (i = 0; i < length; i++) {
		if (arr[i] == '\n') {
			arr[i] = '\0';
			break;
		}
	}
}

void print_client_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d",
		addr.sin_addr.s_addr & 0xff,
		(addr.sin_addr.s_addr & 0xff00) >> 8,
		(addr.sin_addr.s_addr & 0xff0000) >> 16,
		(addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void get_client_ip(struct sockaddr_in addr, char *buffer) {
	sprintf(buffer, "%d.%d.%d.%d",
		addr.sin_addr.s_addr & 0xff,
		(addr.sin_addr.s_addr & 0xff00) >> 8,
		(addr.sin_addr.s_addr & 0xff0000) >> 16,
		(addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

/* Send encrypted message to all clients (each with their own session key) */
void send_message_encrypted(char *plaintext, int sender_uid){
	pthread_mutex_lock(&clients_mutex);
	
	size_t plaintext_len = strlen(plaintext);
	
	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			unsigned char encrypted[ENCRYPTED_BUFFER_SZ];
			
			// Encrypt with recipient's session key and nonce counter
			int encrypted_len = encrypt_message(
				(unsigned char*)plaintext, plaintext_len,
				encrypted + sizeof(unsigned long long),
				clients[i]->tx_nonce_counter,
				clients[i]->session_key
			);
			
			if(encrypted_len < 0){
				fprintf(stderr, "ERROR: Encryption failed for client %d\n", clients[i]->uid);
				continue;
			}
			
			// Prepend nonce counter
			memcpy(encrypted, &clients[i]->tx_nonce_counter, sizeof(unsigned long long));
			clients[i]->tx_nonce_counter++;
			
			int total_len = sizeof(unsigned long long) + encrypted_len;
			if(write(clients[i]->sockfd, encrypted, total_len) < 0){
				perror("ERROR: write to descriptor failed");
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;
	unsigned char encrypted_buff[ENCRYPTED_BUFFER_SZ];
	unsigned char decrypted_buff[BUFFER_SZ];

	cli_count++;
	client_t *cli = (client_t *)arg;
	cli->rx_nonce_counter = 0;
	cli->tx_nonce_counter = 0;

	// Perform Diffie-Hellman key exchange
	printf("Performing key exchange with client...\n");
	if (perform_key_exchange(cli->sockfd, cli->session_key) != 0) {
		fprintf(stderr, "ERROR: Key exchange failed\n");
		leave_flag = 1;
		goto cleanup;
	}
	printf("Key exchange successful! Session key established.\n");

	// Receive encrypted name
	ssize_t name_recv = recv(cli->sockfd, encrypted_buff, ENCRYPTED_BUFFER_SZ, 0);
	if(name_recv <= sizeof(unsigned long long)){
		printf("ERROR: Invalid name packet\n");
		leave_flag = 1;
	} else {
		unsigned long long received_nonce;
		memcpy(&received_nonce, encrypted_buff, sizeof(unsigned long long));
		
		int decrypted_len = decrypt_message(
			encrypted_buff + sizeof(unsigned long long),
			name_recv - sizeof(unsigned long long),
			decrypted_buff,
			received_nonce,
			cli->session_key
		);
		
		if(decrypted_len < 0 || decrypted_len < 2 || decrypted_len >= 32){
			printf("ERROR: Name decryption failed or invalid length\n");
			leave_flag = 1;
		} else {
			decrypted_buff[decrypted_len] = '\0';
			strcpy(name, (char*)decrypted_buff);
			strcpy(cli->name, name);
			
			char ip_buff[16];
			char internal_buff[BUFFER_SZ];
			get_client_ip(cli->address, ip_buff);
			sprintf(buff_out, "%s has joined\n", cli->name);
			sprintf(internal_buff, "%s (%s) has joined\n", cli->name, ip_buff);
			printf("%s", internal_buff);
			send_message_encrypted(buff_out, cli->uid);
		}
	}

	bzero(buff_out, BUFFER_SZ);

	while(1){
		if (leave_flag) {
			break;
		}

		ssize_t receive = recv(cli->sockfd, encrypted_buff, ENCRYPTED_BUFFER_SZ, 0);
		if (receive > sizeof(unsigned long long)){
			unsigned long long received_nonce;
			memcpy(&received_nonce, encrypted_buff, sizeof(unsigned long long));
			
			int decrypted_len = decrypt_message(
				encrypted_buff + sizeof(unsigned long long),
				receive - sizeof(unsigned long long),
				decrypted_buff,
				received_nonce,
				cli->session_key
			);
			
			if(decrypted_len > 0){
				decrypted_buff[decrypted_len] = '\0';
				strcpy(buff_out, (char*)decrypted_buff);
				
				if(strlen(buff_out) > 0){
					send_message_encrypted(buff_out, cli->uid);
					str_trim_lf(buff_out, strlen(buff_out));
					printf("%s\n", buff_out);
				}
			} else {
				printf("ERROR: Decryption failed from client %s\n", cli->name);
			}
		} else if (receive == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message_encrypted(buff_out, cli->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: recv failed\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
		bzero(encrypted_buff, ENCRYPTED_BUFFER_SZ);
		bzero(decrypted_buff, BUFFER_SZ);
	}

cleanup:
	close(cli->sockfd);
	
	// Zero out session key before freeing
	sodium_memzero(cli->session_key, sizeof(cli->session_key));
	
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Initialize libsodium
	if (sodium_init() < 0) {
		fprintf(stderr, "ERROR: libsodium initialization failed\n");
		return EXIT_FAILURE;
	}

	// Load or generate server keypair
	if (load_or_generate_server_keypair() < 0) {
		return EXIT_FAILURE;
	}

	int port = atoi(argv[1]);
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	signal(SIGPIPE, SIG_IGN);

	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	if (listen(listenfd, 10) < 0) {
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	printf("=== FINAL PROJECT: CHAT SERVER ===\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		if((cli_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		sleep(1);
	}

	// Zero out server secret key on exit
	sodium_memzero(server_secret_key, sizeof(server_secret_key));

	return EXIT_SUCCESS;
}

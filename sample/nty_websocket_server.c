
#include "nty_coroutine.h"

#include <arpa/inet.h>

#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>


#define MAX_CLIENT_NUM			1000000
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

#define NTY_WEBSOCKET_SERVER_PORT	9096
#define GUID 					"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define MAX_BUFFER_LENGTH			1024

struct _nty_ophdr {

	unsigned char opcode:4,
		 rsv3:1,
		 rsv2:1,
		 rsv1:1,
		 fin:1;
	unsigned char payload_length:7,
		mask:1;

} __attribute__ ((packed));

struct _nty_websocket_head_126 {
	unsigned short payload_length;
	char mask_key[4];
	unsigned char data[8];
} __attribute__ ((packed));

struct _nty_websocket_head_127 {

	unsigned long long payload_length;
	char mask_key[4];

	unsigned char data[8];
	
} __attribute__ ((packed));

#define UNION_HEADER(type1, type2) 	\
	struct {						\
		struct type1 hdr;			\
		struct type2 len;			\
	}


typedef struct _nty_websocket_head_127 nty_websocket_head_127;
typedef struct _nty_websocket_head_126 nty_websocket_head_126;
typedef struct _nty_ophdr nty_ophdr;


static int ntySetNonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

static int ntySetBlock(int fd)  
{
    int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return flags; 
    flags &=~O_NONBLOCK;  
    if (fcntl(fd, F_SETFL, flags) < 0) return -1;

	return 0;
} 


int base64_encode(char *in_str, int in_len, char *out_str) {    
	BIO *b64, *bio;    
	BUF_MEM *bptr = NULL;    
	size_t size = 0;    

	if (in_str == NULL || out_str == NULL)        
		return -1;    

	b64 = BIO_new(BIO_f_base64());    
	bio = BIO_new(BIO_s_mem());    
	bio = BIO_push(b64, bio);
	
	BIO_write(bio, in_str, in_len);    
	BIO_flush(bio);    

	BIO_get_mem_ptr(bio, &bptr);    
	memcpy(out_str, bptr->data, bptr->length);    
	out_str[bptr->length-1] = '\0';    
	size = bptr->length;    

	BIO_free_all(bio);    
	return size;
}


int readline(char* allbuf,int level,char* linebuf){    
	int len = strlen(allbuf);    

	for (;level < len; ++level)    {        
		if(allbuf[level]=='\r' && allbuf[level+1]=='\n')            
			return level+2;        
		else            
			*(linebuf++) = allbuf[level];    
	}    

	return -1;
}

void umask(char *data,int len,char *mask){    
	int i;    
	for (i = 0;i < len;i ++)        
		*(data+i) ^= *(mask+(i%4));
}


char* decode_packet(unsigned char *stream, char *mask, int length, int *ret) {

	nty_ophdr *hdr =  (nty_ophdr*)stream;
	unsigned char *data = stream + sizeof(nty_ophdr);
	int size = 0;
	int start = 0;
	//char mask[4] = {0};
	int i = 0;

	if ((hdr->mask & 0x7F) == 126) {

		nty_websocket_head_126 *hdr126 = (nty_websocket_head_126*)data;
		size = hdr126->payload_length;
		
		for (i = 0;i < 4;i ++) {
			mask[i] = hdr126->mask_key[i];
		}
		
		start = 8;
		
	} else if ((hdr->mask & 0x7F) == 127) {

		nty_websocket_head_127 *hdr127 = (nty_websocket_head_127*)data;
		size = hdr127->payload_length;
		
		for (i = 0;i < 4;i ++) {
			mask[i] = hdr127->mask_key[i];
		}
		
		start = 14;

	} else {
		size = hdr->payload_length;

		memcpy(mask, data, 4);
		start = 6;
	}

	*ret = size;
	umask(stream+start, size, mask);

	return stream + start;
	
}

int encode_packet(char *buffer,char *mask, char *stream, int length) {

	nty_ophdr head = {0};
	head.fin = 1;
	head.opcode = 1;
	int size = 0;

	if (length < 126) {
		head.payload_length = length;
		memcpy(buffer, &head, sizeof(nty_ophdr));
		size = 2;
	} else if (length < 0xffff) {
		nty_websocket_head_126 hdr = {0};
		hdr.payload_length = length;
		memcpy(hdr.mask_key, mask, 4);

		memcpy(buffer, &head, sizeof(nty_ophdr));
		memcpy(buffer+sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_126));
		size = sizeof(nty_websocket_head_126);
		
	} else {
		
		nty_websocket_head_127 hdr = {0};
		hdr.payload_length = length;
		memcpy(hdr.mask_key, mask, 4);
		
		memcpy(buffer, &head, sizeof(nty_ophdr));
		memcpy(buffer+sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_127));

		size = sizeof(nty_websocket_head_127);
		
	}

	memcpy(buffer+2, stream, length);

	return length + 2;
}



void server_reader(void *arg) {
	int fd = *(int *)arg;
	int length = 0;
	int ret = 0;

	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLIN;

	while (1) {
		
		char stream[MAX_BUFFER_LENGTH] = {0};
		length = nty_recv(fd, stream, MAX_BUFFER_LENGTH, 0);
		if (length > 0) {
			if(fd > MAX_CLIENT_NUM) 
				printf("read from server: %.*s\n", length, stream);

			char mask[4] = {0};
			char *data = decode_packet(stream, mask, length, &ret);

			printf(" data : %s , length : %d\n", data, ret);
			
#if 1					
			char buffer[MAX_BUFFER_LENGTH+14] = {0};
			ret = encode_packet(buffer, mask, data, ret);
			ret = nty_send(fd, buffer, ret, 0);
#endif	
		} else if (length == 0) {	
			nty_close(fd);
			break;
		}

	}
}


int handshake(int cli_fd) {    
	int level = 0;     
	char buffer[MAX_BUFFER_LENGTH];    
	char linebuf[128];    //Sec-WebSocket-Accept    
	char sec_accept[32] = {0};    //sha1 data    
	unsigned char sha1_data[SHA_DIGEST_LENGTH+1] = {0};    //reponse head buffer    
	//char head[MAX_BUFFER_LENGTH] = {0};    

#if 1
	if (read(cli_fd, buffer, sizeof(buffer))<=0)        
		perror("read");
#else

	int ret = 0;
	int length = recv_buffer(cli_fd, buffer, MAX_BUFFER_LENGTH, &ret);
	if (ret < 0) perror("read");

#endif
	printf("request\n");    
	printf("%s\n",buffer);   
	
	do {        
		memset(linebuf, 0, sizeof(linebuf));        
		level = readline(buffer,level,linebuf); 

		if (strstr(linebuf,"Sec-WebSocket-Key") != NULL)        {   
			
			strcat(linebuf, GUID);    
			
			SHA1((unsigned char*)&linebuf+19,strlen(linebuf+19),(unsigned char*)&sha1_data);  

			memset(buffer, 0, MAX_BUFFER_LENGTH);
			base64_encode(sha1_data,strlen(sha1_data),sec_accept);           
			sprintf(buffer, "HTTP/1.1 101 Switching Protocols\r\n" \
				"Upgrade: websocket\r\n" \
				"Connection: Upgrade\r\n" \
				"Sec-WebSocket-Accept: %s\r\n" \
				"\r\n", sec_accept);  
			

			printf("response\n");       
			printf("%s",buffer);            
#if 1
			if (write(cli_fd, buffer, strlen(buffer)) < 0)                
				perror("write");            
#else

			length = send_buffer(cli_fd, head, strlen(head));
			assert(length == strlen(head));

#endif
			printf("accept : %s\n", sec_accept);
			break;        
		}    

	}while((buffer[level]!='\r' || buffer[level+1]!='\n') && level!=-1);    

	return 0;

}


int init_server(void) {

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("socket failed\n");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NTY_WEBSOCKET_SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
		printf("bind failed\n");
		return -2;
	}

	if (listen(sockfd, 5) < 0) {
		printf("listen failed\n");
		return -3;
	}

	return sockfd;
	
}

void server(void *arg) {

	int sockfd = init_server();

	struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);

	while (1) {
		socklen_t len = sizeof(struct sockaddr_in);
		struct sockaddr_in remote;
		
		int cli_fd = nty_accept(sockfd, (struct sockaddr*)&remote, &len);
		if (cli_fd % 1000 == 999) {

			struct timeval tv_cur;
			memcpy(&tv_cur, &tv_begin, sizeof(struct timeval));
			
			gettimeofday(&tv_begin, NULL);
			int time_used = TIME_SUB_MS(tv_begin, tv_cur);
			
			printf("client fd : %d, time_used: %d\n", cli_fd, time_used);
		}
		printf("new client comming\n");
		ntySetBlock(cli_fd);
		handshake(cli_fd);
		ntySetNonblock(cli_fd);

		nty_coroutine *read_co = NULL;
		nty_coroutine_create(&read_co, server_reader, &cli_fd);

	}
	
}

int main() {

	nty_coroutine *co = NULL;
	nty_coroutine_create(&co, server, NULL); 

	nty_schedule_run();
}




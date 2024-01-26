#include "cse2303header.h"


//client for fs
int main(int argc, char* argv[]){
	char buffer[2003];
	if(argc != 2){
		printf("invalid argument!\n");
		exit(-1);
	}
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// error check
	if(sockfd <= 0){
		perror("socket");
		exit(1);
	}

	struct sockaddr_in serv_addr;
	struct hostent *host;
	serv_addr.sin_family = AF_INET;
	host = gethostbyname("localhost");
	// error check
	if(host == NULL){
		perror("host");
		exit(2);
	}
	memcpy(&serv_addr.sin_addr.s_addr, host->h_addr, host->h_length);
	serv_addr.sin_port = htons(atoi(argv[1]));
	// error check
	if(connect(sockfd, (struct sockaddr_in *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("connect");
		exit(3);
	}

	while(1){
		printf(">");
		bzero(buffer, 2003);
		fgets(buffer, 2003, stdin);
		int length = strlen(buffer);
		write(sockfd, buffer, strlen(buffer));
		bzero(buffer, 2003);
		read(sockfd, buffer, 2003);
		printf("%s", buffer);
		if(!strcmp(buffer, "Goodbye!\n")) break;
	}
	
	close(sockfd);
	exit(0);
}

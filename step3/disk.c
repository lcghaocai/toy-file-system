#include "cse2303header.h"

const int block = 256;

int main(int argc, char* argv[]){
	int cylinder, sector, c = 0, t;
	int c1, s1;
	char op;
	char buffer[1005], sockbuf[1005];

	if(argc < 6){
		printf("invalid argument!\n");
		exit(-1);
	}
	
	cylinder = atoi(argv[1]);
	sector = atoi(argv[2]);
	t = atoi(argv[3]);
	
	
	
	if(cylinder <= 0 || sector <= 0 || t < 0 && strcmp(argv[3], "0")){
		printf("invalid argument!\n");
		exit(-1);
	}
	
	int sockfd;
	int client_sockfd;
	struct sockaddr_in client_addr;
	int len = sizeof(client_addr);
	struct sockaddr_in serv_addr;
	
	int fd = open (argv[4], O_RDWR | O_CREAT, 0777);
	if (fd < 0) {
		printf("Error: Could not open file '%s'.\n", argv[4]);
		exit(-1);
	}
	
	long FILESIZE = cylinder * sector * block;
	int result = lseek (fd, FILESIZE-1, SEEK_SET);
	if (result == -1) {
		perror ("Error calling lseek() to 'stretch' the file");
		close (fd);
		exit(-1);
	}

	result = write (fd, "", 1);
	if (result != 1) {
		perror("Error writing last byte of the file");
		close(fd);
		exit(-1);
	}
	
	char* diskfile;
	diskfile = (char *) mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (diskfile == MAP_FAILED){
		close(fd);
		printf("Error: Could not map file.\n");
		exit(-1);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("error opening socket");
		exit(2);
	}
	printf("socket open successfully.\n");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[5]));
	if(bind(sockfd, (struct sockaddr_in*) &serv_addr, sizeof(serv_addr)) < 0){
		perror("error binding");
		exit(3);
	}
	printf("bind successfully.\n");

	if(listen(sockfd, 5) < 0){
		perror("error listening");
		exit(4);
	}
	printf("listen successfully.\n");
	
	client_sockfd = accept(sockfd, (struct sockaddr_in *) &client_addr, &len);
	int childport;
	
	if(client_sockfd < 0){
		perror("error accepting client");
		exit(5);
	}else {
		childport = ntohs(client_addr.sin_port);
		printf("accept client in %d.\n", childport);
	}
	
	int length, n;
	
	while(1){
		bzero(sockbuf, 1003);
		read(client_sockfd, sockbuf, 1003);
		switch(sockbuf[0]){
			case 'I':
				sprintf(sockbuf, "%d %d", cylinder, sector);
				write(client_sockfd, sockbuf, strlen(sockbuf) + 1);
				printf("send info.\n");
				break;
			case 'R':
				sscanf(sockbuf, "R %d %d", &c1, &s1);
				if(c1 < cylinder && s1 < sector && c1 >= 0 && s1 >= 0){
					usleep(1000 * t * abs(c - c1));
					memcpy (buffer, &diskfile[block * (c1 * sector + s1)], block);
					write(client_sockfd, buffer, 256);
					printf("read from (%d, %d) successfully in %d ms.\n", c1, s1, t * abs(c - c1));
					c = c1;
				}else {
					printf("read failed!\n");
				}
				break;
			case 'W':
				sscanf(sockbuf, "W %d %d%n", &c1, &s1, &n);
				if(c1 < cylinder && s1 < sector && c1 >= 0 && s1 >= 0){
					usleep(1000 * t * abs(c - c1));
					memcpy (&diskfile[block * (c1 * sector + s1)], sockbuf + n + 1, block);
					printf("write to (%d, %d) successfully in %d ms.\n", c1, s1, t * abs(c - c1));
					write(client_sockfd, "Y", 1);
					c = c1;
				}else {
					printf("write failed!\n");
					write(client_sockfd, "N", 1);
				}
				break;
			case 'E':
				close(fd);
				close(client_sockfd);
				close(sockfd);
				exit(0);
		}
	}
}

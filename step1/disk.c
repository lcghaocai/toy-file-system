#include "cse2303header.h"

const int block = 256;

int main(int argc, char* argv[]){
	int cylinder, sector, c = 0, t;
	int c1, s1;
	char op;
	char buffer[1005];

	if(argc < 4){
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
	
	freopen("disk.log", "w+", stdout);
	
	int length;
	while(1){
		op = getchar();
		while(!isalpha(op) && op != EOF)
			op = getchar();
		switch(op){
			case 'I':
				printf("%d %d\n", cylinder, sector);
				break;
			case 'R':
				scanf("%d %d", &c1, &s1);
				if(c1 < cylinder && s1 < sector && c1 >= 0 && s1 >= 0){
					usleep(100 * t * abs(c - c1));
					memcpy (buffer, &diskfile[block * (c1 * sector + s1)], block);
					printf("Yes %s\n", buffer);
					c = c1;
				}else {
					printf("No\n");
				}
				break;
			case 'W':
				scanf("%d %d ", &c1, &s1);
				fgets(buffer, 1003, stdin);
				length = strlen(buffer);
				buffer[length - 1] = buffer[length - 1] == '\n' ? 0 : buffer[length - 1];
				if(c1 < cylinder && s1 < sector && c1 >= 0 && s1 >= 0 && length < 256){
					usleep(100 * t * abs(c - c1));
					memcpy (&diskfile[block * (c1 * sector + s1)], buffer, strlen(buffer));
					printf("Yes\n");
					c = c1;
				}else {
					printf("No\n");
				}
				break;
			case 'E':
				printf("Goodbye!\n");
				exit(0);
			default:
				printf("invalid instruction!\n");
				break;
		}
	}
}

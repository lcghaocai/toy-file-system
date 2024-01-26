#include "cse2303header.h"

const uint32_t DELETED = 0xffffffff;
const int block = 256, inode_size = 32, inode_num = 8;
char file_buf[17417];
int c, s, sockfd;
struct super_block{
    uint32_t inode_count, data_count, free_inode_count, free_data_count;
    uint32_t root_inode, first_free_inode, first_free_data;
    uint8_t valid_bit;
    uint8_t map[227];
}sb_cache;

struct inode{
    uint16_t type;
    uint16_t deleted_record;
    uint32_t creation_time;
    uint32_t file_size;
    uint32_t direct_block[4];
    uint32_t single_indirect;
};

enum inode_type{
    directory = 1,
    data = 0
};

uint32_t maxu(uint32_t a, uint32_t b){return a<b?b:a;}
void send_write(uint32_t block_num, void* data);
void send_read(uint32_t block_num, void* data);
void read_inode(uint32_t inode, void* buffer);
void write_inode(uint32_t inode, void* buffer);
void read_file(struct inode* cur, void* buffer);
void write_file(struct inode* cur, void* buffer, uint32_t cur_size);
uint32_t find_new_block(int op);
void free_block(uint32_t num, int op);
void add_to_directory(struct inode* cur, uint32_t id, uint8_t type, char* name);
uint32_t find_file(struct inode* cur, uint8_t type, char* name);
uint32_t delete_file(struct inode* cur, uint8_t type, char* name);
void dir_init(void* cur, uint32_t father_id, char* father_name, uint32_t cur_id, char* cur_name);

int main(int argc, char* argv[]){


    FILE* log = fopen("fs.log", "w+");
    if(log < 0){
        printf("fail to open log file!\n");
        exit(-1);
    }

    if(argc < 2){
        printf("invalid argument!\n");
        exit(-1);
    }

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

    char op[10], buffer[1005], cmd_buf[1005], send_buf[257], cur_dir[257], file_name[257];
    char names[2][19][307];
    uint32_t order[2][19];
    struct inode tmp, cur;

    write(sockfd, "I", 1);
    read(sockfd, buffer, 1003);
    sscanf(buffer, "%d%d", &c, &s);

    uint32_t p, cur_inode, inode_id, data_id;
    send_read(0, &sb_cache);
    if(sb_cache.valid_bit == 'c'){
        printf("formated.\n");
        cur_inode = sb_cache.root_inode;
        read_inode(cur_inode, buffer);
        memcpy(&cur, buffer + (cur_inode - 1) % inode_num * inode_size, inode_size);
    }

    while(1){
        scanf("%s", op);
        switch(op[0]){
            case 'f':{
                sb_cache.inode_count = c * s / 6;
                sb_cache.data_count = c * s - sb_cache.inode_count - 1;
                sb_cache.free_data_count = sb_cache.data_count - 1;
                sb_cache.free_inode_count = sb_cache.inode_count * 4 - 1;
                sb_cache.root_inode = 1;
                sb_cache.first_free_inode = 2;
                sb_cache.first_free_data = 2;
                sb_cache.valid_bit = 'c';

                bzero(sb_cache.map, sizeof(sb_cache.map));
                sb_cache.map[0] = sb_cache.map[sb_cache.inode_count * 4 + 1] = 1;

                tmp.type = directory;
                tmp.creation_time= time(NULL);
                tmp.file_size = 11;
                tmp.deleted_record = 0;
                bzero(tmp.direct_block, sizeof(tmp.direct_block));
                tmp.direct_block[0] = sb_cache.inode_count + 1;
                tmp.single_indirect = 0;
                bzero(send_buf, 256);
                memcpy(send_buf, &tmp, 32);
                memcpy(&cur, &tmp, 32);
                send_write(0, &sb_cache);
                send_write(1, send_buf);

                bzero(send_buf, 256);
                cur_inode = 1;
                strcpy(cur_dir, "/");
                dir_init(send_buf, 1, cur_dir, 1, cur_dir);

                send_write(sb_cache.inode_count + 1, send_buf);
                fprintf(log, "Done!\n");
                break;
            }

            case 'm':{ //mkdir or mk
                if(sb_cache.valid_bit != 'c') {
                    printf("please format the disk first!\n");
                    break;
                }

                scanf("%s", cmd_buf);

                if(!strcmp("mkdir", op)){

                    inode_id = find_new_block(directory);
                    add_to_directory(&cur, inode_id, directory, cmd_buf);

                    tmp.type = directory;
                    tmp.creation_time = time(NULL);
                    tmp.file_size = 10 + strlen(cmd_buf);
                    bzero(tmp.direct_block, sizeof(tmp.direct_block));
                    data_id = find_new_block(data);
                    tmp.deleted_record = 0;

                    tmp.direct_block[0] = data_id;
                    tmp.single_indirect = 0;

                    read_inode(inode_id, buffer);
                    memcpy(buffer + (inode_id - 1) % inode_num * inode_size, &tmp, inode_size);
                    write_inode(inode_id, buffer);

                    bzero(buffer, block);
                    dir_init(buffer, cur_inode, cur_dir, inode_id, cmd_buf);
                    send_write(data_id, buffer);

                }else{
                    inode_id = find_new_block(directory);
                    add_to_directory(&cur, inode_id, data, cmd_buf);

                    tmp.type = data;
                    tmp.deleted_record = 0;
                    tmp.creation_time = time(NULL);
                    tmp.file_size = 0;
                    bzero(tmp.direct_block, sizeof(tmp.direct_block));
                    tmp.single_indirect = 0;

                    read_inode(inode_id, buffer);
                    memcpy(buffer + (inode_id - 1) % inode_num * inode_size, &tmp, inode_size);
                    write_inode(inode_id, buffer);

                }
                int i;
//                for(i = 0; i < 4; ++i){
//                    printf("direct block %u:%u\n", i, cur.direct_block[i]);
//                }
                read_inode(cur_inode, buffer);
                memcpy(buffer + (cur_inode - 1) % inode_num * inode_size, &cur, inode_size);
                write_inode(cur_inode, buffer);

                break;
            }
            case 'l': {
                if(sb_cache.valid_bit != 'c') {
                    printf("please format the disk first!\n");
                    break;
                }
                read_file(&cur, file_buf);
                uint32_t i, j, k, it = 0, tmp_record, cnt[2] = {0};
                uint8_t len, type, cnt_hidden = 2;
                while(1){
                    memcpy(&tmp_record, file_buf + it, 4);
                    if(tmp_record == 0) break;

                    if(tmp_record != DELETED){
                        memcpy(&type, file_buf + it + 8, 1);
                        memcpy(&len, file_buf + it + 9, 1);
                        if(type == directory && cnt_hidden > 0) --cnt_hidden;
                        else {
                            memcpy(names[type][cnt[type]], file_buf + it + 10, len);
                            names[type][cnt[type]++][len] = 0;
                        }
                    }
                    memcpy(&tmp_record, file_buf + it + 4, 4);
                    it = tmp_record;
                }

                for(i = 0; i < 2; ++i)
                    for(j = 0; j < cnt[i]; ++j){
                        order[i][j] = j;
                    }
                for(i = 0; i < 2; ++i){
                    for(j = 1; j < cnt[i]; ++j){
                        uint32_t tmp = order[i][j];
                        for(k = j; k >= 0 && strcmp(names[i][order[i][k]], names[i][order[i][k - 1]]) < 0; --k){
                            order[i][k] = order[i][k - 1];
                        }
                        order[i][k] = tmp;
                    }
                }

                for (p = 0; p < cnt[0]; ++p) {
                    fprintf(log, "%s ", names[0][order[0][p]]);
                }
                fprintf(log, "& ");
                for (p = 0; p < cnt[1]; ++p) {
                    fprintf(log, "%s ", names[1][order[1][p]]);
                }
                fprintf(log, "\n");
                break;

            }
            case 'w': {
                if(sb_cache.valid_bit != 'c') {
                    printf("please format the disk first!\n");
                    break;
                }
                uint32_t length, len_data;
                scanf("%s %u", file_name, &length);
                getchar();
                fgets(cmd_buf, 1003, stdin);
                len_data = strlen(cmd_buf);
                if(cmd_buf[len_data - 1] == '\n') cmd_buf[len_data - 1] = 0;

                inode_id = find_file(&cur, data, file_name);
                if(inode_id == 0){
                    fprintf(log, "No\n");
                    break;
                }
                read_inode(inode_id, buffer);
                memcpy(&tmp, buffer + (inode_id - 1) % inode_num * inode_size, inode_size);
                read_file(&tmp, file_buf);
                memcpy(file_buf, cmd_buf, length);
                write_file(&tmp, file_buf, maxu(tmp.file_size, length));
                memcpy( buffer + (inode_id - 1) % inode_num * inode_size, &tmp, inode_size);
                write_inode(inode_id, buffer);
                fprintf(log, "Yes!\n");
                break;
            }
            case 'i':{
                if(sb_cache.valid_bit != 'c') {
                    printf("please format the disk first!\n");
                    break;
                }
                uint32_t length, position, len_data, i;
                scanf("%s %u %u", file_name, &position, &length);
                getchar();
                fgets(cmd_buf, 1003, stdin);
                len_data = strlen(cmd_buf);
                if(cmd_buf[len_data - 1] == '\n') cmd_buf[len_data - 1] = 0;
                inode_id = find_file(&cur, data, file_name);
                if(inode_id == 0 || tmp.file_size + length > 68 * block){
                    fprintf(log, "No\n");
                    break;
                }
                read_inode(inode_id, buffer);
                memcpy(&tmp, buffer + (inode_id - 1) % inode_num * inode_size, inode_size);
                read_file(&tmp, file_buf);
                if(tmp.file_size < position) position = tmp.file_size;
                for(i = tmp.file_size; i >= position; --i) file_buf[i + length] = file_buf[i];
                memcpy(file_buf + position, cmd_buf, length);
                write_file(&tmp, file_buf, tmp.file_size + length);
                memcpy( buffer + (inode_id - 1) % inode_num * inode_size, &tmp, inode_size);
                write_inode(inode_id, buffer);
                fprintf(log, "Yes!\n");
                break;
            }
            case 'd':{
                if(sb_cache.valid_bit != 'c') {
                    printf("please format the disk first!\n");
                    break;
                }
                uint32_t length, position, i;
                scanf("%s %u %u", file_name, &position, &length);
                inode_id = find_file(&cur, data, file_name);
                if(inode_id == 0){
                    fprintf(log, "No\n");
                    break;
                }
                read_inode(inode_id, buffer);
                memcpy(&tmp, buffer + (inode_id - 1) % inode_num * inode_size, inode_size);
                read_file(&tmp, file_buf);
                if(tmp.file_size < position + length) length = tmp.file_size - position;
                for(i = position; position + length <= tmp.file_size; ++i) file_buf[i] = file_buf[i + length];
                write_file(&tmp, file_buf, tmp.file_size - length);
                memcpy( buffer + (inode_id - 1) % inode_num * inode_size, &tmp, inode_size);
                write_inode(inode_id, buffer);
                fprintf(log, "Yes!\n");
                break;
            }
            case 'r':{
                if(sb_cache.valid_bit != 'c') {
                    printf("please format the disk first!\n");
                    break;
                }
                scanf("%s", cmd_buf);
                if(delete_file(&cur, strcmp(op, "rmdir") == 0, cmd_buf))
                    fprintf(log, "Yes!\n");
                else fprintf(log, "No\n");
                break;
            }
            case 'c':{
                if(sb_cache.valid_bit != 'c') {
                    printf("please format the disk first!\n");
                    break;
                }
                if(!strcmp(op, "cat")){
                    scanf("%s", file_name);
                    inode_id = find_file(&cur, data, file_name);
                    if(inode_id == 0){
                        fprintf(log, "No\n");
                        break;
                    }
                    read_inode(inode_id, buffer);
                    memcpy(&tmp, buffer + (inode_id - 1) % inode_num * inode_size, inode_size);
                    read_file(&tmp, file_buf);
                    file_buf[tmp.file_size] = 0;
                    fprintf(log, "%s\n", file_buf);
                }else{  //cd
                    scanf("%s", cmd_buf);
                    char* dir_name;
                    if(cmd_buf[0] == '/'){  //absolute
                        dir_name = strtok(cmd_buf + 1, "/");
                        cur_inode = sb_cache.root_inode;
                        read_inode(cur_inode, buffer);
                        memcpy(&cur, buffer + (cur_inode - 1) % inode_num * inode_size, inode_size);
                    }else dir_name = strtok(cmd_buf, "/");
                    while(dir_name){
                        if(strcmp(dir_name, ".")){
                            cur_inode = find_file(&cur, directory, dir_name);
                            if(cur_inode == 0){
                                fprintf(log, "No\n");
                                break;
                            }
                            read_inode(cur_inode, buffer);
                            memcpy(&cur, buffer + (cur_inode - 1) % inode_num * inode_size, inode_size);
                        }
                        dir_name = strtok(NULL, "/");
                    }
                    if(cur_inode != 0){
                        fprintf(log, "Yes!\n");
                    }
                }
                break;
            }
            case 'e': {
                send_write(0, &sb_cache);
                write(sockfd, "E", 1);
                close(sockfd);
                fprintf(log, "Goodbye!\n");
                fclose(log);
                exit(0);
            }
        }
    }
}

void read_inode(uint32_t inode, void* buffer){
    uint32_t inode_block = (inode - 1) / inode_num;
    send_read(1 + inode_block, buffer);
}
void write_inode(uint32_t inode, void* buffer){
    uint32_t inode_block = (inode - 1) / inode_num;
    send_write(1 + inode_block, buffer);
}

void send_read( uint32_t block_num, void* data){
    char buffer[307];
    uint32_t c1 = block_num / s, s1 = block_num % s;
    sprintf(buffer, "R %u %u", c1, s1);
    write(sockfd, buffer, strlen(buffer));
    bzero(data, 256);
    read(sockfd, data, 256);
}

void send_write( uint32_t block_num, void* data){
    char buffer[307];
    uint32_t c1 = block_num / s, s1 = block_num % s;
    uint32_t length;
    sprintf(buffer, "W %u %u ", c1, s1);
    length = strlen(buffer);
    memcpy(buffer + length, data, block);
    write(sockfd, buffer, block + length);
    read(sockfd, buffer, 1);
    if(buffer[0] == 'Y')
        return;
    printf("error occurs when writing!\n");
}

uint32_t find_new_block(int op){
    uint32_t res;
    if(op == directory){
        if(--sb_cache.free_inode_count == 0)
            return 0;
        res = sb_cache.first_free_inode;
        sb_cache.map[sb_cache.first_free_inode++ - 1] = 1;

        while(sb_cache.map[sb_cache.first_free_inode - 1]) ++sb_cache.first_free_inode;
    }else {
        if(--sb_cache.free_data_count == 0)
            return 0;
        res = sb_cache.first_free_data + sb_cache.inode_count;
        sb_cache.map[sb_cache.inode_count * 4 + sb_cache.first_free_data++] = 1;
        while(sb_cache.map[sb_cache.inode_count * 4 + sb_cache.first_free_data]) ++sb_cache.first_free_data;
    }
    int i;
//    for(i = 0; i < 20; ++i)
//        printf("%d", sb_cache.map[i]);
//    printf(" %d\n", res);
    return res;
}
void free_block(uint32_t num, int op){
    if(op == directory){
        sb_cache.map[num - 1] = 0;
        if(num < sb_cache.first_free_inode) sb_cache.first_free_inode = num;
    }else{
        sb_cache.map[sb_cache.inode_count * 3 + num] = 0;
        if(num < sb_cache.first_free_data - sb_cache.inode_count) sb_cache.first_free_data = num - sb_cache.inode_count;
    }
}
void read_file(struct inode* cur, void* buffer){
    char read_buf[307], single_direct[307];
    uint32_t tmp_record;
    int i;
    for(i = 0; i < 4; ++i){
        if(cur->direct_block[i] != 0){
//            printf("reading %d\n", cur->direct_block[i]);
            send_read(cur->direct_block[i],read_buf);
            memcpy(buffer + i * block, read_buf, block);
        }else break;
    }
    if(cur->direct_block[3] != 0){
        send_read(cur->single_indirect , single_direct);
        for(i = 0; i < 64; ++i){
            memcpy(&tmp_record, single_direct + i * 4, 4);
            if(!tmp_record) break;
            send_read(tmp_record, read_buf);
            memcpy(buffer + block * (4 + i), read_buf, block);
        }
    }
}

void write_file(struct inode* cur, void* buffer, uint32_t cur_size){
    char single[307];
    int read = 0;
    uint32_t tmp_record;
    uint32_t original_block = (cur->file_size + block - 1) / block;
    cur->file_size = cur_size;
    cur_size = (cur_size + block - 1) / block;
//    printf("ori = %d, cur = %d\n", original_block, cur_size);
    uint32_t i;
    if(cur_size < original_block){//shrink
        for(i = 0; i < cur_size && i < 4; ++i){
            send_write(cur->direct_block[i], buffer + i * block);
        }
        if(cur->single_indirect){
            if(cur_size <= 4){
                for(i = cur_size; i < original_block; ++i) {
                    memcpy(&tmp_record, single + (i - 4) * 4, 4);
                    free_block(tmp_record, data);
                    tmp_record = 0;
                    memcpy(single + (i - 4) * 4, &tmp_record, 4);
                }
                free_block(cur->single_indirect, data);
                cur->single_indirect = 0;
            }else {
                read = 1;
                send_read(cur->single_indirect, single);
                for(i = 4; i < cur_size; ++i){
                    memcpy(&tmp_record, single + (i - 4) * 4, 4);
                    if(!tmp_record) break;
                    send_write(tmp_record, buffer + block * i);
                }
                for(i = cur_size; i < original_block; ++i) {
                    memcpy(&tmp_record, single + (i - 4) * 4, 4);
                    free_block(tmp_record, data);
                    tmp_record = 0;
                    memcpy(single + (i - 4) * 4, &tmp_record, 4);
                }
            }
        }


    } else{
        for(i = 0; i < original_block && i < 4; ++i){
            send_write(cur->direct_block[i], buffer + i * block);
        }
        if(original_block > 4){
            send_read(cur->single_indirect, single);
            read = 1;
            for(i = 4; i < original_block; ++i){
                memcpy(&tmp_record, single + (i - 4) * 4, 4);
                if(!tmp_record) break;
                send_write(tmp_record, buffer + block * i);
            }
        }

        for(i = original_block; i < cur_size; ++i){ // allocate part
            if(i < 4){
                cur->direct_block[i] = find_new_block(data);
//                printf("allocate %d\n", cur->direct_block[i]);
                send_write(cur->direct_block[i], buffer + i * block);
            } else{
                if(!cur->single_indirect){
                    cur->single_indirect = find_new_block(data);
                    read = 1;
                }else if(!read){
                    send_read(cur->single_indirect, single);
                    read = 1;
                }
                tmp_record = find_new_block(data);
                memcpy(single + (i - 4) * 4, &tmp_record, 4);
                send_write(tmp_record, buffer + i * block);
            }
        }

    }
    if(read)
        send_write(cur->single_indirect, single);

}

void add_to_directory(struct inode* cur, uint32_t id, uint8_t type, char* name){
    uint32_t tmp_record, p = 0;
    uint8_t length;
    bzero(file_buf, sizeof(file_buf));
    read_file(cur, file_buf);

//    for(p = 0; p < 4; ++p){
//        printf("direct block %u:%u\n", p, cur->direct_block[p]);
//    }
    p = 0;
    while(1){
        memcpy(&tmp_record, file_buf + p, 4);
        if(tmp_record == 0) break;
//        printf("p in %u\n", p);
        memcpy(&tmp_record, file_buf + p + 4, 4);
        p = tmp_record;
//        printf("next p in %u\n", p);
    }
    memcpy(file_buf + p, &id, 4);
    tmp_record = p + 10 + strlen(name);   //next

    memcpy(file_buf + p + 4, &tmp_record, 4);
    memcpy(file_buf + p + 8, &type, 1);
    length = strlen(name);
    memcpy(file_buf + p + 9, &length, 1);
    memcpy(file_buf + p + 10, name, length);
//    printf("file size: %u\n", tmp_record);
    cur->file_size = tmp_record;
    write_file(cur, file_buf, tmp_record);
}

void free_inode(struct inode* tmp){
    uint32_t i, indirect_record;
    char single[257];
    for(i = 0; i < 4; ++i){
        if(tmp->direct_block[i] != 0) free_block(tmp->direct_block[i], data);
    }
    if(tmp->single_indirect != 0){
        send_read(tmp->single_indirect, single);
        for (i = 0; i < 64; ++i) {
            memcpy(&indirect_record, single + 4 * i, 4);
            if(indirect_record != 0)
                free_block(indirect_record, data);
        }
        free_block(tmp->single_indirect, data);
    }
}
uint32_t find_file(struct inode* cur, uint8_t type, char* name){
    uint32_t tmp_record, p = 0;
    uint8_t length, tmp_char;
    char name_buf[257];
    bzero(file_buf, sizeof(file_buf));
    read_file(cur, file_buf);

    if(type == directory){
        if(!strcmp(name, "..")){
            memcpy(&tmp_record, file_buf, 4);
            return tmp_record;
        }
    }
    while(1){
        memcpy(&tmp_record, file_buf + p, 4);
        if(tmp_record == 0) return 0;
//        printf("p in %u\n", p);
        if(tmp_record != DELETED){
            memcpy(&tmp_char, file_buf + p + 8, 1);
            if(tmp_char == type){
                memcpy(&length, file_buf + p + 9, 1);
                memcpy(name_buf, file_buf + p + 10, length);
                if(!strcmp(name_buf, name)) return tmp_record;
            }
        }
        memcpy(&tmp_record, file_buf + p + 4, 4);
        p = tmp_record;
//        printf("next p in %u\n", p);
    }
}
uint32_t is_null_dir(struct inode* cur){
    uint32_t tmp_record, p = 0, depth = 0;
    uint8_t length, tmp_char;
    char name_buf[257];
    bzero(file_buf, sizeof(file_buf));
    read_file(cur, file_buf);

    while(depth < 3){
        memcpy(&tmp_record, file_buf + p, 4);
        if(tmp_record == 0) break;
        memcpy(&tmp_record, file_buf + p + 4, 4);
        p = tmp_record;
        ++depth;
    }
    return depth == 2;
}
uint32_t rearrange(uint32_t size){
    char* buffer = (char*) malloc(size);
    uint32_t p_new = 0, p_ori = 0, tmp_record;
    uint8_t length;
    while(1){
        memcpy(&tmp_record, file_buf + p_ori, 4);
        if(tmp_record == 0) break;
        if(tmp_record != DELETED){
            memcpy(buffer + p_new, &tmp_record, 4);
            memcpy(buffer + p_new + 8, file_buf + p_ori + 8, 1);
            memcpy(&length, file_buf + p_ori + 9, 1);
            memcpy(buffer + p_new + 10, file_buf + p_ori + 10, length);
            tmp_record = p_new + 10 + length;
            memcpy(buffer + p_new + 4, &tmp_record, 4);
            p_new = tmp_record;
        }
        memcpy(&tmp_record, file_buf + p_ori + 4, 4);
        p_ori = tmp_record;
    }
    memcpy(file_buf, buffer, p_new);
    free(buffer);
    return p_new;

}
uint32_t delete_file(struct inode* cur, uint8_t type, char* name){
    uint32_t tmp_record, p = 0, cur_size;
    uint8_t length, tmp_char;
    char name_buf[257], inode_buf[257];
    struct inode tmp;
    bzero(file_buf, sizeof(file_buf));
    read_file(cur, file_buf);

    while(1){
        memcpy(&tmp_record, file_buf + p, 4);
        if(tmp_record == 0) return 0;
//        printf("p in %u\n", p);
        if(tmp_record != DELETED){
            memcpy(&tmp_char, file_buf + p + 8, 1);
            if(tmp_char == type){
                memcpy(&length, file_buf + p + 9, 1);
                memcpy(name_buf, file_buf + p + 10, length);
                if(!strcmp(name_buf, name)) break;
            }
        }
        memcpy(&tmp_record, file_buf + p + 4, 4);
        p = tmp_record;
//        printf("next p in %u\n", p);
    }
    read_inode(tmp_record, inode_buf);
    memcpy(&tmp, inode_buf + (tmp_record - 1) % inode_num * inode_size, inode_size);
    if(type == directory && !is_null_dir(&tmp)){  //directory must be null
        return 0;
    }

    free_inode(&tmp);
    tmp_record = DELETED;

    if(++cur->deleted_record > 5)
        cur_size = rearrange(cur->file_size);
    else cur_size = cur->file_size;
    write_file(cur, file_buf, cur_size);
    return 1;

}

void dir_init(void* cur, uint32_t father_id, char* father_name, uint32_t cur_id, char* cur_name){
    uint32_t tmp_int, p;
    uint8_t tmp_char;
    bzero(cur, 256);
    tmp_int = father_id;    //inode_id
    memcpy(cur, &tmp_int, 4);
    tmp_int = 10 + strlen(father_name);   //next
    memcpy(cur + 4, &tmp_int, 4);
    tmp_char = directory;//type
    memcpy(cur + 8, &tmp_char, 1);
    tmp_char = strlen(father_name);//length
    memcpy(cur + 9, &tmp_char, 1);
    memcpy(cur + 10, father_name, tmp_char);
    p = 10 + tmp_char;
    tmp_int = cur_id;    //inode_id
    memcpy(cur + p , &tmp_int, 4);
    tmp_int = p + 10 + strlen(cur_name);   //next
    memcpy(cur + p + 4, &tmp_int, 4);
    tmp_char = directory;//type
    memcpy(cur + p + 8, &tmp_char, 1);
    tmp_char = strlen(cur_name);//length
    memcpy(cur + p + 9, &tmp_char, 1);
    memcpy(cur + p + 10, cur_name, tmp_char);

}

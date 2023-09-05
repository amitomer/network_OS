//Copied the include statements from recitation 10 code.
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>

#define MEGABYTE 1000000

int main(int argc, char *argv[])
{
	FILE *file_send ;
	int sckt, port_num, int_N; 
	struct sockaddr_in sctin;
	uint32_t N, n_htonl, C, c_print; 
	int curr_bytes_f, all_bytes_n, all_bytes_f, bytes_written, bytes_alloc, all_bytes_c;
	char *n_buff, *c_buff, *file_content;
	
	
	if (argc !=4){
		fprintf(stderr, "Error: Invalid number of arguments");
		exit(1);
	}
	
	file_send = fopen(argv[3], "r");
	if (file_send == NULL){
		perror("Error: ");
		exit(1);
	}

	
	sckt = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sckt<0)
	{
    	fprintf(stderr, "Error: Socket creation failed");
		exit(1);
 	}
 	
 	port_num = atoi(argv[2]);
	
	//Setting attributes for our sockaddr_in.
	memset(&sctin, 0, sizeof(sctin));
	sctin.sin_family = AF_INET;
	sctin.sin_port = htons(port_num);
	if (inet_pton(AF_INET, argv[1], &sctin.sin_addr) ==0){
		fprintf(stderr, "Error: couldnt convert IP address");
		exit(1);
	}
	
	//Creating TCP connection
	if (connect(sckt,(struct sockaddr*)&sctin, sizeof(sctin)) < 0)
	{
    	fprintf(stderr, "Error: Couldn't connect");
    	exit(1);
    }
	
	//Determining the size of N to be passed.
	fseek(file_send, 0L, SEEK_END);
	N = ftell(file_send);
	
	//Sending N to server.
	all_bytes_n =0;
	n_htonl = htonl(N);
	n_buff = (char*)&n_htonl;
	int_N =(int)N;
	
	while (all_bytes_n < 4){
		bytes_written = write(sckt, n_buff+all_bytes_n, 4-all_bytes_n);
	
		if (bytes_written == -1){
			fprintf(stderr, "Error: Couldn't write N to buffer");
    		exit(1);
    	}
    	
    	all_bytes_n += bytes_written;
    }
    
	//Sending file content to server, 1MB allocation at a time.
	all_bytes_f =0;
	while (all_bytes_f < int_N){
	
		if ( int_N - all_bytes_f > MEGABYTE){
			bytes_alloc = MEGABYTE;
		}
		else{
			bytes_alloc = int_N - all_bytes_f ;
		}
	
		file_content = malloc(bytes_alloc);
		if (file_content == NULL){
			fprintf(stderr, "Error: Couldn't allocate memory for file content (client side)");
    		exit(1);
		}
		
		fseek(file_send, all_bytes_f, SEEK_SET);
		
		if (fread(file_content, 1, bytes_alloc, file_send) !=  bytes_alloc){//copying file content to the buffer(file_content).
			fprintf(stderr, "Error: Couldn't read from file");
    		exit(1);
		}
		curr_bytes_f = 0;
		
		while (curr_bytes_f < bytes_alloc){
		
			bytes_written = write(sckt, file_content+curr_bytes_f, bytes_alloc- curr_bytes_f);
	
			if (bytes_written == -1){
				fprintf(stderr, "Error: Couldn't write file content to buffer");
    			exit(1);
    		}
			
			curr_bytes_f += bytes_written;
		}
		free(file_content);
		all_bytes_f += bytes_alloc;
	}
		
	fclose(file_send);	
		
	//Receive C from the server.
	
	c_buff = (char*)&C;
	
	all_bytes_c = 0;
	
	while (all_bytes_c < 4){
		bytes_written = read(sckt, c_buff+all_bytes_c, 4-all_bytes_c);
	
		if (bytes_written == -1){
			fprintf(stderr, "Error: Couldn't read C");
    		exit(1);
    	}
    	
    	all_bytes_c += bytes_written;
    }
	close(sckt);
	c_print = ntohl(C);
	printf("# of printable characters: %u\n", c_print);
	exit(0);
}

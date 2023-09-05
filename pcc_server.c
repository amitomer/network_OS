//Copied some of the include statements from recitation 10 code.
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#define MEGABYTE 1000000

uint32_t count_arr[127];

void exit_on_sigint(){
	int j;
	for (j=32; j<127; j++){
		printf("char '%c' : %u times\n", j, count_arr[j]);
	}
	exit(0);
}

int should_exit = 0;
int con = -1;

void SIGINT_handler(){
	if (con !=-1){
		should_exit =1;
		return;
	}
	
	exit_on_sigint();
		
}

int main(int argc, char *argv[])
{

	
	struct sockaddr_in sin;
	int listen_sckt, port_num, opt_val, i, con, bytes_read, bytes_written;
	int bytes_alloc, break_connection, curr_bytes_f, c_count;
	int all_bytes_n, all_bytes_f, all_bytes_c;
	uint32_t N, n_ntohl, C;
	char *n_buff, *file_buff, *c_buff;
	//In case of an error mid process, we wouldnt want to update count_arr directly, so we'll keep curr_count instead.
	uint32_t curr_count[127];
	
	
	opt_val=1;
	listen_sckt=-1;
	
	if (argc !=2){
		fprintf(stderr, "Error: Invalid number of arguments");
		exit(1);
	}
	
	//Initializing count array.
	for (i=0; i<127; i++){
		count_arr[i] = 0;
	}
	
	port_num = atoi(argv[1]);
	
	//Setting the SIGINT handler.
	struct sigaction newAct;
	newAct.sa_handler = &SIGINT_handler;
	newAct.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &newAct, 0) < 0) {
		fprintf(stderr, "Error in setting sigint handler");
		exit(1);
	}
	
	listen_sckt = socket( AF_INET, SOCK_STREAM, 0 );
	if (listen_sckt <0){
    	fprintf(stderr, "Error: Socket creation failed");
		exit(1);
	}
	 
	//Setting attributes for our sockaddr_in.
	memset( &sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port_num);
	 
	if (setsockopt(listen_sckt, SOL_SOCKET, SO_REUSEADDR, &opt_val, 4) == -1){
	 	fprintf(stderr, "Error: Couldn't set socket options");
		exit(1);
	}
	 
	//Bind
	if (bind(listen_sckt,(struct sockaddr*) &sin, sizeof(struct sockaddr_in))== -1){
    	fprintf(stderr, "Error: failed to bind");
    	exit(1);
    }
     
    //Listen queue 
    if (listen(listen_sckt, 10) ==-1){
    	fprintf(stderr, "Error: listen failed");
    	exit(1);
    }
    
    while (1){
    	break_connection =0;
    	if (should_exit ==1){
    		exit_on_sigint();
    	}
    	// Accepting a connection.
    	con = accept(listen_sckt, NULL, NULL);
	
    	if (con < 0){
    		fprintf(stderr, "Error: accept failed");
   			exit(1);
    	}
    	
    	//Receiving N from client.
    	all_bytes_n =0;
    	n_buff = (char*)&N;
    	
    	while (all_bytes_n < 4){
			bytes_read = read(con, n_buff+all_bytes_n, 4-all_bytes_n);
		
			if (bytes_read == -1){
				break_connection =1;
				if ((errno != EPIPE) &&(errno != ECONNRESET) &&(errno != ETIMEDOUT)){
					fprintf(stderr, "Error: couldnt read N from client");
					exit(1);
				}
				break;	
    		}
    	
    		all_bytes_n += bytes_read;
    	}
    	
    	if (break_connection || all_bytes_n != 4){
    		fprintf(stderr, "Error: TCP error occurred");
    		close(con);
    		con = -1;
    		continue;
    	}
    	
    	n_ntohl = ntohl(N);
    	

		//Initializing curr_count and printable chars count.
		for (i=0; i<127; i++){
			curr_count[i] = 0;
		} 
		c_count =0;   	
		
		//Receiving file content from client.
    	all_bytes_f =0;
		
    	while (all_bytes_f< n_ntohl ){
    		if ( n_ntohl - all_bytes_f > MEGABYTE){
				bytes_alloc = MEGABYTE;
			}
			else{
				bytes_alloc = n_ntohl - all_bytes_f ;
			}
			
			file_buff = malloc(bytes_alloc);
			if (file_buff == NULL){
				fprintf(stderr, "Error: Couldn't allocate memory for file content (server side)");
    			exit(1);
			}
			
			curr_bytes_f = 0;
			
			while (curr_bytes_f < bytes_alloc){
				bytes_read = read(con, file_buff+curr_bytes_f, bytes_alloc- curr_bytes_f);
				if (bytes_read == -1){
					break_connection =1;
					if ((errno != EPIPE) &&(errno != ECONNRESET) &&(errno != ETIMEDOUT)){
						fprintf(stderr, "Error: Couldn't read file content from client");
						free(file_buff);
    					exit(1);
    				}
    				break;
    			}
    			
    			curr_bytes_f += bytes_read;
    		}
    		if (break_connection){
    			free(file_buff);
				break;
			}
			
			
    		
    		//Calculating C from current file_buff and updating curr_count array.
    		for (i=0; i<bytes_alloc; i++){
    			if ((file_buff[i] >= 32) &&(file_buff[i] <= 126)){
    				c_count += 1;
    				curr_count[(int)file_buff[i]] +=1;
    			}
    		}
    		
    		
    		free(file_buff);
			all_bytes_f += bytes_alloc;
			
			
    	}
    	if (break_connection || all_bytes_f != n_ntohl){
    		fprintf(stderr, "Error: TCP error occurred");
    		close(con);
    		con = -1;
    		continue;
    	}    	
    	
    	//Sending C to client.
    	C = htonl(c_count);
    	c_buff = (char*)&C;
    	all_bytes_c= 0;
    	
    	while (all_bytes_c < 4){
			bytes_written = write(con, c_buff+all_bytes_c, 4-all_bytes_c);
		
			if (bytes_written == -1){
				break_connection =1;
				if ((errno != EPIPE) &&(errno != ECONNRESET) &&(errno != ETIMEDOUT)){
					fprintf(stderr, "Error: Couldn't send C to client");
    				exit(1);
    			}
    			break;
    		}
    		
    		all_bytes_c += bytes_written;
    	}
    	
    	if (break_connection || all_bytes_c != 4){
    		fprintf(stderr, "Error: TCP error occurred");
    		close(con);
    		con = -1;
    		continue;
    	}      	
    	
    	//Updating count_arr using curr_count.
    	for (i=0; i<127; i++){
    		count_arr[i] += curr_count[i];
    	}
    	
    	//Disconnecting.
    	close(con);
    	con =-1;
    	
    }
    	
    close(listen_sckt);
    	
}
		
		
			
	
			

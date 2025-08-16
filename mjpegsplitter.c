// data come from stdin
// they are parsed to get the JPEG frames
// when a complete JPEG image is received, it is written to a file
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <linux/limits.h>
#include <sys/time.h>

#define BUFFER_SIZE (60 * 1000 * 1000)

typedef struct {
	int index;
	uint8_t *outputBuffer;
	ssize_t outputBufferIndex;
} parserContext_s;

static void contextInitialize(parserContext_s *context){
	context->index = 0;
	context->outputBuffer = (uint8_t *)malloc(BUFFER_SIZE);
	context->outputBufferIndex = 0;
}

static void analyze_and_forward(parserContext_s *context, const uint8_t *buffer, ssize_t length){
	ssize_t i = length;
	const uint8_t *p = buffer;
	while(i--){
		int doFlush = 0;
		uint8_t octet = *p++;
		if((0 == context->index) && (0xFF == octet)){
			context->index++;
		}else if((1 == context->index) && (0xD8 == octet)){
			context->index++;
		}else if((2 == context->index) && (0xFF == octet)){
			context->index++;
		}else if((3 == context->index) && (0xE0 == octet)){
			// printf("0xFFD8FFE0" "\n", octet);
			context->index = 0;
			doFlush = 1;
		}else{
			context->index = 0;
		}
		if(context->outputBufferIndex < BUFFER_SIZE){
			context->outputBuffer[context->outputBufferIndex++] = octet;
		}else{
			// the 'picture' doesn't fit into our buffer
			printf("discard buffer" "\n"); 
			context->outputBufferIndex = 0;
			context->index = 0;
		}
		if(doFlush){
			ssize_t lengthToFlush = context->outputBufferIndex - 4;
			if(lengthToFlush > 0){
				char filename[256];
				time_t t = time(NULL);
				struct tm *tmp = localtime(&t);
				strftime(filename, sizeof(filename) - 1, "%F_%T.JPEG", tmp); 
				int fd = open(filename, O_CREAT|O_RDWR, 0666);
				fprintf(stderr, "open(%s)=>%d" "\n", filename, fd);
				if(lengthToFlush != write(fd, context->outputBuffer, lengthToFlush)){
				}
				close(fd);
				context->outputBufferIndex = 4;
			}else{
				// printf("nothing to flush" "\n");
			}
		}
	}
}

int main(int argc, const char *argv[]){
	int in  = STDIN_FILENO;
	uint8_t *buffer = (uint8_t *)malloc(BUFFER_SIZE);
	if(buffer != NULL){
		parserContext_s context;
		contextInitialize(&context);
		for(;;){
			void updateMax(int *m, int n){
				int max = *m;
				if(n > max){
					*m = n;
				}
			}
			int max = -1;
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(STDIN_FILENO, &fds); updateMax(&max, STDIN_FILENO);
			struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
			int selected = select(max + 1, &fds, NULL, NULL, &timeout);
			if(selected > 0){
				if(FD_ISSET(STDIN_FILENO, &fds)){
					// printf("data available on stdin" "\n");
					ssize_t lus = read(in, buffer, BUFFER_SIZE);
					if(lus <= 0){
						break;
					}
					analyze_and_forward(&context, buffer, lus);
				}
				fflush(stdout);
			}
		}
		free(buffer);
	}
	return(0);
}



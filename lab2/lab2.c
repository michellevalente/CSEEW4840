#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

#define SERVER_HOST "192.168.1.1"
#define SERVER_PORT 42000
#define BUFFER_SIZE 126
#define END_CHAT 43
#define MAX_ROWS 2
#define START_EDITOR 45
#define START_CHAT 4
#define RED 2
#define GREEN 1
#define BLUE 0

/*
* Michelle Valente, Ariel Faria
* UNI: ma3360, af2791
*
*/

int sockfd; /* Socket file des	criptor */
int count_row_rec = START_CHAT;
struct libusb_device_handle *keyboard;
uint8_t endpoint_address;
pthread_t network_thread;
void *network_thread_f(void *);
char input_to_ascii(int in, int modifier);
void clearBuffer(char * buffer);
void cleanChat();
void cleanLine(int row);
int network_lock = 0;

int main()
{
  int err, col, row, i;
  int count_row_send = START_EDITOR;
	int count_col_send = 0;
	int count_col_rec =0;
	int count_cursor = 0;
  struct sockaddr_in serv_addr;
  struct usb_keyboard_packet packet;
  int transferred;
	int prevKey = 0x00;
	char messageBuffer[MAX_ROWS * BUFFER_SIZE];
	char printBuffer[2 * BUFFER_SIZE];
	int num_line = 1;

	// Clear buffer
	clearBuffer(messageBuffer);

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }
	
	// Clear screen
  for(col = 0; col < 128; col++)
  {
    for(row = 0; row < 48; row++)
    {
        fbputchar(' ', row, col, BLUE);
    }
  }

  // Separate the screen
  for (col = 0 ; col < 128 ; col++) {
    fbputchar('-', START_EDITOR - 1, col, GREEN);
  }

  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
   
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  for (;;) {

    libusb_interrupt_transfer(keyboard, endpoint_address,
                  (unsigned char *) &packet, sizeof(packet),
                  &transferred, 0);
	  if (transferred == sizeof(packet)) {

			if(packet.keycode[0] != 0x00)
			{	
				/* Condition to send message */
				if( packet.keycode[0] == 0x58 || packet.keycode[0] == 0x28)
				{
          network_lock = 1;
					write(sockfd, messageBuffer, strlen(messageBuffer));

					/* Clean line */
					cleanLine(count_row_rec);
			
					/* Show my messages on the chat screen */
					count_col_rec = 0;
					int line = strlen(messageBuffer) / 125 + 1;
      		if( strlen(messageBuffer) > 125 && (count_row_rec + line > END_CHAT ))
      		{
						count_row_rec = START_CHAT;
						cleanChat();
      		}
					for(i = 0; i < strlen(messageBuffer); i++)
					{
						fbputchar(messageBuffer[i], count_row_rec, count_col_rec, GREEN);
						count_col_rec++;
						if(i % 125 == 0 && i != 0)
						{
							count_row_rec++;
							cleanLine(count_row_rec);
							count_col_rec = 0;
						}
						if( count_row_rec >= END_CHAT)
						{	
    					count_row_rec = START_CHAT;
							cleanChat();						
						}
					}	
					
					count_row_rec++;

					// Clean editor
					for(col = 0; col < 128; col++)
					{
					  for(row = START_EDITOR; row < 48; row++)
					  {
					      fbputchar(' ', row, col, BLUE);
					  }
					}

					// Clear buffer
					count_row_send = START_EDITOR;
					count_col_send = 0;
					count_cursor = 0;
					clearBuffer(messageBuffer);
          network_lock = 0;	

				}
				
				else if (packet.keycode[0] == 0x29) { /* ESC pressed? */
		    	break;
		    }
				else
				{
					int len = strlen(messageBuffer);

					/* Backspace */
					if( packet.keycode[0] == 0x2a )	
					{
						if(count_cursor > 0)
						{
							int row = (len / 125) + START_EDITOR ;
							fbputchar(' ', row, count_col_send, BLUE);
							if(count_cursor == strlen(messageBuffer))
							{
								messageBuffer[len - 1] = '\0';
								len--;
							}else
							{
								messageBuffer[count_cursor - 1]= ' ';
							}
							count_cursor--;
						}
					}

					/* left arrow */
					else if( packet.keycode[0] == 0x50 )
					{
						int row = (len / 125) + START_EDITOR ;
						fbputchar(' ', row, count_col_send, BLUE);
						if(count_cursor > 0)
							count_cursor--;
					}

					/* right arrow */
					else if( packet.keycode[0] == 0x4f )
					{
						int row = (len / 125) + START_EDITOR ;
						fbputchar(' ', row, count_col_send, BLUE);
						if(count_cursor < strlen(messageBuffer))
							count_cursor++;
					}
				
					/* New character */
					else{
						printf("%02x %02x\n", packet.keycode[0], packet.keycode[1]);
						if(packet.keycode[0] != prevKey  && strlen(messageBuffer) < 250)
						{
							printf("%d\n", count_cursor);
							char c = input_to_ascii(packet.keycode[0], packet.modifiers);
							messageBuffer[count_cursor] = c;
							count_cursor++;
							//messageBuffer[count_cursor] = '\0';
						}
					}
				

					/* print editor */
					count_col_send = 0;
					count_row_send = START_EDITOR;
					cleanLine(count_row_send);
					num_line = 1;
					for(i = 0; i < strlen(messageBuffer); i++)
					{
						if(count_cursor == i)
						{
							fbputchar('_', count_row_send, count_col_send, GREEN);
						}
						else
						{	
							fbputchar(messageBuffer[i], count_row_send, count_col_send, GREEN);
						}
						count_col_send++;
						if(i % 125 == 0 && i != 0)
						{
							count_row_send++;	
							cleanLine(count_row_send);
							count_col_send = 0;
							num_line++;
							
						}
						if(count_row_send > 46)
						{
							count_row_send = START_EDITOR;
							cleanLine(count_row_send);
						}
						
					}	

					// keep the previous key
					prevKey = packet.keycode[0];

					// print cursor		
					if(count_cursor == strlen(messageBuffer))
					{
						int row = (len / 126);
						fbputchar('_', count_row_send, count_cursor - (126 * row), GREEN);	
		    	}
				}  	
			}
			else
			{
				prevKey = 0x00;
			}
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE * MAX_ROWS];
  int n, i;
	int count_col_rec = 0;

	/* Print the header chat */
	n = read(sockfd, &recvBuf, BUFFER_SIZE - 1);   
  recvBuf[n - 1] = '\0';
	for(i = 0; i < strlen(recvBuf); i++)
	{
		fbputchar(recvBuf[i], 1, count_col_rec,BLUE);
		count_col_rec++;
	}
	count_row_rec++;
	for(i = 0; i < 128; i++)
	{
		fbputchar('-', 2, count_col_rec,BLUE);
		count_col_rec++;
	}

	/* Receive and print messages received */
  for(;;) {
	  if(network_lock == 0){   

			/* Read the received message */
  	  n = read(sockfd, &recvBuf, BUFFER_SIZE - 1);   
   	  recvBuf[n - 1] = '\0';
			count_col_rec = 0;
                        
			cleanLine(count_row_rec);
					count_col_rec = 0;
					int line = strlen(recvBuf) / 125 + 1;
      		if( strlen(recvBuf) > 125 && (count_row_rec + line > END_CHAT ))
      		{
						count_row_rec = START_CHAT;
						cleanChat();
      		}
					for(i = 0; i < strlen(recvBuf); i++)
					{
						fbputchar(recvBuf[i], count_row_rec, count_col_rec, BLUE);
						count_col_rec++;
						if(i % 125 == 0 && i != 0)
						{
							count_row_rec++;
							cleanLine(count_row_rec);
							count_col_rec = 0;
						}
						if( count_row_rec >= END_CHAT)
						{	
    					count_row_rec = START_CHAT;
							cleanChat();						
						}
					}	
					
					count_row_rec++;
		}		
  }

  return NULL;
}	

// Translate the USB keycodes to ASCII
char input_to_ascii(int in, int modifier)
{
  char out;
 
  //lower case:
  if (modifier == 0x00){
    if (in == 0x04){out = 0x61;}
    else if (in == 0x05){out = 0x62;}
    else if (in == 0x06){out = 0x63;}
    else if (in == 0x07){out = 0x64;}
    else if (in == 0x08){out = 0x65;}
    else if (in == 0x09){out = 0x66;}
    else if (in == 0x0a){out = 0x67;}
    else if (in == 0x0b){out = 0x68;}
    else if (in == 0x0c){out = 0x69;}
    else if (in == 0x0d){out = 0x6a;}
    else if (in == 0x0e){out = 0x6b;}
    else if (in == 0x0f){out = 0x6c;}
    else if (in == 0x10){out = 0x6d;}
    else if (in == 0x11){out = 0x6e;}
    else if (in == 0x12){out = 0x6f;}
    else if (in == 0x13){out = 0x70;}
    else if (in == 0x14){out = 0x71;}
    else if (in == 0x15){out = 0x72;}
    else if (in == 0x16){out = 0x73;}
    else if (in == 0x17){out = 0x74;}
    else if (in == 0x18){out = 0x75;}
    else if (in == 0x19){out = 0x76;}
    else if (in == 0x1a){out = 0x77;}
    else if (in == 0x1b){out = 0x78;}
    else if (in == 0x1c){out = 0x79;}
    else if (in == 0x1d){out = 0x7a;}
    else if (in == 0x27){out = 0x30;}
    else if (in == 0x1e){out = 0x31;}
    else if (in == 0x1f){out = 0x32;}
    else if (in == 0x20){out = 0x33;}
    else if (in == 0x21){out = 0x34;}
		else if (in == 0x34){out = 0x27;}
    else if (in == 0x22){out = 0x35;}
    else if (in == 0x23){out = 0x36;}
    else if (in == 0x24){out = 0x37;}
    else if (in == 0x25){out = 0x38;}
    else if (in == 0x26){out = 0x39;}
    else if (in == 0x2d){out = 0x2d;}
    else if (in == 0x2e){out = 0x3d;}
    else if (in == 0x31){out = 0x5c;}
    else if (in == 0x35){out = 0x27;}
    else if (in == 0x36){out = 0x2c;}
    else if (in == 0x37){out = 0x2e;}
    else if (in == 0x38){out = 0x2f;}
		else if (in == 0x2c){out = 0x20;}
		else if (in == 0x33){out = 0x3b;}
    else if (in == 0x30){out = 0x5d;}
    else if (in == 0x2f){out = 0x5b;}
    else{out = 0x20;}

  }
  //upper case:
  else if (modifier == 0x02 || modifier == 0X20){
    if (in == 0x04){out = 0x41;}
    else if (in == 0x05){out = 0x42;}
    else if (in == 0x06){out = 0x43;}
    else if (in == 0x07){out = 0x44;}
    else if (in == 0x08){out = 0x45;}
    else if (in == 0x09){out = 0x46;}
    else if (in == 0x0a){out = 0x47;}
    else if (in == 0x0b){out = 0x48;}
    else if (in == 0x0c){out = 0x49;}
    else if (in == 0x0d){out = 0x4a;}
    else if (in == 0x0e){out = 0x4b;}
    else if (in == 0x0f){out = 0x4c;}
    else if (in == 0x10){out = 0x4d;}
    else if (in == 0x11){out = 0x4e;}
    else if (in == 0x12){out = 0x4f;}
    else if (in == 0x13){out = 0x50;}
    else if (in == 0x14){out = 0x51;}
    else if (in == 0x15){out = 0x52;}
    else if (in == 0x16){out = 0x53;}
    else if (in == 0x17){out = 0x54;}
    else if (in == 0x18){out = 0x55;}
    else if (in == 0x19){out = 0x56;}
    else if (in == 0x1a){out = 0x57;}
    else if (in == 0x1b){out = 0x58;}
    else if (in == 0x1c){out = 0x59;}
    else if (in == 0x1d){out = 0x5a;}
    else if (in == 0x27){out = 0x29;}
    else if (in == 0x1e){out = 0x21;}
    else if (in == 0x1f){out = 0x40;}
    else if (in == 0x20){out = 0x23;}
    else if (in == 0x21){out = 0x24;}
    else if (in == 0x22){out = 0x25;}
    else if (in == 0x23){out = 0x5e;}
    else if (in == 0x24){out = 0x26;}
    else if (in == 0x25){out = 0x2a;}
    else if (in == 0x26){out = 0x28;}
    else if (in == 0x2d){out = 0x5f;}
		else if (in == 0x34){out = 0x22;}
    else if (in == 0x2e){out = 0x2b;}      
    else if (in == 0x31){out = 0x7c;}
    else if (in == 0x35){out = 0x7e;}
    else if (in == 0x36){out = 0x3c;}
    else if (in == 0x37){out = 0x3e;}
    else if (in == 0x38){out = 0x3f;}
		else if (in == 0x33){out = 0x3a;}
    else if (in == 0x30){out = 0x7d;}
    else if (in == 0x2f){out = 0x7b;} 
    else{out = 0x20;}
  }

return out;
}

void clearBuffer(char * buffer)
{
	int i;
	for(i = 0; i < (MAX_ROWS * BUFFER_SIZE); i++)
	{
		buffer[i] = '\0';
	}
}

void cleanChat()
{
	int i;
        for(i = 0; i < END_CHAT + 1; i++)
        {
                cleanLine(i);
        }
}

// Clean a line in the screen
void cleanLine(int row)
{
	int j;
	for(j = 0; j < 128; j++)
	{
  	fbputchar(' ', row, j, BLUE);
  }
}

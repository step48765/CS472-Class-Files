#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <ctype.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "http.h"

//---------------------------------------------------------------------------------
// TODO:  Documentation
//
// Note that this module includes a number of helper functions to support this
// assignment.  YOU DO NOT NEED TO MODIFY ANY OF THIS CODE.  What you need to do
// is to appropriately document the socket_connect(), get_http_header_len(), and
// get_http_content_len() functions. 
//
// NOTE:  I am not looking for a line-by-line set of comments.  I am looking for 
//        a comment block at the top of each function that clearly highlights you
//        understanding about how the function works and that you researched the
//        function calls that I used.  You may (and likely should) add additional
//        comments within the function body itself highlighting key aspects of 
//        what is going on.
//
// There is also an optional extra credit activity at the end of this function. If
// you partake, you need to rewrite the body of this function with a more optimal 
// implementation. See the directions for this if you want to take on the extra
// credit. 
//--------------------------------------------------------------------------------

char *strcasestr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

char *strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == '\0' || slen-- < 1)
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}


/**
 * socket_connect function establishes a TCP connection to a specified host and port.
 *
 * It takes a hostname (host) and a port number (port) as input 
 * and then attempts to setup a TCP connection with the following steps:
 *
 * 1. Links the hostname to an IP address using gethostbyname().
 *    - If the hostname resolution fails the function returns -2.
 * 2. it then copies the IP address into a sockaddr_in structure using bcopy().
 *    - Sets the port using htons() to ensure the correct byte order.
 *    - Specifies the address family as IPv4 (AF_INET).
 * 3. Ones thats done it creates a socket with the socket() system call.
 *    - If socket creation fails it logs the error and returns a -1.
 * 4. Finally it attempts to connect to the server using connect().
 *    - If the connection fails it will log the error then closes the socket and returns -1.
 * 
 * If everything works correctly with no errors the function returns the socket descriptor. This can 
 * be used for communication with the server.
 *
 * Research Notes:
 * - gethostbyname() resolves the hostname to an IP address but is considered 
 *   deprecated. For modern code, getaddrinfo() is preferred.
 * - bcopy() is used to copy the resolved address ensuring compatibility 
 *   with the sockaddr_in structure.
 * - The socket is set up for IPv4 (AF_INET) and TCP (SOCK_STREAM).
 */


int socket_connect(const char *host, uint16_t port){
    struct hostent *hp;
    struct sockaddr_in addr;
    int sock;

    if((hp = gethostbyname(host)) == NULL){
		herror("gethostbyname");
		return -2;
	}
    
    
	bcopy(hp->h_addr_list[0], &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	sock = socket(PF_INET, SOCK_STREAM, 0); 
	
	if(sock == -1){
		perror("socket");
		return -1;
	}

    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
		perror("connect");
		close(sock);
        return -1;
	}

    return sock;
}

/**
 * get_http_header_len is meant to alculate the length of the HTTP header in the response buffer.
 *
 * This function looks over a provided buffer (http_buff) containing an HTTP response 
 *  with the goal of locating the end of the headers noted by the HTTP_HEADER_END seq 
 * ("\r\n\r\n") basically this means 2 back to back like we talked about in class. 
 * It returns the complete length of the header including the HTTP_HEADER_END 
 * sequence or -1 if the end of the header is not found.
 *
 * Steps:
 * 1. Uses strnstr() for searching for the HTTP_HEADER_END sequence within the buff.
 *    - This ensures the function only searches till the http_buff_len bytes.
 *    - If the sequence is not found the function logs the error and returns -1.
 * 2. Then it calculates the header length:
 *    - Finds the position of HTTP_HEADER_END relative to the start of the buffer 
 *      using pointer subtraction (end_ptr - http_buff).
 *    - Adds the length of the HTTP_HEADER_END sequence (strlen(HTTP_HEADER_END)) 
 *      to include it in the total length.
 * 3. Returns the calculated header length if successful.
 *
 * research Notes:
 * - The function assumes the buff contains a valid response with the HTTP_HEADER_END sequence.
 * - If the header end is not found it indicates a incomplete/ error HTTP response.
 * - strnstr() is a safe way to search within a fixed-size buffer preventing overflows.
 */


int get_http_header_len(char *http_buff, int http_buff_len){
    char *end_ptr;
    int header_len = 0;
    end_ptr = strnstr(http_buff,HTTP_HEADER_END,http_buff_len);

    if (end_ptr == NULL) {
        fprintf(stderr, "Could not find the end of the HTTP header\n");
        return -1;
    }

    header_len = (end_ptr - http_buff) + strlen(HTTP_HEADER_END);

    return header_len;
}

/**
 * the get_http_content_len extracts the Content-Length from an HTTP header buffer.
 *
 * The function searchs the HTTP header section of a response to find the Content-Length
 * header and removes its value. This specifies the size of the body content in bytes.
 * It returns the content length as an int or 0 if the Content-Length header is just not found.
 *
 * It's parameters:
 * - http_buff is the buffer containing the HTTP response.
 * - http_header_len is the length of the HTTP header section.
 *
 *  Steps:
 * 1. First it nitializes the pointers to iterate over the header lines:
 *    - next_header_line points to the current header line.
 *    - end_header_buff marks the end of the header section.
 * 2. It then iterate over each header line:
 *    - It needs to clear header_line using bzero to ensure it starts empty.
 *    - Then extracts a single line using sscanf and stores it in header_line.
 *    - Uses strcasestr to check if the line contains content-Length.
 * 3. Checks if Content-Length is found:
 *    - Uses strchr to locate the delimiter (:) separating the header name and value.
 *    - Extracts the value, converts it to an integer using atoi and returns it.
 * 4. Moves the pointer to the next line by adding the length of the current line and the line-ending sequence.
 * 5. If no Content-Length is found after scanning all headers logs a warning and returns 0.
 *
 * Research Notes:
 * - The function uses case-insensitive comparisons (strcasecmp, strcasestr) to handle HTTP header variations.
 * - atoi is used to convert the header value to an integer if invalid or missing the function gracefully returns 0.
 * - The function assumes http_buff contains valid header data up to http_header_len.
 */


int get_http_content_len(char *http_buff, int http_header_len){
    char header_line[MAX_HEADER_LINE];

    char *next_header_line = http_buff;
    char *end_header_buff = http_buff + http_header_len;

    while (next_header_line < end_header_buff){
        bzero(header_line,sizeof(header_line));
        sscanf(next_header_line,"%[^\r\n]s", header_line);

        //char *isCLHeader2 = strcasecmp(header_line,CL_HEADER);
        int isCLHeader2 = strcasecmp(header_line, CL_HEADER);
        char *isCLHeader = strcasestr(header_line,CL_HEADER);
        if(isCLHeader != NULL){
            char *header_value_start = strchr(header_line, HTTP_HEADER_DELIM);
            if (header_value_start != NULL){
                char *header_value = header_value_start + 1;
                int content_len = atoi(header_value);
                return content_len;
            }
        }
        next_header_line += strlen(header_line) + strlen(HTTP_HEADER_EOL);
    }
    fprintf(stderr,"Did not find content length\n");
    return 0;
}

//This function just prints the header, it might be helpful for your debugging
//You dont need to document this or do anything with it, its self explanitory. :-)
void print_header(char *http_buff, int http_header_len){
    fprintf(stdout, "%.*s\n",http_header_len,http_buff);
}

//--------------------------------------------------------------------------------------
//EXTRA CREDIT - 10 pts - READ BELOW
//
// Implement a function that processes the header in one pass to figure out BOTH the
// header length and the content length.  I provided an implementation below just to 
// highlight what I DONT WANT, in that we are making 2 passes over the buffer to determine
// the header and content length.
//
// To get extra credit, you must process the buffer ONCE getting both the header and content
// length.  Note that you are also free to change the function signature, or use the one I have
// that is passing both of the values back via pointers.  If you change the interface dont forget
// to change the signature in the http.h header file :-).  You also need to update client-ka.c to 
// use this function to get full extra credit. 
//--------------------------------------------------------------------------------------
int process_http_header(char *http_buff, int http_buff_len, int *header_len, int *content_len){
    int h_len, c_len = 0;
    h_len = get_http_header_len(http_buff, http_buff_len);
    if (h_len < 0) {
        *header_len = 0;
        *content_len = 0;
        return -1;
    }
    c_len = get_http_content_len(http_buff, http_buff_len);
    if (c_len < 0) {
        *header_len = 0;
        *content_len = 0;
        return -1;
    }

    *header_len = h_len;
    *content_len = c_len;
    return 0; //success
}
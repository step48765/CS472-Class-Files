#include "client.h"
#include "cs472-proto.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <getopt.h>

#define BUFF_SZ 512
static uint8_t send_buffer[BUFF_SZ];
static uint8_t recv_buffer[BUFF_SZ];

/*
 *  Helper function that processes the command line arguments.
 */
static int initParams(int argc, char *argv[], char **p) {
    int option;

    // Setup defaults if no arguments are passed
    static char cmdBuffer[16] = "CS472"; 
    int cmdType = CMD_CLASS_INFO;

    //
    // usage client [-p "ping pong message"] | [-c COURSEID]
    //
    while ((option = getopt(argc, argv, ":p:c:")) != -1) {
        switch(option) {
            case 'p':
                strncpy(cmdBuffer, optarg, sizeof(cmdBuffer));
                cmdType = CMD_PING_PONG;
                break;
            case 'c':
                strncpy(cmdBuffer, optarg, sizeof(cmdBuffer));
                cmdType = CMD_CLASS_INFO;
                break;
            case ':':
                perror("Option missing value");
                exit(-1);
            default:
            case '?':
                perror("Unknown option");
                exit(-1);
        }
    }
    *p = cmdBuffer;
    return cmdType;
}

/*
 *  This function helps to initialize the packet header we will be sending
 *  to the server from the client.
 */
static void init_header(cs472_proto_header_t *header, int req_cmd, char *reqData) {
    memset(header, 0, sizeof(cs472_proto_header_t));

    header->proto = PROTO_CS_FUN;
    header->cmd = req_cmd;

    // Setup other header fields
    header->ver = PROTO_VER_1; // Set protocol version
    header->dir = DIR_SEND;     // Set direction to send
    header->atm = TERM_FALL;    // Set academic term
    header->ay = 2022;          // Example academic year

    // Switch based on the command
    switch(req_cmd) {
        case CMD_PING_PONG:
            strncpy(header->course, "NONE", sizeof(header->course));
            // Length will be the header plus the size of the message
            header->len = sizeof(cs472_proto_header_t) + strlen(reqData) + 1; 
            break;
        case CMD_CLASS_INFO:
            strncpy(header->course, reqData, sizeof(header->course));
            header->len = sizeof(cs472_proto_header_t);
            break;
    }
}

/*
 *  This function "starts the client". 
 */
static void start_client(cs472_proto_header_t *header, uint8_t *packet) {
    struct sockaddr_in addr;
    int data_socket;
    int ret;

    /* Create local socket. */
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Clear the whole structure. */
    memset(&addr, 0, sizeof(struct sockaddr_in));

    /* Connect socket to socket address */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT_NUM);

    // Connect to the server
    if (connect(data_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    // Send the packet to the server
    
    ret = send(data_socket, packet, header->len, 0);
    if (ret == -1) {
        perror("send");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    // Receive the response from the server
    ret = recv(data_socket, recv_buffer, sizeof(recv_buffer), 0);
    if (ret == -1) {
        perror("recv");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    // Now process what the server sent
    cs472_proto_header_t *pcktPointer = (cs472_proto_header_t *)recv_buffer;
    uint8_t *msgPointer = NULL;
    uint8_t msgLen = 0;

    process_recv_packet(pcktPointer, recv_buffer, &msgPointer, &msgLen);
    
    print_proto_header(pcktPointer);
    printf("RECV FROM SERVER -> %s\n", msgPointer);

    close(data_socket);
}

int main(int argc, char *argv[]) {
    cs472_proto_header_t header;
    int cmd = 0;
    char *cmdData = NULL;

    // Process the parameters and init the header
    cmd = initParams(argc, argv, &cmdData);
    init_header(&header, cmd, cmdData);

    // Prepare the request packet based on the type of the command
    switch(cmd) {
        case CMD_CLASS_INFO:
            prepare_req_packet(&header, 0, 0, send_buffer, sizeof(send_buffer));
            break;
        case CMD_PING_PONG:
            prepare_req_packet(&header, (uint8_t *)cmdData, strlen(cmdData) + 1, 
                send_buffer, sizeof(send_buffer));
            break;
        default:
            perror("usage requires zero or one parameter");
            exit(EXIT_FAILURE);
    }

    // Start the client
    start_client(&header, send_buffer);
}

#include "server.h"
#include "cs472-proto.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>

#define BUFF_SZ 512
static uint8_t send_buffer[BUFF_SZ];
static uint8_t recv_buffer[BUFF_SZ];

/*
 *  A simple database structure for this assignment with hardcoded courses.
 *  The format is: first field is a key, the second is a value.
 */
static course_item_t course_db[] = {
    {"cs472", "CS472: Welcome to computer networks"},
    {"cs281", "CS281: Hello from computer architecture"},
    {"cs575", "CS575: Software Design is fun"},
    {"cs577", "cs577: Software architecture is important"}
};

/*
 * Helper function that returns a course item given a course_id.
 * Returns a default if no match is found.
 */
course_item_t *lookup_course_by_id(char *course_id) {
    static course_item_t NOT_FOUND_COURSE = {"NONE", "Requested Course Not Found"};
    int count = sizeof(course_db) / sizeof(course_db[0]);
    for (int i = 0; i < count; i++) {
        if (strcasecmp(course_db[i].id, course_id) == 0)
            return &course_db[i];
    }
    return &NOT_FOUND_COURSE;
}

/*
 * Processes incoming requests, continuously handling connections.
 */
static void process_requests(int listen_socket) {
    cs472_proto_header_t header;
    char msg_out_buffer[MAX_MSG_BUFFER];
    int data_socket;
    course_item_t *details;
    int ret;

    while (1) {
        memset(&header, 0, sizeof(cs472_proto_header_t));
        memset(msg_out_buffer, 0, sizeof(msg_out_buffer));

        data_socket = accept(listen_socket, NULL, NULL);
        if (data_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("\t RECEIVED REQ...\n");

        // Receive data from the client
        ret = recv(data_socket, recv_buffer, sizeof(recv_buffer), 0);
        if (ret == -1) {
            perror("recv");
            close(data_socket);
            continue;
        }
        
        // Process received packet
        cs472_proto_header_t *pcktPointer = (cs472_proto_header_t *)recv_buffer;
        printf("length: %d \n", pcktPointer->len);
        uint8_t *msgPointer = NULL;
        uint8_t msgLen = 0;
        process_recv_packet(pcktPointer, recv_buffer, &msgPointer, &msgLen);
        msgPointer[msgLen] = '\0';
        printf("length: %hhu\n", msgLen);

        memcpy(&header, pcktPointer, sizeof(cs472_proto_header_t));
        header.dir = DIR_RECV;

        switch (header.cmd) {
            case CMD_CLASS_INFO:
                details = lookup_course_by_id(header.course);

                uint8_t *payload = (uint8_t *)details->description;
                header.len = strlen(details->description);  // Set to length of course info
                
                int packet_size = sizeof(cs472_proto_header_t) + header.len;
                prepare_req_packet(&header, payload, header.len, send_buffer, sizeof(send_buffer));
                if (send(data_socket, send_buffer, packet_size, 0) == -1) {
                    perror("send");
                    close(data_socket);
                }
                break;

        case CMD_PING_PONG:
            strcpy(msg_out_buffer, "PONG: ");
            printf("the length here: %lu \n", strlen((const char *)msgPointer));
            strncat(msg_out_buffer, (char *)msgPointer, msgLen);
            for (size_t i = 0; i < msgLen; i++) {  // -1 to exclude the null terminator
                printf("Character: %c, Unsigned int value: %u\n", msgPointer[i], (uint8_t)msgPointer[i]);
            }
            printf("buffer: %lu\n", strlen(msg_out_buffer));
            header.len = strlen(msg_out_buffer) + sizeof(header);
            //header.len = strlen(msg_out_buffer) + sizeof(header)+1;
            packet_size = header.len;
            printf("Server header len: %hu, packet size: %d\n", header.len, packet_size);

            prepare_req_packet(&header, (uint8_t *)msg_out_buffer, strlen(msg_out_buffer), send_buffer, sizeof(send_buffer));
            if (send(data_socket, send_buffer, packet_size, 0) == -1) {
                perror("send");
                close(data_socket);
            }
            break;

            default:
                perror("Invalid command");
                close(data_socket);
                continue;
        }

        close(data_socket);
    }
}

/*
 * Initializes and starts the server, binding to all interfaces.
 */
static void start_server() {
    int listen_socket;
    int ret;
    struct sockaddr_in addr;

    unlink(SOCKET_NAME);

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT_NUM);

    ret = bind(listen_socket, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    ret = listen(listen_socket, 20);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    process_requests(listen_socket);
    close(listen_socket);
}

int main(int argc, char *argv[]) {
    printf("STARTING SERVER - CTRL+C to EXIT\n");
    start_server();
}

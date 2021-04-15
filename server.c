#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include "fsm.h"
#include "games/ttt.h"
#include "games/rps.h"

// Default values:
#define DEFAULT_PORT    49152
#define MAX_CLIENTS     1023
#define BUFSIZE         256
#define VERSION         1

typedef enum {
    TTT = 1,
    RPS = 2
} GAME_ID;

typedef enum {
    CONFIRMATION = 1,
    INFORMATION  = 2,
    META_ACTION  = 3,
    GAME_ACTION  = 4
} REQ_TYPE;

typedef enum {
    SUCCESS            = 10,
    UPDATE             = 20,
    INVALID_REQ        = 30,
    INVALID_UID        = 31,
    INVALID_TYPE       = 32,
    INVALID_CONTEXT    = 33,
    INVALID_PAYLOAD    = 34,
    SERVER_ERROR       = 40,
    INVALID_ACTION     = 50,
    ACTION_OUT_OF_TURN = 51
} RES_TYPE;

char* wait_msg = "Please wait for your opponent(s)!";

static int start_server(struct sockaddr_in * server_addresss, int port);
static int user_port();
static int reset_rfds(fd_set *rfds, int server_socket, int *max_fd, int *client_socket);

StateTransition ttt_transitions[] =
{
    { FSM_INIT,         VERIFY_DATA_RECV, &ttt_verify_move       },
    { VERIFY_DATA_RECV, UPDATE_BOARD,     &ttt_update_game_board },
    { VERIFY_DATA_RECV, SENT_2_CLIENTS,   &ttt_send_move_list    },
    { UPDATE_BOARD,     CHECK_GAME_STATE, &ttt_check_end_state   },
    { CHECK_GAME_STATE, SENT_2_CLIENTS,   &ttt_send_move_list    },
    { SENT_2_CLIENTS,   FSM_EXIT,         NULL               }, 
};


int main() {
    // Starting up server
    struct sockaddr_in server_address;
    int server_socket = start_server(&server_address, user_port());
    puts("[ ~ ] Server Started!\n");
    int addrlen = sizeof(server_address);

    // Empty client list
    int client_socket[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) { client_socket[i] = 0;}

    // Empty ttt game env list
    TTTGameEnv ttt_game_envs[MAX_CLIENTS/2];
    for (int i = 0; i < MAX_CLIENTS/2; i++) { ttt_game_env_init((Environment *)&ttt_game_envs[i]); }

    // Empty rps game env list
    RPSGameEnv rps_game_envs[MAX_CLIENTS/2];
    for (int i = 0; i < MAX_CLIENTS/2; i++) { rps_game_env_init((Environment *)&rps_game_envs[i]); }


    fd_set rfds;
    int max_fd, cfd, new_socket, nbytes;
    char buffer[BUFSIZE];
    int ttt_game_index;
    int rps_game_index;

    while(1) {
        puts("\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");

        reset_rfds(&rfds, server_socket, &max_fd, client_socket);

        puts("[ ~ ] selecting");
        if ((select( max_fd + 1, &rfds, NULL, NULL, NULL) < 0) && (errno!=EINTR)) {printf("select error\n");}

        // Server got new connection
        if (FD_ISSET(server_socket, &rfds)) {

            // Get the new connection
            if ((new_socket = accept(server_socket, (struct sockaddr*)&server_address, (socklen_t*)&addrlen)) < 0) {perror("accept");exit(EXIT_FAILURE);}
            
            //inform server admin of socket number - used in send and receive commands
            printf("[N] New connection , socket fd is %d , ip is : %s , port : %d \n", 
                new_socket,
                inet_ntoa(server_address.sin_addr),
                ntohs(server_address.sin_port)
            );

            // PARSE CLIENT REQUEST - to see which game they wanna play
            nbytes = read(new_socket, buffer, BUFSIZE);
            buffer[nbytes] = '\0';
            printf("[[[[ MSG FROM CLIENT]]]]::::: %s\n", buffer);


            // add new socket to smallest available index in the array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                // if position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("[ + ] adding to list of sockets at %d\n[ + ] adding to list of sockets as %d\n", i, new_socket);
                    ttt_game_index = ttt_get_game_index(i);

                    int opponent_client_number = ttt_get_opponent_client_number(i);
                    int ofd = client_socket[opponent_client_number];

                    if (ofd > 0)    
                        ttt_game_envs[ttt_game_index].p_ready = 1;
                    
                    printf("client_number %d, client_fd %d\n", i, client_socket[i]);
                    printf("p_ready %d\n", ttt_game_envs[ttt_game_index].p_ready + 1);

                    if (ttt_game_envs[ttt_game_index].p_ready == 1) {
                        ttt_game_envs[ttt_game_index].game_state = ONGOING;
                        send(client_socket[ttt_game_index*2], &ttt_game_envs[ttt_game_index].moves[0], 1, MSG_NOSIGNAL );
                        send(client_socket[ttt_game_index*2], &ttt_game_envs[ttt_game_index].game_state, 1, MSG_NOSIGNAL );
                        printf("Start msg sent to client_number %d, client_fd %d\n", ttt_game_index*2, client_socket[ttt_game_index*2]);
                    }
                    break;
                }
            }

        } else {
            // Loop through the client list and get the first one that is ready
            for (int client_number = 0; client_number < MAX_CLIENTS; client_number++) {
                cfd = client_socket[client_number];
                if (FD_ISSET(cfd, &rfds)) {
                    
                    // check if cfd is closed while reading incoming msg
                    nbytes = read(cfd, buffer, BUFSIZE);
                    buffer[nbytes] = '\0';
                    printf("[[[[ MSG FROM CLIENT]]]]::::: %s\n", buffer);

                    int opponent_client_number = ttt_get_opponent_client_number(client_number);
                    int opponent_fd = client_socket[opponent_client_number];
                    if (nbytes == 0) { // Client disconnected
                        getpeername(cfd, (struct sockaddr*)&server_address, (socklen_t*) &addrlen);
                        printf("[ X ] Client disonnected, socket fd %d,  port %d \n", cfd, ntohs(server_address.sin_port));
                        close(cfd);
                        cfd = 0;
                        client_socket[client_number] = cfd;

                        ttt_game_index = ttt_get_game_index(client_number);
                        ttt_game_envs[ttt_game_index].p_ready = 0;
                        ttt_game_envs[ttt_game_index].game_state = -1;
                        if (opponent_fd > 0) {
                            if (opponent_client_number % 2 == 0)
                                ttt_game_envs[ttt_game_index].game_state = 1;
                            else
                                ttt_game_envs[ttt_game_index].game_state = 2;
                            ttt_game_envs[ttt_game_index].cfd = cfd;
                            ttt_game_envs[ttt_game_index].ofd = opponent_fd;
                            ttt_send_move_list((Environment *)&ttt_game_envs[ttt_game_index]);
                        }
                    } 
                    else { 
                        // Here is where the logic of the game lies
                        // Process client's move
                        // Identify the client and decide which game this client belongs to and who the opponent is
                        printf("[ i ] port %d\n", ntohs(server_address.sin_port));
                        printf("[ i ] client_number: %d, cfd: %d\n", client_number, cfd);
                        printf("[ i ] Client's move received as:\n[<= ] "); fflush(stdout);
                        printf("%s\n", buffer);
                        printf("\n[ i ] opponent_client_number: %d, ", opponent_client_number);
                        
                        // Verify opponent is connected
                        if (opponent_fd <= 0) {
                            printf("is not ready!");
                            send(cfd, wait_msg, strlen(wait_msg), MSG_NOSIGNAL );
                            break;
                        }

                        printf("cfd: %d\n", opponent_fd);
                        ttt_game_index = ttt_get_game_index(client_number);

                        ttt_game_envs[ttt_game_index].cfd = cfd;
                        ttt_game_envs[ttt_game_index].ofd = opponent_fd;
                        ttt_game_envs[ttt_game_index].buffer = buffer; 

                        int start_state = FSM_INIT;
                        int end_state   = VERIFY_DATA_RECV;
                        int code        = fsm_run((Environment *)&ttt_game_envs[ttt_game_index], 
                                                   &start_state, 
                                                   &end_state, 
                                                   ttt_transitions);
                        if(code != 0){fprintf(stderr, "Cannot move from %d to %d\n", start_state, end_state); return EXIT_FAILURE;}
                        
                        break;
                    }
                    
                }
            }
        }
        puts("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    }   
    return EXIT_SUCCESS;
}

// TCP
static int user_port() {
    int port = -1;
    char str[BUFSIZE];
    while (port < 0 || port > 65535) {
        fprintf(stderr, "Enter empty field to use default port [ 49152 ]\n");
        fprintf(stderr, "Enter the port number of your choice [0 - 65536]: ");
        fgets(str, BUFSIZE, stdin);
        if (strlen(str) == 1) {
            port = DEFAULT_PORT;
            break;
        }
        sscanf(str, "%d", &port);
    }
    fprintf(stderr, "Your port is: %d \n", port);
    return port;
}

static int start_server(struct sockaddr_in * server_addresss, int port) {

    int server_socket;
    int retval;

    // define server address
    struct sockaddr_in server_address = *server_addresss;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0); if (server_socket == 0) {perror("Socket failed"); exit(EXIT_FAILURE);}
    
    int enable_reuse_sockopt = 1;
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    retval = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable_reuse_sockopt, sizeof(int)); if(retval < 0){perror("setsockopt(SO_REUSEADDR)");}

    retval = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)); if (retval < 0) {perror("bind");exit(EXIT_FAILURE);}

    retval = listen(server_socket, 3); if (retval < 0) {perror("listen");exit(EXIT_FAILURE);}

    printf("Listening on port %d \n", port);

    return server_socket;
}

// SELECT
static int reset_rfds(fd_set *rfds, int server_socket, int *max_fd, int *client_socket) {
    int cfd;
    // Reset all rfds (&rfds, server_socket, &max_fd)
    FD_ZERO(rfds);
    FD_SET(server_socket, rfds);
    // Count and set fd from client_socket array
    *max_fd = server_socket;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        cfd = client_socket[i];
        if (cfd > 0) FD_SET(cfd, rfds); // if valid socket desc add to read list
        if (cfd > *max_fd) *max_fd = cfd;  // keep track of max_fd just for the .. size?
    }
    return *max_fd;
}





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
#include "fsm/fsm.h"
#include "games/ttt.h"
#include "games/rps.h"

// Default values:
#define DEFAULT_PORT    49152
#define MAX_CLIENTS     1023
#define BUFSIZE         256
#define VERSION         1

typedef enum {
    RPS,
    TTT
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

static int start_server(struct sockaddr_in * server_addresss, int port);
static int user_port();
static int reset_rfds(fd_set *rfds, int server_socket, int *max_fd, int *client_socket);
int get_opponent_client_number(int client_number);
int get_game_index(int client_number);

StateTransition ttt_transitions[] =
{
    { FSM_INIT,         VERIFY_DATA_RECV, &ttt_verify_move       },
    { VERIFY_DATA_RECV, UPDATE_BOARD,     &ttt_update_game_board },
    { VERIFY_DATA_RECV, SENT_2_CLIENTS,   &ttt_send_move_list    },
    { UPDATE_BOARD,     CHECK_GAME_STATE, &ttt_check_end_state   },
    { CHECK_GAME_STATE, SENT_2_CLIENTS,   &ttt_send_move_list    },
    { SENT_2_CLIENTS,   FSM_EXIT,         NULL               }, 
};

static uint8_t readn(int fd, int n) {
    uint8_t byte;
    printf("request:: ");
    for (int i = 0; i < n; i++) {
        read(fd, &byte, 1);
        printf("%d ", byte);

    }
    printf("Last byte RECV:: %d\n", byte);
    return byte;
}

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
    uint8_t byte;
    int ttt_game_index;
    int rps_game_index;
    int game_type = 0;
    uint8_t first_reply[7] = {10,1,4,0,0,0,69}; // needs to change 6
    uint8_t start_game_p1[4] = {20,1,1,1};
    uint8_t start_game_p2[4] = {20,1,1,2};
    uint8_t start_game_rps[3] = {20,1,0};
    uint8_t confirm_move[3] = {10,4,0};
    uint8_t reject_move[3] = {34,4,0};
    uint8_t move_update[4] = {20,2,1,0}; // needs to change 4
    uint8_t end_update[5] = {20,3,2,3,0}; // needs to change 3 and 4
    uint8_t o_disconnect[3] = {20,4,0};

    while(1) {
        puts("\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
        reset_rfds(&rfds, server_socket, &max_fd, client_socket);
        puts("[ ~ ] selecting");
        if ((select( max_fd + 1, &rfds, NULL, NULL, NULL) < 0) && (errno!=EINTR)) {printf("select error\n");}

        // Server got new connection
        if (FD_ISSET(server_socket, &rfds)) {

            // Get the new connection
            if ((new_socket = accept(server_socket, (struct sockaddr*)&server_address, (socklen_t*)&addrlen)) < 0) {perror("accept");exit(EXIT_FAILURE);}
            printf("[N] New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

            // PARSE CLIENT REQUEST - to see which game they wanna play
            byte = readn(new_socket, 9);
            // Decide game type. 1=> odd => ttt  2=>even => rps
            game_type = byte%2;

            // add new socket to smallest available index in the array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                // if position is empty
                if (client_socket[i] == 0) {
                    if (i%2==0 && game_type == TTT) { // even slots for  => set up for ttt games
                        printf("SHOVE INTO TTT at: %d\n", i);
                        first_reply[6] = i;
                        send(new_socket, &first_reply, 7, MSG_NOSIGNAL);

                        client_socket[i] = new_socket;
                        printf("[ + ] adding to list of sockets at i= %d\n[ + ] adding to list of sockets as fd %d\n", i, new_socket);

                        ttt_game_index = get_game_index(i/2);
                        int ofd = client_socket[get_opponent_client_number(i/2)*2];

                        if (ofd > 0)    
                            ttt_game_envs[ttt_game_index].p_ready = 1;
                        
                        printf("Number of players in room %d is: %d\n", ttt_game_index, ttt_game_envs[ttt_game_index].p_ready + 1);

                        if (ttt_game_envs[ttt_game_index].p_ready == 1) {
                            ttt_game_envs[ttt_game_index].game_state = ONGOING;
                            printf("Start game msg sent to p1 with fd:: %d\n", client_socket[ttt_game_index*2*2]);
                            send(client_socket[ttt_game_index*2*2], &start_game_p1, 4, MSG_NOSIGNAL );
                            printf("Start game msg sent to p2 with fd:: %d\n", client_socket[ttt_game_index*2*2+2]);
                            send(client_socket[ttt_game_index*2*2+2], &start_game_p2, 4, MSG_NOSIGNAL );
                        }
                        break;
                    } // odd slots for rps => set up rps games
                    else if (i%2==1 && game_type == RPS) {
                        printf("SHOVE INTO RPS: %d\n", i);
                        first_reply[6] = i;
                        send(new_socket, &first_reply, 7, MSG_NOSIGNAL);

                        client_socket[i] = new_socket;
                        printf("[ + ] adding to list of sockets at %d\n[ + ] adding to list of sockets as %d\n", i, new_socket);

                        i = (i-1)/2;
                        rps_game_index = get_game_index(i);
                        int opponent_client_number = get_opponent_client_number(i);
                        int ofd = client_socket[opponent_client_number*2 + 1];

                        if (ofd > 0)
                            rps_game_envs[rps_game_index].p_ready = 1;
                        
                        printf("Number of players in room %d is: %d\n", rps_game_index, rps_game_envs[rps_game_index].p_ready + 1);

                        if (rps_game_envs[rps_game_index].p_ready == 1) {
                            rps_game_envs[rps_game_index].p1fd = client_socket[rps_game_index*2*2+1];
                            rps_game_envs[rps_game_index].p2fd = client_socket[rps_game_index*2*2+3];
                            printf("Start game msg sent to p1 with fd:: %d\n", client_socket[rps_game_index*2*2+1]);
                            send(client_socket[rps_game_index*2*2+1], &start_game_rps, 3, MSG_NOSIGNAL );
                            printf("Start game msg sent to p2 with fd:: %d\n", client_socket[rps_game_index*2*2+3]);
                            send(client_socket[rps_game_index*2*2+3], &start_game_rps, 3, MSG_NOSIGNAL );
                        }
                        break;
                    }
                }
            }

        } else { // A client is ready to be read.
            // Loop through the client list and get the first one that is ready
            for (int client_number = 0; client_number < MAX_CLIENTS; client_number++) {
                cfd = client_socket[client_number];
                if (FD_ISSET(cfd, &rfds)) {
                    if (client_number%2==0) { // TTT
                        printf("IN TICTACTOE GAME ~~~~~~~~~~~~~~~~~");
                        int opponent_client_number = get_opponent_client_number(client_number/2)*2;
                        int opponent_fd = client_socket[opponent_client_number];
                        // read incoming msg
                        byte = readn(cfd, 5);

                        printf("[[[[ MSG FROM CLIENT]]]]::::: %d\n", byte);

                        if (byte == 0) { // Client disconnected
                            getpeername(cfd, (struct sockaddr*)&server_address, (socklen_t*) &addrlen);
                            printf("[ X ] Client disonnected, socket fd %d,  port %d \n", cfd, ntohs(server_address.sin_port));
                            close(cfd);
                            cfd = 0;
                            client_socket[client_number] = cfd;

                            ttt_game_index = get_game_index(client_number);
                            ttt_game_envs[ttt_game_index].p_ready = 0;
                            ttt_game_envs[ttt_game_index].game_state = -1;
                            if (opponent_fd > 0) { // client disconnected, notify opponent
                                send(opponent_fd, &o_disconnect, 3, MSG_NOSIGNAL);
                            }
                        }
                        else { 
                            byte = readn(cfd, 3);

                            // Here is where the logic of the game lies
                            // Process client's move
                            // Identify the client and decide which game this client belongs to and who the opponent is
                            printf("[ i ] port %d\n", ntohs(server_address.sin_port));
                            printf("[ i ] client_number: %d, cfd: %d\n", client_number, cfd);
                            printf("[ i ] Client's move received as:\n[<= ] "); fflush(stdout);
                            printf("%d\n", byte);
                            printf("\n[ i ] opponent_client_number: %d, ", opponent_client_number);
                            
                            // Verify opponent is connected
                            if (opponent_fd <= 0) {
                                printf("is not ready!");
                                break;
                            }

                            printf("cfd: %d\n", opponent_fd);
                            ttt_game_index = get_game_index(client_number/2);

                            ttt_game_envs[ttt_game_index].cfd = cfd;
                            ttt_game_envs[ttt_game_index].ofd = opponent_fd;
                            ttt_game_envs[ttt_game_index].buffer = byte; 

                            int start_state = FSM_INIT;
                            int end_state   = VERIFY_DATA_RECV;
                            int code        = fsm_run((Environment *)&ttt_game_envs[ttt_game_index], 
                                                       &start_state, 
                                                       &end_state, 
                                                       ttt_transitions);
                            if(code != 0){fprintf(stderr, "Cannot move from %d to %d\n", start_state, end_state); return EXIT_FAILURE;}
                            
                            break;
                        }
                    } else {
                        printf("IN RPS GAME ~~~~~~~~~~~~~~~~~");
                        int opponent_client_number = get_opponent_client_number((client_number-1)/2)*2+1;
                        int opponent_fd = client_socket[opponent_client_number];
                        // read incoming msg
                        byte = readn(cfd, 5);

                        printf("[[[[ MSG FROM CLIENT]]]]::::: %d\n", byte);

                        if (byte == 0) { // Client disconnected
                            getpeername(cfd, (struct sockaddr*)&server_address, (socklen_t*) &addrlen);
                            printf("[ X ] Client disonnected, socket fd %d,  port %d \n", cfd, ntohs(server_address.sin_port));
                            close(cfd);
                            cfd = 0;
                            client_socket[client_number] = cfd;

                            rps_game_index = get_game_index(client_number);
                            rps_game_envs[rps_game_index].p_ready = 0;
                            if (opponent_fd > 0) { // client disconnected, notify opponent
                                send(opponent_fd, &o_disconnect, 3, MSG_NOSIGNAL);
                            }
                        }
                        else { 
                            byte = readn(cfd, 3);
                            printf("[ i ] port %d\n", ntohs(server_address.sin_port));
                            printf("[ i ] client_number: %d, cfd: %d\n", client_number, cfd);
                            printf("[ i ] Client's move received as:\n[<= ] "); fflush(stdout);
                            printf("%d\n", byte);
                            printf("\n[ i ] opponent_client_number: %d, ", opponent_client_number);

                            // Verify client's move
                            if (byte < 1 || byte > 3) {
                                printf("Client made illegal move...");
                                send(cfd, &reject_move, 3, MSG_NOSIGNAL);
                            } else {
                                // Send confirmation
                                send(cfd, &confirm_move, 3, MSG_NOSIGNAL);
                                // Identify the client and decide which game this client belongs to and who the opponent is
                                // Verify opponent is connected
                                if (opponent_fd <= 0) {
                                    printf("is not ready!");
                                    break;
                                }
                                printf("cfd: %d\n", opponent_fd);
                                rps_game_index = get_game_index(client_number/2);

                                int player = ((client_number-1)/2)%2 + 1;

                                // Update game env
                                if (player == 1) {
                                    rps_game_envs[rps_game_index].p1move = byte;
                                } else {
                                    rps_game_envs[rps_game_index].p2move = byte;
                                }

                                int p1m = rps_game_envs[rps_game_index].p1move;
                                int p2m = rps_game_envs[rps_game_index].p2move;

                                int p1fd = rps_game_envs[rps_game_index].p1fd;
                                int p2fd = rps_game_envs[rps_game_index].p2fd;

                                // Do end game check
                                if (p1m > 0 && p2m > 0) {

                                    int dif = p1m - p2m;

                                    if (dif == -1 || dif == 2) {
                                        printf("P2Won\n");
                                        end_update[3] = 2;
                                        end_update[4] = p2m; //123 WLT
                                        send(p1fd, &end_update, 5, MSG_NOSIGNAL);
                                        end_update[3] = 1;
                                        end_update[4] = p1m; //123 WLT
                                        send(p2fd, &end_update, 5, MSG_NOSIGNAL);
                                    } else if (dif == 0) {  
                                        printf("TIE\n");

                                        end_update[3] = 3;
                                        end_update[4] = p2m; //123 WLT
                                        send(p1fd, &end_update, 5, MSG_NOSIGNAL);
                                        end_update[3] = 3;
                                        end_update[4] = p1m; //123 WLT
                                        send(p2fd, &end_update, 5, MSG_NOSIGNAL);
                                    } else {
                                        printf("P1Won\n");

                                        end_update[3] = 1;
                                        end_update[4] = p2m; //123 WLT
                                        send(p1fd, &end_update, 5, MSG_NOSIGNAL);
                                        end_update[3] = 2;
                                        end_update[4] = p1m; //123 WLT
                                        send(p2fd, &end_update, 5, MSG_NOSIGNAL);
                                    }
                                    rps_game_env_init((Environment *)&rps_game_envs[rps_game_index]);
                                }
                                break;
                            }
                        }
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



// Common game list dealer:
int get_opponent_client_number(int client_number) {
    // if (client_number is even) => opponent is the odd after
    // else                       => opponent is the even before
    // 0-1   2-3   4-5   6-7 ...
    return (client_number % 2 == 0) 
            ? client_number + 1
            : client_number - 1;
}

int get_game_index(int client_number) {
    return (client_number % 2 == 0) 
            ? client_number/2
            : (client_number - 1)/2;
}

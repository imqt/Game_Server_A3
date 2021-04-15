#include "ttt.h"



// TTT CLIENT CONTROLS
void ttt_game_env_init(Environment *env) {
    TTTGameEnv *game_env;
    game_env = (TTTGameEnv *)env;
    // Initializing some game variables
    game_env->p_ready = 0;

    game_env->turn_counter = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        game_env->board[i] = BOARD_FILLER;
        game_env->moves[i] = LIST_END;
    }
    game_env->moves[9] = LIST_END;

    game_env->game_state = -1;
}

int ttt_get_opponent_client_number(int client_number) {
    // if (client_number is even) => opponent is the odd after
    // else                       => opponent is the even before
    // 0-1   2-3   4-5   6-7 ...
    return (client_number % 2 == 0) 
            ? client_number + 1
            : client_number - 1;
}

int ttt_get_game_index(int client_number) {
    return (client_number % 2 == 0) 
            ? client_number/2
            : (client_number - 1)/2;
}

// TURN FSM
int ttt_verify_move(Environment *env) {
    
    TTTGameEnv *game_env;
    game_env = (TTTGameEnv *)env;

    int move = -1;
    int sscanf_val = sscanf(game_env->buffer, "%d", &move);
    if (sscanf_val != 0) {
        if (TOP_LEFT <= move && move <= BOT_RIGHT) {
            printf("RECEIVED move: %d\n", move);
            game_env->cmove = move;
        } else {
            printf("\n%d is not a move!!!\n", move);
            return 0;
        }
    }
    if (game_env->board[game_env->cmove] != BOARD_FILLER) {
        printf("Move %d received from P%d is invalid!\n", game_env->cmove, (game_env->turn_counter%2)+1);
        return SENT_2_CLIENTS;
    }
    printf("Move %d is verified!\n", game_env->cmove);

    game_env->board[game_env->cmove] = (game_env->turn_counter%2);
    game_env->moves[game_env->turn_counter] = (uint8_t) game_env->cmove;


    return UPDATE_BOARD;
}

int ttt_update_game_board(Environment *env) {
    
    TTTGameEnv *game_env;
    game_env = (TTTGameEnv *)env;
    
    printf("Current board:");
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (i%3 == 0) {printf("\n    ");}
        game_env->board[i] == BOARD_FILLER ? printf("  %s",  "-") : printf("  %s",  game_env->board[i] ? "O" : "X");
    }
    printf("\nMoves that have been made in chronological order:\n::");
    for (int i = 0; i < MOVES_LIST_SIZE; i++) {
        if (game_env->moves[i] == LIST_END) break;
        printf("%d ",  game_env->moves[i]);
    }

    return CHECK_GAME_STATE;
}

int ttt_check_end_state(Environment *env) {
    
    TTTGameEnv *game_env;
    game_env = (TTTGameEnv *)env;
    
    printf("\n==== Checking End Game ====\n");
    // FOR NOW WE ARE JUST CHECKING IF BOARD IS FILLED

    int *board = game_env->board;
    int current_player = game_env->turn_counter%2;
    //Check Horizontal win
    int game_ended = ONGOING; //Still going
    for (int i =0 ; i < 3; i++) {
        if (board[i*3 + 0] + board[i*3 + 1] + board[i*3 + 2] == current_player*3){
        printf("P%d WON on the Horizontal!\n", current_player+1);
            game_ended = current_player+1;
        }
    }
    //Check Vertical win
    for (int i =0 ; i < 3; i++) {
        if (board[0+i]+board[3+i] + board[6+i] == current_player*3) {
        printf("P%d WON on the Vertical!\n", current_player+1);
            game_ended = current_player+1;
        }
    }
    //Check Diagonal win
    if (board[0] + board[4] + board[8] == current_player*3 
        || board[2] + board[4] + board[6] == current_player*3) {
        printf("P%d WON on the Diagonal!\n", current_player+1);
        game_ended = current_player+1;
    }
    // Check tie
    if (!game_ended && game_env->turn_counter == 8) {
        printf("BORAD IS FILLED\n IT'S A TIE!\nGAME ENDING!");
        game_ended = TIE;
    }

    game_env->game_state = game_ended;  // 0 1 2 3

    if (!game_ended) 
        game_env->turn_counter += 1;
    return SENT_2_CLIENTS;
}

int ttt_send_move_list(Environment *env) {
    
    TTTGameEnv *game_env;
    game_env = (TTTGameEnv *)env;

    int cfd = game_env->cfd;
    int ofd = game_env->ofd;

    if (game_env->turn_counter == 0 && game_env->game_state != 0) {
        puts("Game ended before any move has been registered");
        game_env->moves[0] = 4;
    }
    puts("Data sent:");
    for (int i = 0; i < MOVES_LIST_SIZE; i++) {
        if (cfd > 0)
            send(cfd, &game_env->moves[i], 1, MSG_NOSIGNAL );
        if (ofd > 0)
            send(ofd, &game_env->moves[i], 1, MSG_NOSIGNAL );
        
        printf("%d ",  game_env->moves[i]);
        if (game_env->moves[i] == LIST_END) {
            if (cfd > 0) {
                send(cfd, &game_env->game_state, 1, MSG_NOSIGNAL );
                printf("socket fd %d\n", cfd);
            }
            if (ofd > 0) {
                send(ofd, &game_env->game_state, 1, MSG_NOSIGNAL );
                printf("socket fd %d\n", ofd);
            }
            if (game_env->game_state)
                ttt_game_env_init((Environment *)game_env);
            break;
        }
    }
    printf("\n");
    return FSM_EXIT;
}
#include "ttt.h"

uint8_t confirm_move[3] = {10,4,0};
uint8_t reject_move[3] = {34,4,0};
uint8_t move_update[4] = {20,2,1,0}; // needs to change 4
uint8_t end_update[5] = {20,3,2,3,0}; // needs to change 3 and 4

// TTT CLIENT CONTROLS
void ttt_game_env_init(Environment *env) {
    TTTGameEnv *game_env;
    game_env = (TTTGameEnv *)env;
    // Initializing some game variables
    game_env->p_ready = 0;
    game_env->p1fd = 0;
    game_env->p2fd = 0;

    game_env->turn_counter = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        game_env->board[i] = BOARD_FILLER;
        game_env->moves[i] = LIST_END;
    }
    game_env->moves[9] = LIST_END;

    game_env->game_state = -1;
}

// TURN FSM
int ttt_verify_move(Environment *env) {
    
    TTTGameEnv *game_env;
    game_env = (TTTGameEnv *)env;

    int move = game_env->buffer;
    if (TOP_LEFT <= move && move <= BOT_RIGHT) {
        printf("RECEIVED move: %d\n", move);
        game_env->cmove = move;
    } else {
        printf("\n%d is not a move!!!\n", move);
        return 0;
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
    if (cfd > 0) {
        send(cfd, &confirm_move, 3, MSG_NOSIGNAL);
        printf("Sent cofirmation.\n");
    }
    if (game_env->turn_counter == 0 && game_env->game_state != 0) {
        puts("Game ended before any move has been registered");
        game_env->buffer = 4;
    }
    int p1fd = game_env->p1fd;
    int p2fd = game_env->p2fd;
    if (game_env->game_state == 1) { // P1 Won
        printf("P1WON\n");
        end_update[3] = 1;
        end_update[4] = game_env->buffer; //123 WLT
        send(p1fd, &end_update, 5, MSG_NOSIGNAL);
        end_update[3] = 2;
        end_update[4] = game_env->buffer; //123 WLT
        send(p2fd, &end_update, 5, MSG_NOSIGNAL);
    } else if (game_env->game_state == 2) { // P2 Won
        printf("P2WON\n");
        end_update[3] = 2;
        end_update[4] = game_env->buffer; //123 WLT
        send(p1fd, &end_update, 5, MSG_NOSIGNAL);
        end_update[3] = 1;
        end_update[4] = game_env->buffer; //123 WLT
        send(p2fd, &end_update, 5, MSG_NOSIGNAL);
    } else if (game_env->game_state == 3) { // Tie
        printf("TIE\n");
        end_update[3] = 3;
        end_update[4] = game_env->buffer; //123 WLT
        send(p1fd, &end_update, 5, MSG_NOSIGNAL);
        end_update[3] = 3;
        end_update[4] = game_env->buffer; //123 WLT
        send(p2fd, &end_update, 5, MSG_NOSIGNAL);
    }

    if (game_env->game_state) {
        ttt_game_env_init((Environment *)game_env);
        return FSM_EXIT;
    }

    move_update[3] = game_env->buffer;
    if (ofd > 0)
        send(ofd, &move_update, 4, MSG_NOSIGNAL);


    printf("\n");
    return FSM_EXIT;
}
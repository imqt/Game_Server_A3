#ifndef TTT_GAME_H
#define TTT_GAME_H

#include <stdio.h>
#include <netinet/in.h>
#include "../fsm/fsm.h"

#define BOARD_SIZE      9
#define MOVES_LIST_SIZE 10
#define BOARD_FILLER    5
#define LIST_END        9
#define TOP_LEFT        0   
#define BOT_RIGHT       8

// Game environment variables
typedef struct {
    int p_ready;
    int cmove;
    uint8_t moves[MOVES_LIST_SIZE];
    int board[BOARD_SIZE];
    int turn_counter;
    int game_state; // 0 still going, 1 p1 won, 2 p2 won, 3 tie game
    uint8_t buffer;
    int cfd;
    int ofd;
} TTTGameEnv;

int ttt_verify_move(Environment *env);
int ttt_update_game_board(Environment *env);
int ttt_check_end_state(Environment *env);
int ttt_send_move_list(Environment *env);
void ttt_game_env_init(Environment *env);

typedef enum {
    ONGOING,
    P1_WON,
    P2_WON,
    TIE
} TTT_Game_states;

typedef enum {
    VERIFY_DATA_RECV = FSM_APP_STATE_START,
    UPDATE_BOARD,
    CHECK_GAME_STATE,
    SENT_2_CLIENTS,                         
} States;

#endif

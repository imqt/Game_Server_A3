#ifndef RPS_GAME_H
#define RPS_GAME_H

#include <stdio.h>
#include <netinet/in.h>
#include "../fsm.h"

typedef enum {
    ROCK     = 1,
    PAPER    = 2,
    SCISSORS = 3
} RPS_MOVES;

// Game environment variables
typedef struct {
    int p_ready;
    int game_state; // 0 still going, 1 p1 won, 2 p2 won, 3 tie game
    int p1move;     // 1, 2, 3
    int p2move;     // 1, 2, 3
    char * buffer;
    int cfd;
    int ofd;
} RPSGameEnv;



void rps_game_env_init(Environment *env);


#endif

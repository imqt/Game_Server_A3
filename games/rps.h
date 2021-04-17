#ifndef RPS_GAME_H
#define RPS_GAME_H

#include <stdio.h>
#include <netinet/in.h>
#include "../fsm/fsm.h"

typedef enum {
    ROCK     = 1,
    PAPER    = 2,
    SCISSORS = 3
} RPS_MOVES;

// Game environment variables
typedef struct {
    int p_ready;
    int p1move;     // 1, 2, 3
    int p2move;     // 1, 2, 3
    int p1fd;     // 1, 2, 3
    int p2fd;     // 1, 2, 3
} RPSGameEnv;

void rps_game_env_init(Environment *env);

#endif

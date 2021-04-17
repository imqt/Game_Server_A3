#include "rps.h"

// RPS CLIENT CONTROLS
void rps_game_env_init(Environment *env) {
    RPSGameEnv *game_env;
    game_env = (RPSGameEnv *)env;

    // Initializing some game variables
    game_env->p_ready = 0;
    game_env->p1move = -1;
    game_env->p2move = -1;
    game_env->p1fd = -1;
    game_env->p2fd = -1;
}
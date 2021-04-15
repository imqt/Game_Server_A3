#ifndef DC_FSM_H
#define DC_FSM_H

typedef enum {
    FSM_IGNORE = -1,     // -1
    FSM_INIT,		     //  0
    FSM_EXIT,			 //  1
    FSM_APP_STATE_START, //  2
} FSMStates;

typedef struct {
    int from_state_id;
    int current_state_id;
} Environment;


// param:  env: Environment pointer
// return: int: id of next state to go to
typedef int (*state_func)(Environment *env);

typedef struct {
    int from_id;
    int to_id;
    state_func perform;
} StateTransition;

int fsm_run(Environment *env, 
            int *from_id, 
            int *to_id, 
            const StateTransition transitions[]);

#endif // DC_FSM_H

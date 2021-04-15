#include "fsm.h"
#include <stdio.h>

static state_func fsm_transition(int from_id, 
                                 int to_id, 
                                 const StateTransition transitions[]) {

    // printf("\n==fsm_transition start\n");
    const StateTransition *transition;
    transition = &transitions[0];
    int index = 0;
    while(transition->from_id != FSM_IGNORE) {

        // printf("====fsm_transition while transition: %d\n", index);
        // printf("====fsm_transition from_id => to_id: %d => %d\n", transition->from_id, transition->to_id);
        if(transition->from_id == from_id &&
           transition->  to_id ==   to_id) {
            // printf("fsm_transition returned to_id: %d\n", transition->to_id);
            return transition->perform;
        }

        transition = transitions++;
        index++;
    }
    // printf("==fsm_transition end\n");

    return NULL;
}

int fsm_run(Environment *env, 
            int *from_state_id, 
            int *to_state_id, 
            const StateTransition transitions[]) {
    
    int from_id = *from_state_id;
    int to_id = *to_state_id;

    printf("Before do{}while loop\n");

    do {
        state_func perform;
        perform = fsm_transition(from_id, to_id, transitions);
        if(perform == NULL) {
            *from_state_id = from_id;
            *to_state_id   = to_id;
            return -1;
        }
        env->from_state_id     = from_id;
        env->current_state_id  = to_id;
        from_id                = to_id;
        to_id                  = perform(env);
    }
    while(to_id != FSM_EXIT);


    printf("After do{}while loop\n");
    printf("SERVER EXITED\n");

    *from_state_id = from_id;
    *to_state_id   = to_id;

    return 0;
}


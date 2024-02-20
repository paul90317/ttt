#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "agents/rl_agent.h"
#include "agents/util.h"
#include "game.h"

#define LEARNING_RATE 0.02
#define GAMMA 1
#define NUM_ITERATION 10000

static unsigned int N_STATES = 1;
static rl_agent_t agent[2];

static void init_mc_agent(rl_agent_t *agent,
                          unsigned int state_num,
                          char player)
{
    init_rl_agent(agent, state_num, player);
    for (unsigned int i = 0; i < state_num; i++)
        agent->state_value[i] = get_score(hash_to_table(i), player) * 0.0001;
}

static void init_training()
{
    srand(time(NULL));
    CALC_STATE_NUM(N_STATES);
    init_mc_agent(&agent[0], N_STATES, 'O');
    init_mc_agent(&agent[1], N_STATES, 'X');
}

static float update_state_value(int cur_state_hash,
                                float gain,
                                rl_agent_t *agent)
{
    agent->state_value[cur_state_hash] =
        (1 - LEARNING_RATE) * agent->state_value[cur_state_hash] +
        LEARNING_RATE * gain;
    return -gain * GAMMA;
}

static void train(int iter)
{
    int episode_moves[N_GRIDS];  // from 0 moves to N_GRIDS moves.
    int episode_len = 0;
    char table[N_GRIDS];
    memset(table, ' ', N_GRIDS);
    int turn = (iter & 1) ? 0 : 1;  // 0 for 'O', 1 for 'X'
    char win = ' ';
    while (1) {
        char win = check_win(table);
        if (win == 'D') {
            draw_board(table);
            printf("It is a draw!\n");
            break;
        } else if (win != ' ') {
            draw_board(table);
            printf("%c won!\n", win);
            break;
        }
        int move = get_action_exploit(table, &agent[turn]);
        table[move] = "OX"[turn];
        episode_moves[episode_len++] = table_to_hash(table);
        draw_board(table);
        turn = !turn;
    }
    float gain = win == 'D' ? 0 : 1;
    turn = !turn;  // the player who makes last move will win or draw.
    for (int i_move = episode_len - 1; i_move >= 0; --i_move) {
        gain = update_state_value(episode_moves[i_move], gain, &agent[turn]);
    }
}

int main()
{
    init_training();
    for (unsigned int i = 0; i < NUM_ITERATION; i++) {
        train(i);
    }
    store_state_value(agent, N_STATES);
    free(agent[0].state_value);
    free(agent[1].state_value);
}

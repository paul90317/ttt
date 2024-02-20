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
#define GAMMA 0.99
#define NUM_ITERATION 10000
#define EPSILON_START 0.5
#define EPSILON_END 0.001
static double epsilon = EPSILON_START;
static double decay_factor;
static unsigned int N_STATES = 1;
static rl_agent_t agent[2];
#define RAND_UNIFORM ((float) rand() / (float) RAND_MAX)

static void init_td_agent(rl_agent_t *agent,
                          unsigned int state_num,
                          char player)
{
    init_rl_agent(agent, state_num, player);
    for (unsigned int i = 0; i < state_num; i++)
        agent->state_value[i] = get_score(hash_to_table(i), player);
}

static void init_training()
{
    srand(time(NULL));
    CALC_STATE_NUM(N_STATES);
    init_td_agent(&agent[0], N_STATES, 'O');
    init_td_agent(&agent[1], N_STATES, 'X');
    decay_factor =
        pow((EPSILON_END / EPSILON_START), (1.0 / (NUM_ITERATION * N_GRIDS)));
}

static int *get_available_moves(char *table, int *ret_size)
{
    int move_cnt = 0;
    int *available_moves = malloc(N_GRIDS * sizeof(int));
    if (!available_moves) {
        perror("Failed to allocate memory");
        exit(1);
    }
    for_each_empty_grid (i, table) {
        available_moves[move_cnt++] = i;
    }
    *ret_size = move_cnt;
    return available_moves;
}

static int get_action_epsilon_greedy(char *table, rl_agent_t *agent)
{
    int move_cnt = 0;
    int *available_moves = get_available_moves(table, &move_cnt);
    if (RAND_UNIFORM < epsilon) {  // explore
        printf("explore %d\n", available_moves[rand() % move_cnt]);
        return available_moves[rand() % move_cnt];
    }
    int act = get_action_exploit(table, agent);
    epsilon *= decay_factor;
    return act;
}

static void update_state_value(char *cur_state,
                               char *nxt_state,
                               rl_agent_t *agent)
{
    int cur_state_hash = table_to_hash(cur_state);
    int nxt_state_hash = table_to_hash(nxt_state);
    int nxt_reward = get_score(hash_to_table(nxt_state_hash), agent->player);
    agent->state_value[cur_state_hash] =
        (1 - LEARNING_RATE) * agent->state_value[cur_state_hash] +
        LEARNING_RATE *
            (nxt_reward + GAMMA * agent->state_value[nxt_state_hash]);
}

static void train(int iter)
{
    char table[N_GRIDS], nxt_table[N_GRIDS];
    memset(table, ' ', N_GRIDS);
    memset(nxt_table, ' ', N_GRIDS);
    int turn = (iter & 1) ? 0 : 1;  // 0 for 'O', 1 for 'X'
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
        int move = get_action_epsilon_greedy(table, &agent[turn]);
        nxt_table[move] = "OX"[turn];
        update_state_value(table, nxt_table, &agent[0]);
        update_state_value(table, nxt_table, &agent[1]);
        table[move] = "OX"[turn];
        draw_board(table);
        turn = !turn;
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

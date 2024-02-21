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

// for training
#define INITIAL_MUTIPLIER 0.0001
#define LEARNING_RATE 0.02
#define NUM_EPISODE 10000
#define EPSILON_GREEDY 0
#define MONTE_CARLO 1

// for Markov decision model
#define GAMMA 1
#define EPISODE_REWARD 1

// for epsilon greedy
#define EPSILON_START 0.5
#define EPSILON_END 0.001

#define RAND_UNIFORM ((float) rand() / (float) RAND_MAX)

#if EPSILON_GREEDY
static double epsilon = EPSILON_START;
static double decay_factor;
#endif

static unsigned int N_STATES = 1;
static rl_agent_t agent[2];

_Static_assert(INITIAL_MUTIPLIER >= 0,
               "Initial mutiplier must not be less than 0");
_Static_assert(LEARNING_RATE > 0, "Learning rate must be greater than 0");
_Static_assert(NUM_EPISODE > 0,
               "The number of episodes must be greater than 0.");
_Static_assert(EPSILON_GREEDY == 0 || EPSILON_GREEDY == 1,
               "EPSILON_GREEDY must be a boolean that is 0 or 1");
_Static_assert(MONTE_CARLO == 0 || MONTE_CARLO == 1,
               "MONTE_CARLO must be a boolean that is 0 or 1");
_Static_assert(GAMMA >= 0, "Gamma must not be less than 0");
_Static_assert(EPISODE_REWARD == 0 || EPISODE_REWARD == 1,
               "EPISODE_REWARD must be a boolean that is 0 or 1");
_Static_assert(EPSILON_END >= 0, "EPSILON_END must not be less than 0");
_Static_assert(EPSILON_START >= EPSILON_END,
               "EPSILON_START must not be less than EPSILON_END");


static void init_agent(rl_agent_t *agent, unsigned int state_num, char player)
{
    init_rl_agent(agent, state_num, player);
    for (unsigned int i = 0; i < state_num; i++)
        agent->state_value[i] =
            get_score(hash_to_table(i), player) * INITIAL_MUTIPLIER;
}

static void init_training()
{
    srand(time(NULL));
    CALC_STATE_NUM(N_STATES);
    init_agent(&agent[0], N_STATES, 'O');
    init_agent(&agent[1], N_STATES, 'X');
#if EPSILON_GREEDY
    decay_factor =
        pow((EPSILON_END / EPSILON_START), (1.0 / (NUM_EPISODE * N_GRIDS)));
#endif
}

#if EPSILON_GREEDY
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
#endif

static float update_state_value(int after_state_hash,
                                float next_target,
                                rl_agent_t *agent)
{
#if EPISODE_REWARD
    float target = -GAMMA * next_target;
#else
    float target =
        -GAMMA * next_target + get_score(after_state_hash, agent->player);
#endif
    agent->state_value[after_state_hash] =
        (1 - LEARNING_RATE) * agent->state_value[after_state_hash] +
        LEARNING_RATE * target;
#if MONTE_CARLO
    return target;
#else
    return agent->state_value[after_state_hash];
#endif
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
#if EPSILON_GREEDY
        int move = get_action_epsilon_greedy(table, &agent[turn]);
#else
        int move = get_action_exploit(table, &agent[turn]);
#endif
        table[move] = "OX"[turn];
        episode_moves[episode_len++] = table_to_hash(table);
        draw_board(table);
        turn = !turn;
    }
    turn = !turn;  // the player who makes the last move.
#if EPISODE_REWARD
    float next_target =
        win == 'D' ? 0 : -1;  // because the player who makes the last move will
                              // win or draw, the next player will lose or draw.
#else
    float next_target = 0;
#endif
    for (int i_move = episode_len - 1; i_move >= 0; --i_move) {
        next_target = update_state_value(episode_moves[i_move], next_target,
                                         &agent[turn]);
    }
}

int main()
{
    init_training();
    for (unsigned int i = 0; i < NUM_EPISODE; i++) {
        train(i);
    }
    store_state_value(agent, N_STATES);
    free(agent[0].state_value);
    free(agent[1].state_value);
}

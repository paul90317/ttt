PROG = ttt
CFLAGS := -Wall -Wextra -std=c11
CFLAGS += -I. -MMD
LDFLAGS :=
TRAIN_TD = train_td
TRAIN_MC = train_mc
RL = rl
MCTS = mcts
RL_CFLAGS := $(CFLAGS) -D USE_RL
MCTS_CFLAGS := $(CFLAGS) -D USE_MCTS
MCTS_LDFLAGS := $(LDFLAGS) -lm

GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS) $(PROG)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

OBJS := \
	game.o \
	mt19937-64.o \
	zobrist.o \
	agents/negamax.o \
	main.o
deps := $(OBJS:%.o=%.d)
deps += $(RL).d
deps += $(TRAIN_TD).d
deps += $(TRAIN_MC).d
deps += $(MCTS).d

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(RL): main.c agents/rl_agent.c game.c
	$(CC) -o $@ $^ $(RL_CFLAGS)

$(TRAIN_TD): $(TRAIN_TD).c agents/rl_agent.c game.c
	$(CC) $(CFLAGS) -o $@ $^

$(TRAIN_MC): $(TRAIN_MC).c agents/rl_agent.c game.c
	$(CC) $(CFLAGS) -o $@ $^

$(MCTS): main.c agents/mcts.c game.c
	$(CC) -o $@ $^ $(MCTS_CFLAGS) $(MCTS_LDFLAGS)

clean:
	-$(RM) $(PROG) $(OBJS) $(deps) $(TRAIN_TD) $(TRAIN_MC) $(RL) $(MCTS)
	-$(RM) *.bin

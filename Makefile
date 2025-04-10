CC = gcc
CFLAGS = -Wall -Wextra -O2

AGENT_SRC = agent.c
ORCH_SRC = orchestrator.c

AGENT_BIN = agent
ORCH_BIN = orchestrator

all: $(AGENT_BIN) $(ORCH_BIN)

$(AGENT_BIN): $(AGENT_SRC)
	$(CC) $(CFLAGS) -o $(AGENT_BIN) $(AGENT_SRC)

$(ORCH_BIN): $(ORCH_SRC)
	$(CC) $(CFLAGS) -o $(ORCH_BIN) $(ORCH_SRC)

clean:
	rm -f $(AGENT_BIN) $(ORCH_BIN)

run-agent:
	./$(AGENT_BIN)

run-orchestrator:
	./$(ORCH_BIN)

.PHONY: all clean run-agent run-orchestrator


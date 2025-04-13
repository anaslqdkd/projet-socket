CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lssl -lcrypto

AGENT_SRC = src/agent.c
ORCH_SRC = src/orchestrator.c

AGENT_BIN = agent
ORCH_BIN = orchestrator

# Include the OpenSSL headers and link against the libraries
OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl)
OPENSSL_LIBS = $(shell pkg-config --libs openssl)

all: $(AGENT_BIN) $(ORCH_BIN)

$(AGENT_BIN): $(AGENT_SRC)
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -o $(AGENT_BIN) $(AGENT_SRC) $(OPENSSL_LIBS)

$(ORCH_BIN): $(ORCH_SRC)
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -o $(ORCH_BIN) $(ORCH_SRC) $(OPENSSL_LIBS)

clean:
	rm -f $(AGENT_BIN) $(ORCH_BIN)

run-agent:
	./$(AGENT_BIN)

run-orchestrator:
	./$(ORCH_BIN)

.PHONY: all clean run-agent run-orchestrator

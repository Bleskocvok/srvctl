
CC = $(CXX)

INCLUDE = ./
CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE)

CON_SRC = src/srvctl.cpp src/commands.cpp src/signames.cpp
DAE_SRC = src/daemon.cpp src/commands.cpp src/signames.cpp

CON = srvctl
DAE = srvd

CON_OBJ = $(patsubst src/%,obj/%, $(patsubst %.cpp,%.o,$(CON_SRC)))
DAE_OBJ = $(patsubst src/%,obj/%, $(patsubst %.cpp,%.o,$(DAE_SRC)))

BIN_DIR ?= /usr/local/bin
CONFIG = ~/.srvctl/.apps.json


all: $(CON) $(DAE)


install: all
	sudo install -D -m 755 ./$(CON) ./$(DAE) $(BIN_DIR)
	install -D -m 644 $(CONFIG)

uninstall:
	sudo rm -f $(BIN_DIR)/$(DAE) $(BIN_DIR)/$(CON)
	rm -rf ~/.srvctl/


$(CON): $(CON_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

$(DAE): $(DAE_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


DEPEND = $(DAE_OBJ:.o=.d) $(CON_OBJ:.o=.d)

%.o: CXXFLAGS += -MMD -MP

-include $(DEPEND)


clean:
	$(RM) $(DAE_OBJ) $(CON_OBJ) $(DEPEND)

distclean: clean
	$(RM) $(CON) $(DAE)

.PHONY: clean distclean install uninstall


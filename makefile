PATH_BIN = bin
PATH_LIB = lib
PATH_OBJ = obj

PATH_LIGHTRPC = lightrpc
PATH_COMM = $(PATH_LIGHTRPC)/common
PATH_NET = $(PATH_LIGHTRPC)/net
PATH_TCP = $(PATH_LIGHTRPC)/net/tcp
PATH_TINYPB = $(PATH_LIGHTRPC)/net/tinypb
PATH_HTTP = $(PATH_LIGHTRPC)/net/http
PATH_RPC = $(PATH_LIGHTRPC)/net/rpc

PATH_TESTCASES = testcases

# will install lib to /usr/lib/liblightrpc.a
PATH_INSTALL_LIB_ROOT = /usr/lib

# will install all header file to /usr/include/lightrpc
PATH_INSTALL_INC_ROOT = /usr/include

PATH_INSTALL_INC_COMM = $(PATH_INSTALL_INC_ROOT)/$(PATH_COMM)
PATH_INSTALL_INC_NET = $(PATH_INSTALL_INC_ROOT)/$(PATH_NET)
PATH_INSTALL_INC_TCP = $(PATH_INSTALL_INC_ROOT)/$(PATH_TCP)
PATH_INSTALL_INC_TINYPB = $(PATH_INSTALL_INC_ROOT)/$(PATH_TINYPB)
PATH_INSTALL_INC_HTTP = $(PATH_INSTALL_INC_ROOT)/$(PATH_HTTP)
PATH_INSTALL_INC_RPC = $(PATH_INSTALL_INC_ROOT)/$(PATH_RPC)


CXX := g++
CXXFLAGS += -g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable
CXXFLAGS += -I./ -I$(PATH_LIGHTRPC)	-I$(PATH_COMM) -I$(PATH_NET) -I$(PATH_TCP) -I$(PATH_TINYPB) -I$(PATH_HTTP) -I$(PATH_RPC)
LIBS += /usr/local/lib/libprotobuf.a /usr/lib/libtinyxml.a /usr/local/lib/libzookeeper_mt.a 

COMM_OBJ := $(patsubst $(PATH_COMM)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_COMM)/*.cc))
NET_OBJ := $(patsubst $(PATH_NET)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_NET)/*.cc))
TCP_OBJ := $(patsubst $(PATH_TCP)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_TCP)/*.cc))
TINYPB_OBJ := $(patsubst $(PATH_TINYPB)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_TINYPB)/*.cc))
HTTP_OBJ := $(patsubst $(PATH_HTTP)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_HTTP)/*.cc))
RPC_OBJ := $(patsubst $(PATH_RPC)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_RPC)/*.cc))

ALL_TESTS : $(PATH_BIN)/test_rpc_client $(PATH_BIN)/test_rpc_server

TEST_CASE_OUT := $(PATH_BIN)/test_rpc_client $(PATH_BIN)/test_rpc_server

LIB_OUT := $(PATH_LIB)/liblightrpc.a

$(PATH_BIN)/test_rpc_client: $(LIB_OUT)
	$(CXX) -o $@ $(PATH_TESTCASES)/test_rpc_client.cc $(PATH_TESTCASES)/order.pb.cc $(LIB_OUT) $(CXXFLAGS) $(LIBS) -ldl -pthread

$(PATH_BIN)/test_rpc_server: $(LIB_OUT)
	$(CXX) -o $@ $(PATH_TESTCASES)/test_rpc_server.cc $(PATH_TESTCASES)/order.pb.cc $(LIB_OUT) $(CXXFLAGS) $(LIBS) -ldl -pthread

$(LIB_OUT): $(COMM_OBJ) $(NET_OBJ) $(TCP_OBJ) $(TINYPB_OBJ) $(HTTP_OBJ) $(RPC_OBJ)
	cd $(PATH_OBJ) && ar rcv liblightrpc.a *.o && cp liblightrpc.a ../lib/

$(PATH_OBJ)/%.o : $(PATH_COMM)/%.cc
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(PATH_OBJ)/%.o : $(PATH_NET)/%.cc
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(PATH_OBJ)/%.o : $(PATH_TCP)/%.cc
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(PATH_OBJ)/%.o : $(PATH_TINYPB)/%.cc
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(PATH_OBJ)/%.o : $(PATH_HTTP)/%.cc
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(PATH_OBJ)/%.o : $(PATH_RPC)/%.cc
	$(CXX) -o $@ $(CXXFLAGS) -c $<

# print something test
# like this: make PRINT-PATH_BIN, and then will print variable PATH_BIN
PRINT-% : ; @echo $* = $($*)


# to clean 
clean :
	rm -f $(COMM_OBJ) $(NET_OBJ) $(TESTCASES) $(TEST_CASE_OUT) $(PATH_LIB)/liblightrpc.a $(PATH_OBJ)/liblightrpc.a $(PATH_OBJ)/*.o

# install
install:
	mkdir -p $(PATH_INSTALL_INC_COMM) $(PATH_INSTALL_INC_NET) $(PATH_INSTALL_INC_TCP) $(PATH_INSTALL_INC_TINYPB) $(PATH_INSTALL_INC_RPC)\
		&& cp $(PATH_COMM)/*.h $(PATH_INSTALL_INC_COMM) \
		&& cp $(PATH_NET)/*.h $(PATH_INSTALL_INC_NET) \
		&& cp $(PATH_TCP)/*.h $(PATH_INSTALL_INC_TCP) \
		&& cp $(PATH_TINYPB)/*.h $(PATH_INSTALL_INC_TINYPB) \
		&& cp $(PATH_HTTP)/*.h $(PATH_INSTALL_INC_HTTP) \
		&& cp $(PATH_RPC)/*.h $(PATH_INSTALL_INC_RPC) \
		&& cp $(LIB_OUT) $(PATH_INSTALL_LIB_ROOT)/


# uninstall
uninstall:
	rm -rf $(PATH_INSTALL_INC_ROOT)/lightrpc && rm -f $(PATH_INSTALL_LIB_ROOT)/liblightrpc.a
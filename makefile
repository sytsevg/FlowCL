BIN_PATH         := linuxbin/
SRC_PATH         := src/
CPP_FILES        := $(wildcard $(SRC_PATH)*.cpp)
OBJ_FILES        := $(addprefix $(BIN_PATH),$(notdir $(CPP_FILES:.cpp=.o)))
OPENCL_LIB       := -L$(AMDAPPSDKROOT)/lib/x86_64 -lOpenCL
LD_FLAGS         := $(OPENCL_LIB) -lpthread
HPP_INC_PATHS    := -Iinclude
CC_FLAGS         := -Wall -std=c++0x $(HPP_INC_PATHS)

program: $(OBJ_FILES)
	g++ -o $(BIN_PATH)$@ $^ $(LD_FLAGS) 

$(BIN_PATH)%.o: $(SRC_PATH)%.cpp
	g++ $(CC_FLAGS) -c -o $@ $<
	
clean:
	rm -f $(BIN_PATH)*.o

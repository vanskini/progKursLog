CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc1

LDLIBS   = -lcryptopp
TARGET   = KursProg
SRCS     = inc/main.cpp inc/Interface.cpp inc/Logger.cpp inc/Base.cpp inc/Calc.cpp inc/Communicator.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

TEST_TARGET   = run_tests
TEST_SRCS     = test_main.cpp
TEST_APP_SRCS = inc/Interface.cpp inc/Logger.cpp inc/Base.cpp inc/Calc.cpp inc/Communicator.cpp

test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) $(TEST_APP_SRCS)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) $(TEST_APP_SRCS) -o $@ -lUnitTest++ -lpthread $(LDLIBS)

run-test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean-test:
	rm -f $(TEST_TARGET) test_base.txt test_log.txt

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET) test_base.txt test_log.txt

.PHONY: all clean test run-test clean-test

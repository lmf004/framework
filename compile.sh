

g++  -std=c++1z `pkg-config --cflags --libs gtk+-3.0` \
     test_socket_helper.cpp \
     treasure/socket_helper.cpp \

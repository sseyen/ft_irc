NAME     = ircserv

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I include

SRC_DIR  = src
OBJ_DIR  = obj

SRCS     = $(SRC_DIR)/main.cpp \
           $(SRC_DIR)/Server.cpp \
           $(SRC_DIR)/Client.cpp \
           $(SRC_DIR)/Channel.cpp \
           $(SRC_DIR)/Command.cpp \
		   $(SRC_DIR)/Registration.cpp \
           $(SRC_DIR)/Mode.cpp \
           $(SRC_DIR)/Join.cpp \
           $(SRC_DIR)/Privmsg.cpp

OBJS     = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

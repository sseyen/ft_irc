NAME = deus_irc

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98

INC = -I./include

SRC = src/main.cpp src/Server.cpp src/Client.cpp src/Command.cpp src/Registration.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

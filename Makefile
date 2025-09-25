NAME	=	Irc_server
CMD		=	c++
FLAGS	=	-Wall -Werror -Wextra -std=c++98

SRCS	=	src/Channel.cpp \
			src/Server.cpp \
			src/main.cpp


OFILES	=	src/Channel.o \
			src/Server.o \
			src/main.o



H		=	include/Channel.hpp \
			include/Client.hpp \
			include/Server.hpp




all : $(NAME)


$(NAME): $(OFILES) 
	$(CMD) $(OFILES) -o $(NAME)


%.o : %.cpp $(H)
	$(CMD) $(FLAGS) -c $< -o $@


clean:
	rm -rf $(OFILES)


fclean: clean
	rm -rf $(NAME)

re: fclean all


.PHONY: all clean fclean re
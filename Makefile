NAME	=	Irc_server

NAME_BONUS = Bot

CMD		=	c++
FLAGS	=	-Wall -Werror -Wextra -std=c++98

SRCS	=	src/Channel.cpp \
			src/Server.cpp \
			src/main.cpp

SRCS_B	=	src_bonus/Bot_bonus.cpp \
			src_bonus/main_bonus.cpp

OFILES_B	=	src_bonus/Bot_bonus.o \
				src_bonus/main_bonus.o

OFILES	=	src/Channel.o \
			src/Server.o \
			src/main.o



H		=	include/Channel.hpp \
			include/Client.hpp \
			include/Server.hpp

H_BONUS =	include_bonus/Bot_bonus.hpp


all : $(NAME)


$(NAME): $(OFILES) 
	$(CMD) $(OFILES) -o $(NAME)


bonus : $(NAME_BONUS)

$(NAME_BONUS): $(OFILES_B) 
	$(CMD) $(OFILES_B) -o $(NAME_BONUS)

%.o : %.cpp $(H)
	$(CMD) $(FLAGS) -c $< -o $@

%_bonus.o : %_bonus.cpp $(H_BONUS)
	$(CMD) $(FLAGS) -c $< -o $@

clean:
	rm -rf $(OFILES) $(OFILES_B)


fclean: clean
	rm -rf $(NAME) $(NAME_BONUS)

re: fclean all


.PHONY: all clean fclean re
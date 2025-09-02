all : S

C:
	c++ src/Channel.cpp -o C


S:
	c++ src/Server.cpp src/main.cpp src/Channel.cpp -o S

clean :
	rm S 

re : clean all
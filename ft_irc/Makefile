all : C S


C:
	c++ src/Client.cpp -o C


S:
	c++ src/Server.cpp src/main.cpp -o S

clean :
	rm S C

re : clean all
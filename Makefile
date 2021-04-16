# build an executable named Nitro from main.cpp
all: main.cpp
	gcc -g -Wall -o Nitro main.cpp

clean:
	$(RM) Nitro
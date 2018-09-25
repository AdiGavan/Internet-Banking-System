CPP=g++
LIBSOCKET=-lnsl
CCFLAGS=-Wall -g
SRV=server
SEL_SRV=server
CLT=client

all: build

build: $(SEL_SRV) $(CLT)

$(SEL_SRV):$(SEL_SRV).cpp
	$(CPP) -std=c++11 -Wall -o $(SEL_SRV) $(LIBSOCKET) $(SEL_SRV).cpp

$(CLT):	$(CLT).cpp
	$(CPP) -std=c++11 -Wall -o $(CLT) $(LIBSOCKET) $(CLT).cpp

clean:
	rm -f *.o *~
	rm -f $(SEL_SRV) $(CLT)
	rm -f *.log



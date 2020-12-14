
#This is a hack to pass arguments to the run command and probably only 
#works with gnu make. 
ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif


all: FTPserver

#The following lines contain the generic build options
CC=gcc
CPPFLAGS=
CFLAGS=-g -Werror-implicit-function-declaration

#List all the .o files here that need to be linked 
OBJS=FTPserver.o usage.o dir.o passiveHelper.o socketHelper.o
LIBS=-pthread
usage.o: usage.c usage.h

dir.o: dir.c dir.h

passiveHelper.o: passiveHelper.c passiveHelper.h

socketHelper.o: socketHelper.c socketHelper.h

FTPserver.o: FTPserver.c dir.h usage.h passiveHelper.h socketHelper.h

FTPserver: $(OBJS) 
	$(CC) -o FTPserver $(OBJS) $(LIBS)

clean:
	rm -f *.o
	rm -f FTPserver

.PHONY: run
run: FTPserver  
	./FTPserver $(RUN_ARGS)

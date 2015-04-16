PROJ = evproxy
CC = gcc
CFLAGS =  -Ijni \
		  -g 
	      #-DDEBUG #-m32
LDFLAGS = -lpthread

SRC = $(wildcard jni/*.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

$(PROJ) : $(OBJ)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

.PHONEY:clean $(PROJ) 
clean:
	@rm -rvf $(PROJ) $(OBJ)

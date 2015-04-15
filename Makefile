PROJ = evproxy
CC = gcc
CFLAGS =  -Ijni \
		  -g 
	      #-DDEBUG #-m32

SRC = $(wildcard jni/*.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

$(PROJ) : $(OBJ)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

	$(CC) $^ $(CFLAGS) -o $@

.PHONEY:clean $(PROJ) 
clean:
	@rm -rvf $(PROJ) $(OBJ)

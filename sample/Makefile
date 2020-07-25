

CUR_SOURCE = ${wildcard *.c}

CUR_OBJS = ${patsubst %.c, %.o, $(CUR_SOURCE)}

all : $(SUB_DIR) $(CUR_OBJS)

$(SUB_DIR) : ECHO
	make -C $@

$(CUR_OBJS) : %.o : %.c
	$(CC) -c $^ -o $(OBJS_DIR)/$@ $(FLAG)

ECHO : 
	@echo $(SUB_DIR)

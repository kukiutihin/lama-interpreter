SRC = src

all:
	@make -s -C $(SRC) 

clean:
	@make -s -C $(SRC) clean
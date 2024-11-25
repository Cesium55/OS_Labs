# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBFLAGS = -lpthread

# Названия файлов
EXEC = parallel_sum
LIB = libsum.a
LIB_SRC = sum_lib.c
LIB_OBJ = sum_lib.o
MAIN_SRC = parallel_sum.c
HEADER = sum_lib.h

# Правило по умолчанию
all: $(LIB) $(EXEC)

# Компиляция библиотеки
$(LIB): $(LIB_OBJ)
	ar rcs $(LIB) $(LIB_OBJ)

$(LIB_OBJ): $(LIB_SRC) $(HEADER)
	$(CC) $(CFLAGS) -c $(LIB_SRC) -o $(LIB_OBJ)

# Компиляция основной программы
$(EXEC): $(MAIN_SRC) $(LIB)
	$(CC) $(CFLAGS) $(MAIN_SRC) -L. -lsum $(LIBFLAGS) -o $(EXEC)

# Очистка
clean:
	rm -f $(EXEC) $(LIB) $(LIB_OBJ)

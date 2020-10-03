MYDU	= mydu
MYDU_OBJ = mydu.o
OUTPUT	= $(MYDU)

.SUFFIXES: .cpp .o

all: $(OUTPUT)

$(MYDU): $(MYDU_OBJ)
	g++ -Wall -g -o $@ $(MYDU_OBJ)

.cpp.o:
	g++ -Wall -g -c $<

.PHONY: clean
clean:
	/bin/rm -f $(OUTPUT) *.o


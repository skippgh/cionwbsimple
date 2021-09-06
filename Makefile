all:	cionset cionstatus
cionset: cionset.o Makefile cionset.c
	$(CC) $(LDFLAGS) -g -pthread -lmodbus -o cionset cionset.c
cionstatus: cionstatus.o Makefile cionstatus.c
	$(CC) $(LDFLAGS) -g -lmodbus -o cionstatus cionstatus.o
.c.o:	Makefile
	$(CC) $(CFLAGS) -pthread -g -c $<
clean:
	rm *.o cionstatus cionset

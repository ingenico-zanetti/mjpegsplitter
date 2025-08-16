mjpegsplitter: mjpegsplitter.c
	$(CC) -o mjpegsplitter mjpegsplitter.c

install: mjpegsplitter
	strip mjpegsplitter
	cp -vf ./mjpegsplitter ~/bin



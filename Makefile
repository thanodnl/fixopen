postgres_bin_dir = $(shell pg_config --bindir)

fixopen.dylib: fixopen.c
	gcc -dynamiclib fixopen.c -o fixopen.dylib

install: fixopen.dylib postgres.fixopen
	install $? $(postgres_bin_dir)

all:: fixopen.dylib install

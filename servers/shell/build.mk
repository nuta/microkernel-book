objs-y += main.o command.o fs.o http.o
cflags-y += -D'AUTORUN="$(AUTORUN)"'

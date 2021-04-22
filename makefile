all: shellexe
shellexe: history.c builtin.c pipehandler.c shell.c
	gcc history.c builtin.c pipehandler.c shell.c -o shellexe
clean:
	rm -f *.o
	rm -f ./d1/mymod.o
install: shellexe
	@cp shellexe /usr/bin
	@chmod a+x /usr/bin/shellexe
	@chmod og-w /usr/bin/shellexe
	@echo "shellexe installed successfully in /usr/bin"
uninstall:
	@rm -f /usr/bin/shellexe
	@echo "shellexe successfully un-installed from /usr/bin"

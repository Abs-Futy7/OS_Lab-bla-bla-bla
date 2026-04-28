How to compile
1. Open terminal in this folder.
2. Make script executable:
	chmod +x run.sh
3. Run one of the following:
	sh run.sh
	gcc shell.c -o shell

How to run
1. If we used manual compile:
	chmod +x shell
	./shell
2. If we use script (compile + run):
	chmod +x run.sh
	sh run.sh

Features implemented
- Interactive prompt with current directory: myshell:/path>
- Built-in commands: pwd, ls, ls -l, cd, echo, mkdir, touch, rm, mv, cat, cp, exit
- External command execution using fork + execvp
- Background execution with &

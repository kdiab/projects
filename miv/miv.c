#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

/*** data stream ***/

struct termios original_termios;

/*** terminal ***/

void die(const char *s){
	perror(s);
	exit(1);
}

void disableRawMode(){
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1);
		die("tcsetattr, error disabling raw mode");
}

void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &original_termios) == -1) die("tcgetattr, turning on raw mode");
	atexit(disableRawMode);

	struct termios raw = original_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //turn off echo flag
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr, turning on raw mode");
}

/*** init and main loop ***/

int main() {
	enableRawMode();
	while (1){
		char c = '\0';
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
		if (iscntrl(c)){
			printf("%d\r\n", c);
		}
		else {
			printf("%d ('%c')\r\n", c, c);
		}
		if (c == 'q') break;
	};
	return 0;
}

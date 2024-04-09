#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data stream ***/

struct termios original_termios;

/*** terminal ***/

void die(const char *s){
	perror(s);
	exit(1);
}

void disableRawMode(){
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1) die("tcsetattr, error disabling raw mode");
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

char readKey(){
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

/*** input ***/

void processKeys(){
	char c = readKey();
	switch(c) {
		case CTRL_KEY('q'):
			exit(0);
			break;
	}
}

/*** init and main loop ***/

int main() {
	enableRawMode();
	while (1){
		processKeys();
	}
	return 0;
}

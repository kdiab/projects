// THIS PROJECT IS BASED ON KILO

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

/*** defines ***/
void handleExit();
#define CTRL_KEY(k) ((k) & 0x1f)

/*** global state ***/

struct editorConfig {
	struct termios term;
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s){
	handleExit();
	perror(s);
	exit(1);
}

void disableRawMode(){
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.term) == -1) die("tcsetattr, error disabling raw mode");
}

void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &E.term) == -1) die("tcgetattr, turning on raw mode");
	atexit(disableRawMode);

	struct termios raw = E.term;
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
			handleExit();
			exit(0);
			break;
	}
}

/*** output ***/

void handleExit(){
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
}

void drawRows(){
	int y;
	for (y = 0; y < 24; y++) {
		write (STDOUT_FILENO, "~\r\n", 3);
	}
}

void refreshScreen(){
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	drawRows();
	write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** init and main loop ***/

int main() {
	enableRawMode();
	while (1){
		refreshScreen();
		processKeys();
	}
	return 0;
}

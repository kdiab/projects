// THIS PROJECT IS BASED ON KILO

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <string.h>

/*** defines ***/
void handleExit();
#define CTRL_KEY(k) ((k) & 0x1f)
#define MIV_VERSION "0.0.1"

/*** global state ***/

struct editorConfig {
	int screenrows;
	int screencols;
	struct termios term;
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s) {
	handleExit();
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.term) == -1) die("tcsetattr, error disabling raw mode");
}

void enableRawMode() {
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

char readKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

int getCursorPosition(int *rows, int *cols) {
  	char buf[32];
  	unsigned int i = 0;
  	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  	while (i < sizeof(buf) - 1) {
  	  	if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
  	  	if (buf[i] == 'R') break;
  	  	i++;
  	}
  	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	return 0;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** append buffer ***/

struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

void appendbuffer(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);

	if (new == NULL) return;
	memcpy (&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void freebuffer(struct abuf *ab) {
	free(ab->b);
}


/*** input ***/

void processKeys() {
	char c = readKey();
	switch(c) {
		case CTRL_KEY('q'):
			handleExit();
			exit(0);
			break;
	}
}

/*** output ***/

void handleExit() {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
}

void drawRows(struct abuf *ab) {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		if (y == E.screenrows / 3) {
			char welcome[80];
			int welcomelen = snprintf(welcome, sizeof(welcome), "MIV editor, it's VIM backwards -- version %s", MIV_VERSION);
			if (welcomelen > E.screencols) welcomelen = E.screencols;
			int padding = (E.screencols - welcomelen) / 2;
			if (padding) {
				appendbuffer(ab, "~", 1);
				padding--;
			}
			while (padding--) appendbuffer(ab, " ", 1);
			appendbuffer(ab, welcome, welcomelen);
		} else {
			appendbuffer(ab, "~", 1);
		}
		appendbuffer(ab, "\x1b[K", 3);
		if (y < E.screenrows - 1) {
			appendbuffer(ab, "\r\n", 2);
		}
	}
}

void refreshScreen() {
	struct abuf ab = ABUF_INIT;

	appendbuffer(&ab, "\x1b[?25l", 6);
	appendbuffer(&ab, "\x1b[H", 3);

	drawRows(&ab);
	appendbuffer(&ab, "\x1b[H", 3);
	appendbuffer(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	freebuffer(&ab);
}

/*** init and main loop ***/

void init() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
	enableRawMode();
	init();

	while (1) {
		refreshScreen();
		processKeys();
	}
	return 0;
}

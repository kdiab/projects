// THIS PROJECT IS BASED ON KILO

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>

/*** defines ***/
void handleExit();
#define CTRL_KEY(k) ((k) & 0x1f)
#define MIV_VERSION "0.0.1"

enum keymap {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	PAGE_UP,
	PAGE_DOWN
};

/*** global state ***/

typedef struct trow {
	int size;
	char *chars;
} trow;

struct editorConfig {
	int cx, cy;
	int rowoffset;
	int coloffset;
	int screenrows;
	int screencols;
	int numrows;
	trow *row;
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

int readKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	if (c == '\x1b') {
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
		

		if (seq[0] == '['){
			if (seq[2] >= '0' && seq[2] <= '9'){
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
					switch(seq[1]) {
				}
			} else {
			switch (seq[1]) {
				case 'A': return ARROW_UP;
				case 'B': return ARROW_DOWN;
				case 'C': return ARROW_RIGHT;
				case 'D': return ARROW_LEFT;
				case '5': return PAGE_UP;
				case '6': return PAGE_DOWN;
				}
			}
		}
		return '\x1b';
	} else {
		return c;
	}
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

/*** row operations ***/

void appendRow(char *s, size_t len){
	E.row = realloc(E.row, sizeof(trow) * (E.numrows + 1));
	
	int at = E.numrows;
	E.row[at].size = len;
	E.row[at].chars = malloc (len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';
	E.numrows++;
}


/*** file io ***/

void open(char *filename) {
	FILE *fp = fopen(filename, "r");
	if (!fp) die("fopen fail"); //maybe print help at somepoint

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelength;
	while((linelength = getline(&line, &linecap, fp)) != -1) {
		while (linelength > 0 && (line[linelength - 1] == '\n' || 
					line[linelength - 1] == '\r'))
			linelength--;
		appendRow(line, linelength);
	}
	free(line);
	fclose(fp);
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

void moveCursor(int key) {
	trow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

	switch(key) {
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;
			}
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size) {
				E.cx++;
			}
			break;
		case ARROW_UP:
			if (E.cy != 0) {
				E.cy--;
			}
			break;
		case ARROW_DOWN:
			if (E.cy < E.numrows - 1) {
				E.cy++;
			}
			break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen) {
		E.cx = rowlen;
	}
}

void processKeys() {
	int c = readKey();
	switch(c) {
		case CTRL_KEY('q'):
			handleExit();
			exit(0);
			break;
		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_RIGHT:
		case ARROW_LEFT:
			moveCursor(c);
			break;
		case PAGE_UP:
		case PAGE_DOWN:
			{
				int times = E.screenrows;
				while (times --)
					moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
			break;

	}
}

/*** output ***/

void handleExit() {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
}

void scroll(){
	if (E.cy < E.rowoffset) {
		E.rowoffset = E.cy;
	}
	if (E.cy >= E.rowoffset + E.screenrows) {
		E.rowoffset = E.cy - E.screenrows + 1;
	}
	if (E.cx < E.coloffset) {
		E.coloffset = E.cx;
	}
	if (E.cx >= E.coloffset + E.screencols) {
		E.coloffset = E.cx - E.screencols + 1;
	}
}

void drawRows(struct abuf *ab) {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoffset;
		if (filerow >= E.numrows) {
		if (y == E.screenrows / 3 && E.numrows == 0) {
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
		} else {
			int len = E.row[filerow].size - E.coloffset;
			if (len < 0) len = 0; 
			if (len > E.screencols) len = E.screencols;
			appendbuffer(ab, &E.row[filerow].chars[E.coloffset], len);
		}
		appendbuffer(ab, "\x1b[K", 3);
		if (y < E.screenrows - 1) {
			appendbuffer(ab, "\r\n", 2);
		}
	}
}

void refreshScreen() {
	scroll();

	struct abuf ab = ABUF_INIT;

	appendbuffer(&ab, "\x1b[?25l", 6);
	appendbuffer(&ab, "\x1b[H", 3);

	drawRows(&ab);
	
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoffset) + 1, (E.cx - E.coloffset) + 1);
	appendbuffer(&ab, buf, strlen(buf));

	appendbuffer(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	freebuffer(&ab);
}

/*** init and main loop ***/

void init() {
	E.cx = 0;
	E.cy = 0;
	E.numrows = 0;
	E.rowoffset = 0;
	E.coloffset = 0;
	E.row = NULL;

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(int argc, char *argv[]) {
	enableRawMode();
	init();
	if (argc >= 2) {
		open(argv[1]);
	}

	while (1) {
		refreshScreen();
		processKeys();
	}
	return 0;
}

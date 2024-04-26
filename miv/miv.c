// THIS PROJECT IS BASED ON KILO

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>

/*** defines ***/
void handleExit();
#define CTRL_KEY(k) ((k) & 0x1f)
#define MIV_VERSION "0.0.1"
#define MIV_TAB_STOP 8
#define MIV_QUIT 1

enum keymap {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	PAGE_UP,
	PAGE_DOWN
};

/*** global state ***/

typedef struct trow {
	int size;
	int rsize;
	char *chars;
	char *render;
} trow;

struct editorConfig {
	int cx, cy;
	int rx;
	int rowoffset;
	int coloffset;
	int screenrows;
	int dirty;
	int screencols;
	int numrows;
	trow *row;
	char *filename;
	char statusmsg[80];
	time_t statusmsg_time;
	struct termios term;
};

struct editorConfig E;

/*** prototypes ***/

void setStatusMessage(const char *fmt, ...);
void refreshScreen();
char *promptUser(char *prompt);

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

int cxtorx(trow *row, int cx){
	int rx = 0;
	int j;
	for (j = 0; j < cx; j++){
		if (row->chars[j] == '\t')
			rx += (MIV_TAB_STOP - 1) -  (rx % MIV_TAB_STOP);
		rx++;
	}
	return rx;
}

void updateRow(trow *row){
	int tabs = 0;
	int j;
	for (j = 0; j < row->size; j++)
		if (row->chars[j] == '\t') tabs++;
	free(row->render);
	row->render = malloc(row->size + tabs * (MIV_TAB_STOP - 1) + 1);
	int idx = 0;
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') {
			row->render[idx++] = ' ';
			while (idx % MIV_TAB_STOP != 0) row->render[idx++] = ' ';
		} else {
			row->render[idx++] = row->chars[j];
		}
	}
	row->render[idx] = '\0';
	row->rsize = idx;
}

void insertRow(int at, char *s, size_t len){
	if (at < 0 || at > E.numrows) return;
	E.row = realloc(E.row, sizeof(trow) * (E.numrows + 1));
	memmove(&E.row[at + 1], &E.row[at], sizeof(trow) * (E.numrows - at));

	E.row[at].size = len;
	E.row[at].chars = malloc (len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';
	
	E.row[at].rsize = 0;
	E.row[at].render = NULL;
	updateRow(&E.row[at]);

	E.numrows++;
	E.dirty++;
}

void freeRow(trow *row) {
	free(row->render);
	free(row->chars);
}

void delRow(int at) {
	if (at < 0 || at >= E.numrows) return;
	freeRow(&E.row[at]);
	memmove(&E.row[at], &E.row[at + 1], sizeof(trow) * (E.numrows - at - 1));
	E.numrows--;
	E.dirty++;
}

void moveRowChars(trow *row, int at, int c) {
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	updateRow(row);
	E.dirty++;
}

void appendRowString(trow *row, char *s, size_t len) {
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	updateRow(row);
	E.dirty++;
}

void delRowChar(trow *row, int at) {
	if (at < 0 || at >= row->size) return;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;
	updateRow(row);
	E.dirty++;
}

/*** editor operations ***/

void insertChar(int c) { 
	if (E.cy == E.numrows) {
		insertRow(E.numrows, "", 0);
	}
	moveRowChars(&E.row[E.cy], E.cx, c);
	E.cx++;
}

void insertNewLine() {
	if (E.cx == 0) {
		insertRow(E.cy, "", 0);
	} else { 
		trow *row = &E.row[E.cy];
		insertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
		row = &E.row[E.cy];
		row->size = E.cx;
		row->chars[row->size] = '\0';
		updateRow(row);
	}
	E.cy++;
	E.cx = 0;
}

void delChar() {
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;

	trow *row = &E.row[E.cy];
	if (E.cx > 0) {
		delRowChar(row, E.cx - 1);
		E.cx--;
	} else {
		E.cx = E.row[E.cy - 1].size;
		appendRowString(&E.row[E.cy - 1], row->chars, row->size);
		delRow(E.cy);
		E.cy--;
	}
}

/*** file io ***/

char *rowsToString(int *buflen){
	int total_len = 0;
	int j;
	for (j = 0; j < E.numrows; j++)
		total_len += E.row[j].size + 1;
	*buflen = total_len;
	char *buf = malloc(total_len);
	char *p = buf;
	for (j = 0; j < E.numrows; j++) {
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}
	return buf;
}

void Open(char *filename) {
	free(E.filename);
	E.filename = strdup(filename);

	FILE *fp = fopen(filename, "r");
	if (!fp) die("fopen fail"); //maybe print help at somepoint

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelength;
	while((linelength = getline(&line, &linecap, fp)) != -1) {
		while (linelength > 0 && (line[linelength - 1] == '\n' || 
					line[linelength - 1] == '\r'))
			linelength--;
		insertRow(E.numrows, line, linelength);
	}
	free(line);
	fclose(fp);
	E.dirty = 0;
}

void save() {
	if (E.filename == NULL) {
		E.filename = promptUser("Save as: %s (ESC to cancel)");
		if (E.filename == NULL) {
			setStatusMessage("Save aborted");
			return;
		}
	}

	int len;
	char *buf = rowsToString(&len);

	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf,len) == len) {
				close(fd);
				free(buf);
				E.dirty = 0;
				setStatusMessage("%d bytes written to disk", len);
				return;
			}
		}
		close(fd);
	}
	free(buf);
	setStatusMessage("Can't save file, I/O error: %s", strerror(errno));
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

char *promptUser(char *prompt) {
	size_t bufsize = 128;
	char *buf = malloc(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';

	while (1) {
		setStatusMessage(prompt, buf);
		refreshScreen();
		int c = readKey();
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (buflen != 0) buf[--buflen] = '\0';
		} else if (c == '\x1b') {
			setStatusMessage("");
			free(buf);
			return NULL;
		} else if (c == '\r') {
			if (buflen != 0) {
				setStatusMessage("");
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
	}
}

void moveCursor(int key) {
	trow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

	switch(key) {
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;
			}
			else if (E.cy > 0) {
				E.cy--;
				E.cx = E.row[E.cy].size;
			}
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size) {
				E.cx++;
			}
			else if (row && E.cx == row->size) {
				E.cy++;
				E.cx = 0;
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
	static int quit_times = MIV_QUIT;

	int c = readKey();
	switch(c) {
		case '\r':
			insertNewLine();
			break;
		case CTRL_KEY('q'):
			if (E.dirty && quit_times > 0) {
				setStatusMessage("WARNING!!! File has unsaved changes. Press Ctrl-Q %d more times to quit without saving.", quit_times);
				quit_times--;
				return;
			}
			handleExit();
			exit(0);
			break;
		
		case CTRL_KEY('s'):
			save();
			break;

		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_RIGHT:
		case ARROW_LEFT:
			moveCursor(c);
			break;
		
		case CTRL_KEY('l'):
		case '\x1b':
			break;

		case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY:
			if (c == DEL_KEY) moveCursor(ARROW_RIGHT);
			delChar();
			break;
		case PAGE_UP:
		case PAGE_DOWN:
			if (c == PAGE_UP) {
				E.cy = E.rowoffset;
			}
			else if (c == PAGE_DOWN) {
				E.cy = E.rowoffset + E.screenrows - 1;
				if (E.cy > E.numrows) E.cy = E.numrows;
			}

			{
				int times = E.screenrows;
				while (times --)
					moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
			break;

		default: 
			insertChar(c);
			break;
	}
	quit_times = MIV_QUIT;
}

/*** output ***/

void handleExit() {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
}

void scroll(){
	E.rx = 0;

	if (E.cy < E.numrows) {
		E.rx = cxtorx(&E.row[E.cy], E.cx);
	}

	if (E.cy < E.rowoffset) {
		E.rowoffset = E.cy;
	}
	if (E.cy >= E.rowoffset + E.screenrows) {
		E.rowoffset = E.cy - E.screenrows + 1;
	}
	if (E.rx < E.coloffset) {
		E.coloffset = E.rx;
	}
	if (E.rx >= E.coloffset + E.screencols) {
		E.coloffset = E.rx - E.screencols + 1;
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
			int len = E.row[filerow].rsize - E.coloffset;
			if (len < 0) len = 0; 
			if (len > E.screencols) len = E.screencols;
			appendbuffer(ab, &E.row[filerow].render[E.coloffset], len);
		}
		appendbuffer(ab, "\x1b[K", 3);
		appendbuffer(ab, "\r\n", 2);
	}
}

void drawStatusBar(struct abuf *ab) {
	appendbuffer(ab,"\x1b[7m", 4);
	
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", E.filename ? E.filename : "[Unnamed File]", E.numrows, E.dirty ? "(File Modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", E.cy + 1, E.numrows);

	if (len > E.screencols) len = E.screencols;
	appendbuffer(ab, status, len);

	while (len < E.screencols) {
		if (E.screencols - len == rlen) {
			appendbuffer(ab, rstatus, rlen);
			break;
		} else {
			appendbuffer(ab, " ", 1);
			len++;
		}
	}
	appendbuffer(ab, "\x1b[m", 3);
}

void drawMessageBar(struct abuf *ab) {
	appendbuffer(ab, "\x1b[K", 3);
	int msglen = strlen(E.statusmsg);
	if (msglen > E.screencols) msglen = E.screencols;
	if (msglen && time(NULL) - E.statusmsg_time < 5)
		appendbuffer(ab, E.statusmsg, msglen);
}


void refreshScreen() {
	scroll();

	struct abuf ab = ABUF_INIT;

	appendbuffer(&ab, "\x1b[?25l", 6);
	appendbuffer(&ab, "\x1b[H", 3);

	drawRows(&ab);
	drawStatusBar(&ab);
	drawMessageBar(&ab);
	
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoffset) + 1, (E.rx - E.coloffset) + 1);
	appendbuffer(&ab, buf, strlen(buf));

	appendbuffer(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	freebuffer(&ab);
}

void setStatusMessage(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}

/*** init and main loop ***/

void init() {
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.numrows = 0;
	E.rowoffset = 0;
	E.coloffset = 0;
	E.dirty = 0;
	E.row = NULL;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	E.screenrows -= 2;
}

int main(int argc, char *argv[]) {
	enableRawMode();
	init();
	if (argc >= 2) {
		Open(argv[1]);
	}

	setStatusMessage("HELP: Ctrl-Q to quit | Ctrl-S to Save");

	while (1) {
		refreshScreen();
		processKeys();
	}
	return 0;
}

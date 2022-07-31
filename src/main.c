#define _XOPEN_SOURCE 700

#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#define ANSI_COL_DROP "\x1b[97m"
#define ANSI_CUR_HIDE "\e[?25l"
#define ANSI_CUR_SHOW "\e[?25h"
#define ANSI_CUR_RESET "\x1b[H"
#define ANSI_SCRN_CLEAR "\x1b[2J"
#define ANSI_FONT_RESET "\x1b[0m"

typedef struct matrix_tag {
	int rows;
	int cols;
	wchar_t *code;
} matrix;

static int mat_index(const matrix *mat, int row, int col) {
	return mat->cols * row + col;
}

static int mat_init(matrix *mat, struct winsize *ws) {
	int i, j;

	static wchar_t code[] = {
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	  'a', 'b', 'c', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	  'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

	static int codelen = sizeof(code) / sizeof(code[0]);

	mat->rows = ws->ws_row + 1;
	mat->cols = ws->ws_col;
	mat->code = realloc(mat->code, sizeof mat->code[0] * mat->rows * mat->cols);
	if (mat->code) {
		for (i = 0; i < mat->rows; i++) {
			for (j = 0; j < mat->cols; j++)
				mat->code[mat_index(mat, i, j)] = code[rand() % codelen];
		}
		return 1;
	}
	return 0;
}

typedef struct table_tag {
	int len;   // number of columns
	int *cols; // column indices
	int *rows; // row indices ((rows[i], cols[i]) points to a cell in the matrix)
	int *attr; // cell attributes
} table;

static void shuffle(int *a, int n) {
	int i, j, tmp;

	for (i = n - 1; i > 0; i--) {
		j = rand() % (i + 1);
		tmp = a[j];
		a[j] = a[i];
		a[i] = tmp;
	}
}

static int tab_init(table *tab, const matrix *mat) {
	size_t i;

	tab->len = mat->cols;
	tab->cols = realloc(tab->cols, sizeof tab->cols[0] * tab->len);
	tab->rows = realloc(tab->rows, sizeof tab->rows[0] * tab->len);
	tab->attr = realloc(tab->attr, sizeof tab->attr[0] * tab->len);

	if (tab->cols && tab->rows && tab->attr) {
		for (i = 0; i < mat->cols; i++) {
			tab->rows[i] = 0;
			tab->cols[i] = i;
			tab->attr[i] = 0;
		}
		shuffle(tab->cols, tab->len);
		return 1;
	}
	return 0;
}

static void tab_free(table *tab) {
	free(tab->cols);
	free(tab->rows);
	free(tab->attr);
}

static int term_init() {
	struct termios ta;

	if (tcgetattr(STDIN_FILENO, &ta) == 0) {
		ta.c_lflag &= ~ECHO;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &ta) == 0) {
			wprintf(L"%s", ANSI_CUR_HIDE);
			wprintf(L"%s", ANSI_CUR_RESET);
			wprintf(L"%s", ANSI_SCRN_CLEAR);
			setvbuf(stdout, 0, _IOFBF, 0);
			return 1;
		}
	}
	return 0;
}

static void term_reset() {
	struct termios ta;

	wprintf(L"%s", ANSI_FONT_RESET);
	wprintf(L"%s", ANSI_CUR_SHOW);
	wprintf(L"%s", ANSI_SCRN_CLEAR);
	wprintf(L"%s", ANSI_CUR_RESET);

	if (tcgetattr(STDIN_FILENO, &ta) == 0) {
		ta.c_lflag |= ECHO;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &ta) != 0)
			perror("term_reset()");
	}
	setvbuf(stdout, 0, _IOLBF, 0);
}

static void term_size(struct winsize *ws) {
	ioctl(STDOUT_FILENO, TIOCGWINSZ, ws);
}

static void term_clear_cell(int row, int col) {
	wprintf(L"\x1b[%d;%dH ", row, col);
}

static void term_reset_cell(const matrix *mat, int row, int col) {
	wprintf(L"\x1b[%d;%dH\x1b[39m%lc", row, col,
	        mat->code[mat_index(mat, row, col)]);
}

static void term_print(const matrix *mat, int row, int col) {
	wprintf(L"\x1b[%d;%dH%s%lc", row, col, ANSI_COL_DROP,
	        mat->code[mat_index(mat, row, col)]);
}

static void term_glitch(int row, int col) {
	static wchar_t code[] = {
	  0x0413, // Г
	  0x0418, // И
	  0x042F, // Я
	  0x2200, // ∀
	  0x04D9, // ә
	  0x0500, // Ԁ
	  0x2203, // ∃
	  0x2132, // Ⅎ
	  0x0245, // Ʌ
	  0x037B, // ͻ
	  0x03FD, // Ͻ
	  0x041F, // П
	  0x04D4, // Ӕ
	  0x0424, // Ф
	};

	static int codelen = sizeof(code) / sizeof(code[0]);

	wprintf(L"\x1b[%d;%dH\x1b[39m%lc", row, col, code[rand() % codelen]);
}

static volatile int run;

static void handle_signal(int sa) {
	switch (sa) {
	case SIGINT:
	case SIGQUIT:
	case SIGTERM:
		run = 0;
		break;
	}
}

int main(int argc, char *argv[]) {
	matrix mat;
	table tab;
	struct winsize ws;
	struct timespec ts;
	struct sigaction sa;
	int i, cols, blank, delay, tmp;

	setlocale(LC_CTYPE, "");

	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGQUIT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	srand(time(0));

	if (!term_init())
		return 1;

	term_size(&ws);
	mat = (matrix){0};
	if (!mat_init(&mat, &ws)) {
		term_reset();
		return 1;
	}

	tab = (table){0};
	if (!tab_init(&tab, &mat)) {
		term_reset();
		return 1;
	}

	delay = 75;
	ts.tv_sec = delay / 1000;
	ts.tv_nsec = (delay % 1000) * 1000000;

	run = cols = 1;
	blank = tab.cols[tab.len - 1];
	while (run) {
		for (i = 0; run && i < cols; i++) {
			if (tab.rows[i] == mat.rows) {
				tab.rows[i] = 0;
				term_reset_cell(&mat, mat.rows - 1, tab.cols[i]);
			}
			if (tab.attr[i] == 0) {
				// hard-coding to 5 (row[i] - rand() % rows[i] glitches on empty rows)
				if (tab.rows[i] > 5 && rand() % 6 == 0)
					term_glitch(tab.rows[i] - 5, tab.cols[i]);
				if (tab.rows[i] > 0)
					term_reset_cell(&mat, tab.rows[i] - 1, tab.cols[i]);
				term_print(&mat, tab.rows[i], tab.cols[i]);
				if (tab.rows[i] == mat.rows - 1)
					tab.attr[i] = 1;
				tab.rows[i]++;
			} else {
				term_clear_cell(tab.rows[i], tab.cols[i]);
				if (tab.rows[i] == mat.rows - 1) {
					tab.rows[i] = 0;
					tab.attr[i] = 0;
					tmp = blank;
					blank = tab.cols[i];
					tab.cols[i] = tmp;
				} else
					tab.rows[i]++;
			}
		}
		if (cols < tab.len * 0.6)
			tab.rows[cols++] = 0;
		fflush(stdout);
		nanosleep(&ts, &ts);
	}

	term_reset();
	tab_free(&tab);
	free(mat.code);
	return 0;
}

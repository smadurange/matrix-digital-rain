#include <locale.h>

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <uchar.h>
#include <wchar.h>

#define UNICODE_MIN 0x0021
#define UNICODE_MAX 0x007E

#define RHO         0.5 /* Rain density */

#define RGB_BG_RED   34 /* RGB values of the background */
#define RGB_BG_GRN   34
#define RGB_BG_BLU   34

#define RGB_HD_RED  255 /* RGB values of the first drop */
#define RGB_HD_GRN  255
#define RGB_HD_BLU  255

#define RGB_TL_RED    0 /* RGB values of the foreground */
#define RGB_TL_GRN  177
#define RGB_TL_BLU   64

#define ANSI_CUR_HIDE    "\e[?25l"
#define ANSI_CUR_SHOW    "\e[?25h"
#define ANSI_CUR_RESET   "\x1b[H"
#define ANSI_FONT_BOLD   "\x1b[1m"
#define ANSI_FONT_RESET  "\x1b[0m"
#define ANSI_SCRN_CLEAR  "\x1b[2J"

enum {
	R, /* Red */
	G, /* Green */
	B, /* Blue */ 
	M  /* Phosphor decay multiplier */
};

typedef union color_tag {
	uint32_t value;
	unsigned char color[4];
} color;

typedef struct matrix_tag {
	size_t rows;
	size_t cols;
	size_t *col;
	size_t *row;
	color *rgb;
	char32_t *code;
} matrix;

static size_t mat_idx(const matrix *mat, size_t row, size_t col)
{
	return mat->cols * row + col;
}

static void mat_put_code(matrix *mat, size_t row, size_t col) 
{
	mat->code[mat_idx(mat, row, col)] =
	  rand() % (UNICODE_MAX - UNICODE_MIN) + UNICODE_MIN;
}

static void shuffle(size_t *a, size_t n) 
{
	size_t i, j;

	for (i = n - 1; i > 0; i--) {
		j = rand() % (i + 1);
		a[j] = a[i] ^ a[j];
		a[i] = a[i] ^ a[j];
		a[j] = a[i] ^ a[j];
	}
}

static int mat_init(matrix *mat, const struct winsize *ws)
{
	size_t i;

	mat->cols = ws->ws_col;
	mat->rows = ws->ws_row + 1;

	mat->code = realloc(mat->code, 
		sizeof mat->code[0] * mat->rows * mat->cols);
	if (!mat->code)
		return 0;

	mat->rgb = realloc(mat->rgb,
		sizeof mat->rgb[0] * mat->rows * mat->cols);
	if (!mat->rgb)
		return 0;

	mat->col = realloc(mat->col, sizeof mat->col[0] * mat->cols);
	if (!mat->col)
		return 0;

	mat->row = realloc(mat->row, sizeof mat->row[0] * mat->cols);
	if (!mat->row)
		return 0;

	for (i = 0; i < mat->cols; i++) {
		mat->row[i] = 0;
		mat->col[i] = i;
	}

	shuffle(mat->col, mat->cols);
	return 1;
}

static void mat_reset_head(matrix *mat, size_t row, size_t col) 
{
	unsigned char *sc, *tc;

	sc = mat->rgb[mat_idx(mat, 0, col)].color;
	tc = mat->rgb[mat_idx(mat, row, col)].color;

	tc[R] = sc[R];
	tc[G] = sc[G];
	tc[B] = sc[B];
}

static void mat_set_tail(matrix *mat, size_t row, size_t col)
{
	unsigned char *color;

	color = mat->rgb[mat_idx(mat, row, col)].color;
	color[R] = RGB_TL_RED;
	color[G] = RGB_TL_GRN;
	color[B] = RGB_TL_BLU;
}

static void mat_set_head(matrix *mat, size_t row, size_t col)
{
	unsigned char *color;

	color = mat->rgb[mat_idx(mat, row, col)].color;
	color[R] = RGB_HD_RED;
	color[G] = RGB_HD_GRN;
	color[B] = RGB_HD_BLU;
}

static uint8_t is_head(matrix *mat, size_t row, size_t col)
{
	size_t idx;
	unsigned char *color;

	idx = mat_idx(mat, row, col);
	color = mat->rgb[idx].color;
	return color[R] == RGB_HD_RED && color[G] == RGB_HD_GRN
		&& color[B] == RGB_HD_BLU;
}

static void mat_decay(matrix *mat, size_t row, size_t col)
{
	size_t idx;
	unsigned char *color;

	idx = mat_idx(mat, row, col);
	color = mat->rgb[idx].color;
	
	if (is_head(mat, row, col)) {
		// Glitch, so reset head
		mat_reset_head(mat, row, col);
		return;
	}

	color[R] = color[R] - (color[R] - RGB_BG_RED) / 2;
	color[G] = color[G] - (color[G] - RGB_BG_GRN) / 2;
	color[B] = color[B] - (color[B] - RGB_BG_BLU) / 2;
}

static void mat_free(matrix *mat)
{
	free(mat->code);
	free(mat->col);
	free(mat->row);
	free(mat->rgb);
}

static int term_init() 
{
	struct termios ta;

	if (tcgetattr(STDIN_FILENO, &ta) == 0) {
		ta.c_lflag &= ~ECHO;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &ta) == 0) {
			wprintf(L"\x1b[48;2;%d;%d;%dm", 
				RGB_BG_RED, RGB_BG_GRN, RGB_BG_BLU);
			wprintf(L"%s", ANSI_FONT_BOLD);
			wprintf(L"%s", ANSI_CUR_HIDE);
			wprintf(L"%s", ANSI_CUR_RESET);
			wprintf(L"%s", ANSI_SCRN_CLEAR);

			setvbuf(stdout, 0, _IOFBF, 0);
			return 1;
		}
	}
	return 0;
}

static void term_reset()
{
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

static void term_size(const struct winsize *ws)
{
	ioctl(STDOUT_FILENO, TIOCGWINSZ, ws);
}

static void term_print(const matrix *mat, size_t row, size_t col)
{
	size_t idx;

	idx = mat_idx(mat, row, col);
	wprintf(L"\x1b[%d;%dH\x1b[38;2;%d;%d;%dm%lc", row, col,
	        mat->rgb[idx].color[R], mat->rgb[idx].color[G],
	        mat->rgb[idx].color[B], mat->code[idx]);
}

static uint8_t is_tail(matrix *mat, size_t row, size_t col)
{
	size_t idx;
	unsigned char *color;

	idx = mat_idx(mat, row, col);
	color = mat->rgb[idx].color;
	return color[R] == RGB_TL_RED && color[G] == RGB_TL_GRN
		&& color[B] == RGB_TL_BLU;
}

static void glitch(matrix *mat)
{
	size_t i, j, idx;
	
	i = rand() % (mat->rows - 1);
	j = rand() % mat->cols;
	idx = mat_idx(mat, i, j);
	if (mat->code[idx] != ' ' && is_tail(mat, i, j)) {
		mat_put_code(mat, i, j);
		term_print(mat, i, j);
	}
}

static volatile int run;

static void handle_signal(int sa) 
{
	switch (sa) {
	case SIGINT:
	case SIGQUIT:
	case SIGTERM:
		run = 0;
		break;
	}
}

int main(int argc, char *argv[]) 
{
	matrix mat;

	struct winsize ws;
	struct sigaction sa;
	size_t i, j, len, maxlen;

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

	run = 1, len = 1;
	maxlen = mat.cols * RHO;

	while (run) {
		for (i = 0; run && i < len; i++) {
			if (mat.row[i] == mat.rows) {
				mat_reset_head(&mat, mat.row[i] - 1, mat.col[i]);
				term_print(&mat, mat.rows - 1, mat.col[i]);
				mat.row[i] = 0;
			}

			if (mat.rgb[i].color[M] == 0) {
				if (mat.row[i] > 0) {
					mat_set_tail(&mat, mat.row[i] - 1, mat.col[i]);
					term_print(&mat, mat.row[i] - 1, mat.col[i]);
				}

				mat_set_head(&mat, mat.row[i], mat.col[i]);
				mat_put_code(&mat, mat.row[i], mat.col[i]);
				term_print(&mat, mat.row[i], mat.col[i]);
				
				if (mat.row[i] == mat.rows - 1)
					mat.rgb[i].color[M] = 1;

				mat.row[i]++;
			} else if (mat.rgb[i].color[M] == 1 || mat.rgb[i].color[M] == 2) {
				mat_decay(&mat, mat.row[i], mat.col[i]);
				term_print(&mat, mat.row[i], mat.col[i]);

				if (mat.row[i] == mat.rows - 1)
					mat.rgb[i].color[M]++;

				mat.row[i]++;
			} else {
				mat.code[mat_idx(&mat, mat.row[i], mat.col[i])] = ' ';
				term_print(&mat, mat.row[i], mat.col[i]);

				if (mat.row[i] == mat.rows - 1) {
					mat.row[i] = 0;
					mat.rgb[i].color[M] = 0;
					j = rand() % (mat.cols - maxlen) + maxlen;
					mat.col[i] = mat.col[i] ^ mat.col[j];
					mat.col[j] = mat.col[i] ^ mat.col[j];
					mat.col[i] = mat.col[i] ^ mat.col[j];
				} else
					mat.row[i]++;
			}

			glitch(&mat);
		}

		if (len < maxlen &&
		    mat.row[len - 1] >= rand() % (int)(mat.rows * 0.25)) {
			mat.row[len++] = 0;
		}

		fflush(stdout);
		usleep(50000);
	}

	term_reset();
	mat_free(&mat);

	return 0;
}


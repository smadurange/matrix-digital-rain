#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <uchar.h>
#include <wchar.h>
#include <sys/ioctl.h>

#define UNICODE_MIN  0x0021  /* Beginning of Unicode block: Katakana: 0x30A1 */
#define UNICODE_MAX  0x007E  /* End of Unicode block: Katakana: 0x30F6 */

#define RHO             0.5  /* Rain density: (0, 1) */

#define RGB_BG_RED       34  /* Background color */
#define RGB_BG_GRN       34
#define RGB_BG_BLU       34

#define RGB_HD_RED      255  /* Color of the first raindrop */
#define RGB_HD_GRN      255
#define RGB_HD_BLU      255

#define RGB_TL_RED        0  /* Color of the rain */
#define RGB_TL_GRN      177
#define RGB_TL_BLU       64

#define DECAY_MPLIER      2  /* Phosphor decay multiplier */
#define DELAY_US      60000  /* Delay between frames: increase to slow the rain */

#define ANSI_CRSR_HIDE   "\e[?25l"
#define ANSI_CRSR_SHOW   "\e[?25h"
#define ANSI_CRSR_RESET  "\x1b[H"
#define ANSI_FONT_BOLD   "\x1b[1m"
#define ANSI_FONT_RESET  "\x1b[0m"
#define ANSI_SCRN_CLEAR  "\x1b[2J"

enum {
	R,  /* Red   */
	G,  /* Green */
	B,  /* Blue  */ 
	PD  /* Phosphor decay multiplier */
};

typedef union color_tag {
	uint32_t value;
	unsigned char color[4];
} color;

typedef struct matrix_tag {
	size_t rowlen;   /* Row count: terminal screen height */
	size_t collen;   /* Column count: terminal screen width */
	size_t *col;     /* Column indices in random order (shuffle()) */
	size_t *row;     /* Current row index of columns: 1-1 map with col */
	color *rgb;      /* RGB color of the cell */
	char32_t *code;  /* Code point */
} matrix;

static inline size_t index(const matrix *mat,
	size_t row, size_t col)
{
	return mat->collen * row + col;
}

static inline void print(const matrix *mat,
	size_t row, size_t col)
{
	size_t idx;

	idx = index(mat, row, col);
	wprintf(L"\x1b[%d;%dH\x1b[38;2;%d;%d;%dm%lc", row, col,
	        mat->rgb[idx].color[R], mat->rgb[idx].color[G],
	        mat->rgb[idx].color[B], mat->code[idx]);
}

static inline void insert_code(matrix *mat,
	size_t row, size_t col) 
{
	mat->code[index(mat, row, col)] = rand()
		% (UNICODE_MAX - UNICODE_MIN)
		+ UNICODE_MIN;
}

static inline void delete_code(matrix *mat,
	size_t row, size_t col)
{
	mat->code[index(mat, row, col)] = ' ';
	print(mat, row, col);
}

static inline void recolor_head(matrix *mat,
	size_t row, size_t col) 
{
	unsigned char *sc, *tc;

	sc = mat->rgb[index(mat, 0, col)].color;
	tc = mat->rgb[index(mat, row, col)].color;
	tc[R] = sc[R];
	tc[G] = sc[G];
	tc[B] = sc[B];
	print(mat, row, col);
}

static inline void draw_tail(matrix *mat,
	size_t row, size_t col)
{
	unsigned char *color;

	color = mat->rgb[index(mat, row, col)].color;
	color[R] = RGB_TL_RED;
	color[G] = RGB_TL_GRN;
	color[B] = RGB_TL_BLU;
	print(mat, row, col);
}

static inline void draw_head(matrix *mat,
	size_t row, size_t col)
{
	unsigned char *color;

	color = mat->rgb[index(mat, row, col)].color;
	color[R] = RGB_HD_RED;
	color[G] = RGB_HD_GRN;
	color[B] = RGB_HD_BLU;

	insert_code(mat, row, col);
	print(mat, row, col);
}

static inline void blend(matrix *mat,
	size_t row, size_t col)
{
	unsigned char *color;

	color = mat->rgb[index(mat, row, col)].color;
	color[R] = color[R] - (color[R] - RGB_BG_RED) / DECAY_MPLIER;
	color[G] = color[G] - (color[G] - RGB_BG_GRN) / DECAY_MPLIER;
	color[B] = color[B] - (color[B] - RGB_BG_BLU) / DECAY_MPLIER;
}

static inline void swap_col(matrix *mat, size_t i, size_t max)
{
	size_t j;

	mat->row[i] = 0;
	mat->rgb[i].color[PD] = 0;

	j = rand() % (mat->collen - max) + max;
	mat->col[i] = mat->col[i] ^ mat->col[j];
	mat->col[j] = mat->col[i] ^ mat->col[j];
	mat->col[i] = mat->col[i] ^ mat->col[j];
}

static inline uint8_t is_tail(matrix *mat,
	size_t row, size_t col)
{
	unsigned char *color;

	color = mat->rgb[index(mat, row, col)].color;
	return color[R] == RGB_TL_RED 
		&& color[G] == RGB_TL_GRN
		&& color[B] == RGB_TL_BLU;
}

static inline void glitch(matrix *mat)
{
	size_t i, j;
	
	i = rand() % (mat->rowlen - 1);
	j = rand() % mat->collen;
	if (mat->code[index(mat, i, j)] != ' ' 
		&& is_tail(mat, i, j)) {
		insert_code(mat, i, j);
		print(mat, i, j);
	}
}

static inline void shuffle(size_t *a, size_t n) 
{
	size_t i, j;

	for (i = n - 1; i > 0; i--) {
		j = rand() % (i + 1);
		a[j] = a[i] ^ a[j];
		a[i] = a[i] ^ a[j];
		a[j] = a[i] ^ a[j];
	}
}

static inline int init_matrix(matrix *mat,
	const struct winsize *ws)
{
	size_t i;

	mat->collen = ws->ws_col;
	mat->rowlen = ws->ws_row + 1;

	mat->code = realloc(mat->code, 
		sizeof mat->code[0] * mat->rowlen * mat->collen);
	if (!mat->code)
		return 0;

	mat->rgb = realloc(mat->rgb,
		sizeof mat->rgb[0] * mat->rowlen * mat->collen);
	if (!mat->rgb)
		return 0;

	mat->col = realloc(mat->col,
		sizeof mat->col[0] * mat->collen);
	if (!mat->col)
		return 0;

	mat->row = realloc(mat->row,
		sizeof mat->row[0] * mat->collen);
	if (!mat->row)
		return 0;

	for (i = 0; i < mat->collen; i++) {
		mat->row[i] = 0;
		mat->col[i] = i;
	}

	shuffle(mat->col, mat->collen);
	return 1;
}

static inline void destroy_matrix(matrix *mat)
{
	free(mat->code);
	free(mat->col);
	free(mat->row);
	free(mat->rgb);
}

static inline int init_term() 
{
	struct termios ta;

	if (tcgetattr(STDIN_FILENO, &ta) == 0) {
		ta.c_lflag &= ~ECHO;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &ta) == 0) {
			wprintf(L"\x1b[48;2;%d;%d;%dm", 
				RGB_BG_RED, RGB_BG_GRN, RGB_BG_BLU);
			wprintf(L"%s", ANSI_FONT_BOLD);
			wprintf(L"%s", ANSI_CRSR_HIDE);
			wprintf(L"%s", ANSI_CRSR_RESET);
			wprintf(L"%s", ANSI_SCRN_CLEAR);

			setvbuf(stdout, 0, _IOFBF, 0);
			return 1;
		}
	}
	return 0;
}

static inline void reset_term()
{
	struct termios ta;

	wprintf(L"%s", ANSI_FONT_RESET);
	wprintf(L"%s", ANSI_CRSR_SHOW);
	wprintf(L"%s", ANSI_SCRN_CLEAR);
	wprintf(L"%s", ANSI_CRSR_RESET);

	if (tcgetattr(STDIN_FILENO, &ta) == 0) {
		ta.c_lflag |= ECHO;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &ta) != 0)
			perror("reset_term()");
	}
	setvbuf(stdout, 0, _IOLBF, 0);
}

static inline void term_size(const struct winsize *ws)
{
	ioctl(STDOUT_FILENO, TIOCGWINSZ, ws);
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
	size_t i, j, n, nmax;

	setlocale(LC_CTYPE, "");

	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGQUIT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	srand(time(0));

	if (!init_term())
		return 1;

	term_size(&ws);

	mat = (matrix){0};
	if (!init_matrix(&mat, &ws)) {
		reset_term();
		return 1;
	}

	run = 1; 
	n = 1;  /* Used to ramp up the tracks from 1 to nmax to stagger the tracks. */
	nmax = mat.collen * RHO;  /* Number of rain tracks. */

	while (run) {
		for (i = 0; run && i < n; i++) {
			if (mat.row[i] == mat.rowlen) {
				recolor_head(&mat, mat.row[i] - 1, mat.col[i]);
				mat.row[i] = 0;
			}

			if (mat.rgb[i].color[PD] == 0) {
				if (mat.row[i] > 0)
					draw_tail(&mat, mat.row[i] - 1, mat.col[i]);
				draw_head(&mat, mat.row[i], mat.col[i]);
				if (mat.row[i] == mat.rowlen - 1)
					mat.rgb[i].color[PD] = 1;
				mat.row[i]++;
			} else if (mat.rgb[i].color[PD] > 0 &&
				mat.rgb[i].color[PD] <= DECAY_MPLIER) {
				blend(&mat, mat.row[i], mat.col[i]);
				print(&mat, mat.row[i], mat.col[i]);
				if (mat.row[i] == mat.rowlen - 1)
					mat.rgb[i].color[PD]++;
				mat.row[i]++;
			} else {
				delete_code(&mat, mat.row[i], mat.col[i]);
				if (mat.row[i] == mat.rowlen - 1)
					/* Track complete, start a new one. */
					swap_col(&mat, i, nmax); 
				else
					mat.row[i]++;
			}
			glitch(&mat);
		}

		if (n < nmax &&
			/* Track ramp up: add a new one when the first track 
			 * exceeds a random distance beyond 25% of the screen
			 * length. Do that until we reach the target density. */
			mat.row[n - 1] >= rand() % (int)(mat.rowlen * 0.25)) {
			mat.row[n++] = 0;
		}

		fflush(stdout);
		usleep(DELAY_US);
	}

	reset_term();
	destroy_matrix(&mat);
	return 0;
}


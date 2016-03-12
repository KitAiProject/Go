#define _CRT_SECURE_NO_WARNINGS     // sprintf is Err in VC++

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

double komi = 6.5;

#define B_SIZE     9
#define WIDTH      (B_SIZE + 2)
#define BOARD_MAX  (WIDTH * WIDTH)

int board[BOARD_MAX] = {
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};
int dir4[4] = { +1, +WIDTH, -1, -WIDTH };
int dir8[8] = { +1, +WIDTH, -1, -WIDTH, +1 + WIDTH, +WIDTH - 1, -1 - WIDTH, -WIDTH + 1 };

int ko_z;

#define MAX_MOVES 1000
int record[MAX_MOVES];
int moves = 0;

int all_playouts = 0;
int flag_test_playout = 0;

#define D_MAX 1000
int path[D_MAX];
int depth;

int get_z(int x, int y)
{
	return y*WIDTH + x;  // 1<= x <=9, 1<= y <=9
}

void init_board()
{
	int i, x, y;
	for (i = 0; i<BOARD_MAX; i++) board[i] = 3;
	for (y = 0; y<B_SIZE; y++) for (x = 0; x<B_SIZE; x++) board[get_z(x + 1, y + 1)] = 0;
	moves = 0;
	ko_z = 0;
}

int get81(int z)            // for display only
{
	int y = z / WIDTH;
	int x = z - y*WIDTH;    // 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
	if (z == 0) return 0;
	return x * 10 + y;        // x*100+y for 19x19
}

char *get_char_z(int z)
{
	int x, y, ax;
	static char buf[16];
	sprintf(buf, "pass");
	if (z == 0) return buf;
	y = z / WIDTH;
	x = z - y*WIDTH;
	ax = x - 1 + 'A';
	if (ax >= 'I') ax++;  // from 'A' to 'T', excluding 'I'
	sprintf(buf, "%c%d", ax, B_SIZE + 1 - y);
	return buf;
}

int flip_color(int col)
{
	return 3 - col;
}

void prt(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	//{ FILE *fp = fopen("out.txt","a"); if ( fp ) { vfprt( fp, fmt, ap ); fclose(fp); } }
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
void send_gtp(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

int check_board[BOARD_MAX];

void count_liberty_sub(int tz, int color, int *p_liberty, int *p_stone)
{
	int z, i;

	check_board[tz] = 1;     // search flag
	(*p_stone)++;            // number of stone
	for (i = 0; i<4; i++) {
		z = tz + dir4[i];
		if (check_board[z]) continue;
		if (board[z] == 0) {
			check_board[z] = 1;
			(*p_liberty)++;      // number of liberty
		}
		if (board[z] == color) count_liberty_sub(z, color, p_liberty, p_stone);
	}
}

void count_liberty(int tz, int *p_liberty, int *p_stone)
{
	int i;
	*p_liberty = *p_stone = 0;
	for (i = 0; i<BOARD_MAX; i++) check_board[i] = 0;
	count_liberty_sub(tz, board[tz], p_liberty, p_stone);
}

void take_stone(int tz, int color)
{
	int z, i;

	board[tz] = 0;
	for (i = 0; i<4; i++) {
		z = tz + dir4[i];
		if (board[z] == color) take_stone(z, color);
	}
}

const int FILL_EYE_ERR = 1;
const int FILL_EYE_OK = 0;

// put stone. success returns 0. in playout, fill_eye_err = 1.
int put_stone(int tz, int color, int fill_eye_err)
{
	int around[4][3];
	int un_col = flip_color(color);
	int space = 0;
	int wall = 0;
	int mycol_safe = 0;
	int capture_sum = 0;
	int ko_maybe = 0;
	int liberty, stone;
	int i;

	if (tz == 0) { ko_z = 0; return 0; }  // pass

	// count 4 neighbor's liberty and stones.
	for (i = 0; i<4; i++) {
		int z, c, liberty, stone;
		around[i][0] = around[i][1] = around[i][2] = 0;
		z = tz + dir4[i];
		c = board[z];  // color
		if (c == 0) space++;
		if (c == 3) wall++;
		if (c == 0 || c == 3) continue;
		count_liberty(z, &liberty, &stone);
		around[i][0] = liberty;
		around[i][1] = stone;
		around[i][2] = c;
		if (c == un_col && liberty == 1) { capture_sum += stone; ko_maybe = z; }
		if (c == color  && liberty >= 2) mycol_safe++;
	}

	if (capture_sum == 0 && space == 0 && mycol_safe == 0) return 1; // suicide
	if (tz == ko_z) return 2; // ko
	if (wall + mycol_safe == 4 && fill_eye_err) return 3; // eye
	if (board[tz] != 0) return 4;

	for (i = 0; i<4; i++) {
		int lib = around[i][0];
		int c = around[i][2];
		if (c == un_col && lib == 1 && board[tz + dir4[i]]) {
			take_stone(tz + dir4[i], un_col);
		}
	}

	board[tz] = color;

	count_liberty(tz, &liberty, &stone);
	if (capture_sum == 1 && stone == 1 && liberty == 1) ko_z = ko_maybe;
	else ko_z = 0;
	return 0;
}

void print_board()
{
	int x, y;
	const char *str[4] = { ".", "X", "O", "#" };

	prt("   ");
	//for (x=0;x<B_SIZE;x++) prt("%d",x+1);
	for (x = 0; x<B_SIZE; x++) prt("%c", 'A' + x + (x>7));
	prt("\n");
	for (y = 0; y<B_SIZE; y++) {
		//  prt("%2d ",y+1);
		prt("%2d ", B_SIZE - y);
		for (x = 0; x<B_SIZE; x++) {
			prt("%s", str[board[get_z(x + 1, y + 1)]]);
		}
		if (y == 4) prt("  ko_z=%d,moves=%d", get81(ko_z), moves);
		prt("\n");
	}
}

int count_score(int turn_color)
{
	int x, y, i;
	int score = 0, win;
	int black_area = 0, white_area = 0, black_sum, white_sum;
	int mk[4];
	int kind[3];

	kind[0] = kind[1] = kind[2] = 0;
	for (y = 0; y<B_SIZE; y++) for (x = 0; x<B_SIZE; x++) {
		int z = get_z(x + 1, y + 1);
		int c = board[z];
		kind[c]++;
		if (c != 0) continue;
		mk[1] = mk[2] = 0;
		for (i = 0; i<4; i++) mk[board[z + dir4[i]]]++;
		if (mk[1] && mk[2] == 0) black_area++;
		if (mk[2] && mk[1] == 0) white_area++;
	}

	black_sum = kind[1] + black_area;
	white_sum = kind[2] + white_area;
	score = black_sum - white_sum;

	win = 0;
	if (score - komi > 0) win = 1;

	if (turn_color == 2) win = -win;

	//prt("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
	//prt("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
	//prt("score=%d, win=%d\n",score, win);
	return win;
}


int get_best_uct(int color){
	int place;
	NODE *PointerNode;
	int BestPlace;
	int BestChildNum=-1,max=-10000;

	node_num = 0;//éËÇ™ïœÇÌÇÈÇ≤Ç∆Ç…èâä˙âªÇµÇƒÇ‡ëÂè‰ïv

	for (int i = 0; i, uct_loop;i++){
		int CopyBoard[BOARD_MAX];
		int KoPlaceCopy = ko_z;
		memcpy(CopyBoard, board, sizeof(board));

		search_uct(color, next);

		ko_z = KoPlaceCopy;
		memcpy(board,board_copy,sizeof(board));
	}
	
	PointerNode = &node[next];
	for (int i = 0; i < PointerNode->child_num;i++){
		CHILD *PChild = &PChild->child[i];
		if (PChild->games){
			BestChildNum = i;
			max = PChild->games;
		}
		printf("childnum:%2d,z=%2d,rate=%.4f,games=%3d,bonus=%.4f\n", i, get81(PChild->z), PChild->rate, PChild->games, PChild->bonus);
	}

	BestPlace = PChild->child[BestChildNum].z;
	printf("bestPlace=%d,rate=%.4f,max_games_try=%d,playouts=%d,nodes=%d\n",
		get81(best_z), PChild->child[best_i].rate, max, all_playouts, node_num);
	
	return place;
}




const int SEARCH_PRIMITIVE = 0;
const int SEARCH_UCT = 1;

void print_sgf()
{
	int i;
	prt("(;GM[1]SZ[%d]KM[%.1f]PB[]PW[]\n", B_SIZE, komi);
	for (i = 0; i<moves; i++) {
		int z = record[i];
		int y = z / WIDTH;
		int x = z - y*WIDTH;
		const char *sStone[2] = { "B", "W" };
		prt(";%s", sStone[i & 1]);
		if (z == 0) {
			prt("[]");
		}
		else {
			prt("[%c%c]", x + 'a' - 1, y + 'a' - 1);
		}
		if (((i + 1) % 10) == 0) prt("\n");
	}
	prt(")\n");
}
void addMove(int place, int color)
{
	int err = put_stone(color, color, FILL_EYE_OK);
	if (err != 0) {
		prt("Err!\n"); exit(0);

	}
}

#define STR_MAX 256
#define TOKEN_MAX 3

	int main()
	{
		int color = 1;
		int place;
		srand((unsigned)time(NULL));
		init_board();
		//expand_pattern3x3();
		for (;;) {
			print_board();
			printf("colorÇ1Ç©2Ç≈ì¸óÕÇµÇƒÇ≠ÇæÇ≥Ç¢\n");
			scanf("%d", &color);
			place = get_best_uct(color);
			addMove(place, color);
		}
		return 0;
	}

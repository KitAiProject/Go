#ifndef _CALC_H_
#define _CALC_H_

#define _CRT_SECURE_NO_WARNINGS     // sprintf is Err in VC++

double komi = 6.5;

#define B_SIZE     9
#define WIDTH      (B_SIZE + 2)
#define BOARD_MAX  (WIDTH * WIDTH)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <vector>   

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
const int ON = 1;
const int OFF = 0;
const int DAME = 4;
const int ILLEGAL = 5;
int ko_z;

#define MAX_MOVES 1000
int Record[MAX_MOVES];
int moves = 0;

int all_playouts = 0;
int flag_test_playout = 0;

#define D_MAX 1000
int path[D_MAX];
int depth;
int MoveNum;

int check_board[BOARD_MAX];

const int FILL_EYE_ERR = 1;
const int FILL_EYE_OK = 0;

typedef struct{
	int place;//position
	int games;//number of tried games
	double rate;//win rate
	int NextNode;
	double bonus;
}CHILD;

#define CHILD_SIZE  (B_SIZE*B_SIZE+1)
typedef struct{
	int CurrentChildNum;
	CHILD child[CHILD_SIZE];
	int ChildGamesSum;
}NODE;
#define NODE_MAX 20000
NODE NodeArray[NODE_MAX];
const int NODE_EMPTY = -1; // no next node
const int ILLEGAL_PLACE = -1; // illegal move
const int DEPTH_MAX = 1000;
int Path[DEPTH_MAX];
int NodeNum = 0;

int get_z(int x, int y);
void init_board();
int get81(int z);
char *get_char_z(int z);
int flip_color(int col);
void prt(const char *fmt, ...);
void send_gtp(const char *fmt, ...);
void count_liberty_sub(int tz, int color, int *p_liberty, int *p_stone);
void count_liberty(int tz, int *p_liberty, int *p_stone);
void take_stone(int tz, int color);
int FlipColor(int color);
int put_stone(int tz, int color, int fill_eye_err);
void print_board();
int count_score(int turn_color);
int EvalMatch(int place, int BeforePlace, int color);
//class PlaceInfo
int playout(int CurrentColor);
void add_child(NODE *PointerNode, int place, double bonus);
double GetBonus(int Place, int BeforePlace);
int CreateNode(int BeforePlace);
int SelectbestUCB(int NextNode);
double get_prob(int Place, int BeforePlace, int color);
int search_uct(int color, int NextNode);
int get_best_uct(int color);
void print_sgf();
void addMove(int place, int color);

int countDiscScore(int turn_color);
int DamePlaceSearch(int place, int color);
void MakeDamePlace(int color);
int SaikiDamePut(int AiPlayer, int color, int BeforePlace, int alpha, int beta);
int DamePut(int color);



#endif // _CALC_H_
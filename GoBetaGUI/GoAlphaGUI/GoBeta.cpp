#define _CRT_SECURE_NO_WARNINGS     // sprintf is Err in VC++

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <vector>

using namespace std;

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
int Record[MAX_MOVES];
int moves = 0;

int all_playouts = 0;
int flag_test_playout = 0;

#define D_MAX 1000
int path[D_MAX];
int depth;
int MoveNum;

const int ON = 1;
const int OFF = 0;
const int DAME = 4;
const int ILLEGAL = 5;

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

int FlipColor(int color)
{
	return 3 - color;
}
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


int count_score(int turn_color) {
	int x, y, i;
	int score = 0, win;
	int black_area = 0, white_area = 0, black_sum, white_sum;
	int mk[4];
	int kind[3];

	kind[0] = kind[1] = kind[2] = 0;
	for (y = 1; y <= B_SIZE; y++) for (x = 1; x <= B_SIZE; x++) {
		int z = get_z(x, y);
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
int EvalMatch(int place, int BeforePlace, int color){
	int point = rand() % 49;

	return point;
}

class PlaceInfo{
public:
	int place;
	int matchPoint;
};
int playout(int CurrentColor){
	int loop;
	const int loopMax = 1000;
	int BeforePlace = 0;
	int PlayoutMove[loopMax];
	int depth = 0;

	PlaceInfo BoardPutPlace[82];
	for (loop = 0; loop<loopMax; loop++){
		int index = 0;
		for (int y = 1; y <= B_SIZE; y++) {
			for (int x = 1; x <= B_SIZE; x++) {
				int place = get_z(x, y);
				if (board[place] != 0){
					continue;
				}
				BoardPutPlace[index].place = place;
				BoardPutPlace[index].matchPoint = EvalMatch(place, BeforePlace, CurrentColor);
				index++;
			}
		}

		int Max = 0, MaxPlaceIndex = 0;
		if (index == 0){
			MaxPlaceIndex = 0;
			BoardPutPlace[index].place = 0;
		}
		else {
			for (int i = 0; i < index; i++) {
				if (BoardPutPlace[i].matchPoint > Max) {
					Max = BoardPutPlace[i].matchPoint;
					MaxPlaceIndex = i;
				}
			}
		}
		int err;
		int BestPlace = BoardPutPlace[MaxPlaceIndex].place;
		err = put_stone(BestPlace, CurrentColor, FILL_EYE_ERR);

		PlayoutMove[depth++] = BestPlace;
		if (BestPlace == 0 && BeforePlace == 0) break;

		BeforePlace = BestPlace;
		CurrentColor = flip_color(CurrentColor);
	}

	return count_score(CurrentColor);
}

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
void add_child(NODE *PointerNode, int place, double bonus)
{
	int n = PointerNode->CurrentChildNum;
	PointerNode->child[n].place = place;
	PointerNode->child[n].games = 0;
	PointerNode->child[n].rate = 0;
	PointerNode->child[n].NextNode = NODE_EMPTY;
	PointerNode->child[n].bonus = bonus;  // from 0 to 10, good move has big bonus.
	PointerNode->CurrentChildNum++;
}

double GetBonus(int Place, int BeforePlace){
	int y = Place / WIDTH;
	int x = Place - y*WIDTH;
	int BeforeY = BeforePlace / WIDTH;
	int BeforeX = BeforePlace - BeforeY *WIDTH;
	int dx = abs(x - BeforeX);
	int dy = abs(y - BeforeY);
	double Bonus = 1.0;

	if (x == 1 || x == B_SIZE) Bonus = Bonus * 0.5;
	if (y == 1 || y == B_SIZE) Bonus = Bonus * 0.5;
	if (x == 2 || x == B_SIZE - 1) Bonus = Bonus * 0.8;
	if (y == 2 || y == B_SIZE - 1) Bonus = Bonus * 0.8;

	if (BeforePlace != 0){
		Bonus = Bonus + ((9 - dx)*(9 - dx) + (9 - dy)*(9 - dy)) / 150;
	}
	return Bonus;
}

int CreateNode(int BeforePlace){
	NODE *PointerNode;
	int x, y, Place;
	double bonus;

	if (NodeNum == NODE_MAX){
		printf("node over\n");
		exit(0);
	}
	PointerNode = &NodeArray[NodeNum];
	PointerNode->CurrentChildNum = 0;
	PointerNode->ChildGamesSum = 0;

	for (y = 1; y <= B_SIZE; y++){
		for (x = 1; x <= B_SIZE; x++){
			Place = get_z(x, y);
			if (board[Place] != 0) continue;//石が置いてあったら
			bonus = GetBonus(Place, BeforePlace);
			add_child(PointerNode, Place, bonus);
		}

	}
	add_child(PointerNode, 0, 0);//add pass move

	for (int i = 0; i<PointerNode->CurrentChildNum - 1; i++) {
		double max_b = PointerNode->child[i].bonus;
		int    max_i = i;
		CHILD tmp;
		for (int j = i + 1; j<PointerNode->CurrentChildNum; j++) {
			CHILD *c = &PointerNode->child[j];
			if (max_b < c->bonus) {
				max_b = c->bonus;
				max_i = j;
			}
		}
		if (max_i == i) continue;
		tmp = PointerNode->child[i];
		PointerNode->child[i] = PointerNode->child[max_i];
		PointerNode->child[max_i] = tmp;
	}

	NodeNum++;
	return NodeNum - 1;
}

int SelectbestUCB(int NextNode){
	NODE *PointerNode = &NodeArray[NextNode];
	int SelectChildIndex = -1;
	double MaxUCB = -10000;
	double Ucb = 0;
	int OkMoveNum = 0;
	int select;
	int BreakNum = (int)(1.0 + log(PointerNode->ChildGamesSum + 1.0) / log(1.8));
	//printf("ChildGamesSum:%d",PointerNode->ChildGamesSum);
	for (int i = 0; i<PointerNode->CurrentChildNum; i++){
		CHILD *PointerChild = &PointerNode->child[i];
		if (PointerChild->place == ILLEGAL_PLACE) continue;

		OkMoveNum++;

		if (OkMoveNum > BreakNum) break;

		if (PointerChild->games == 0){
			Ucb = 10000;
		}
		else{
			const double C = 1.0;
			const double B0 = 0.1;
			const double B1 = 100;
			double plus = B0 * log(1.0 + PointerChild->bonus) * sqrt(B1 / (B1 + PointerChild->games));
			Ucb = PointerChild->rate + C * sqrt(log((double)PointerNode->ChildGamesSum) / PointerChild->games) + plus;
		}
		if (Ucb > MaxUCB){
			MaxUCB = Ucb;
			select = i;
		}
	}
	if (select == -1){ exit(0); }

	return select;
}

double get_prob(int Place, int BeforePlace, int color){
	int y = Place / WIDTH;
	int x = Place - y*WIDTH;
	int BeforeY = BeforePlace / WIDTH;
	int BeforeX = BeforePlace - BeforeY *WIDTH;
	int dx = abs(x - BeforeX);
	int dy = abs(y - BeforeY);
	double Bonus = 1.0;

	if (x == 1 || x == B_SIZE) Bonus = Bonus * 0.5;
	if (y == 1 || y == B_SIZE) Bonus = Bonus * 0.5;
	if (x == 2 || x == B_SIZE - 1) Bonus = Bonus * 0.8;
	if (y == 2 || y == B_SIZE - 1) Bonus = Bonus * 0.8;

	if (BeforePlace != 0){
		Bonus = Bonus * (9 - dx) *(9 - dy) / 81;
	}
	return Bonus;
}

int countDiscScore(int turn_color) {
	int x, y, i;
	int score = 0;
	int black_area = 0, white_area = 0, black_sum, white_sum;
	int mk[4];
	int kind[3];

	kind[0] = kind[1] = kind[2] = 0;
	for (y = 1; y <= B_SIZE; y++) for (x = 1; x <= B_SIZE; x++) {
		int z = get_z(x, y);
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

	if (turn_color == 2){
		score = -score;
	}

	return score;
}

int FlagBoard[BOARD_MAX];

int DamePlaceSearch(int place, int color){//placeの最初の場所を自分で決めるようにプログラミングする
	vector<int> queue;
	queue.push_back(place);
	vector<int> array;
	int NowPlace = place;
	int flagColor = OFF;
	int flagOpColor = OFF;

	while (queue.size() != 0){
		NowPlace = queue[0];
		//printf("queue[0]:%d\n",queue[0]);
		queue.erase(queue.begin());
		for (int i = 0; i < 4; i++){
			if (FlagBoard[NowPlace + dir4[i]] == OFF && board[NowPlace + dir4[i]] == 0){
				//board[NowPlace + dir4[i]+dir4[j]]
				for (int j = 0; j<4; j++) {
					if (board[NowPlace + dir4[i] + dir4[j]] == color) {
						flagColor = ON;
					}
					if (board[NowPlace + dir4[i] + dir4[j]] == 3 - color) {
						flagOpColor = ON;
					}
				}
				FlagBoard[NowPlace + dir4[i]] = ON;
				queue.push_back(NowPlace + dir4[i]);
				array.push_back(NowPlace + dir4[i]);
			}
		}
	}

	if (flagColor == ON&&flagOpColor == ON){
		if (array.size() != 0){
			for (int i = 0; (int)i < (int)array.size(); i++){
				FlagBoard[array[i]] = DAME;
			}
		}
		
		//printBoard(FlagBoard);
		return 1;
	}
	//else{
	return 0;
	//}
}

void MakeDamePlace(int color){
	int NowPlace;


	for (int i = 0; i < BOARD_MAX; i++){
		FlagBoard[i] = OFF;
	}
	for (int y = 1; y <= B_SIZE; y++){
		for (int x = 1; x <= B_SIZE; x++){
			NowPlace = get_z(x, y);
			if (board[NowPlace] != 0){
				FlagBoard[NowPlace] = ILLEGAL;
			}
		}
	}

	for (int x = 1; x <= B_SIZE; x++){
		for (int y = 1; y <= B_SIZE; y++){
			NowPlace = get_z(x, y);
			if (board[NowPlace] == 0 && FlagBoard[NowPlace] == OFF){
				DamePlaceSearch(NowPlace, color);
			}
		}
	}
}

int SaikiDamePut(int AiPlayer, int color, int BeforePlace, int alpha, int beta){
	int score, score_max = -9000, score_min = 9000;
	int CopyBoard[BOARD_MAX];
	int exist = OFF;

	memcpy(CopyBoard, board, sizeof(board));

	for (int y = 1; y <= B_SIZE; y++){
		for (int x = 1; x <= B_SIZE; x++){
			int place;
			place = get_z(x, y);
			//printBoard(board);
			if (FlagBoard[place] == DAME){
				if (put_stone(place, color, FILL_EYE_ERR) == 0) {
					//printBoard(board);
					//printBoard(board);
					exist = ON;
					score = SaikiDamePut(AiPlayer, 3 - color, place, alpha, beta);

					memcpy(board, CopyBoard, sizeof(board));

					if (color == AiPlayer) {
						if (score > score_max) {//score_max が　alpha
							score_max = score;
							alpha = score;
							if (alpha >= beta) {
								return beta;
							}
						}
					}
					else {
						if (score < score_min) {
							score_min = score;
							beta = score;
							if (beta <= alpha) {
								return alpha;
							}
						}
					}
				}
			}
		}
	}

	if (BeforePlace == 0 && exist == OFF){//END GAME
		//print_board();
		return countDiscScore(color);
	}

	if (exist == OFF){
		return SaikiDamePut(AiPlayer, 3 - color, 0, -beta, -alpha);
	}

	if (color == AiPlayer){
		return score_max;
	}
	else{
		return score_min;
	}
}
int DamePut(int color){
	int CopyBoard[BOARD_MAX];
	int place, BestPlace = 0;
	int MaxScore = -10000, Score;
	int alpha = -10000, beta = 10000;
	MakeDamePlace(color);

	memcpy(CopyBoard, board, sizeof(board));
	for (int y = 1; y <= B_SIZE; y++){
		for (int x = 1; x <= B_SIZE; x++){
			place = get_z(x, y);
			if (FlagBoard[place] == DAME) {

				if (put_stone(place, color, FILL_EYE_ERR) == 0) {
					//printBoard(board);
					Score = SaikiDamePut(color, 3 - color, place, alpha, beta);
					memcpy(board, CopyBoard, sizeof(board));

					if (Score >= MaxScore) {
						MaxScore = Score;
						BestPlace = place;
					}
				}
			}
		}
	}

	return BestPlace;
}

int search_uct(int color, int NextNode){
	NODE *PointerNode = &NodeArray[NextNode];
	CHILD *BestChild = NULL;
	int SelectChildIndex;
	int place, err, win;
	while (1){
		SelectChildIndex = SelectbestUCB(NextNode);
		BestChild = &PointerNode->child[SelectChildIndex];//ノードの中で一つ良いchildを選んでノードを下っていく
		place = BestChild->place;
		err = put_stone(place, color, FILL_EYE_ERR);
		if (err == 0) break;
		BestChild->place = ILLEGAL_PLACE;
	}
	Path[depth++] = BestChild->place;

	if (BestChild->games == 0 || depth == DEPTH_MAX || (BestChild->place == 0 && depth >= 2 && path[depth - 2] == 0)){
		win = -playout(FlipColor(color));
	}
	else {
		if (BestChild->NextNode == NODE_EMPTY) {
			BestChild->NextNode = CreateNode(BestChild->place);
		}
		win = -search_uct(FlipColor(color), BestChild->NextNode);
	}
	BestChild->rate = (BestChild->rate * BestChild->games + win) / (BestChild->games + 1);
	BestChild->games++;
	PointerNode->ChildGamesSum++;

	return win;

}
const int UctLoop = 700;
int get_best_uct(int color){
	int  next;
	NODE *PointerNode;
	int BestPlace = 0;
	int BestChildNum = -1, max = -10000;
	int BeforePlace = 0;
	if (MoveNum > 0){ BeforePlace = Record[MoveNum - 1]; }
	NodeNum = 0;//手が変わるごとに初期化しても大丈夫
	next = CreateNode(BeforePlace);
	for (int i = 0; i<UctLoop; i++){
		int CopyBoard[BOARD_MAX];
		int KoPlaceCopy = ko_z;
		memcpy(CopyBoard, board, sizeof(board));

		search_uct(color, next);

		ko_z = KoPlaceCopy;
		memcpy(board, CopyBoard, sizeof(board));
	}

	PointerNode = &NodeArray[next];
	for (int i = 0; i < PointerNode->CurrentChildNum; i++){
		CHILD *PChild = &PointerNode->child[i];
		if (PChild->games>max){
			BestChildNum = i;
			max = PChild->games;
		}
		prt("childnum:%2d,z=%2d,rate=%.4f,games=%3d,bonus=%.4f\n", i, get81(PChild->place), PChild->rate, PChild->games, PChild->bonus);
	}

	BestPlace = PointerNode->child[BestChildNum].place;
	prt("bestPlace=%d,rate=%.4f,max_games_try=%d,playouts=%d,nodes=%d\n",
		get81(BestChildNum), PointerNode->child[BestChildNum].rate, max, all_playouts, NodeNum);

	if (moves>50 && PointerNode->child[BestChildNum].rate<0.03 && color == 1 || moves>50 && PointerNode->child[BestChildNum].rate>-0.03 && color == 2){
		BestPlace = DamePut(color);

		//ダメ詰めアルゴリズム作成　再帰でalphabeta
	}

	return BestPlace;
}




const int SEARCH_PRIMITIVE = 0;
const int SEARCH_UCT = 1;

void print_sgf()
{
	int i;
	prt("(;GM[1]SZ[%d]KM[%.1f]PB[]PW[]\n", B_SIZE, komi);
	for (i = 0; i<moves; i++) {
		int z = Record[i];
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
	int err = put_stone(place, color, FILL_EYE_OK);
	if (err != 0) {
		prt("Err!\n");
		exit(0);
	}
	Record[moves] = place;
}

#define STR_MAX 256
#define TOKEN_MAX 3


int main()
{
	char str[STR_MAX];
	char sa[TOKEN_MAX][STR_MAX];
	char seps[] = " ";
	char *token;
	int x, y, z, ax, count;

	srand((unsigned)time(NULL));
	//srand( 0 );  // rand() generates same random number.
	init_board();
	//expand_pattern3x3();

	//if (0) { selfplay(); return 0; }
	//if (0) { test_playout(); return 0; }

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	for (;;) {
		if (fgets(str, STR_MAX, stdin) == NULL) break;
		//  prt("gtp<-%s",str);
		count = 0;
		token = strtok(str, seps);
		while (token != NULL) {
			strcpy(sa[count], token);
			count++;
			if (count == TOKEN_MAX) break;
			token = strtok(NULL, seps);
		}

		if (strstr(sa[0], "boardsize")) {
			//    int new_board_size = atoi( sa[1] );
			send_gtp("= \n\n");
		}
		else if (strstr(sa[0], "clear_board")) {
			init_board();
			send_gtp("= \n\n");
		}
		else if (strstr(sa[0], "quit")) {
			break;
		}
		else if (strstr(sa[0], "protocol_version")) {
			send_gtp("= 2\n\n");
		}
		else if (strstr(sa[0], "name")) {
			send_gtp("= your_program_name\n\n");
		}
		else if (strstr(sa[0], "version")) {
			send_gtp("= 0.0.1\n\n");
		}
		else if (strstr(sa[0], "list_commands")) {
			send_gtp("= boardsize\nclear_board\nquit\nprotocol_version\n"
				"name\nversion\nlist_commands\nkomi\ngenmove\nplay\n\n");
		}
		else if (strstr(sa[0], "komi")) {
			komi = atof(sa[1]);
			send_gtp("= \n\n");
		}
		else if (strstr(sa[0], "genmove")) {
			int color = 1;
			if (tolower(sa[1][0]) == 'w') color = 2;

			z = get_best_uct(color);
			addMove(z, color);
			send_gtp("= %s\n\n", get_char_z(z));
		}
		else if (strstr(sa[0], "play")) {  // "play b c4", "play w d17"
			int color = 1;
			if (tolower(sa[1][0]) == 'w') color = 2;
			ax = tolower(sa[2][0]);
			x = ax - 'a' + 1;
			if (ax >= 'i') x--;
			y = atoi(&sa[2][1]);
			z = get_z(x, B_SIZE - y + 1);
			if (tolower(sa[2][0]) == 'p') z = 0;  // pass
			addMove(z, color);
			send_gtp("= \n\n");
		}
		else {
			send_gtp("? unknown_command\n\n");
		}
	}
	return 0;
}
/*add_childのソートをもっと早くする
*get_bonus考える
*石の置いてある場所に打ってしまう。
*/
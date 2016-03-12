//#pragma comment(linker, "/STACK:90000000000")
//#pragma comment(linker, "/HEAP:90000000000")
#include <stdio.h>
#include <vector>
#include <string.h>
using namespace std;
const int WIDTH = 11;
const int B_SIZE = 9;
#define BOARD_MAX  (WIDTH * WIDTH)
const int ON = 1;
const int OFF = 0;
const int DAME = 4;
const int ILLEGAL = 5;
int ko_z;
int moves = 0;
int dir4[4] = { +1, +WIDTH, -1, -WIDTH };
const int FILL_EYE_ERR = 1;
const double komi = 5.5;
int board[BOARD_MAX] = {
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3,
	3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 3,
	3, 1, 1, 1, 0, 0, 0, 0, 0, 1, 3,
	3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 3,
	3, 0, 2, 2, 2, 2, 2, 2, 2, 2, 3,
	3, 0, 2, 2, 2, 2, 2, 2, 2, 2, 3,
	3, 0, 2, 2, 2, 0, 0, 0, 0, 2, 3,
	3, 2, 2, 2, 0, 0, 0, 0, 0, 2, 3,
	3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};
int check_board[BOARD_MAX];
int flip_color(int col)
{
	return 3 - col;
}

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
int get_z(int x, int y)
{
	return y*WIDTH + x;  // 1<= x <=9, 1<= y <=9
}
void print_board()
{
	int x, y;
	const char *str[4] = { ".", "X", "O", "#" };

	printf("   ");
	//for (x=0;x<B_SIZE;x++) printf("%d",x+1);
	for (x = 0; x<B_SIZE; x++) printf("%c", 'A' + x + (x>7));
	printf("\n");
	for (y = 0; y<B_SIZE; y++) {
		//  printf("%2d ",y+1);
		printf("%2d ", B_SIZE - y);
		for (x = 0; x<B_SIZE; x++) {
			printf("%s", str[board[get_z(x + 1, y + 1)]]);
		}

		printf("\n");
	}
}
void printBoard(int board[]){
	int place;
	for (int y = 0; y<WIDTH; y++){
		for (int x = 0; x<WIDTH; x++){
			place = get_z(x, y);
			if (board[place] == 1)
			{
				printf("○");
			}
			else if (board[place] == 2)
			{
				printf("●");
			}
			else if (board[place] == 3){
				printf("×");
			}
			else if (board[place] == DAME){
				printf("△");
			}
			else{
				printf("＋");
			}
		}
		printf("\n");
	}
}
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
int countDiscScore(int turn_color) {
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

	if (turn_color==2){
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
	int IllegalPlace;
	int Place;

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
		for (int i = 0; i<array.size(); i++){
			FlagBoard[array[i]] = DAME;
		}
		printf("ダメです！");
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

int main(void){
	int color = 1;
	int place;
	int flag;
	int err;

	while (1) {
		place = DamePut(color);
		printBoard(FlagBoard);
		err = put_stone(place, color, FILL_EYE_ERR);
		if (place == 0){
			break;
		}
		printBoard(board);
		color = 3 - color;
	}
	put_stone(get_z(5,5),color,FILL_EYE_ERR);
	print_board();
}
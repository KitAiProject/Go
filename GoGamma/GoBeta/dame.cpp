#include "GoBeta.h"

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
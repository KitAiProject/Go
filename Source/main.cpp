#include <stdio.h>
#include <vector>
#include <stdlib.h>
using namespace std;
const int WIDTH = 11;
const int B_SIZE = 9;
#define BOARD_MAX  (WIDTH * WIDTH)
const int ON =1;
const int OFF =0;
const int DAME =4;
const int ILLEGAL =5;
int ko_z;
int moves=0;
int dir4[4] = { +1, +WIDTH, -1, -WIDTH };
const int FILL_EYE_ERR = 1;
int board[BOARD_MAX] = {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3,
        3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 3,
        3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 3,
        3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3,
        3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
        3, 2, 0, 0, 0, 0, 0, 0, 0, 2, 3,
        3, 2, 0, 0, 0, 0, 0, 0, 0, 2, 3,
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
    for(int y=0;y<WIDTH;y++){
        for(int x=0;x<WIDTH;x++){
            place=get_z(x,y);
            if (board[place]==1)
            {
                printf("○");
            }
            else if (board[place]==2)
            {
                printf("●");
            }
            else if(board[place]==3){
                printf("×");
            }
            else{
                printf("□");
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

int FlagBoard[BOARD_MAX];

int DamePlaceSearch(int place,int color){//placeの最初の場所を自分で決めるようにプログラミングする
    vector<int> queue;
    queue.push_back(place);
    int NowPlace=place;
    int flagColor=OFF;
    int flagOpColor = OFF;
    int IllegalPlace;
    int Place;
    for (int i = 0; i < BOARD_MAX; i++){
        FlagBoard[i] = OFF;
    }
    for(int y=1;y<=B_SIZE;y++){
        for(int x=1;x<=B_SIZE;x++){
            Place=get_z(x,y);
            if(board[Place]!=0){
                FlagBoard[Place]=ILLEGAL;
            }
        }
    }
    while (queue.size()!=0){
        NowPlace = queue[0];
        printf("queue[0]:%d\n",queue[0]);
        queue.erase(queue.begin());
        for (int i = 0; i < 4;i++){
            if (FlagBoard[NowPlace+dir4[i]] == OFF && board[NowPlace+dir4[i]]==0){
                if(board[NowPlace+dir4[i]]==color){
                    flagColor = ON;
                }
                if(board[NowPlace+dir4[i]]==3-color){
                    flagOpColor = ON;
                }

                FlagBoard[NowPlace+dir4[i]] = ON;
                queue.push_back(NowPlace+dir4[i]);
            }
        }
    }

    if(flagColor==ON&&flagOpColor==ON){
        return 1;
    }
    else{
        return 0;
    }
}

void MakeDamePlace(int color){
    int NowPlace;
    for(int x=1;x<=B_SIZE;x++){
        for(int y=1;y<=B_SIZE;y++){
            NowPlace=get_z(x,y);
            if (board[NowPlace]==0&&FlagBoard[NowPlace]==OFF){
                DamePlaceSearch(NowPlace,color);
            }
        }
    }
}

int SaikiDamePut(int color){
    return 0;
}
int DamePut(int color){
    int CopyBoard[BOARD_MAX];
    int place,BestPlace=0;
    int MaxScore = -10000,Score;

    MakeDamePlace(color);

    memcpy(CopyBoard, board, sizeof(board));
    for (int y = 1; y <= B_SIZE;y++){
        for (int x = 1; x <= B_SIZE;x++){

            place = get_z(x, y);
            if (FlagBoard[place]==DAME){
                int err = put_stone(place, color, FILL_EYE_ERR);
                Score = SaikiDamePut(3 - color);
                memcpy(board, CopyBoard, sizeof(board));

                if (Score > MaxScore){
                    MaxScore = Score;
                    BestPlace = get_z(x, y);
                }
            }
        }
    }

    return BestPlace;
}

int main(void){
    int color=1;
    int place;
    int flag;
    int x,y;
    scanf("%d",&x);
    scanf("%d",&y);
    MakeDamePlace(color);
    printBoard(FlagBoard);
}
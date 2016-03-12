#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>


double komi = 6.5;

#define B_SIZE     19
#define WIDTH      (B_SIZE + 2)
#define BOARD_MAX  (WIDTH * WIDTH)

int board[BOARD_MAX] = {
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
};

int dir4[4] = { +1,+WIDTH,-1,-WIDTH };
int dir8[8] = { +1,+WIDTH,-1,-WIDTH,  +1+WIDTH, +WIDTH-1, -1-WIDTH, -WIDTH+1 };

int ko_z;

#define MAX_MOVES 1000
int record[MAX_MOVES];
int moves = 0;

int all_playouts = 0;
int flag_test_playout = 0;

#define D_MAX 1000
int path[D_MAX];
int depth;


int get_z(int x,int y)
{
  return y*WIDTH + x;  // 1<= x <=9, 1<= y <=9
}

int get81(int z)            // for display only
{
  int y = z / WIDTH;
  int x = z - y*WIDTH;    // 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
  if ( z==0 ) return 0;
  return x*10 + y;        // x*100+y for 19x19
}

// don't call twice in same sentence. like prt("z0=%s,z1=%s\n",get_char_z(z0),get_char_z(z1));
char *get_char_z(int z)
{
  int x,y,ax;
  static char buf[16];
  sprintf(buf,"pass");
  if ( z==0 ) return buf;
  y = z / WIDTH;
  x = z - y*WIDTH;
  ax = x-1+'A';
  if ( ax >= 'I' ) ax++;  // from 'A' to 'T', excluding 'I'
  sprintf(buf,"%c%d",ax,B_SIZE+1 - y);
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
  vfprintf( stderr, fmt, ap );
  va_end(ap);
}
void send_gtp(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf( stdout, fmt, ap );
  va_end(ap);
}

int check_board[BOARD_MAX];

void count_liberty_sub(int tz, int color, int *p_liberty, int *p_stone)
{
  int z,i;

  check_board[tz] = 1;     // search flag
  (*p_stone)++;            // number of stone
  for (i=0;i<4;i++) {
    z = tz + dir4[i];
    if ( check_board[z] ) continue;
    if ( board[z] == 0 ) {
      check_board[z] = 1;
      (*p_liberty)++;      // number of liberty
    }
    if ( board[z] == color ) count_liberty_sub(z, color, p_liberty, p_stone);
  }
}

void count_liberty(int tz, int *p_liberty, int *p_stone)
{
  int i;
  *p_liberty = *p_stone = 0;
  for (i=0;i<BOARD_MAX;i++) check_board[i] = 0;
  count_liberty_sub(tz, board[tz], p_liberty, p_stone);
}

void take_stone(int tz,int color)
{
  int z,i;
  
  board[tz] = 0;
  for (i=0;i<4;i++) {
    z = tz + dir4[i];
    if ( board[z] == color ) take_stone(z,color);
  }
}

const int FILL_EYE_ERR = 1;
const int FILL_EYE_OK  = 0;

// put stone. success returns 0. in playout, fill_eye_err = 1.
int put_stone(int tz, int color, int fill_eye_err)
{
  int around[4][3];
  int un_col = flip_color(color);
  int space = 0;
  int wall  = 0;
  int mycol_safe = 0;
  int capture_sum = 0;
  int ko_maybe = 0;
  int liberty, stone;
  int i;

  if ( tz == 0 ) { ko_z = 0; return 0; }  // pass

  // count 4 neighbor's liberty and stones.
  for (i=0;i<4;i++) {
    int z, c, liberty, stone;
    around[i][0] = around[i][1] = around[i][2] = 0;
    z = tz+dir4[i];
    c = board[z];  // color
    if ( c == 0 ) space++;
    if ( c == 3 ) wall++;
    if ( c == 0 || c == 3 ) continue;
    count_liberty(z, &liberty, &stone);
    around[i][0] = liberty;
    around[i][1] = stone;
    around[i][2] = c;
    if ( c == un_col && liberty == 1 ) { capture_sum += stone; ko_maybe = z; }
    if ( c == color  && liberty >= 2 ) mycol_safe++;
  }

  if ( capture_sum == 0 && space == 0 && mycol_safe == 0 ) return 1; // suicide
  if ( tz == ko_z                                        ) return 2; // ko
  if ( wall + mycol_safe == 4 && fill_eye_err            ) return 3; // eye
  if ( board[tz] != 0                                    ) return 4;

  for (i=0;i<4;i++) {
    int lib = around[i][0];
    int c   = around[i][2];
    if ( c == un_col && lib == 1 && board[tz+dir4[i]] ) {
      take_stone(tz+dir4[i],un_col);
    }
  }

  board[tz] = color;

  count_liberty(tz, &liberty, &stone);
  if ( capture_sum == 1 && stone == 1 && liberty == 1 ) ko_z = ko_maybe;
  else ko_z = 0;
  return 0;
}

void print_board()
{
  int x,y;
  const char *str[4] = { "Å{","Åú","Åõ","#" };

  prt("   ");
//for (x=0;x<B_SIZE;x++) prt("%d",x+1);
  for (x=0;x<B_SIZE;x++) prt("%c",'A'+x+(x>7));
  prt("\n");
  for (y=0;y<B_SIZE;y++) {
//  prt("%2d ",y+1);
    prt("%2d ",B_SIZE-y);
    for (x=0;x<B_SIZE;x++) {
      prt("%s",str[board[get_z(x+1,y+1)]]);
    }
    if ( y==4 ) prt("  ko_z=%d,moves=%d",get81(ko_z), moves);
    prt("\n");
  }
}

#define EXPAND_PATTERN_MAX  (8*100)
int e_pat_num = 0;
int e_pat[EXPAND_PATTERN_MAX][9];      // rotate and flip pattern
int e_pat_bit[EXPAND_PATTERN_MAX][2];  // [0] ...pattern, [1]...mask
int dir_3x3[9] = { -WIDTH-1, -WIDTH, -WIDTH+1,  -1, 0, +1,  +WIDTH-1, +WIDTH, +WIDTH+1 };


int count_score(int turn_color)
{
  int x,y,i;
  int score = 0, win;
  int black_area = 0, white_area = 0, black_sum, white_sum;
  int mk[4];
  int kind[3];

  kind[0] = kind[1] = kind[2] = 0;
  for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) {
    int z = get_z(x+1,y+1);
    int c = board[z];
    kind[c]++;
    if ( c != 0 ) continue;
    mk[1] = mk[2] = 0;  
    for (i=0;i<4;i++) mk[ board[z+dir4[i]] ]++;
    if ( mk[1] && mk[2]==0 ) black_area++;
    if ( mk[2] && mk[1]==0 ) white_area++;
  }
 
  black_sum = kind[1] + black_area;
  white_sum = kind[2] + white_area;
  score = black_sum - white_sum;

  win = 0;
  if ( score - komi > 0 ) win = 1;

  if ( turn_color == 2 ) win = -win; 

//prt("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
//prt("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
//prt("score=%d, win=%d\n",score, win);
  return win;
}


// following are for UCT

typedef struct {
  int z;        // move position
  int games;    // number of games
  double rate;  // winrate
  int next;     // next node
  double bonus; // shape bonus
} CHILD;

#define CHILD_SIZE  (B_SIZE*B_SIZE+1)  // +1 for PASS


#define NODE_MAX 
int node_num = 0;
const int NODE_EMPTY = -1; // no next node
const int ILLEGAL_Z  = -1; // illegal move





int uct_loop = 1000;  // number of uct loop


void init_board()
{
  int i,x,y;
  for (i=0;i<BOARD_MAX;i++) board[i] = 3;
  for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) board[get_z(x+1,y+1)] = 0;
  moves = 0;
  ko_z = 0;
}

void add_moves(int z, int color)
{
  int err = put_stone(z, color, FILL_EYE_OK);
  if ( err != 0 ) { prt("Err!\n"); exit(0); }
  record[moves] = z;
  moves++;
  print_board();
}

const int SEARCH_PRIMITIVE = 0;
const int SEARCH_UCT       = 1;



// print SGF game record
void print_sgf()
{
  int i;
  prt("(;GM[1]SZ[%d]KM[%.1f]PB[]PW[]\n",B_SIZE,komi); 
  for (i=0; i<moves; i++) {
    int z = record[i];
    int y = z / WIDTH;
    int x = z - y*WIDTH;
    const char *sStone[2] = { "B", "W" };
    prt(";%s",sStone[i&1]);
    if ( z == 0 ) {
      prt("[]");
    } else {
      prt("[%c%c]",x+'a'-1, y+'a'-1 );
    }
    if ( ((i+1)%10)==0 ) prt("\n");
  }
  prt(")\n");
}




#define STR_MAX 256
#define TOKEN_MAX 3
#define MAX 2048
int main(void)
{
	FILE *fp;
	char LineData[MAX];
	char *sp=NULL;
	char *data[MAX];
	char *matchToken=";B[";
	char result[MAX];
	char *chp[MAX];
	int i=0,num=0,color=1;
	int kifuNum=0;
	if((fp=fopen("2015-05-01-9.sgf","r"))==NULL){
		printf("file open err!");
		exit(0);
	}
	
	while(fgets(LineData,MAX,fp)!=NULL){//LineDataÇ∆MAXÇÃîzóÒÇÃëÂÇ´Ç≥Ç™ìôÇµÇ≠Ç»Ç¢Ç∆Ç¢ÇØÇ»Ç¢ÅI
		sp=strstr(LineData,"RE[");
		if(sp!= NULL ){
			strcpy(result,LineData);
			//printf("%s",result);
			sp=NULL;
		}
		data[i]=LineData;
		i++;
	}
	
	kifuNum=i-1;
	
	fclose(fp);
	
	if(*(data[kifuNum]+1)=='B'){
		char *ch = "b";
		char *tp;

		tp=strtok(data[kifuNum],";");
		chp[num]=tp;
		num++;
		//printf("ch->num: %d  a:%d \n",(int)*ch,'a');
		//printf( "%dî‘ñ⁄Ç…î≠å©ÇµÇ‹ÇµÇΩ\n",pointer - data[i-1] );
		
		while(tp!=NULL){
			tp=strtok(NULL,";");
			if(tp!=NULL){
				chp[num]=tp;
				num++;
			}
		}
	}
	else{
		printf( "î≠å©èoóàÇ‹ÇπÇÒ\n");
	}
	
	for(i=0;i<num;i++){
		int z;
		char BW;
		printf("%s\n",chp[i]);
		//printf("%c\n",*(chp[i]+2));
		//printf("%d\n",(int)*(chp[i]+2)-96);
		BW = *chp[i];
		printf("%c\n",BW);
		if(BW=='B'){
			color=1;
		}
		else{
			color=2;
		}
		z=get_z((int)*(chp[i]+2)-96,(int)*(chp[i]+3)-96);
		put_stone(z,color,1);
		print_board();
	}
	
	
	
	return 0;
}
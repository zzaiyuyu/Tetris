#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "./keyboard/keyboard/keyboard.h"

//前景色背景色，宽高，左上边界
int FC =  5;
#define BC 7
#define Weight 10
#define Height 20
#define MAR_LEFT 30
#define MAR_TOP	10
struct data {
	int x;
	int y;
};

//贯穿整个游戏的位置信息,当前图形的左上角点
struct data pos = {.x=3, .y=5};
int shapeNum = 0;//当前图案的序号
int nextBuf = 1;//下一个要出现的图案序号

struct shape {
	int s[5][5];
};

//用来判断背景数组是否有图案
int background[Height][Weight];
//用来保存背景的每种图形的颜色
int backColor[Height][Weight];
//用5x5数组模拟图案
struct shape shape_arr[7] = {
	{ 0,0,0,0,0, 0,0,1,0,0, 0,1,1,1,0, 0,0,0,0,0, 0,0,0,0,0},
	{ 0,0,0,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,0,0,0},
	{ 0,0,0,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,1,0, 0,0,0,0,0},
	{ 0,0,0,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,0,0, 0,0,0,0,0},
	{ 0,0,0,0,0, 0,1,1,0,0, 0,1,1,0,0, 0,0,0,0,0, 0,0,0,0,0},
	{ 0,0,0,0,0, 0,1,1,0,0, 0,0,1,1,0, 0,0,0,0,0, 0,0,0,0,0},
	{ 0,0,0,0,0, 0,0,1,1,0, 0,1,1,0,0, 0,0,0,0,0, 0,0,0,0,0} 
};

//利用VT100终端控制
//在坐标位置打印一个块
void draw_element(int x, int y, int c)
{
	//设置光标位置
	//设置前景，背景色
	//画[]
	x *= 2;
	x += MAR_LEFT;
	y += MAR_TOP;
	printf("\033[%d;%dH", y, x);
	printf("\033[3%dm\033[4%dm", c, c);
	printf("[]");
	//消去光标
	printf("\033[?25l");
	printf("\033[0m");
	fflush(stdout);
}

//图形不能下落时，图案保存到背景数组
void set_back(struct data *t, struct shape p, int color)
{
	for (int i=0; i<5; i++){
		for (int j=0; j<5; j++) {
			if ( p.s[i][j] != 0 ){
				background[t->y+i][t->x+j] = 1;
				backColor[t->y+i][t->x+j] = color;
			}
		}
	}
}

//画一个完整的图形,x是列坐标
void draw_shape(int x, int y, struct shape p, int c)
{
	//打印5x5数组
	for (int i=0; i<5; i++) {
		for (int j=0; j<5; j++) {
			if (p.s[i][j] != 0 )
				draw_element(x+j, y+i, c);
		}

	}
}
//画整个游戏界面
void draw_bk( void )
{
	//主游戏界面
	for (int i=0; i<Height; i++) {
		for (int j=0; j<Weight; j++) {
			if ( background[i][j] == 0 )
				draw_element(j, i, BC);
			else
				draw_element(j, i, backColor[i][j]);
		}
	}
	//下一个图案区域
	for(int i=0; i<5; i++){
		for(int j=0; j<5; j++){
			draw_element(10+j, 5+i, BC);
		}
	}
	draw_shape(10, 5, shape_arr[nextBuf], FC);
}

//判断合法移动
int can_move(int x, int y, struct shape p)
{
	//遍历图案,对每一个点检测三个边界和背景

	for (int i=0; i<5; i++) {
		for (int j=0; j<5; j++) {
			//数组透明部分不考虑
			if (p.s[i][j] == 0) 
				continue;
			if ( y+i >= Height )
				return 0;
			if ( x+j < 0 )
				return 0;
			if ( x+j >= Weight )
				return 0;
			//位置已经有块
			if (background[y+i][x+j] != 0 )
				return 0;
		}
	}
	return 1;
}
//消除,从上到下判断，如果有行满，全部下移一行
void clean_line( )
{
	int i;
	int total;
	for (i=0; i<Height; i++) {
		total = 0;
		for (int j=0; j<Weight; j++) {
			if ( background[i][j] != 0 )
				total++;
		}
		//第i行满了,把第i-1行移到i行，清空第0行
		if ( total == Weight ) {
			for (int k=i; k>0; k--) {
				memcpy(background[k], background[k-1], sizeof(background[k]));
				memcpy(backColor[k], backColor[k-1], sizeof(backColor[k]));
			}
			memset(background[0], 0x00, sizeof(background[shapeNum]));
		}
	}
}
//设定一个检测线，如果该行全满失败
void over()
{
	int total = 0;
	for (int i=0; i<Weight; i++){
		if ( background[3][i] != 0 ) {
			printf("\033[u");
			printf("\033[2J");
			printf("\033[?25h你输了\n");
			recover_keyboard();
			exit(0);
		}
	}
}
//定时下落，核心函数*****************
void timer(struct data *p)
{
	//将本次位置清空	
	draw_shape(p->x, p->y, shape_arr[shapeNum], BC);
	//位置加1
	//画下次一前景
	if (can_move(p->x, p->y+1, shape_arr[shapeNum]) ) {
		p->y++;
		draw_shape(p->x, p->y, shape_arr[shapeNum], FC);
	} else { 
		//下次位置不合法，则绘制本次前景,保存前景,检查结束和消除
		set_back(p, shape_arr[shapeNum], FC);
		draw_shape(p->x, p->y, shape_arr[shapeNum], FC);
		clean_line();
		over();
		p->y = 1;
		p->x = 3;
		int newRand = rand()%7;
		FC = newRand;
		shapeNum = nextBuf;
		nextBuf = newRand;
		draw_bk();
	}
}

struct shape trun_90(struct shape p)
{
	struct shape tmp;
	for (int i=0; i<5; i++) {
		for (int j=0; j<5; j++) {
			tmp.s[i][j] =  p.s[4-j][i];
		}
	}
	return tmp;
}

//处理按键
void is_key(struct data *p)
{
	int n = get_key();
	if ( is_up(n) ) {
		draw_shape(p->x, p->y, shape_arr[shapeNum], BC);
		shape_arr[shapeNum] = trun_90(shape_arr[shapeNum]);
		if ( can_move(p->x, p->y, shape_arr[shapeNum]) == 1) {

		}else{
			//不合法,把图案转回去
			shape_arr[shapeNum] = trun_90(shape_arr[shapeNum]);
			shape_arr[shapeNum] = trun_90(shape_arr[shapeNum]);
			shape_arr[shapeNum] = trun_90(shape_arr[shapeNum]);
		}
		draw_shape(p->x, p->y, shape_arr[shapeNum], FC);
	} else if ( is_left(n) ) {
		//如果左是合法位置，绘制当前背景，下一位置前景
		if ( can_move(p->x-1, p->y, shape_arr[shapeNum]) ) {
			draw_shape(p->x, p->y, shape_arr[shapeNum], BC);
			p->x--;
			draw_shape(p->x, p->y, shape_arr[shapeNum], FC);
		}
	}
	else if ( is_right(n) ) {
		if ( can_move(p->x+1, p->y, shape_arr[shapeNum]) ) {
			draw_shape(p->x, p->y, shape_arr[shapeNum], BC);
			p->x++;
			draw_shape(p->x, p->y, shape_arr[shapeNum], FC);
		}
	}
	else if ( is_down(n) ) {
		if ( can_move(p->x, p->y+1, shape_arr[shapeNum]) ) {
			draw_shape(p->x, p->y, shape_arr[shapeNum], BC);
			p->y++;
			draw_shape(p->x, p->y, shape_arr[shapeNum], FC);
		}
	}
}

void handler(int s)
{
	timer(&pos);
}

void handler_int(int s)
{
	recover_keyboard();
	//显示光标，恢复位置，清屏
	printf("\033[?25h");
	printf("\033[u");
	printf("\033[2J");
	exit(0);
}

int main( void )
{
	srand((unsigned)time(NULL));
	//注册ALRM信号，收到后执行定时下落
	struct sigaction act;
	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, NULL);

	signal(SIGINT, handler_int);
	signal(SIGQUIT, SIG_IGN);

	//设置定器
	struct itimerval it;
	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = 1;
	it.it_interval.tv_sec = 1;
	it.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);

	printf("\033[2J");
	init_keyboard();
	draw_bk();
	//不停的判断是否有按键产生
	while ( 1 ) {
		is_key(&pos);
	}
	recover_keyboard();
	printf("\033[?25h");
	printf("\033[u");
	printf("\033[2J");
}


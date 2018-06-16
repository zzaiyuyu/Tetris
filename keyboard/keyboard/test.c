#include "keyboard.h"
#include <stdio.h>
int main()
{
	init_keyboard();
	while(1){
		int n = get_key();
		if(is_up(n))
		{
			printf("up\n");
		}
		if(is_down(n))
		{
			printf("down\n");
		}
	}
}

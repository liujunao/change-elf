#include<stdio.h>


void main()
{
	printf("this is the elf file which should be modify!\n");
	printf("this program is used to calculate 1+2+```+100\n");
	int sum=0;
	for(int i=0;i<=100;i++)
	{
		sum+=i;
	}
	printf("1+2+```+100=%d\n", sum);
}


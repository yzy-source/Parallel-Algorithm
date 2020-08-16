// Matrix.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <time.h>
#include <stdio.h>
using namespace std;



int main()
{
	for (int number = 0; number < 8; number++) {
		int num = 1000;
		int **a = new int *[num];
		for (int i = 0; i<num; i++)
		{
			a[i] = new int[num];          //分配空间
		}

		for (int j = 0; j < num; j++) {
			for (int k = j; k < num; k++) {
				if (k == j)
					a[j][k] = 0;
				else
					a[j][k] = rand() % 100;
			}

		}

		for (int j = 0; j < num; j++) {
			for (int k = 0; k < j; k++) {
				a[j][k] = a[k][j];
			}

		}

		char s[20];
		char filename[20];
		sprintf_s( s, "%s%d.txt","1000",number );
	/*	printf("%s",s);*/
		ofstream out(s);
		out << num<<"\n";
		for (int i = 0; i < num; i++) {
			for (int j = 0; j < num; j++) {
				out << " " << a[i][j];
			}
			out << "\n";
		}
		
	}
	
	system("pause");
    return 0;
}


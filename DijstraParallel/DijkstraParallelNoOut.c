/**输入邻接矩阵和源顶点，求最短路径。输入邻接矩阵中，位置(x,y)为顶点y到x的边权**/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/**定义M为无穷大**/
#define M 1.7e308

#define BOOL int
#define FALSE 0
#define TRUE  1
#define W(x,y) W[x*nodenum+y]

FILE* fp;
char ch;
char id[20];
int point;
double* W;
double* dist;
BOOL* bdist;
int nodenum = 0;
int S = 0;

/* show err and exit*/
void fatal(char* err)
{
	printf("%s/n", err);
	exit(0);
}

/**从文件中读取字符**/
void GetChar()
{
	fread(&ch,sizeof(char),1,fp);
}

/**读取一个小数，如果为字符M或m，令其值为无穷大**/
double GetNextNum()
{
	double num;

	while(!isdigit(ch) && ch!='M' && ch!='m')
		GetChar();

	if(isdigit(ch))
	{
		point = 0;
		while(isdigit(ch))
		{
			id[point] = ch;
			point ++;
			GetChar();
		}
		id[point] = 0;
		num = atof(id);
	}else{
		num = M;
		GetChar();
	}
	return num;
}


/**读入邻接矩阵**/
void ReadMatrix()
{
	char file[40];
	int i,j;
	double num;

	printf("Begin to read the matrix!\n");
	printf("The first integer of the file should be the size of the matrix!\n");
	printf("Input the file name of the matrix:");
	scanf("%s",file);

	if((fp = fopen(file,"r")) == NULL)
	{
		fatal("File name input error!");
	}
	num = GetNextNum();
	if(num < 0 || num > 10000)
	{
		fclose(fp);
		fatal("The matrix row input error!");
	}
	nodenum = (int)num;
	printf("Input the start node:");
	scanf("%d",&S);
	if(S >= nodenum) fatal("The start node input too big!\n");

	W = (double*)malloc(sizeof(double)*num*num);
	if( W == NULL)
	{
		fclose(fp);
		fatal("Dynamic allocate space for matrix fail!");
	}
	for(i=0;i<nodenum;i++)
		for(j=0;j<nodenum;j++)
		{
			W(i,j) = GetNextNum();
		}
	fclose(fp);
	printf("Finish reading the matrix,the nodenum is: %d;\n",nodenum);
}

/**各处理器数据初始化**/
void Init(int my_rank,int group_size,int ep)
{
	int i;
	MPI_Status status;
	if(my_rank == 0)
	{
		for(i=1;i<group_size;i++)
		{
			MPI_Send(&W(ep*(i-1),0),ep*nodenum,MPI_DOUBLE,i,i,MPI_COMM_WORLD);//向各个处理器传送所需要的邻接矩阵块 
		}
	}else{
		dist = (double*)malloc(sizeof(double)*ep);
		bdist = (int*) malloc(sizeof(BOOL)*ep);
		W = (double*)malloc(sizeof(double)*ep*nodenum);
		if(W == NULL || dist == NULL || bdist == NULL)
			fatal("Dynamic allocate space for matrix fail!");
		MPI_Recv(W,ep*nodenum,MPI_DOUBLE,0,my_rank,MPI_COMM_WORLD,&status);//接收各自所需要的邻接矩阵块 
		/*
		初始化dist和bdist 
		*/
		for(i=0;i<ep; i++)
		{
			if(i+(my_rank-1)*ep == S)
			{
				dist[i] = 0;
				bdist[i] = TRUE;
			}else{
				dist[i] = W(i,S);
				bdist[i] = FALSE;
			}
		}
	}
}


/**输出邻接矩阵**/
void OutPutMatrix(int my_rank,int group_size,int ep,int mynum)
{
	int i,j;

	if(my_rank != 0)
	{
		for(i=0;i<mynum;i++)
		{
			printf("Processor %d:\t",my_rank);
			for(j=0;j<nodenum;j++)
			{
				if(W(i,j) > 1000000) printf("M\t");
				else printf("%d\t",(int)W(i,j));
			}
			printf("\n");
		}
	}
}


/**输出结果**/
void OutPutResult(int my_rank,int group_size,int ep,int mynum)
{
	int i;
	if(my_rank != 0)
	{
		for(i=0;i<mynum;i++)
		{
			printf("node  %d:\t%d\n",(my_rank-1)*ep+i,(int)dist[i]);
		}
	}
}

/**算法主循环**/
void FindMinWay(int my_rank,int group_size,int ep,int mynum)
{
	int i,j;
	int index,index2;
	double num,num2;
	int calnum;
	MPI_Status status;
	/**int p = group_size;**/

	for(i=0; i<nodenum;i++)
	{
		index = 0;
		num = M;

		/**步骤(3.1)**/
		for(j=0;j<mynum;j++)
		{
			if(dist[j] < num && bdist[j]==FALSE)
			{
				num = dist[j];
				index = ep*(my_rank-1)+j;
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);

		/**步骤(3.2)**/
		calnum = group_size-1;
		while(calnum > 1)//选取最小值，复杂度为n/p+logp 
		{
			/**节点数目为偶数时**/
			if(calnum % 2 == 0)
			{
				calnum = calnum/2;
				if(my_rank > calnum)//后一半向前一半处理器发送各自的index和num 
				{
					MPI_Send(&index,1,MPI_INT,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
					MPI_Send(&num,1,MPI_DOUBLE,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
				}else if(my_rank!=0){//前一半接收后一半相应的index和num 
					MPI_Recv(&index2,1,MPI_INT,my_rank+calnum,my_rank,
						MPI_COMM_WORLD,&status);
					MPI_Recv(&num2,1,MPI_DOUBLE,my_rank+calnum,my_rank,
						MPI_COMM_WORLD,&status);
					if(num2 < num)//选取其中较小的作为自己的index和num 
					{
						num = num2;
						index = index2;
					}
				}
			}else{//居中的处理器不变，其它同上 
			/**节点数目为奇数时**/
				calnum = (calnum+1)/2; 
				if(my_rank > calnum)
				{
					MPI_Send(&index,1,MPI_INT,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
					MPI_Send(&num,1,MPI_DOUBLE,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
				}else if(my_rank!=0 && my_rank < calnum){
					MPI_Recv(&index2,1,MPI_INT,my_rank+calnum,my_rank,
						MPI_COMM_WORLD,&status);
					MPI_Recv(&num2,1,MPI_DOUBLE,my_rank+calnum,my_rank,
						MPI_COMM_WORLD,&status);
					if(num2 < num)
					{
						num = num2;
						index = index2;
					}
				}
			}
			MPI_Barrier(MPI_COMM_WORLD);//保持同步 
		}
		/**步骤(3.3)**/
		/*
		向各个处理器广播index和num 
        */ 
		MPI_Bcast(&index,1,MPI_INT,1,MPI_COMM_WORLD);
		MPI_Bcast(&num,  1,MPI_DOUBLE,1,MPI_COMM_WORLD);
		/**步骤(3.4)**/
		for(j=0;j<mynum;j++)//p 个处理器并行，将复杂度由n变为了n/p 
		{
			if((bdist[j]==FALSE)&&(num + W(j,index) < dist[j]))//必须是无向图，也即对称矩阵才成立 
				dist[j] =num  + W(j,index);
		}

		/**步骤(3.5)**/
		if(my_rank == index/ep+1)//更新bdist 
		{
			bdist[index%ep] = TRUE;
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
}

/**主函数**/
int main(int argc,char** argv)
{
	int group_size,my_rank;//进程数目和当前进程编号 
	MPI_Status status;//状态 
	int i;
	int ep;
	int mynum;//分配给当前进程的顶点数 
      clock_t start,finish;
      double totaltime;

           
  start=clock();
	MPI_Init(&argc,&argv);/*MPI begin*/
	MPI_Comm_size(MPI_COMM_WORLD,&group_size);//获取进程数目 
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);//获取当前进程编号 
	/*
	if(group_size <= 1)
	{
		printf("Not enough processor!\n");
		exit(0);
	}
	*/
	/**步骤(1)**/
  
	if(my_rank == 0)//0号进程负责读取数据 
	{
		ReadMatrix();//读取邻接矩阵 

		for(i=1;i<group_size;i++)// 也可用广播的方法 
		{
			MPI_Send(&nodenum,1,MPI_INT,i,i,MPI_COMM_WORLD);//向其它进程发送顶点数目 
			MPI_Send(&S,1,MPI_INT,i,i,MPI_COMM_WORLD);//向其它进程发送起始点 
		}
	}else{
		MPI_Recv(&nodenum,1,MPI_INT,0,my_rank,MPI_COMM_WORLD,&status);//其它进程接收顶点数目 
		MPI_Recv(&S,1,MPI_INT,0,my_rank,MPI_COMM_WORLD,&status);//其它进程接收起始点 
	}

    /*
    认为下面分配的方法不太合理。
    比如nodenum=100，groupsize=4，则编号依次为0，1，2，3。0号进程不分配顶点
    则按下面的方法ep=34;分配的顶点数依次为34，34，32，显然保证负载最均衡的分配应该为34，33，33
    但是也有它的优点，就是方便计算，只要给出进程编号和进程中序号，就可很方便的计算出实际的顶点号 
    比如已知2号进程中的4号顶点，实际的顶点编号即为1*ep+4。
     
    我认为合理的分配方法：
     ep = nodenum/(group_size-1);
     mod=nodenum%ep;
     if(my_rank<=mod) mynum=ep+1;
     else mynum=ep;
     
     这样可以保证任意两个处理器之间的任务数最多差1 ，但是不方便计算
     计算方法如下：比如i号进程中的j号节点，实际的编号 id为：
     if(i<=mod+1)
     {
        id=(ep+1)*(i-1)+j;           
     } 
     else
     {
         id=(ep+1)*mod+ep*(i-mod-1)+j;    
     }
    */
	ep = nodenum/(group_size-1);
	if(ep*group_size-ep < nodenum) ep++;

	if(ep*my_rank <= nodenum)
	{
		mynum = ep;
	}else if(ep*my_rank < nodenum+ep)
	{
		mynum = nodenum - ep*(my_rank-1);
	}
	else mynum = 0;
	if (my_rank == 0) mynum = 0;
    /**步骤(2)**/
    Init(my_rank,group_size,ep);
    //初始化各个处理器，首先向各个处理器传送所需的邻接矩阵部分，然后初始化dist和bdist 

	/**OutPutMatrix(my_rank, group_size, ep, mynum);//输出各自处理器所需的邻接矩阵 **/

	/**步骤(3)**/
		FindMinWay(my_rank,group_size,ep,mynum);//主算法 

	/**OutPutResult(my_rank,group_size,ep,mynum);//输出结果**/ 
   
	MPI_Finalize();
      
	free(W);
	free(dist);
	free(bdist);
      finish=clock();
      totaltime=(double)((finish-start)/CLOCKS_PER_SEC);
      printf("The Parallel time is %fs\n",totaltime);
	return 0;
}


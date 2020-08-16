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
int pos=0;
int mod;
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
int getW(int my_rank,int ep,int i,int j)
{
	if(my_rank==0)
	{
		return (int)W[i*nodenum+j];
	}
	if(my_rank<mod)
	{
		return (int)W[i*ep+j];
	}
	else
	{
		return (int)W[i*(ep-1)+j];
	}
}
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
	int i,j;
	MPI_Status status;

	if(my_rank == 0)
	{
 dist = (double*)malloc(sizeof(double)*ep);
		bdist = (int*)malloc(sizeof(BOOL)*ep);
  // W = (double*)malloc(sizeof(double)*nodenum*ep);
	//printf("1\n");
		for(i=0;i<nodenum;i++)
		{
  // printf("step:%d\n",i);
			for(j=1;j<mod;j++)
			{				
				MPI_Send(&W(i,j*ep),ep,MPI_DOUBLE,j,i*group_size+j,MPI_COMM_WORLD);				
			}
			for(j=mod;j<group_size;j++)
			{
      if(j==0){
      continue;
     // MPI_Sendrecv(&W(i,mod*ep+(j-mod)*(ep-1)),ep-1,MPI_DOUBLE,j,i*group_size+j,&W[i*(ep-1)],ep-1,MPI_DOUBLE,0,i*group_size+my_rank,MPI_COMM_WORLD,&status);
       
      }
      else
				MPI_Send(&W(i,mod*ep+(j-mod)*(ep-1)),ep-1,MPI_DOUBLE,j,i*group_size+j,MPI_COMM_WORLD);
			
      }
		}
	}
	else
	{
		 dist = (double*)malloc(sizeof(double)*ep);
		bdist = (int*) malloc(sizeof(BOOL)*ep);
		W = (double*)malloc(sizeof(double)*nodenum*ep);
		if(W == NULL || dist == NULL || bdist == NULL)
			fatal("Dynamic allocate space for matrix fail!");
		for(i=0;i<nodenum;i++)
		{
			if(my_rank<mod)
			{
      
				MPI_Recv(&W[i*ep],ep,MPI_DOUBLE,0,i*group_size+my_rank,MPI_COMM_WORLD,&status);//接收各自所需要的邻接矩阵块
      //  printf("receive:%d\n",my_rank);
			}
			else
			{
				MPI_Recv(&W[i*(ep-1)],ep-1,MPI_DOUBLE,0,i*group_size+my_rank,MPI_COMM_WORLD,&status);//接收各自所需要的邻接矩阵块 
				//printf("receive:%d\n",my_rank);
			}	
		}
		/*
		初始化dist和bdist 
		*/
	}
		if(my_rank<=mod)
		{
			pos=ep*my_rank;
		}
		else
		{
			pos=ep*mod+(my_rank-mod)*(ep-1);
		}
		for(i=0;i<ep; i++)
		{
			if(i+pos == S)
			{
				dist[i] = 0;
				bdist[i] = TRUE;
			}else{
				dist[i] = getW(my_rank,ep,S,i);
				
				bdist[i] = FALSE;
			}
      
		}
   
  /* if(my_rank==0)
   {
   for(i=0;i<ep; i++)
   {
   printf("dist:%f\n",dist[i]);
   printf("bdist:%f\n",bdist[i]);
   }
   }
   */
	
}


/**输出邻接矩阵**/

void OutPutMatrix(int my_rank,int group_size,int ep,int mynum)
{
	int i,j;

	
		for(i=0;i<nodenum;i++)
		{
			//printf("Processor %d:\t",my_rank);
			for(j=0;j<mynum;j++)
			{
				if(getW(my_rank,ep,i,j) > 1000000) printf("M\t");
				else printf("%d\t",getW(my_rank,ep,i,j));
			}
			printf("\n");
		}
	
}


/**输出结果**/
void OutPutResult(int my_rank,int group_size,int ep,int mynum)
{
	int i;
	
		for(i=0;i<mynum;i++)
		{
			printf("node  %d:\t%d\n",pos+i,(int)dist[i]);
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
				index = pos+j;
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
		
		/**步骤(3.2)**/
		calnum = group_size;
		while(calnum > 1)//选取最小值，复杂度为n/p+logp 
		{
			
			/**节点数目为偶数时**/
			if(calnum % 2 == 0)
			{
				calnum = calnum/2;
				if(my_rank+1 > calnum)//后一半向前一半处理器发送各自的index和num 
				{
					MPI_Send(&index,1,MPI_INT,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
					MPI_Send(&num,1,MPI_DOUBLE,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
				}else{//前一半接收后一半相应的index和num 
        
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
       
		MPI_Bcast(&index,1,MPI_INT,0,MPI_COMM_WORLD);
		MPI_Bcast(&num,  1,MPI_DOUBLE,0,MPI_COMM_WORLD);
		/**步骤(3.4)**/
		for(j=0;j<mynum;j++)//p 个处理器并行，将复杂度由n变为了n/p 
		{
  
			if((bdist[j]==FALSE)&&(num + getW(my_rank,ep,index,j) < dist[j]))//必须是无向图，也即对称矩阵才成立 
				dist[j] =num  + getW(my_rank,ep,index,j);
		}

		/**步骤(3.5)**/
		int x,y;
		if(index<ep*mod)
		{
			x=index/ep;
			y=index%ep;
		}
		else
		{
			int temp=index-mod*ep;
			x=mod+temp/(ep-1);
			y=temp%(ep-1);
		}
		if(my_rank == x)//更新bdist 
		{
			bdist[y] = TRUE;
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
}

/**主函数**/
int main(int argc,char** argv)
{
	int group_size,my_rank;//进程数目和当前进程编号 
	MPI_Status status;//状态 
 MPI_Request handle;
	//int i;
	int ep;
	int mynum;//分配给当前进程的顶点数 
      //clock_t start,finish;
      double start, end;
      double totaltime;
    //  start=clock();

	MPI_Init(&argc,&argv);/*MPI begin*/
	MPI_Comm_size(MPI_COMM_WORLD,&group_size);//获取进程数目 
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);//获取当前进程编号 
 //MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();
	/**步骤(1)**/
	if(my_rank == 0)//0号进程负责读取数据 
	{
 int i;
		ReadMatrix();//读取邻接矩阵 

		for(i=1;i<group_size;i++)// 也可用广播的方法 
		{
   //printf("group:%d\n",group_size);
    //  printf("send to the %d process the message:%d\n", i, nodenum);
			MPI_Send(&nodenum,1,MPI_INT,i,i,MPI_COMM_WORLD);//向其它进程发送顶点数目 
			MPI_Send(&S,1,MPI_INT,i,i,MPI_COMM_WORLD);//向其它进程发送起始点 
		}
		
	}else{
// printf("nodenum:%d\n",my_rank);
		MPI_Recv(&nodenum,1,MPI_INT,0,my_rank,MPI_COMM_WORLD,&status);//其它进程接收顶点数目 
		MPI_Recv(&S,1,MPI_INT,0,my_rank,MPI_COMM_WORLD,&status);//其它进程接收起始点 
	//	printf("%d:%d\n", my_rank, my_rank);
	}

    
     ep = nodenum/group_size;//nodenum=100,group_size=3,25
   //  printf("nodenum:%d\n",nodenum);
     mod=nodenum%group_size;//0
    // printf("mod:%d\n",mod);
     if(my_rank<mod) mynum=ep+1;
     else mynum=ep;
    
    /**步骤(2)**/
    Init(my_rank,group_size,ep+1);
    //初始化各个处理器，首先向各个处理器传送所需的邻接矩阵部分，然后初始化dist和bdist 
    //printf("Init finished\n");

	//OutPutMatrix(my_rank, group_size, ep+1, mynum);//输出各自处理器所需的邻接矩阵 

	/**步骤(3)**/
	
		FindMinWay(my_rank,group_size,ep+1,mynum);//主算法 
  // printf("FindMinWay finished\n");

//	OutPutResult(my_rank,group_size,ep+1,mynum);//输出结果 
//printf("output finished\n");

//	MPI_Barrier(MPI_COMM_WORLD);
 end = MPI_Wtime();
 printf("%d process tick is %9.7f\n", my_rank, MPI_Wtick());
	MPI_Finalize();

	free(W);
	free(dist);
	free(bdist);
      //finish=clock();
     // totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
      //printf("The Promoted Parallel time is %fms, %d\n",totaltime,my_rank);
      printf("Runtime = %f\n", end-start);
      
	return 0;
}

/**�����ڽӾ����Դ���㣬�����·���������ڽӾ����У�λ��(x,y)Ϊ����y��x�ı�Ȩ**/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/**����MΪ�����**/
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

/**���ļ��ж�ȡ�ַ�**/
void GetChar()
{
	fread(&ch,sizeof(char),1,fp);
}

/**��ȡһ��С�������Ϊ�ַ�M��m������ֵΪ�����**/
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


/**�����ڽӾ���**/
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

/**�����������ݳ�ʼ��**/
void Init(int my_rank,int group_size,int ep)
{
	int i;
	MPI_Status status;
	if(my_rank == 0)
	{
		for(i=1;i<group_size;i++)
		{
			MPI_Send(&W(ep*(i-1),0),ep*nodenum,MPI_DOUBLE,i,i,MPI_COMM_WORLD);//�������������������Ҫ���ڽӾ���� 
		}
	}else{
		dist = (double*)malloc(sizeof(double)*ep);
		bdist = (int*) malloc(sizeof(BOOL)*ep);
		W = (double*)malloc(sizeof(double)*ep*nodenum);
		if(W == NULL || dist == NULL || bdist == NULL)
			fatal("Dynamic allocate space for matrix fail!");
		MPI_Recv(W,ep*nodenum,MPI_DOUBLE,0,my_rank,MPI_COMM_WORLD,&status);//���ո�������Ҫ���ڽӾ���� 
		/*
		��ʼ��dist��bdist 
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


/**����ڽӾ���**/
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


/**������**/
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

/**�㷨��ѭ��**/
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

		/**����(3.1)**/
		for(j=0;j<mynum;j++)
		{
			if(dist[j] < num && bdist[j]==FALSE)
			{
				num = dist[j];
				index = ep*(my_rank-1)+j;
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);

		/**����(3.2)**/
		calnum = group_size-1;
		while(calnum > 1)//ѡȡ��Сֵ�����Ӷ�Ϊn/p+logp 
		{
			/**�ڵ���ĿΪż��ʱ**/
			if(calnum % 2 == 0)
			{
				calnum = calnum/2;
				if(my_rank > calnum)//��һ����ǰһ�봦�������͸��Ե�index��num 
				{
					MPI_Send(&index,1,MPI_INT,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
					MPI_Send(&num,1,MPI_DOUBLE,my_rank-calnum,
						my_rank-calnum,MPI_COMM_WORLD);
				}else if(my_rank!=0){//ǰһ����պ�һ����Ӧ��index��num 
					MPI_Recv(&index2,1,MPI_INT,my_rank+calnum,my_rank,
						MPI_COMM_WORLD,&status);
					MPI_Recv(&num2,1,MPI_DOUBLE,my_rank+calnum,my_rank,
						MPI_COMM_WORLD,&status);
					if(num2 < num)//ѡȡ���н�С����Ϊ�Լ���index��num 
					{
						num = num2;
						index = index2;
					}
				}
			}else{//���еĴ��������䣬����ͬ�� 
			/**�ڵ���ĿΪ����ʱ**/
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
			MPI_Barrier(MPI_COMM_WORLD);//����ͬ�� 
		}
		/**����(3.3)**/
		/*
		������������㲥index��num 
        */ 
		MPI_Bcast(&index,1,MPI_INT,1,MPI_COMM_WORLD);
		MPI_Bcast(&num,  1,MPI_DOUBLE,1,MPI_COMM_WORLD);
		/**����(3.4)**/
		for(j=0;j<mynum;j++)//p �����������У������Ӷ���n��Ϊ��n/p 
		{
			if((bdist[j]==FALSE)&&(num + W(j,index) < dist[j]))//����������ͼ��Ҳ���Գƾ���ų��� 
				dist[j] =num  + W(j,index);
		}

		/**����(3.5)**/
		if(my_rank == index/ep+1)//����bdist 
		{
			bdist[index%ep] = TRUE;
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
}

/**������**/
int main(int argc,char** argv)
{
	int group_size,my_rank;//������Ŀ�͵�ǰ���̱�� 
	MPI_Status status;//״̬ 
	int i;
	int ep;
	int mynum;//�������ǰ���̵Ķ����� 
      clock_t start,finish;
      double totaltime;

           
  start=clock();
	MPI_Init(&argc,&argv);/*MPI begin*/
	MPI_Comm_size(MPI_COMM_WORLD,&group_size);//��ȡ������Ŀ 
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);//��ȡ��ǰ���̱�� 
	/*
	if(group_size <= 1)
	{
		printf("Not enough processor!\n");
		exit(0);
	}
	*/
	/**����(1)**/
  
	if(my_rank == 0)//0�Ž��̸����ȡ���� 
	{
		ReadMatrix();//��ȡ�ڽӾ��� 

		for(i=1;i<group_size;i++)// Ҳ���ù㲥�ķ��� 
		{
			MPI_Send(&nodenum,1,MPI_INT,i,i,MPI_COMM_WORLD);//���������̷��Ͷ�����Ŀ 
			MPI_Send(&S,1,MPI_INT,i,i,MPI_COMM_WORLD);//���������̷�����ʼ�� 
		}
	}else{
		MPI_Recv(&nodenum,1,MPI_INT,0,my_rank,MPI_COMM_WORLD,&status);//�������̽��ն�����Ŀ 
		MPI_Recv(&S,1,MPI_INT,0,my_rank,MPI_COMM_WORLD,&status);//�������̽�����ʼ�� 
	}

    /*
    ��Ϊ�������ķ�����̫����
    ����nodenum=100��groupsize=4����������Ϊ0��1��2��3��0�Ž��̲����䶥��
    ������ķ���ep=34;����Ķ���������Ϊ34��34��32����Ȼ��֤���������ķ���Ӧ��Ϊ34��33��33
    ����Ҳ�������ŵ㣬���Ƿ�����㣬ֻҪ�������̱�źͽ�������ţ��Ϳɺܷ���ļ����ʵ�ʵĶ���� 
    ������֪2�Ž����е�4�Ŷ��㣬ʵ�ʵĶ����ż�Ϊ1*ep+4��
     
    ����Ϊ����ķ��䷽����
     ep = nodenum/(group_size-1);
     mod=nodenum%ep;
     if(my_rank<=mod) mynum=ep+1;
     else mynum=ep;
     
     �������Ա�֤��������������֮�������������1 �����ǲ��������
     ���㷽�����£�����i�Ž����е�j�Žڵ㣬ʵ�ʵı�� idΪ��
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
    /**����(2)**/
    Init(my_rank,group_size,ep);
    //��ʼ���������������������������������������ڽӾ��󲿷֣�Ȼ���ʼ��dist��bdist 

	/**OutPutMatrix(my_rank, group_size, ep, mynum);//������Դ�����������ڽӾ��� **/

	/**����(3)**/
		FindMinWay(my_rank,group_size,ep,mynum);//���㷨 

	/**OutPutResult(my_rank,group_size,ep,mynum);//������**/ 
   
	MPI_Finalize();
      
	free(W);
	free(dist);
	free(bdist);
      finish=clock();
      totaltime=(double)((finish-start)/CLOCKS_PER_SEC);
      printf("The Parallel time is %fs\n",totaltime);
	return 0;
}


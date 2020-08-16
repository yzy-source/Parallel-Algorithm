#include "stdafx.h"
#include <pthread.h> 
#include <windows.h>
#include <rwlock.h>
#include <semaphore.h> 
#include<ctime>


#define THREAD_NUM 32
#define LOOP_NUM 5000
pthread_mutex_t	mutex;
my_rwlock_t rwlock;
long long head1,head2,head3, freq;        // timers
long long tail1, tail2,tail3;
list_node_s *my_list;
int consult_num = 0;
int insert_num = 0;
int delete_num = 0;
int judge_num;
sem_t r_sem;     //定义信号量    
sem_t w_sem;     //定义信号量   
int readers = 0;
float freq1 = 0.8000;
float freq2 = 0.1000;
float freq3 = 0.1000;
float total_time1=0.0;
float total_time2=0.0;
float total_time3=0.0;


//初始化条件读写锁
void my_rwlock_init(my_rwlock_t* rwlock) {
	pthread_mutex_init(&(rwlock->mutex), NULL);
	pthread_cond_init(&(rwlock->read), NULL);
	pthread_cond_init(&(rwlock->write), NULL);

	rwlock->read_now = 0;
	rwlock->read_wait = 0;
	rwlock->write_now = 0;
	rwlock->write_wait = 0;
	//printf("init successfully");
};

//销毁读写锁
void my_rwlock_destroy(my_rwlock_t* rwlock) {
	pthread_mutex_destroy(&(rwlock->mutex));

	pthread_cond_destroy(&(rwlock->read));

	pthread_cond_destroy(&(rwlock->write));
};

//读加锁
void my_rwlock_rdlock(my_rwlock_t* rwlock) {
	//头尾互斥量加、解锁
	pthread_mutex_lock(&(rwlock->mutex));

//#   ifdef WRITE_FIRST
	// write first
	//写优先，有等待写或者正在写的线程不能加锁，等待
	if (rwlock->write_wait > 0 || rwlock->write_now > 0) {

		rwlock->read_wait = rwlock->read_wait + 1;

		//等待
		pthread_cond_wait(&(rwlock->read), &(rwlock->mutex));

		//唤醒后，等待线程数减1，正在读线程数加1
		rwlock->read_wait = rwlock->read_wait - 1;
		rwlock->read_now = rwlock->read_now + 1;
	}
	else 
		rwlock->read_now = rwlock->read_now + 1; 
//#   else
//	// read first
//	// if no write now
//	if (rwlock->write_now == 0) rwlock->read_now = rwlock->read_now + 1;
//	// writing now, this thread have to wait 
//	else {
//		rwlock->read_wait = rwlock->read_wait + 1;
//
//		// wait here
//		pthread_cond_wait(&(rwlock->read), &(rwlock->mutex));
//
//		// wake up
//		rwlock->read_wait = rwlock->read_wait - 1;
//		rwlock->read_now = rwlock->read_now + 1;
//	}
//#   endif
	pthread_mutex_unlock(&(rwlock->mutex));
};

//写加锁
void my_rwlock_wrlock(my_rwlock_t* rwlock) {
	pthread_mutex_lock(&(rwlock->mutex));

	//没有读没有写情况下，可以写加锁
	if (rwlock->read_now == 0 && rwlock->write_now == 0) 
		rwlock->write_now = rwlock->write_now + 1;
	else {
		//写等待线程数加1
		rwlock->write_wait = rwlock->write_wait + 1;

		// 等待
		pthread_cond_wait(&(rwlock->write), &(rwlock->mutex));

		// 唤醒后，写等待线程减1，正在写线程加1
		rwlock->write_wait = rwlock->write_wait - 1;
		rwlock->write_now = rwlock->write_now + 1;
	}
	pthread_mutex_unlock(&(rwlock->mutex));
};

//解锁
void my_rwlock_unlock(my_rwlock_t* rwlock) {
	pthread_mutex_lock(&(rwlock->mutex));

	// 读锁：当前读的线程数大于1，减1
	if (rwlock->read_now > 1) 
		rwlock->read_now = rwlock->read_now - 1;

	// 只有一个在读，唤醒写等待（如果有）
	else if (rwlock->read_now == 1) {
		rwlock->read_now = rwlock->read_now - 1;

		if (rwlock->write_wait > 0)
			pthread_cond_signal(&(rwlock->write));
	}

	// 写锁
	else {
		//写锁：正在写的线程减1
		rwlock->write_now = rwlock->write_now - 1;

//# ifndef WRITE_FIRST
//		// read first
//		if (rwlock->read_wait > 0) pthread_cond_broadcast(&(rwlock->read));
//
//		else if (rwlock->write_wait > 0) pthread_cond_signal(&rwlock->write);
//#else
		//写优先，唤醒写等待（如果有），没有唤醒所有读等待
		if (rwlock->write_wait > 0) 
			pthread_cond_signal(&rwlock->write);
		else if (rwlock->read_wait > 0)
			pthread_cond_broadcast(&(rwlock->read));

//# endif
	}

	pthread_mutex_unlock(&(rwlock->mutex));
};

//查询->读
int Member(int value, struct list_node_s* head_p) {
	struct list_node_s* curr_p = head_p;
	while (curr_p != NULL && curr_p->data < value)
		curr_p = curr_p->next;

	if (curr_p == NULL || curr_p->data > value) {
		//printf("No such data %d", value);
		return 0;
	}
	else {
		//printf("Successfully consult number %d", value);
		return 1;
	}
}

//插入->写
int Insert(int value, struct list_node_s** head_p) {
	struct list_node_s* curr_p = *head_p;
	struct list_node_s* pred_p = NULL;
	struct list_node_s* temp_p;

	while (curr_p != NULL && curr_p->data < value) {
		pred_p = curr_p;
		curr_p = curr_p->next;
	}
	if (curr_p == NULL || curr_p->data > value) {
		temp_p = (struct list_node_s*)malloc(sizeof(struct list_node_s));
		temp_p->data = value;
		temp_p->next = curr_p;
		if (pred_p == NULL)
			*head_p = temp_p;
		else
			pred_p->next = temp_p;
		//printf("Successfully Insert number %d", value);
		return 1;
	}
	else {
		return 0;
	}
}

//删除->写
int Delete(int value, struct list_node_s** head_p) {
	struct list_node_s* curr_p = *head_p;
	struct list_node_s* pred_p = NULL;

	while (curr_p != NULL && curr_p->data < value) {
		pred_p = curr_p;
		curr_p = curr_p->next;
	}

	if (curr_p == NULL || curr_p->data > value) {
		if (pred_p == NULL) {
			*head_p = curr_p->next;
			free(curr_p);
		}
		else if (curr_p == NULL) {
			return 0;
		}else{
			pred_p->next = curr_p->next;
			/*free(curr_p);*/
		}
		//printf("Successfully delete number %d",value);
		return 1;
	}
	else {
		return 0;
	}


}

//创建链表，赋随机值
list_node_s *Creatlist(int n) {
	list_node_s *head, *tail, *s;
	head = tail = (list_node_s*)malloc(sizeof(list_node_s));
	for (int i = 0; i < n; i++)
	{
		s = (list_node_s*)malloc(sizeof(list_node_s));
		s->data = rand() % 99;
		s->next = tail->next;
		tail->next = s;
		tail = tail->next;
	}
	tail->next = NULL;
	return head;
}

//输出链表
void Outlink(list_node_s *head) {
	list_node_s *p;
	p = head->next;
	printf("\n\n The list:\n\n head");
	while (p)
	{
		printf("->%d", p->data);
		p = p->next;
	}
	printf("\n");
}

void *ListOperation1(void *parm){
	threadParm_t *p = (threadParm_t *)parm;
	int r = p->threadId;
	int operation_num;
	int num;

	srand((unsigned)time(NULL));
	for (num = 0; num < LOOP_NUM; num++) {
		judge_num = rand() % 9999;
		if (judge_num < freq1*10000) {
			consult_num+=1;
			pthread_mutex_lock(&mutex);
			operation_num = rand() % 99;
			Member(operation_num, my_list);
			pthread_mutex_unlock(&mutex);
			continue;
			}
		else if (judge_num >= freq1 * 10000 && judge_num < (freq1+freq2) * 10000) {//插入
			insert_num+=1;
			pthread_mutex_lock(&mutex);
			operation_num = rand() % 99;
			Insert(operation_num, &my_list);
			pthread_mutex_unlock(&mutex);
			continue;
			}
		else {//删除
			delete_num+=1;
			pthread_mutex_lock(&mutex);
			operation_num = rand() % 99;
			Delete(operation_num, &my_list);
			pthread_mutex_unlock(&mutex);
			continue;
			}
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&tail1);
	printf("Thread %d: %lfms.\n", r, (tail1- head1) * 1000.0 / freq);
	total_time1 += (tail1 - head1) * 1000.0 / freq;
	pthread_exit(nullptr);
	return parm;
}

void *ListOperation2(void *parm) {
	threadParm_t *p = (threadParm_t *)parm;
	int r = p->threadId;
	int operation_num;
	int num;

	srand((unsigned)time(NULL));
	for (num = 0; num < LOOP_NUM; num++) {
		judge_num = rand() % 9999;
		if (judge_num < freq1 * 10000) {
			consult_num += 1;
			my_rwlock_rdlock(&rwlock);
			operation_num = rand() % 999;
			Member(operation_num, my_list);
			my_rwlock_unlock(&rwlock);
			continue;
		}
		else if (judge_num >= freq1 * 10000 && judge_num <(freq1 + freq2) * 10000) {//插入
			insert_num += 1;
			my_rwlock_wrlock(&rwlock);
			operation_num = rand() % 999;
			Insert(operation_num, &my_list);
			my_rwlock_unlock(&rwlock);
			continue;
		}
		else {//删除
			delete_num += 1;
			my_rwlock_wrlock(&rwlock);
			operation_num = rand() % 999;
			Delete(operation_num, &my_list);
			my_rwlock_unlock(&rwlock);
			continue;
		}
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&tail2);
	printf("Thread %d: %lfms.\n", r, (tail2 - head2) * 1000.0 / freq);
	total_time2 += (tail2 - head2) * 1000.0 / freq;
	pthread_exit(nullptr);
	return parm;
}

void *ListOperation3(void *parm) {
	threadParm_t *p = (threadParm_t *)parm;
	int r = p->threadId;
	int operation_num;
	int num;

	srand((unsigned)time(NULL));
	for (num = 0; num < LOOP_NUM; num++) {
		judge_num = rand() % 9999;
		if (judge_num < freq1 * 10000) {
			consult_num += 1;
			sem_wait(&r_sem);
			if (readers == 0)
				sem_wait(&w_sem);
			readers++;
			sem_post(&r_sem);
			operation_num = rand() % 999;
			Member(operation_num, my_list);
			sem_wait(&r_sem);
			readers--;
			if (readers == 0)
				sem_post(&w_sem);
			sem_post(&r_sem);
			continue;
		}
		else if (judge_num >= freq1 * 10000 && judge_num < (freq1+freq2) * 10000) {//插入
			insert_num += 1;
			sem_wait(&w_sem);
			operation_num = rand() % 999;
			Insert(operation_num, &my_list);
			sem_post(&w_sem);
			continue;
		}
		else {//删除
			delete_num += 1;
			sem_wait(&w_sem);
			operation_num = rand() % 999;
			Delete(operation_num, &my_list);
			sem_post(&w_sem);
			continue;
		}
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&tail3);
	printf("Thread %d: %lfms.\n", r, (tail3 - head3) * 1000.0 / freq);
	total_time3 += (tail3 - head3) * 1000.0 / freq;
	pthread_exit(nullptr);
	return parm;
}

int main(int argc, char *argv[])
{
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
	my_list = Creatlist(1000);
	//Outlink(my_list);

	mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_t thread[THREAD_NUM];
	threadParm_t threadParm[THREAD_NUM];

	printf("The consult operation percent is %f\n", freq1);
	printf("The insert operation percent is %f\n", freq2);
	printf("The delete operation percent is %f\n", freq3);
	printf("The number of the threads is %d\n", THREAD_NUM);

	printf("mutex_rwlock:\n");
	QueryPerformanceCounter((LARGE_INTEGER *)&head1);

	for (int i = 0; i < THREAD_NUM; i++)
	{
		threadParm[i].threadId = i;
		pthread_create(&thread[i], nullptr, ListOperation1,(void *)&threadParm[i]);
	}
	for (int i = 0; i < THREAD_NUM; i++)
	{
		pthread_join(thread[i], nullptr);
	}
	printf("consult times %d\n", consult_num);
	printf("insert times %d\n", insert_num);
	printf("delete times %d\n", delete_num);
	printf("mutex_rwlock total_time: %lfms. \n", total_time1);
	printf("mutex_rwlock avg_time: %lfms. \n", total_time1/THREAD_NUM);
	pthread_mutex_destroy(&mutex);

	printf("conditional_rwlock\n");
	consult_num = 0;
	insert_num = 0;
	delete_num = 0;
	my_rwlock_init(&rwlock);
	QueryPerformanceCounter((LARGE_INTEGER *)&head2);
	for (int i = 0; i < THREAD_NUM; i++)
	{
		threadParm[i].threadId = i;
		pthread_create(&thread[i], nullptr, ListOperation2, (void *)&threadParm[i]);
	}
	for (int i = 0; i < THREAD_NUM; i++)
	{
		pthread_join(thread[i], nullptr);
	}
	printf("consult times %d\n", consult_num);
	printf("insert times %d\n", insert_num);
	printf("delete times %d\n", delete_num);
	printf("conditional_rwlock total_time: %lfms. \n", total_time2);
	printf("conditional_rwlock avg_time: %lfms. \n", total_time2 / THREAD_NUM);
	my_rwlock_destroy(&rwlock);



	printf("signal_rwlock:\n");  
	consult_num = 0;
	insert_num = 0;
	delete_num = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&head3);
	sem_init(&r_sem, 0, 1);     //初始化信号量 
	sem_init(&w_sem, 0, 1);     //初始化信号量 
	for (int i = 0; i < THREAD_NUM; i++)
	{
		threadParm[i].threadId = i;
		pthread_create(&thread[i], nullptr, ListOperation3, (void *)&threadParm[i]);
	}
	for (int i = 0; i < THREAD_NUM; i++)
	{
		pthread_join(thread[i], nullptr);
	}
	printf("consult times %d\n", consult_num);
	printf("insert times %d\n", insert_num);
	printf("delete times %d\n", delete_num);
	printf("signal_rwlock total_time: %lfms. \n", total_time3);
	printf("signal_rwlock avg_time: %lfms. \n", total_time3 / THREAD_NUM);
	sem_destroy(&r_sem);
	sem_destroy(&w_sem);
	

	system("pause");
	return 0;

}

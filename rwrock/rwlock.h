#pragma once
#include "stdafx.h"
#include <pthread.h> 
#include <stdio.h> 
#include <conio.h>
#include <stdlib.h>
#include<errno.h>

//读写锁数据结构
struct my_rwlock_t {

	//互斥量
	pthread_mutex_t mutex;

	//条件读锁
	pthread_cond_t  read;

	//条件写锁
	pthread_cond_t  write;

	//读写相关变量，正在读/写的线程数，等待读/写的线程数
	int read_now;
	int read_wait;
	int write_now;
	int write_wait;
};

//链表数据结构
struct list_node_s {
	int data;
	struct list_node_s* next;
};

typedef struct {
	int	threadId;
}threadParm_t;




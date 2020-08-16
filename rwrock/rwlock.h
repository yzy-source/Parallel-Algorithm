#pragma once
#include "stdafx.h"
#include <pthread.h> 
#include <stdio.h> 
#include <conio.h>
#include <stdlib.h>
#include<errno.h>

//��д�����ݽṹ
struct my_rwlock_t {

	//������
	pthread_mutex_t mutex;

	//��������
	pthread_cond_t  read;

	//����д��
	pthread_cond_t  write;

	//��д��ر��������ڶ�/д���߳������ȴ���/д���߳���
	int read_now;
	int read_wait;
	int write_now;
	int write_wait;
};

//�������ݽṹ
struct list_node_s {
	int data;
	struct list_node_s* next;
};

typedef struct {
	int	threadId;
}threadParm_t;




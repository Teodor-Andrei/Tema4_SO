#include "so_scheduler.h"


#define _DEBUG_
#include "utils.h"

#include <string.h>
#include <stdio.h>

pthread_cond_t condTest = PTHREAD_COND_INITIALIZER;

enum State {newState = 0, readyState, waitingState, runningState, terminatedState};
typedef struct MyThread{

    unsigned int _consumedTimeQuantum;
    
    pthread_t _thread;
    char _pid[11];

    unsigned int _priority;         // 1 -> 5
    enum State _state;            // new,ready,waiting,running,terminated
                                //  0   1       2       3         4

    pthread_cond_t _cond;
    
}MyThread;

typedef struct Node{

    MyThread* _thread;
    struct Node* next;
}Node;

typedef struct MyScheduler{

    unsigned int _timeQuantum;
    unsigned int _ioNumber;

    pthread_mutex_t _mutex;
    pthread_key_t _threadInfo;
    pthread_cond_t _cond;

    // vector/lista de threraduri
    Node* _threadListHead;
    Node* _threadListTail;

    unsigned int _running;  // 0 - nu el; 1 - el

}MyScheduler;

typedef struct MyParams{

    so_handler* _handler;
    unsigned int _priority;

    MyThread* _thread;

}MyParams;

static MyScheduler g_scheduler;

static void *StartThread(void* params);
static void AddToThreadList(MyThread* thread);
static void SortThreadList();
static void CleanupKey();
static void PrintMessage(char* message);

/*
 * creates and initializes scheduler
 * + time quantum for each thread
 * + number of IO devices supported
 * returns: 0 on success or negative on error
 */
int so_init(unsigned int time_quantum, unsigned int io){

    pthread_mutex_init(&g_scheduler._mutex, NULL);
    pthread_mutex_lock(&g_scheduler._mutex);
    PrintMessage("lock mutex, so_init");


    PrintMessage("Initializare scheudele");

    g_scheduler._timeQuantum = time_quantum;
    g_scheduler._ioNumber = io;

    pthread_key_create(&g_scheduler._threadInfo, CleanupKey);
    pthread_cond_init(&g_scheduler._cond,NULL);


    //pthread_cond_wait(&g_scheduler._cond,&g_scheduler._mutex);
    g_scheduler._threadListHead = NULL;
    g_scheduler._threadListTail = NULL;

    g_scheduler._running = 0;

    //apeleaza loop in care verifica lista de threaduri si el seteaza starile!
    // threadurile vor semnaliza singure cand isi termina cuanta de timp
    //                                      sau cand intra in starea de wait dupa un apel de so_wait

    //schedulerul va avea grija ca mereu sa exista un thread in running (cel in starea READY cu prioritatea cea mai mare);
    
    // while(1)
    //     SortThreadList();

    PrintMessage("unlock mutex, so_init");
    pthread_mutex_unlock(&g_scheduler._mutex);
    return 0;

}
static void SortThreadList(){
    PrintMessage("sort list");
    if(g_scheduler._threadListHead != NULL){

        g_scheduler._threadListHead->_thread->_state = runningState;
    }
}
static void CleanupKey(){
    //curata trhrraD? 

    PrintMessage("CleanupFunction for key");
}

/*
 * creates a new so_task_t and runs it according to the scheduler
 * + handler function
 * + priority
 * returns: tid of the new task if successful or INVALID_TID
 */
tid_t so_fork(so_handler *func, unsigned int priority){

    pthread_mutex_lock(&g_scheduler._mutex);
    PrintMessage("lock mutex, so_fork");

    tid_t ret;
    MyThread* thread = (MyThread*)malloc(sizeof(MyThread));
    thread->_consumedTimeQuantum = 0;
    thread->_priority = priority;
    thread->_state = runningState;
    pthread_cond_init(&thread->_cond,NULL);

    //add to scheduler vector/list
    AddToThreadList(thread);

    MyParams params;
    params._handler = func;
    params._priority = priority;
    params._thread = thread;

    ret = pthread_create(&thread->_thread, NULL,StartThread,&params);

    PrintMessage("unlock mutex so_fork");
    pthread_mutex_unlock(&g_scheduler._mutex);
    return 0;
}


static void *StartThread(void* params){

    pthread_mutex_lock(&g_scheduler._mutex);
    
    MyParams* parameter = (MyParams*) params;
    while(parameter->_thread->_state != runningState) //not running
       pthread_cond_wait(&parameter->_thread->_cond, &g_scheduler._mutex);
    
    PrintMessage("lock mutex, StartThread");


    pid_t pid = gettid();
    sprintf(parameter->_thread->_pid,"%d",pid); //setam sa stim pidul threadului sub forma de char, pentru un debug mai usor
    
    pthread_setspecific(g_scheduler._threadInfo, parameter->_thread);


    PrintMessage("StartThread...wait in READY state");
    //o VariabileDeConditie
   // while(parameter->_thread->_state != runningState) //not running
       // pthread_cond_wait(&parameter->_thread->_cond, &g_scheduler._mutex);
    

    PrintMessage("Call handler pentru thread");
    (*parameter->_handler)(parameter->_priority);

    //iesire thread
    g_scheduler._running = 1;
    PrintMessage("unlock mutex, EndtThread");
    pthread_mutex_unlock(&g_scheduler._mutex);
    return NULL;
}

static void AddToThreadList(MyThread* thread){

    /* creem nod, adaugam la final, actualizam tail */
    Node* node = (Node*)malloc(sizeof(Node));
    node->_thread = thread;
    node->next = NULL;

    PrintMessage("add to list");
    if(g_scheduler._threadListHead == NULL){
   
        g_scheduler._threadListHead = node;
        g_scheduler._threadListTail = node;
    }
    else{
        g_scheduler._threadListTail->next = node;
        g_scheduler._threadListTail = node;
    }
    
}

/*
 * waits for an IO device
 * + device index
 * returns: -1 if the device does not exist or 0 on success
 */
DECL_PREFIX int so_wait(unsigned int io){return 0;}

/*
 * signals an IO device
 * + device index
 * return the number of tasks woke or -1 on error
 */
DECL_PREFIX int so_signal(unsigned int io){


    SortThreadList();
    return 0;
}

/*
 * does whatever operation
 */
DECL_PREFIX void so_exec(void){

    
}

/*
 * destroys a scheduler
 */
void so_end(void){

 printf("\nAICI END");
    pthread_mutex_lock(&g_scheduler._mutex);
    while(g_scheduler._running == 0){
        
        //PrintMessage("Da");
        pthread_cond_wait(&condTest, &g_scheduler._mutex);
    }
    PrintMessage("lock mutex, so_end");

    printf("pid: %s",g_scheduler._threadListHead->_thread->_pid);
    pthread_join(g_scheduler._threadListHead->_thread->_thread, NULL);
    PrintMessage("unlock mutex, so_end");

    pthread_mutex_unlock(&g_scheduler._mutex);
}



static void PrintMessage(char* message){


    pid_t pid = gettid();
    char charPid[100];
    sprintf(charPid,"%d",pid);

    char fullMessage[100];
    strcpy(fullMessage,message);
    strcat(fullMessage,": ");
    strcat(fullMessage, charPid);
    printf("\nM: %s",fullMessage);
}




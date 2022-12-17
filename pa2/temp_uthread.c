#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <ucontext.h>
#include "uthread.h"
#include "list_head.h"
#include "types.h"
#include <limits.h>

#define PRINT_MSG

/* You can include other header file, if you want. */

/*******************************************************************
 * struct tcb
 *
 * DESCRIPTION
 *    tcb is a thread control block.
 *    This structure contains some information to maintain thread.
 *
 ******************************************************************/
struct tcb {
    struct list_head list;
    ucontext_t *context;
    enum uthread_state state;
    int tid;

    int lifetime; // This for preemptive scheduling
    int priority; // This for priority scheduling
};

struct terminated_tid{
    struct list_head list;
    int terminated_tid;
    int rem;
};

/***************************************************************************************
 * LIST_HEAD(tcbs);
 *
 * DESCRIPTION
 *    Initialize list head of thread control block.
 *
 **************************************************************************************/
LIST_HEAD(tcbs);
LIST_HEAD(terminatd_tcb);

int n_tcbs = 0;

struct ucontext_t *t_context;

struct tcb * current_tcb;

struct min_tcb* min_ptr;

struct terminated_tid* terminated_list;

enum uthread_sched_policy current_policy;

//extern int main(int argc, char* argv[]);
extern void *__preemptive_worker(void* args);

ucontext_t main_ctx;
ucontext_t exit_ctx;
ucontext_t pa2_ctx;

int main_exit = 0;
int all_done = 0;


int exit_to_main =0;

int exit_cnt = 0;
int join_tid;

void reset_exit_context();
void make_main_context(enum uthread_sched_policy policy);
void reset_pa2_context();
/***************************************************************************************
 * next_tcb()
 *
 * DESCRIPTION
 *
 *    Select a tcb with current scheduling policy
 *
 **************************************************************************************/
void next_tcb() {
    struct tcb *next = current_tcb;
    struct tcb *prev;
    if(current_policy == FIFO){
       next = fifo_scheduling(next);
    }
    else if (current_policy == SJF){
        next = sjf_scheduling(next);
    }
    else if(current_policy == PRIO){
        next = prio_scheduling(next);
    }
    else if(current_policy == RR){
        next = rr_scheduling(next);
    }
    
    prev = current_tcb;
    current_tcb = next;
    if(current_policy == FIFO || current_policy == RR){
        // fprintf(stderr, "current->tid : %d || current->lifetime : %d\n", current->tid, current->lifetime);
        if(prev->tid != -1 || current_tcb->tid != -1){
            //if(prev->tid != current->tid){
                fprintf(stderr, "SWAP %d -> %d\n", prev->tid, current_tcb->tid);
                swapcontext(prev->context, current_tcb->context);
            //}
        }
    }
    else{
        if(prev->tid != current_tcb->tid){
            fprintf(stderr, "SWAP %d -> %d\n", prev->tid, current_tcb->tid);
            swapcontext(prev->context, current_tcb->context);
        }
    }
}

/***************************************************************************************
 * struct tcb *fifo_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using First-In-First-Out policy
 *
 **************************************************************************************/
struct tcb *fifo_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */
    struct tcb* temp;

    if(next == list_first_entry(&tcbs, struct tcb, list))
    {
        temp = list_last_entry(&tcbs, struct tcb, list);
    }
    else
        temp =  list_prev_entry(next, list);
#ifdef PRINT_MSG
    fprintf(stderr, "fifo tid %d\n", temp->tid);
#endif
    return temp;
}

/***************************************************************************************
 * struct tcb *rr_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using round robin policy
 *
 **************************************************************************************/
struct tcb *rr_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */
    struct tcb* temp;

    if(next == list_first_entry(&tcbs, struct tcb, list))
    {
        temp = list_last_entry(&tcbs, struct tcb, list);
    }
    else
    {
        temp =  list_prev_entry(next, list);
       
    }
    return temp;
}

/***************************************************************************************
 * struct tcb *prio_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using priority policy
 *
 **************************************************************************************/
struct tcb *prio_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */
    struct tcb *temp;
    struct tcb *temp1;
    struct tcb *max;

    int max_priority = INT_MIN;

    list_for_each_entry_safe_reverse(temp, temp1, &tcbs, list)
    {
        if(temp->state != TERMINATED)
        {
            if(temp->tid != next->tid)
            {
                if(temp->priority > max_priority)
                {
                    max_priority = temp->priority;
                    max = temp;
                }
            }
        }
    }
    if(!max)
    {
        return max;
    }
    else
        return max;
}

/***************************************************************************************
 * struct tcb *sjf_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using shortest job first policy
 *
 **************************************************************************************/
struct tcb *sjf_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */
    struct tcb *temp;
    struct tcb *temp1;
    struct tcb *min;

    int min_lifetime = INT_MAX;

    list_for_each_entry_safe_reverse(temp, temp1, &tcbs, list)
    {
        if(temp->state != TERMINATED)
        {
            if(temp->tid != next->tid)
            {
                if(temp->lifetime < min_lifetime)
                {
                    min_lifetime = temp->lifetime;
                    min = temp;
                }
            }
        }
    }
    if(!min)
    {
        return min;
    }
    else
        return min;
}

//void *main_handler(void* args) {
//    static int _count = 0;
////    sigset_t mask;
////    sigaddset(&mask, SIGALRM);
//
//#ifdef PRINT_MSG
//    fprintf (stderr, "======> main_handler (%03d)\n", _count++);
//#endif
//
//  while(1)
//  {
//#ifdef PRINT_MSG
//      fprintf(stderr, ".");
//#endif
//
//      if(exit_to_main==1)
//      {
//          //free(exit_ctx.uc_stack.ss_sp);
//          //reset_exit_context();
//#ifdef PRINT_MSG
//          fprintf(stderr, "+");
//#endif
//          exit_to_main = 0;
//      }
//
//      if(main_exit)
//      {
//#ifdef PRINT_MSG
//          fprintf(stderr,"main handler exit\n");
//#endif
//          all_done = 1;
//          list_del(&current_tcb->list);
//          free(current_tcb->context->uc_stack.ss_sp);
//          free(current_tcb);
//          reset_pa2_context();
//          make_main_context(current_policy);
//          main_exit = 0;
//          break;
//      }
//#ifdef PRINT_MSG
//      fprintf(stderr, ".");
//#endif
//
//  }
////   sigprocmask(SIG_BLOCK, &mask, NULL);
//#ifdef PRINT_MSG
//    fprintf(stderr, "main handler end\n");
//#endif
//}


void make_main_context(enum uthread_sched_policy policy){
    struct tcb* ptr = (struct tcb*)malloc(sizeof(struct tcb));
    void *stack = malloc(MAX_STACK_SIZE);
    //ucontext_t *t_context;
    ptr->tid = -1; // main thread
    ptr->lifetime = MAIN_THREAD_LIFETIME;
    ptr->priority = MAIN_THREAD_PRIORITY;
    ptr->state = READY;
    ptr->context = &main_ctx;

    list_add(&(ptr->list), &tcbs);
    n_tcbs++;
#ifdef PRINT_MSG
    fprintf(stderr, "uthread_init_start\n");
#endif
    if (stack == NULL) {
        perror("Allocating stack");
        exit(EXIT_FAILURE);
    }
    if (getcontext(ptr->context) < 0) {
        perror("getcontext");
        exit(EXIT_FAILURE);
      }

    current_tcb = ptr;

    current_policy = policy;
    ptr->context->uc_stack.ss_sp    = stack;
    ptr->context->uc_stack.ss_size  = SIGSTKSZ;
    ptr->context->uc_stack.ss_flags = 0;

    //makecontext(&main_ctx, (void*)main_handler, 0);
}


/***************************************************************************************
 * uthread_init(enum uthread_sched_policy policy)
 *
 * DESCRIPTION

 *    Initialize main tread control block, and do something other to schedule tcbs
 *
 **************************************************************************************/
void uthread_init(enum uthread_sched_policy policy) {
    /* TODO: You have to implement this function. */


    make_main_context(policy);

    /* DO NOT MODIFY THESE TWO LINES */
    __create_run_timer();
    __initialize_exit_context();
#ifdef PRINT_MSG
    fprintf(stderr, "uthread_init_end\n");
#endif
}


/***************************************************************************************
 * uthread_create(void* stub(void *), void* args)
 *
 * DESCRIPTION
 *
 *    Create user level thread. This function returns a tid.
 *
 **************************************************************************************/
int uthread_create(void* stub(void *), void* args) {

    /* TODO: You have to implement this function. */
    int* arg;
    arg = (int*)args;
    struct tcb* ptr = (struct tcb*)malloc(sizeof(struct tcb));
    void *stack = malloc(SIGSTKSZ);
#ifdef PRINT_MSG
    fprintf(stderr, "uthrread_create_start\n");
#endif
    all_done = 0;
    main_exit = 0;
    ptr->tid = arg[0];
    ptr->lifetime = arg[1];
    ptr->priority = arg[2];
    ptr->state = READY;
    ptr->context = (ucontext_t *)malloc(sizeof(ucontext_t));
    list_add(&(ptr->list), &tcbs);
    n_tcbs++;

    if (stack == NULL) {
        perror("Allocating stack");
        exit(EXIT_FAILURE);
    }
    if (getcontext(ptr->context) < 0) {
        perror("getcontext");
        exit(EXIT_FAILURE);
      }
    if(current_policy == FIFO || current_policy == SJF){
        ptr->context->uc_link = &main_ctx;
        sigemptyset(&(ptr->context->uc_sigmask));
    }
    ptr->context->uc_stack.ss_sp    = stack;
    ptr->context->uc_stack.ss_size  = SIGSTKSZ;
    ptr->context->uc_stack.ss_flags = 0;

    makecontext(ptr->context, (void *)stub, 0);
#ifdef PRINT_MSG
    fprintf(stderr, "uthrread_create_end\n");
#endif
}

/***************************************************************************************
 * uthread_join(int tid)
 *
 * DESCRIPTION
 *
 *    Wait until thread context block is terminated.
 *
 **************************************************************************************/
void uthread_join(int tid) {

    /* TODO: You have to implement this function. */
    struct tcb *temp;
    struct tcb *temp1;
    
#ifdef PRINT_MSG
    fprintf(stderr, "uthread_join_start\n");
#endif

    list_for_each_entry_safe(temp, temp1, &tcbs, list)
    {
        if(temp->tid == tid){
            while(1){
                if(temp->state == TERMINATED){
                    break;
                }
            }
            fprintf(stderr, "JOIN %d\n", temp->tid);
#ifdef PRINT_MSG
                    fprintf(stderr, "terminated thread and return\n");
#endif
        }
    }
#ifdef PRINT_MSG
    fprintf(stderr, "uthread_join_end\n");
#endif
}

/***************************************************************************************
 * __exit()
 *
 *    When your thread is terminated, the thread have to modify its state in tcb block.
 *
 **************************************************************************************/
void __exit() {
    /* TODO: You have to implement this function. */
#ifdef PRINT_MSG
    fprintf(stderr, " __exit handler start, \n"); //, current_tcb->tid);//, exit_cnt++);
#endif
    
    if(current_policy == FIFO || current_policy == SJF)
    {
        current_tcb->state = TERMINATED;
        exit_to_main=1;
    }
    else if(current_policy == RR || current_policy == PRIO)
    {
        if(current_tcb->tid != -1)
        {
            current_tcb->lifetime -= 1;
            if(current_tcb->lifetime == 0)
            {
                exit_to_main=1;
                current_tcb->state = TERMINATED;
            }
        }
        else
            exit_to_main=1;
    }
    free(exit_ctx.uc_stack.ss_sp);
    reset_exit_context();
    //swapcontext(&exit_ctx, &main_ctx);
#ifdef PRINT_MSG
    fprintf(stderr, " __exit handler end, \n"); //, current_tcb->tid);//, exit_cnt++);
#endif
}

void reset_exit_context(){
    void *stack = malloc(SIGSTKSZ);
    if (getcontext(&exit_ctx) < 0) {
        perror("getcontext");
        exit(EXIT_FAILURE);
      }
    sigemptyset(&(exit_ctx.uc_sigmask));
    //if(current_policy == FIFO || current_policy == SJF)
        sigaddset(&(exit_ctx.uc_sigmask), SIGALRM);
    exit_ctx.uc_link           = &main_ctx;
    exit_ctx.uc_stack.ss_sp    = stack;
    exit_ctx.uc_stack.ss_size  = SIGSTKSZ;
    exit_ctx.uc_stack.ss_flags = 0;
    makecontext(&exit_ctx, __exit, 0);
}
/***************************************************************************************
 * __initialize_exit_context()
 *
 * DESCRIPTION
 *
 *    This function initializes exit context that your thread context will be linked.
 *
 **************************************************************************************/
void __initialize_exit_context() {

    /* TODO: You have to implement this function. */
    //exit_ctx = (ucontext_t *)malloc(sizeof(ucontext_t));
    void *stack = malloc(SIGSTKSZ);
    if (getcontext(&exit_ctx) < 0) {
        perror("getcontext");
        exit(EXIT_FAILURE);
      }
    exit_ctx.uc_link           = &main_ctx;

    sigemptyset(&(exit_ctx.uc_sigmask));
    //if(current_policy == FIFO || current_policy == SJF)
        sigaddset(&(exit_ctx.uc_sigmask), SIGALRM);

    exit_ctx.uc_stack.ss_sp    = stack;
    exit_ctx.uc_stack.ss_size  = SIGSTKSZ;
    exit_ctx.uc_stack.ss_flags = 0;
    makecontext(&exit_ctx, __exit, 0);
}

/***************************************************************************************
 *
 * DO NOT MODIFY UNDER THIS LINE!!!
 *
 **************************************************************************************/

static struct itimerval time_quantum;
static struct sigaction ticker;

void __scheduler() {
    if(n_tcbs > 1)
        next_tcb();
}

void __create_run_timer() {

    time_quantum.it_interval.tv_sec = 0;
    time_quantum.it_interval.tv_usec = SCHEDULER_FREQ; // 1000
    time_quantum.it_value = time_quantum.it_interval;

    ticker.sa_handler = __scheduler; // signum번호를 가지는 시그널이 발생했을 때 실행된 함수를 설치한다
    sigemptyset(&ticker.sa_mask); // sa_mask-> sa_handler에 등록된 시그널 핸들러 함수가 실행되는 동안 블럭되어야 하는 시그널의 마스크를 제공한다
    sigaction(SIGALRM, &ticker, NULL); // signum, act, oldact
    ticker.sa_flags = 0; // 시그널 처리 프로세스의 행위를 수정하는 일련의 플래그들을 명시한다.

    setitimer(ITIMER_REAL, &time_quantum, (struct itimerval*) NULL); //ITIMER_REAL//수치는 0이고 타이머의 값은 실시간으로 감소하며 보내는 신호는 SIGALRM이다, value가 가리키는 구>조체를 타이머의 현재 값으로 설정하고, ovalue - NULL
}

void __free_all_tcbs() {
    struct tcb *temp;

    list_for_each_entry(temp, &tcbs, list) {
        if (temp != NULL && temp->tid != -1) {
            list_del(&temp->list);
            free(temp->context);
            free(temp);
            n_tcbs--;
            temp = list_first_entry(&tcbs, struct tcb, list);
        }
    }
    temp = NULL;
    free(t_context);
}


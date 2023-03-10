#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <ucontext.h>
#include "uthread.h"
#include "list_head.h"
#include "types.h"
#include <limits.h>

//#define PRINT_MSG

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
/***************************************************************************************
 * next_tcb()
 *
 * DESCRIPTION
 *
 *    Select a tcb with current scheduling policy
 *
 **************************************************************************************/
void next_tcb() {
    /* TODO: You have to implement this function. */

    struct tcb* next, *temp;
    sigset_t mask;
    sigaddset(&mask, SIGALRM);
#ifdef PRINT_MSG
    fprintf(stderr, "start_tcb\n");
#endif
    if(current_policy == FIFO){
        next = fifo_scheduling(current_tcb);
    }
    else if(current_policy == RR)
        next = rr_scheduling(current_tcb);
    else if(current_policy == PRIO)
        next = prio_scheduling(current_tcb);
    else if(current_policy == SJF)
        next = sjf_scheduling(current_tcb);

    if(current_policy == RR || current_policy == PRIO)
    {
        if(n_tcbs == 2 && current_tcb->tid == -1)
        {
            main_exit = 1;
            current_tcb = next;
#ifdef PRINT_MSG
            fprintf(stderr, "return to main RR/PRIO : 0\n");
#endif
        }
        else
        {
            if(current_tcb->lifetime == 0)
            {
#ifdef PRINT_MSG
                fprintf(stderr, "life time 0 to exit\n");
#endif
            
                terminated_list = (struct terminated_tid*)malloc(sizeof(struct terminated_tid));
                terminated_list->terminated_tid = current_tcb->tid;
                list_add(&(terminated_list->list), &terminatd_tcb);
                terminated_list->rem = 1;
                list_del(&current_tcb->list);
                n_tcbs--;
                fprintf(stderr, "SWAP %d -> %d\n", current_tcb->tid, next->tid);
                temp = current_tcb;
                current_tcb = next;
                swapcontext(temp->context, &exit_ctx);
            }
            else
            {
                next->lifetime -= 1;

                fprintf(stderr, "SWAP %d -> %d\n", current_tcb->tid, next->tid);
                temp = current_tcb;
                current_tcb = next;
                if(swapcontext(temp->context, next->context)==-1)
                {
                    fprintf(stderr, "swap error when thread switching\n");
                    exit(0);
                }
            }
        }
        
    }
    else
    {
        if(next->tid == -1)
        {
            fprintf(stderr, "SWAP %d -> %d\n", current_tcb->tid, next->tid);
            
            main_exit = 1;
            current_tcb = next;
#ifdef PRINT_MSG
            fprintf(stderr, "return to main FIFO/SJF\n");
#endif
        }
        else
        {
            fprintf(stderr, "SWAP %d -> %d\n", current_tcb->tid, next->tid);
            current_tcb = next;
            if(swapcontext(&main_ctx, next->context)==-1)
            {
                fprintf(stderr, "swap error when thread switching\n");
                exit(0);
            }
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

void *main_handler(void* args) {
    static int _count = 0;
//    sigset_t mask;
//    sigaddset(&mask, SIGALRM);

#ifdef PRINT_MSG
    fprintf (stderr, "======> main_handler (%03d)\n", _count++);
#endif

  while(1)
  {
#ifdef PRINT_MSG
      fprintf(stderr, ".");
#endif

      if(exit_to_main==1)
      {
          //free(exit_ctx.uc_stack.ss_sp);
          //reset_exit_context();
#ifdef PRINT_MSG
          fprintf(stderr, "+");
#endif
          exit_to_main = 0;
      }

      if(main_exit)
      {
#ifdef PRINT_MSG
          fprintf(stderr,"main handler exit\n");
#endif
          all_done = 1;
          break;
      }
#ifdef PRINT_MSG
      fprintf(stderr, ".");
#endif

  }
//   sigprocmask(SIG_BLOCK, &mask, NULL);
#ifdef PRINT_MSG
    fprintf(stderr, "main handler end\n");
#endif
}


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
    ptr->context->uc_link           = &pa2_ctx;
    ptr->context->uc_stack.ss_sp    = stack;
    ptr->context->uc_stack.ss_size  = SIGSTKSZ;
    ptr->context->uc_stack.ss_flags = 0;

    makecontext(&main_ctx, (void*)main_handler, 0);
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

    void *stack = malloc(SIGSTKSZ);

    if (stack == NULL) {
      perror("Allocating stack");
      exit(EXIT_FAILURE);
    }

    if (getcontext(&pa2_ctx) < 0) {
      perror("getcontext");
      exit(EXIT_FAILURE);
    }

    pa2_ctx.uc_link           = NULL;
    pa2_ctx.uc_stack.ss_sp    = stack;
    pa2_ctx.uc_stack.ss_size  = SIGSTKSZ;
    pa2_ctx.uc_stack.ss_flags = 0;


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
    if(current_policy == FIFO || current_policy == SJF)
        ptr->context->uc_link = &exit_ctx;
    else
        ptr->context->uc_link = NULL;
    sigemptyset(&(ptr->context->uc_sigmask));
    if(current_policy == FIFO || current_policy == SJF)
        sigaddset(&(ptr->context->uc_sigmask), SIGALRM);
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
    struct terminated_tid *temp2, *temp3;
    int join_exit = 0;
    
    //terminated_list = (struct terminated_tid*)malloc(sizeof(struct terminated_tid));

#ifdef PRINT_MSG
    fprintf(stderr, "uthread_join_start\n");
#endif

    setvbuf(stderr, 0, _IOLBF, 0);
    
    join_tid = tid;
    
    list_for_each_entry_safe(temp, temp1, &tcbs, list)
    {
        if(tid == temp->tid && temp->state == TERMINATED)
        {
            return;
        }
        else if(tid == temp->tid && temp->state != TERMINATED)
        {
#ifdef PRINT_MSG
            fprintf(stderr, "JOIN swap\n");
#endif
            if(n_tcbs != 2)
                swapcontext(&pa2_ctx, &main_ctx);
        }
        else
        {
            list_for_each_entry_safe(temp2, temp3, &terminatd_tcb, list)
            {
#ifdef PRINT_MSG
                fprintf(stderr, "%d ", temp2->terminated_tid);
#endif
                if(tid == temp2->terminated_tid)
                {
#ifdef PRINT_MSG
                    fprintf(stderr, "\n");
#endif

                    fprintf(stderr, "JOIN %d\n", tid);

#ifdef PRINT_MSG
                    fprintf(stderr, "terminated thread and return\n");
#endif
                    return;
                }
            }
        }
    }

    while(1)
    {
        if(all_done)
        {
            list_for_each_entry_safe(temp, temp1, &tcbs, list)
            {
                if (temp->state == TERMINATED)
                {
#ifdef PRINT_MSG
                    fprintf(stderr, "del tid: %d\n", temp->tid);
#endif
                    
                    terminated_list = (struct terminated_tid*)malloc(sizeof(struct terminated_tid));
                    terminated_list->terminated_tid = temp->tid;
                    list_add(&(terminated_list->list), &terminatd_tcb);
                    terminated_list->rem = 1;
                    
                    list_del(&temp->list);
                    if(tid == temp->tid)
                    {
                        join_exit = 1;
                    }
                    free(temp->context);
                    free(temp);
                    n_tcbs--;
                }
            }
            if(join_exit)
            {
                fprintf(stderr, "JOIN %d\n", tid);
                list_for_each_entry_safe(temp, temp1, &tcbs, list)
                {
                    if(temp->tid == -1)
                    {
                        list_del(&temp->list);
                        free(temp);
                    }
                }
                make_main_context(current_policy);
                //free(&exit_ctx);
                __initialize_exit_context();
                break;
            }
            else
            {
                swapcontext(&pa2_ctx, &main_ctx);
            }
        }
    }
    all_done = 0;
    main_exit = 0;
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

    ticker.sa_handler = __scheduler; // signum????????? ????????? ???????????? ???????????? ??? ????????? ????????? ????????????
    sigemptyset(&ticker.sa_mask); // sa_mask-> sa_handler??? ????????? ????????? ????????? ????????? ???????????? ?????? ??????????????? ?????? ???????????? ???????????? ????????????
    sigaction(SIGALRM, &ticker, NULL); // signum, act, oldact
    ticker.sa_flags = 0; // ????????? ?????? ??????????????? ????????? ???????????? ????????? ??????????????? ????????????.

    setitimer(ITIMER_REAL, &time_quantum, (struct itimerval*) NULL); //ITIMER_REAL//????????? 0?????? ???????????? ?????? ??????????????? ???????????? ????????? ????????? SIGALRM??????, value??? ???????????? ???>????????? ???????????? ?????? ????????? ????????????, ovalue - NULL
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

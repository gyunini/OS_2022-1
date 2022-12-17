/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.

   On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
*/
#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <signal.h>   // sigaction()
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <string.h>   // memset()
#include <sys/time.h> // ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF, struct itimerval, setitimer()
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX
#include <ucontext.h> /* ucontext_t, getcontext(), makecontext(),

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ
#define TIMER_TYPE ITIMER_REAL
/* Number of iterations for functions foo() and bar(). */
#define N 100000
#define TIMEOUT    1          // ms

/* Two contexts. */
ucontext_t *foo_ctx, *bar_ctx;
ucontext_t main_ctx, pa2_ctx;
ucontext_t foo_done_ctx, bar_done_ctx;
ucontext_t* crt_ctx;

int foo_done_exit=0;
int main_exit = 0;
int all_done = 0;

/* Function executed by the foo context. */
int foo_cnt=0;
int bar_cnt=0;
void foo() {
  printf("foo (%d)\n", foo_cnt++);
  for (int i = 0; i < N; i++);
}

/* Function used by the bar context. */
void bar() {
  printf("bar (%d)\n", bar_cnt++);
  for (int i = 0; i < N; i++);
}

/* Function used by the foo_done context. */
void foo_done(char *msg) {
  foo_done_exit = 1;
  printf("foo  -  %s!\n", msg);
  //swapcontext(&foo_done_ctx, &main_ctx);
}

void bar_done(char *msg) {
  main_exit = 1;
  printf("bar  -  %s!\n", msg);
  //swapcontext(&bar_done_ctx, &main_ctx);
}

void set_timer(int type, void (*handler) (int), int ms) {
  struct itimerval timer;
  struct sigaction sa;
  sigset_t mask;

  /* Install signal handler for the timer. */
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler =  handler;
  sigaction (SIGALRM, &sa, NULL);
  sigaddset(&mask, SIGALRM);

  /* Configure the timer to expire after ms msec... */
  // timer.it_value.tv_sec = 0;
  // timer.it_value.tv_usec = ms*1000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = ms*1000;
  timer.it_value = timer.it_interval;

  if (setitimer (type, &timer, NULL) < 0) {
    perror("Setting timer");
    exit(EXIT_FAILURE);
  };
  sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/* Timer signal handler. */


/* Initialize a context.

   ctxt - context to initialize.

   next - successor context to activate when ctx returns. If NULL, the thread
          exits when ctx returns.
 */
void init_context(ucontext_t *ctx, ucontext_t *next) {
  /* Allocate memory to be used as the stack for the context. */
  void *stack = malloc(STACK_SIZE);

  if (stack == NULL) {
    perror("Allocating stack");
    exit(EXIT_FAILURE);
  }

  if (getcontext(ctx) < 0) {
    perror("getcontext");
    exit(EXIT_FAILURE);
  }

  /* Before invoking makecontext(ctx), the caller must allocate a new stack for
     this context and assign its address to ctx->uc_stack, and define a successor
     context and assigns address to ctx->uc_link.
  */

  ctx->uc_link           = next;
  if(ctx != &main_ctx){
      sigemptyset(&(ctx->uc_sigmask));
      sigaddset(&(ctx->uc_sigmask), SIGALRM);
    }
//  sigprocmask(SIG_BLOCK, &(ctx->uc_sigmask), NULL);
  ctx->uc_stack.ss_sp    = stack;
  ctx->uc_stack.ss_size  = STACK_SIZE;
  ctx->uc_stack.ss_flags = 0;
}




/* Initialize context ctx  with a call to function func with zero argument.
 */
void init_context0(ucontext_t *ctx, void (*func)(), ucontext_t *next) {
  init_context(ctx, next);
  makecontext(ctx, func, 0);
}

/* Initialize context ctx with a call to function func with one string argument.
 */
void init_context1(ucontext_t *ctx, void (*func)(), const char *str, ucontext_t *next) {
  init_context(ctx, next);
  makecontext(ctx, func, 1, str);
}

/* Link context a to context b such that when context a returns context b is resumed.
 */
void link_context(ucontext_t *a, ucontext_t *b) {
  a->uc_link = b;
}

void main_handler (int signum) {
  static int _count = 0;
  fprintf (stderr, "======> main_handler (%03d)\n", _count++);

  while(1){

    if(foo_done_exit){
        fprintf(stderr,"+");
    free(foo_done_ctx.uc_stack.ss_sp);
    //init_context(&foo_done_ctx, &main_ctx);
      init_context1(&foo_done_ctx, foo_done, "done", &main_ctx);
    foo_done_exit = 0;
    }
    if(main_exit){
      fprintf(stderr,"main handler exit\n");
      all_done = 1;
      break;
    }
    fprintf(stderr, ".");
  }
  fprintf(stderr, "main handler end\n");
}

void scheduler (int signum) {
    fprintf(stderr,"next schduling..\n");
  if(crt_ctx == (&main_ctx)){
    crt_ctx = foo_ctx;
    swapcontext(&main_ctx, foo_ctx);
  }
  else if(crt_ctx == (foo_ctx)){
    crt_ctx = bar_ctx;
    swapcontext(&main_ctx, bar_ctx);
  }
  else if(crt_ctx == (bar_ctx)){
    crt_ctx = &main_ctx;
      main_exit = 1;
    //swapcontext(&bar_ctx, &main_ctx);
  }
}

int main() {
  /* Flush each printf() as it happens. */
  setvbuf(stdout, 0, _IOLBF, 0);

  /* Initialize contexts(). */
  init_context(&pa2_ctx, NULL);
  init_context0(&main_ctx, main_handler, &pa2_ctx);

  init_context1(&foo_done_ctx, foo_done, "done", &main_ctx);
    foo_ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
    bar_ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
  init_context0(foo_ctx, foo, &foo_done_ctx);
  init_context0(bar_ctx, bar, &foo_done_ctx);

  set_timer(TIMER_TYPE, scheduler, TIMEOUT);

  /* Transfers control to the foo context. */
  crt_ctx = &main_ctx;
  swapcontext(&pa2_ctx, &main_ctx);
  while(1)
  {
    if(all_done)
    {
      fprintf(stderr, "All done successful return from all thread !\n");
      break;
    }
  }

  all_done = 0;
  main_exit = 0;
  init_context0(&main_ctx, main_handler, &pa2_ctx);
  init_context0(foo_ctx, foo, &foo_done_ctx);
  init_context0(bar_ctx, bar, &foo_done_ctx);

  swapcontext(&pa2_ctx, &main_ctx);
    while(1)
    {
      if(all_done)
      {
        fprintf(stderr, "All done successful return from all thread !\n");
        break;
      }
    }


  fprintf(stderr, "ERROR! A successful call to setcontext() does not return!\n");

}

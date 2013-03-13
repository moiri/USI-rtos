/*************************************************************************/
/*  prodCons.c                                                           */
/*                                                                       */
/*************************************************************************/

/* includes */
#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "taskLib.h"
#include "kernelLib.h"
#include "tickLib.h"
#include "time.h"
#include "sigLib.h"

/* defines */
#define STACK_SIZE    20000
#define MAX_SECONDS   100
#define MAX_DEPTH     1000
#define MAX_PERIOD    100
#define MAX_BOUND     100
#define MIN_BOUND     10
#define MAX_COMP_TIME 100
#define MIN_MSG       2
#define MAX_MSG       100
#define TIMESLICE     6 // set time slice to 100 ms

/* task IDs */
int tidProdPeriodic;			
int tidProdAperiodic;			
int tidConsumer;			

/* function declarations */
void prodPeriodic(int);
void prodAperiodic(int, int);
void consumer(int, int);
void timerHandlerPeriodic(timer_t);
void timerHandlerAperiodic(timer_t);


/*************************************************************************/
/*  main task                                                            */
/*                                                                       */
/*************************************************************************/

int main(void) {
	struct timespec mytime;
	int    nseconds = 0;
	int    depth_q1 = 0;
	int    depth_q2 = 0;
	int    period = 0;
	int    low_bound = 0;
	int    up_bound = 0;
	int    comp_time = 0;
	int    max_read_msg = 0;

	/* get the simulation time */ 
	printf("\n\n");
	while ((nseconds < 1) || (nseconds > MAX_SECONDS)) {
		printf("Enter overall simulation time [1-%d s]: ", MAX_SECONDS);
		scanf("%d", &nseconds);
	};
	printf("Simulating for %d seconds.\n\n", nseconds);

	/* get the maximal queue entries for queue #1*/ 
	printf("\n\n");
	while ((depth_q1 < 1) || (depth_q1 > MAX_DEPTH)) {
		printf("Enter the maximal depth of queue #1 [1-%d]: ", MAX_DEPTH);
		scanf("%d", &depth_q1);
	};
	printf("Depth for queue #1 set to %d.\n\n", depth_q1);

	/* get the maximal queue entries for queue #2*/ 
	printf("\n\n");
	while ((depth_q2 < 1) || (depth_q2 > MAX_DEPTH)) {
		printf("Enter the maximal depth of queue #2 [1-%d]: ", MAX_DEPTH);
		scanf("%d", &depth_q2);
	};
	printf("Depth for queue #2 set to %d.\n\n", depth_q2);

	/* get the period for the periodic producer */ 
	while ((period < 1) || (period > MAX_PERIOD)) {
		printf("Enter period for the periodic producer [1-%d s]: ", MAX_PERIOD);
		scanf("%d", &period);
	};
	printf("Period for the periodic producer set to %ds. \n\n", period);

	/* get the lower bound for the aperiodic timer */ 
	while ((low_bound < 1) || (low_bound > MAX_BOUND)) {
		printf("Enter lower bound for the aperiodic timer [1-%d s]: ", MAX_BOUND);
		scanf("%d", &low_bound);
	};
	printf("Lower bound for the aperiodic timer set to %ds. \n\n", low_bound);

	/* get the upper bound for the aperiodic timer */ 
	while ((up_bound < MIN_BOUND) || (up_bound > MAX_BOUND)) {
		printf("Enter upper bound for the aperiodic timer [%d-%d s]: ", MIN_BOUND, MAX_BOUND);
		scanf("%d", &up_bound);
	};
	printf("Upper bound for the aperiodic timer set to %ds. \n\n", up_bound);

	/* get the consumer computation time */ 
	while ((comp_time < 1) || (comp_time > MAX_COMP_TIME)) {
		printf("Enter consumer computation time [1-%d s]: ", MAX_COMP_TIME);
		scanf("%d", &comp_time);
	};
	printf("Consumer computation time set to %ds. \n\n", comp_time);
	
	/* get the max number of msgs read per consumer loop */ 
	while ((max_read_msg < MIN_MSG) || (max_read_msg > MAX_MSG)) {
		printf("Enter max number of msgs read per consumer loop [%d-%d]: ", MIN_MSG, MAX_MSG);
		scanf("%d", &max_read_msg);
	};
	printf("Max number of msgs read per consumer loop set to %d. \n\n", max_read_msg);


	/* set clock to start at 0 */
	mytime.tv_sec  = 0;
	mytime.tv_nsec = 0;
  
	if (clock_settime(CLOCK_REALTIME, &mytime) < 0)
		printf("Error clock_settime\n");
	else
		printf("Current time set to %d sec %d ns \n\n", (int) mytime.tv_sec, (int)mytime.tv_nsec);

	/* set time slice to 100 ms */	
	kernelTimeSlice(TIMESLICE); 	
     
	/* spawn (create and start) task */
	tidProdPeriodic = taskSpawn("tProdPeriodic", 100, 0, STACK_SIZE,
		(FUNCPTR)prodPeriodic, period, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	
	/* spawn (create and start) task */
	tidProdAperiodic = taskSpawn("tProdAperiodic", 100, 0, STACK_SIZE,
		(FUNCPTR)prodAperiodic, low_bound, up_bound, 0, 0, 0, 0, 0, 0, 0, 0);
	
	/* spawn (create and start) task */
	tidConsumer = taskSpawn("tConsumer", 100, 0, STACK_SIZE,
		(FUNCPTR)consumer, max_read_msg, comp_time, 0, 0, 0, 0, 0, 0, 0, 0);
	
	/* run for given simulation time */
	taskDelay(nseconds*60);
	
	/* delete task */
	taskDelete(tidPeriodic);

	printf("Exiting. \n\n");
	return(0);
}


/*************************************************************************/
/*  task "tProdPeriodic"                                                 */
/*                                                                       */
/*************************************************************************/

void prodPeriodic(int period) {
	int i;
	timer_t ptimer;
	struct  itimerspec intervaltimer;

	/* create timer */
	if ( timer_create(CLOCK_REALTIME, NULL, &ptimer) == ERROR)
		printf("Error create_timer\n");
	else
		printf("Timer for periodic producer created.\n");

	/* connect timer to timer handler routine */
	if ( timer_connect(ptimer, (VOIDFUNCPTR)timerHandlerPeriodic, (int)0) == ERROR)
		printf("Error connect_timer\n");
	else
		printf("Timer handler for periodic producer connected.\n");

	/* set and arm timer */
	intervaltimer.it_value.tv_sec = period;
	intervaltimer.it_value.tv_nsec = 0;
	intervaltimer.it_interval.tv_sec = period;
	intervaltimer.it_interval.tv_nsec = 0;

	if ( timer_settime(ptimer, TIMER_ABSTIME, &intervaltimer, NULL) == ERROR)
		printf("Error set_timer\n");
	else
		printf("Timer for periodic producer set to %ds.\n\n", intervaltimer.it_interval.tv_sec);

	/* idle loop */
	while(1) pause();
}


/*************************************************************************/
/*  task "tProdAeriodic"                                                 */
/*                                                                       */
/*************************************************************************/

void prodAperiodic(int low_bound, int up_bound) {
	int i;
	int period;
	timer_t ptimer;
	struct  itimerspec intervaltimer;

	/* create timer */
	if ( timer_create(CLOCK_REALTIME, NULL, &ptimer) == ERROR)
		printf("Error create_timer\n");
	else
		printf("Timer for aperiodic producer created.\n");

	/* connect timer to timer handler routine */
	if ( timer_connect(ptimer, (VOIDFUNCPTR)timerHandlerAperiodic, (int)0) == ERROR)
		printf("Error connect_timer\n");
	else
		printf("Timer handler for aperiodic producer connected.\n");

	/* generate random period */
	period = random_in_range(low_bound, up_bound+1);

	/* set and arm timer */
	intervaltimer.it_value.tv_sec = period;
	intervaltimer.it_value.tv_nsec = 0;
	intervaltimer.it_interval.tv_sec = period;
	intervaltimer.it_interval.tv_nsec = 0;

	if ( timer_settime(ptimer, TIMER_ABSTIME, &intervaltimer, NULL) == ERROR)
		printf("Error set_timer\n");
	else
		printf("Timer for aperiodic producer set to %ds.\n\n", intervaltimer.it_interval.tv_sec);

	/* idle loop */
	while(1) pause();
}


/*************************************************************************/
/*  task "tConsumer"                                                     */
/*                                                                       */
/*************************************************************************/

void consumer(int comp_time, int max_read_msg) {

}


/*************************************************************************/
/*  function "TimerHandlerPeriodic"                                      */
/*                                                                       */
/*************************************************************************/

void timerHandlerPeriodic(timer_t callingtimer) {
   
	struct timespec mytime;

	if ( clock_gettime (CLOCK_REALTIME, &mytime) == ERROR) 
		printf("Error: clock_gettime \n");

	printf("current time: %d s, %d ms\n", (int) mytime.tv_sec, 
		(int) (mytime.tv_nsec/1000000));
	
}

  
/*************************************************************************/
/*  function "random_in_range"                                           */
/*                                                                       */
/*  proposed by Ryan Reich on http://stackoverflow.com                   */
/*                                                                       */
/*************************************************************************/

/* Would like a semi-open interval [min, max) */
int random_in_range (unsigned int min, unsigned int max)
{
	int base_random = rand(); /* in [0, RAND_MAX] */
	if (RAND_MAX == base_random) return random_in_range(min, max);
	/* now guaranteed to be in [0, RAND_MAX) */
	int range = max - min,
	remainder = RAND_MAX % range,
	bucket    = RAND_MAX / range;
	/* There are range buckets, plus one smaller interval
	*      within remainder of RAND_MAX */
	if (base_random < RAND_MAX - remainder) {
		return min + base_random/bucket;
	}
	else {
		return random_in_range (min, max);
	}
}
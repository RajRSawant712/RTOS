// RTOS Framework - Spring 2020
// J Losh

// Student Name: Raj Sawant
//             : Rahul Kidecha
// Student ID  : 1001716968
//             : 1001735009
// TO DO: Add your name on this line.  Do not include your ID number in the file.

// Add xx_ prefix to all files in your project
// xx_rtos.c
// xx_tm4c123gh6pm_startup_ccs.c
// xx_other files (except uart0.x and wait.x)
// (xx is a unique number that will be issued in class)
// Please do not change any function name in this code or the thread priorities

//-----------------------------------------------------------------------------
//Command List
//-----------------------------------------------------------------------------

//ps
//ipcs
//kill <pidno>
//reboot
//pidof <TaskName>
//sched <rr/pr>
//pi    <on/off>
//preempt <on/off>
//<ThreadName> &
//busFault
//usageFault
//hardFault

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// 6 Pushbuttons and 5 LEDs, UART
// LEDs on these pins:
// Blue:   PF2 (on-board)
// Red:    PA2
// Orange: PA3
// Yellow: PA4
// Green:  PE0
// PBs on these pins
// PB0:    PC4
// PB1:    PC5
// PB2:    PC6
// PB3:    PC7
// PB4:    PD6
// PB5:    PD7
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "wait.h"

// REQUIRED: correct these bitbanding references for the off-board LEDs

#define POWER_LED       (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))   //PF3 on-board green LED
#define BLUE_LED        (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))   //PF2 on-board blue LED
#define RED_LED         (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 2*4)))   //PA2 off-board red LED
#define ORANGE_LED      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 3*4)))   //PA3 off-board orange LED
#define YELLOW_LED      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 4*4)))   //PA4 off-board yellow LED
#define GREEN_LED       (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32)))         //PE0 off-board green LED

#define PB0       (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 4*4)))   //PC4 HW Push button 0
#define PB1       (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 5*4)))   //PC5 HW Push button 1
#define PB2       (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 6*4)))   //PC6 HW Push button 2
#define PB3       (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 7*4)))   //PC7 HW Push button 3
#define PB4       (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 6*4)))   //PD6 HW Push button 4
#define PB5       (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 7*4)))   //PD7 HW Push button 5


#define POWER_LED_MASK 8
#define RED_LED_MASK 4
#define BLUE_LED_MASK 4
#define ORANGE_LED_MASK 8
#define YELLOW_LED_MASK 16
#define GREEN_LED_MASK 1

#define PB0_MASK 16
#define PB1_MASK 32
#define PB2_MASK 64
#define PB3_MASK 128
#define PB4_MASK 64
#define PB5_MASK 128

// PortA UART masks
#define UART_TX_MASK 2
#define UART_RX_MASK 1


//-----------------------------------------------------------------------------
// Define extern assembly functions
//-----------------------------------------------------------------------------

extern void  set_PSP(void *psp);
extern void* get_PSP();
extern void* get_MSP();
extern void* get_R0();
extern void* get_R1();
extern void  setTMPL();

extern void activate_PSP(void *psp);
extern void pushPSP_R4_11();
extern void popPSP_R4_11(void *psp);
extern void moveDummy1R4_11();
extern void moveDummy2R4_11();
extern void dummyStackOP(void *psp, void *pc, uint32_t lr);


//----------------------------------------------------------------------------
//Semaphore index
//----------------------------------------------------------------------------
#define keyPressed  0
#define keyReleased  1
#define flashReq  2
#define resource  3



//-----------------------------------------------------------------------------
//SVC call variable
//-----------------------------------------------------------------------------
#define yield_num   7
#define sleep_num   8
#define wait_num    6
#define post_num    9
#define kill_num    10
#define restart_num 11
#define setThPrNum  12

//-----------------------------------------------------------------------------
//Shell secure variables
//    char instructdb[max_inst][max_inst_size]={"ps","ipcs","kill","reboot","pidof","sched","pi"};

//-----------------------------------------------------------------------------
#define ps_num      51
#define ipcs_num    52
//#define kill NA
#define pidof_num   53
#define sched_num   54
#define pi_num      55
#define preemptNum  56
#define resPIDNum   57
#define reset_num   255


//-----------------------------------------------------------------------------
//Uart Shell variables
//-----------------------------------------------------------------------------
#define  max_inst_size  8
#define  max_inst       8
#define  max_Arg        4
#define  max_position   6
#define  Max_letter     80  // Maximum size of input command buffer.



//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// semaphore
#define MAX_SEMAPHORES 5
#define MAX_QUEUE_SIZE 5
struct semaphore
{   char sname[15];
    uint16_t count;
    uint16_t queueSize;
    uint32_t processQueue[MAX_QUEUE_SIZE]; // store task index here
} semaphores[MAX_SEMAPHORES];
uint8_t semaphoreCount = 0;

//struct semaphore *keyPressed, *keyReleased, *flashReq, *resource, *s;
struct semaphore *s;

// task
#define STATE_INVALID    0 // no task
#define STATE_UNRUN      1 // task has never been run
#define STATE_READY      2 // has run, can resume at any time
#define STATE_DELAYED    3 // has run, but now awaiting timer
#define STATE_BLOCKED    4 // has run, but now blocked by semaphore

#define MAX_TASKS 10       // maximum number of valid tasks
uint8_t taskCurrent = 0;   // index of last dispatched task
uint8_t taskCount = 0;     // total number of valid tasks
bool prInherit = true;
bool preempt = true;
static uint8_t i =0;
static uint8_t read = 0;

uint32_t cputime[MAX_TASKS];
uint32_t stack[MAX_TASKS][512];
bool sched = true;

static  uint32_t *p;
// REQUIRED: add store and management for the memory used by the thread stacks
//           thread stacks must start on 1 kiB boundaries so mpu can work correctly

struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread
    void *spInit;                  // location of original stack pointer
    void *sp;                      // location of stack pointer for thread
    int8_t priority;               // 0=highest to 15=lowest
    int8_t currentPriority;        // used for priority inheritance
    uint32_t ticks;                // ticks until sleep complete
    char name[16];                 // name of task used in ps command
    uint32_t delTime[2];              // to calculate CPU usage.
    void *semaphore;               // pointer to the semaphore that is blocking the thread
} tcb[MAX_TASKS];


//-----------------------------------------------------------------------------
// string.h Functions
//-----------------------------------------------------------------------------
char *myInsertChar(char *destination, char *source, uint8_t position, char tok)
{
    char *start = destination;
    uint8_t val=0;
    while(*source != '\0')
    {
        if(val == position ){
            *destination = tok;
        }else{
        *destination = *source;
        source++;
        }
        destination++;
        val++;
    }

    *destination = '\0'; // add '\0' at the end
    return start;
}


uint8_t mystrlen(char *a){
    uint8_t i=0;
    while(*a){
     i++;
     a++;
    }
   return i;

}

uint32_t my_atoi(char *a, uint8_t len){
   uint32_t val =0;
   uint32_t tempval =0;
   uint32_t templen =0;
   uint8_t  i = 0;
//   val =0;
//   i=0;
//   tempval =0;
    while(a[i]){
        tempval = ( uint32_t ) (a[i] - 48);
        templen =len-1-i;

       while(templen){
           tempval *=10;
           templen--;
       }
       val += tempval;
        i++;

    }
 return val;
}


char *my_strtok(char *src , char tok, char *result){

    while(*src){
        if(*src == tok){
            *result = '\0';
            break;
        }
        *result = *src;
        result++;
        src++;
    }
 return result;
}

char *my_strcpy(char *destination, char *source)
{
    char *start = destination;

    while(*source != '\0')
    {
        *destination = *source;
        destination++;
        source++;
    }

    *destination = '\0'; // add '\0' at the end
    return start;
}

// string compare funtion
bool my_strcmp(char *a, char *b){

 bool i = true;


while((*a !='\0') || (*b !='\0')){

  if (*a != *b){
    i = false;
    break;
  }
  a++;
  b++;
}
return i;
}


//-----------------------------------------------------------------------------
// RTOS Kernel Functions
//-----------------------------------------------------------------------------



// REQUIRED: initialize systick for 1ms system timer
void initRtos()
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }
}

// REQUIRED: Implement prioritization to 16 levels
int rtosScheduler()
{    uint8_t p=0;
    static uint8_t c =0;
    uint8_t i = 0;
    uint8_t val = 0;

    bool ok;
    static uint8_t task = 0xFF;
    ok = false;
    while (!ok)
    {
        task++;
        if (task >= MAX_TASKS)
            task = 0;
        if(!sched){
        ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN);
        }else if(sched){
            while(p<16){
            for(i=1 ; i<=MAX_TASKS; i++){
                val = ((i+(c-1))%MAX_TASKS);
                if(tcb[val].currentPriority== p){
                    ok = (tcb[val].state == STATE_READY || tcb[val].state == STATE_UNRUN);
                    if (ok){
                        c = val+1;
                        if(val ==8)
                        {
                            taskCurrent =val;
                        }
                        return val;
                    }
                }
            }//end for
                p++;
            }
        }//end if sched
    }//end while
    return task;
}

bool createThread(_fn fn, char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    // REQUIRED: store the thread name
    // add task if room in task list
    // allocate stack space for a thread and assign to sp below
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}
            tcb[i].state = STATE_UNRUN;
            tcb[i].pid = fn;
            tcb[i].sp = &stack[i][511];
            tcb[i].priority = priority;
            tcb[i].currentPriority = priority;
            my_strcpy(tcb[i].name,name);
            // increment task count
            taskCount++;
            ok = true;
        }
    }
    // REQUIRED: allow tasks switches again
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
    __asm(" SVC #11");

}

// REQUIRED: modify this function to destroy a thread
// REQUIRED: remove any pending semaphore waiting
void destroyThread(_fn fn)
{
    __asm(" SVC #10");

}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm("  SVC #12");
}

void createSemaphore(uint8_t count, char name[], uint8_t index )
{
    //struct semaphore *pSemaphore = 0;
//    if (semaphoreCount < MAX_SEMAPHORES)
//    {
       // pSemaphore = &semaphores[semaphoreCount++];
        semaphores[index].count = count;
        my_strcpy(semaphores[index].sname, name);

    //}

  //  return pSemaphore;
}

void mpuInitSRAM1(){
    NVIC_MPU_NUMBER_R=0x00000005; // MPU 5
    NVIC_MPU_BASE_R=0x20000000;
    NVIC_MPU_ATTR_R=0x1106001B; // 0001 0001 0000 0110 0000 0000 0001 1011


}

void mpuInitSRAM2(){
    NVIC_MPU_NUMBER_R=0x00000006; // MPU 6
    NVIC_MPU_BASE_R=0x20004000;
    NVIC_MPU_ATTR_R=0x1106001B; // 0001 0001 0000 0110 0000 0000 0001 1011

}

void mpuInitNVIC(){
    NVIC_MPU_NUMBER_R=0x00000004; // MPU 4
    NVIC_MPU_BASE_R=0xE0000000;
    NVIC_MPU_ATTR_R=0x10050073; // 0001 0000 0000 0101 0000 0000 0111 0011

}





void disableSRAM(){
    NVIC_MPU_NUMBER_R=0x00000005;
    NVIC_MPU_BASE_R=0x20000000;
    NVIC_MPU_ATTR_R=0x1106FF1B;

    NVIC_MPU_NUMBER_R=0x00000005;
    NVIC_MPU_BASE_R=0x20004000;
    NVIC_MPU_ATTR_R=0x1106FF1B;


}



void switchMPURegion(uint8_t taskNumber){
    static uint8_t regionNum = 0;
    regionNum = taskNumber%8;
    uint16_t mask = 0;
    mask = 1<<(regionNum+8);
    if(taskNumber < 8 ){
        NVIC_MPU_NUMBER_R   =0x00000005;
        NVIC_MPU_BASE_R     =0x20000000;
        NVIC_MPU_ATTR_R     =0x1106001B | mask;

    }else
    {
        NVIC_MPU_NUMBER_R   = 0x00000006;
        NVIC_MPU_BASE_R     = 0x20004000;
        //NVIC_MPU_ATTR_R     = 0x1106041B | mask;
        NVIC_MPU_ATTR_R     = 0x1106001B | mask;

    }


}

// REQUIRED: modify this function to start the operating system, using all created tasks
void startRtos()
{
    _fn fn;
    //---------------------------------------------------------
    //-----------------------------------------------------------
    // RW access to 4GB space peripheral
            NVIC_MPU_NUMBER_R=0x00000000; // MPU 0
            NVIC_MPU_BASE_R=0x40000000;
            NVIC_MPU_ATTR_R=0x1305003F; //0001 0011 0000 0101 0000 0000 0011 1111   full access /read write access

    //---------------------------------------------------
    //Flash Memory set to RWX
    //MPU Region

            //FLASH
            NVIC_MPU_NUMBER_R=0x00000001; // MPU 1
            NVIC_MPU_BASE_R=0x00000000;
            NVIC_MPU_ATTR_R=0x03020023;  //0000 0011 0000 0010 0000 0000 0010 0011                 RW and execute access

    //---------------------------------------------------------
    //-----------------------------------------------------------
    // Stack MPU
            mpuInitSRAM1(); // Lower 16K
            mpuInitSRAM2(); // upper 16K
            mpuInitNVIC();

            // enable bus , mpu & usage faults
            NVIC_SYS_HND_CTRL_R = NVIC_SYS_HND_CTRL_BUS | NVIC_SYS_HND_CTRL_MEM | NVIC_SYS_HND_CTRL_USAGE ;
            // enable privdefen and mpu
            NVIC_MPU_CTRL_R = 0x00000005 ;

    //systickTimer init
    NVIC_ST_RELOAD_R = 0x00009C3F;
    NVIC_ST_CTRL_R = NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;

    TIMER1_TAV_R = 0;

    taskCurrent =   rtosScheduler();
    switchMPURegion(taskCurrent);
    TIMER1_CTL_R |=  TIMER_CTL_TAEN;

    activate_PSP( tcb[taskCurrent].sp);
    fn =(_fn)tcb[taskCurrent].pid;


    setTMPL();
    (*fn)();

}

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
// push registers, call scheduler, pop registers, return to new function
void yield()
{
    __asm(" SVC #7");
}

// REQUIRED: modify this function to support 1ms system timernmber
// execution yielded back to scheduler until time elapses using pendsv
// push registers, set state to delayed, store timeout, call scheduler, pop registers,
// return to new function (separate unrun or ready processing)
void sleep(uint32_t tick)
{
    __asm(" SVC #8");
}

// REQUIRED: modify this function to wait a semaphore with priority inheritance
// return if avail (separate unrun or ready processing), else yield to scheduler using pendsv
void wait(uint8_t semIndex)
{
    __asm(" SVC #6");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(uint8_t semIndex)
{
    __asm(" SVC #9");
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr()
{   static uint16_t flip = 1000;
    static uint8_t task = 0;
    for (task =0 ;task < MAX_TASKS; task++){
       if(tcb[task].state == STATE_DELAYED){
           tcb[task].ticks--;
           if(tcb[task].ticks ==0){
               tcb[task].state = STATE_READY;
           }

       }

    }

    if(preempt){
        NVIC_INT_CTRL_R = NVIC_INT_CTRL_PEND_SV;
    }
    flip--;
// Ping Pong buffer flip logic
    if(!flip){
        read  ^= 1;
        flip =1000;

        for (i= 0;i<MAX_TASKS;i++){
            tcb[i].delTime[read] = 0;
        }
    }




}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF){               // wait if uart0 tx fifo full
    //yield();
    }
    UART0_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart0(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE)               // wait if uart0 rx fifo empty
    yield();
    return UART0_DR_R & 0xFF;                        // get character from fifo
}

//------------------------------------------------------------------------
//UART task  based functions
//------------------------------------------------------------------------

void putsUartInt (uint32_t val, bool isHex){
    uint8_t i=0;
    uint8_t j=0;
    uint8_t remainder = 0;
    bool isZero = true;
    static char str[10];
    uint32_t valtemp = val;
    while (valtemp != 0){
        if(isHex){
            remainder = valtemp%16;
        }
        else{
            remainder = valtemp%10;
        }

        if(remainder < 10){
            str[i]  = (char) remainder + 48;
        }else{
            str[i]  = (char) remainder + 55;

        }

        if(isHex){
            valtemp /=16;
        }
        else{
            valtemp /=10;
        }

        i++;
        isZero = false;
    //printf("\n str [%d] = %c \n",i,str[i]);
    }
    if(isZero){

        str[i]= 48;
        i++;
    }
    str[i] = '\0';

    //reverse string
    //i is the count
    while (j<(i>>1)){
        //swapp elements
        str[j]= str[j]+str[i-j-1];
        str[i-j-1]= str[j]-str[i-j-1];
        str[j]= str[j]-str[i-j-1];
        j++;
    }

    if(isHex){
        putsUart0("0x");
    }

    putsUart0(str);

}





// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
void pendSvIsr()
{
    if((NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR) || (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR)){
        putsUart0("\r\nPENDSV called due to MPU fault due to task : ");
        putsUart0(tcb[taskCurrent].name);
        putsUart0("\r\n");
        NVIC_FAULT_STAT_R |= (NVIC_FAULT_STAT_IERR | NVIC_FAULT_STAT_DERR);
    }
 // moveDummy1R4_11();
    pushPSP_R4_11();
    tcb[taskCurrent].sp = get_PSP();
 //   moveDummy2R4_11();
    tcb[taskCurrent].delTime[read] = TIMER1_TAV_R ; //new time after last cycle
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer to get ticks
    TIMER1_TAV_R = 0;
    //disableSRAM();
    taskCurrent = rtosScheduler();
    switchMPURegion(taskCurrent);
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on timer for start Tick

    set_PSP(tcb[taskCurrent].sp);


    if(tcb[taskCurrent].state == STATE_READY){

        popPSP_R4_11(tcb[taskCurrent].sp);
    }
    else if(tcb[taskCurrent].state == STATE_UNRUN){
        p = get_PSP();
        p--;
        *p = 0x01000000;                    // store XPSR value in psp
        p--;
        *p = (uint32_t)tcb[taskCurrent].pid;          // store PC value in PSP

        p=p-6;
        set_PSP(p);

        tcb[taskCurrent].state = STATE_READY;   // state current task as Ready.
    }

}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr()
{
    static uint8_t i =0;
    static uint32_t r0 ;
    char *param_array ;
    param_array = (char *) get_R0();
    static char taskName[16] = "";

    //my_strcpy(param_array, (char *)get_R0());
    r0 = (uint32_t) get_R0();
    uint8_t priority = (uint8_t)get_R1();
    uint32_t *psp =  get_PSP();
    _fn fn = (_fn) r0 ;

    psp += 6;  //fetches the PC value of the stack
    uint16_t *pc = *psp;
    pc--;
    uint16_t *svcNum = *pc ;
    uint8_t svc = (uint8_t)(svcNum - 0xD700);
    static uint8_t killTask = 0;
       static uint8_t found = 0;
       bool taskFound = false;


    switch(svc){

    case yield_num:
        NVIC_INT_CTRL_R = NVIC_INT_CTRL_PEND_SV;
        break;

    case sleep_num:

        tcb[taskCurrent].ticks = r0;
        tcb[taskCurrent].state = STATE_DELAYED;
        NVIC_INT_CTRL_R = NVIC_INT_CTRL_PEND_SV;
        break;

    case wait_num:
         s = &semaphores[r0];
         if(s->count > 0 ){
             s->count--;
         }
         else{
             s->processQueue[s->queueSize] = taskCurrent; // to explore
             s->queueSize++;
             tcb[taskCurrent].state = STATE_BLOCKED;
             tcb[taskCurrent].semaphore = s;

             if(prInherit){
                          if(tcb[taskCurrent].state == STATE_BLOCKED){
                          for(i=0 ; i<MAX_TASKS ; i++){
                              if(tcb[i].semaphore == tcb[taskCurrent].semaphore){
                                  if(tcb[taskCurrent].priority < tcb[i].priority )
                                      tcb[i].currentPriority = tcb[taskCurrent].priority;
                                  }

                              }
                          }
                      }


             NVIC_INT_CTRL_R = NVIC_INT_CTRL_PEND_SV;
         }

        break;

    case post_num :
        s = &semaphores[r0];
        if(prInherit){
            tcb[taskCurrent].currentPriority = tcb[taskCurrent].priority;
        }

        s->count++;
        if (s->queueSize > 0){
           uint8_t task=  s->processQueue[0];
           tcb[task].state = STATE_READY;
           s->count--;

           for(i=0 ; i< s->queueSize ; i++){
               s->processQueue[i] = s->processQueue[i+1];
           }
           //assigning null to last value of queue
           if(i< MAX_QUEUE_SIZE){
           s->processQueue[i] = '\0' ;
           }
           s->queueSize--;
        }

        break;

    case kill_num :


           found = 255;
         for(i=0;i<MAX_TASKS;i++){
           if(tcb[i].pid ==fn && tcb[i].state != STATE_INVALID){
               tcb[i].state = STATE_INVALID;
               s = tcb[i].semaphore;
               killTask = i;

               if(s != 0){
                   s->count++;

                   for(i=0 ; i< s->queueSize ; i++){
                       if(s->processQueue[i] == killTask){
                           found = i;
                           s->processQueue[i]= s->processQueue[i+1];
                       }
                       if(i>found){
                           s->processQueue[i]= s->processQueue[i+1];
                       }

                   }
                   if(found == 255){
                    // post code
                               if (s->queueSize > 0){
                                  uint8_t task=  s->processQueue[0];
                                  tcb[task].state = STATE_READY;
                                  s->count--;

                                  for(i=0 ; i< s->queueSize ; i++){
                                      s->processQueue[i] = s->processQueue[i+1];
                                  }
                                  //assigning null to last value of queue
                                  if(i< MAX_QUEUE_SIZE){
                                  s->processQueue[i] = '\0' ;
                                  }
                               }

                   }
                   if(s->queueSize != 0){
                       s->queueSize--;
                   }

               }
               break;
           }if(found == 255){
              // putsUart0("\r\n Error : Task is already terminated.\r\n");
           }

         }



        break;

    case restart_num :

        for(i=0 ; i < MAX_TASKS ; i++){
                if(tcb[i].pid == fn){
                    tcb[i].sp = &stack[i][511];
                    tcb[i].state = STATE_UNRUN;
 //                   tcb[i].priority = 8;
 //                   tcb[i].currentPriority = 8;

                    break;
                }
            }
        break;

    case setThPrNum:



        for(i= 0 ; i<MAX_TASKS ;i++){
                if(tcb[i].pid == (void *)r0){
                    tcb[i].priority = priority;
                    tcb[i].currentPriority = priority;
                }
            }
        break;

    case ps_num :


        for (i=0; i<MAX_TASKS ; i++){
           cputime[i] = tcb[i].delTime[read];
        }
        uint32_t totalTicks =0;
        uint8_t val = 0;
        uint16_t percentageCpu = 0;


        //returns after fetching values
        for(i=0 ;i<MAX_TASKS ;i++){
            totalTicks += cputime[i];
        }
        putsUart0("\r\n Name \t\t %CPU Usage    \t PID    \t State       CurrentPriority \r\n");
        for(i=0 ;i<MAX_TASKS-1 ;i++){
            percentageCpu = cputime[i]*10000/totalTicks;
            putsUart0(tcb[i].name);
            putsUart0("    \t");
            val = percentageCpu/100;
            putsUartInt(val,false);
            putsUart0(".");
            val = percentageCpu %100;
            putsUartInt(val,false);
            putsUart0("        \t");
            putsUartInt((uint32_t)tcb[i].pid,false);
            putsUart0("        \t");
                switch(tcb[i].state){
                    case 0:
                        putsUart0("INVALID") ;
                        break;
                    case 1:
                        putsUart0("UNRUN");
                        break;
                    case 2:
                        putsUart0("READY");
                        break;
                    case 3 :
                        putsUart0("DELAYED");
                        break;
                    case 4 :
                        putsUart0("BLOCKED");
                        break;
            }

            putsUart0("    \t");
            putsUartInt(tcb[i].currentPriority,false);
            putsUart0(" \r\n");
        }

        break;

    case ipcs_num :

        putsUart0("\r\n | Semaphore | Count | QSize | QThread   \t|  Running\r\n");
        uint8_t j =0;
        bool f = false;
        for(i = 0 ; i< MAX_SEMAPHORES-1; i++){
            putsUart0("\r\n ");
            putsUart0(semaphores[i].sname);
            putsUart0("\t");
            val = (uint8_t)semaphores[i].count;
            putsUartInt(val,false);
            putsUart0("\t");
            val = (uint8_t) semaphores[i].queueSize;
            putsUartInt(val,false);
            putsUart0("\t");
            for (j=0; j<MAX_TASKS; j++){
              if(semaphores[i].queueSize != 0){
                if(semaphores[i].processQueue[0] == j){
                    putsUart0(tcb[j].name);
                    f = true;
                    break;
                }
              }
            }//end next thread for loop
            if(!f){
                putsUart0("--Null--");
            }
            putsUart0("    \t");
            f = false;

            for (j = 0; j<MAX_TASKS ;j++){
                if((&semaphores[i] == tcb[j].semaphore) && (tcb[j].state != STATE_BLOCKED) && (tcb[j].state != STATE_INVALID)){
                    putsUart0(tcb[j].name);
                    putsUart0("  ");
                    f = true;
                    break;
                }
            }
            if(!f){
                putsUart0("--Null--");
            }
            f = false;

            putsUart0("\r\n ");
        }

        break;

    case sched_num:
        if (my_strcmp(param_array, "pr")){
                sched = true;
                putsUart0("\r\n Mode: Priority Scheduling \r\n");
            }
            else if(my_strcmp(param_array, "rr")){
                    sched = false;
                    putsUart0("\r\n Mode: Round Robin Scheduling  \r\n");
            }
            else
                putsUart0("\r Invalid parameter input \r\n");
        break;


    case pi_num :
        if (my_strcmp(param_array, "on")){
                prInherit = true;
                putsUart0("\r\n Priority Inheritance : on \r\n");
            }
            else if(my_strcmp(param_array, "off")){
                putsUart0("\r\n Priority Inheritance : off\r\n");
                prInherit = false;
            }
            else
                putsUart0("\r Invalid parameter input \r\n");

        break;

    case preemptNum :
        if (my_strcmp(param_array, "on")){
                preempt = true;
                putsUart0("\r\n Preemption : on\r\n");
            }
            else if(my_strcmp(param_array, "off")){
                preempt = false;
                putsUart0("\r\n Preemption : off \r\n");
            }
            else
                putsUart0("\r Invalid parameter input \r\n");

        break;


    case pidof_num:

            for(i = 0 ; i<MAX_TASKS ; i++) {
               if( my_strcmp(tcb[i].name,param_array) ){
                 uint32_t  pid = (uint32_t)tcb[i].pid;
                   putsUart0("\r\n pid of task is : ");
                   putsUartInt(pid,false);
                   putsUart0("\r\n");
                   taskFound = true;
                   break;
               }
            }
            if(!taskFound){
                putsUart0("\r\n Error: Given Thread Name was not Found. \r\n");
            }
            break;


    case resPIDNum:
           my_strtok(param_array, '&', taskName);
           for(i = 0 ; i< MAX_TASKS ; i++){
               if(my_strcmp(param_array , tcb[i].name) && tcb[i].state == STATE_INVALID){
                   taskFound = true;
                   p= get_PSP();
                   *p = (uint32_t)tcb[i].pid;
                   break;
               }
           }
           if(!taskFound)
               putsUart0("\r\n Error: Task already restored or Wrong Input  \r\n");

        break;


    case reset_num:
                putsUart0("\r\n Reseting System \r\n");
                waitMicrosecond(10000);
                NVIC_APINT_R = 0x04 | (0x05FA << 16);
                break;


    }//end switch case



}


//Project 2 TODO : add error messages

                      // The usage fault handler




//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           6 pushbuttons, and uart
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;


    //enable clk on timer 1
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;

    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOA | SYSCTL_RCGC2_GPIOF | SYSCTL_RCGC2_GPIOE |SYSCTL_RCGC2_GPIOD | SYSCTL_RCGC2_GPIOC ;

    GPIO_PORTD_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTD_CR_R = PB5_MASK;

    // Configure LED pins
    //PORTf
    GPIO_PORTF_DIR_R = POWER_LED_MASK | BLUE_LED_MASK;  // bits 1 and 3 are outputs
    GPIO_PORTF_DR2R_R = POWER_LED_MASK | BLUE_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R = POWER_LED_MASK | BLUE_LED_MASK;  // enable LEDs

    //PORTA
        GPIO_PORTA_DIR_R = RED_LED_MASK | ORANGE_LED_MASK | YELLOW_LED_MASK;
        GPIO_PORTA_DR2R_R = RED_LED_MASK | ORANGE_LED_MASK | YELLOW_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
        GPIO_PORTA_DEN_R = RED_LED_MASK | ORANGE_LED_MASK | YELLOW_LED_MASK;  // enable LEDs

    //PORT E
        GPIO_PORTE_DIR_R = GREEN_LED_MASK;
        GPIO_PORTE_DR2R_R = GREEN_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
        GPIO_PORTE_DEN_R = GREEN_LED_MASK;  // enable LEDs

        //PORTC
        GPIO_PORTC_DEN_R = PB0_MASK | PB1_MASK | PB2_MASK | PB3_MASK;  // enable LEDs and pushbuttons
        GPIO_PORTC_PUR_R = PB0_MASK | PB1_MASK | PB2_MASK | PB3_MASK; // enable internal pull-up for push button

        //PORT D
        GPIO_PORTD_DEN_R = PB4_MASK | PB5_MASK;  // enable LEDs and pushbuttons
        GPIO_PORTD_PUR_R = PB4_MASK | PB5_MASK; // enable internal pull-up for push button


        TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                                         // turn-off timer before reconfiguring
        TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;                                  // configure as 32-bit timer (A+B)
        TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD | TIMER_TAMR_TACDIR;             // configure for periodic mode (count down)
        TIMER1_TAILR_R = 0xFFFFFFFF;                                            // set load value

}

//
void initUart0()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA;

    // Configure UART0 pins
    GPIO_PORTA_DIR_R |= UART_TX_MASK;                   // enable output on UART0 TX pin
    GPIO_PORTA_DIR_R &= ~UART_RX_MASK;                   // enable input on UART0 RX pin
    GPIO_PORTA_DR2R_R |= UART_TX_MASK;                  // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTA_DEN_R |= UART_TX_MASK | UART_RX_MASK;    // enable digital on UART0 pins
    GPIO_PORTA_AFSEL_R |= UART_TX_MASK | UART_RX_MASK;  // use peripheral to drive PA0, PA1
    GPIO_PORTA_PCTL_R &= ~(GPIO_PCTL_PA1_M | GPIO_PCTL_PA0_M); // clear bits 0-7
    GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                        // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to 115200 baud, 8N1 format
    UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                     // use system clock (40 MHz)
    UART0_IBRD_R = 21;                                  // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART0_FBRD_R = 45;                                  // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;    // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                        // enable TX, RX, and module
}



//remove delimiters space and commas from input str
void getposition( char input[Max_letter], uint8_t position[max_position],char temp_param[16], char param_array[max_Arg][16]){
    uint8_t arg_count=0;

    uint8_t i=0;
    uint8_t j=0;
    uint8_t k=0;

    while(input[i]!='\0'){

    //spacebar and comma check
    if(input[i]==' ' || input[i]==',' ){

        i++;

        }if(input[i]==8){
            i--;
        }
    else{

        //save position in position array
        position[j] = i;
    k=0;

    while((input[i]!=' ' || input[i]!=','))
    {
        //get parameter other than space and comma and set in temp_param
      temp_param[k]= input[i];
      if(input[i]==8){
                  i--;
              }
      if(input[i]=='\0'||(input[i]==' ' || input[i]==',')){
      temp_param[k]='\0';
      //copy the value in parameter array
      my_strcpy(param_array[arg_count],temp_param);

      break;
      }
      k++;
        i++;

    }
    j++;
        arg_count++;
        }
    }//end while

}

void clrPositionArray(uint8_t position[max_position]){
    uint8_t iter;
    for(iter=0;iter<max_position;iter++){
        position[iter]=0;
    }
}

void clrParameterArray(char param_array[max_Arg][16]){
    uint8_t i;
for(i=0;i<max_Arg;i++)
    *param_array[i]='\0';

}

// getsUart used from 5313
void getsUart0(char input[Max_letter], uint8_t position[max_position],char param_array[max_Arg][16] )
{
    clrPositionArray(position);
    clrParameterArray(param_array);
     uint8_t cnt = 0;
     uint8_t c;
L:  c = getcUart0();
     putcUart0(c);
     if((c == 0x7F)&&(cnt==0))
         goto L;
     else if((c == 0x7F) && (cnt>0))
        {cnt--;
         goto L;
        }
     else{
         if(c == 0x0D)
         {
             input[cnt] = '\0';
             return;
         }else
         {
             if(c < 0x20)
                 goto L;
            else
               {
                input[cnt++] = c;
                if(cnt == Max_letter)
                {   putsUart0("\r \n Buffer Max Reached \r\n");
                    return;
                }
                else
                 goto L;
               }
         }}}

void printStack(uint32_t *stackNum){
   // uint32_t *stackNum = stack;
    putsUart0("\r\n Stack Dump \r\n");
    putsUart0("R0  \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");
 //   my_strcpy(destination, source)
    putsUart0("R1 \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");

    putsUart0("R2 \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");

    putsUart0("R3 \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");

    putsUart0("R12 \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");

    putsUart0("LR \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");

    putsUart0("PC \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");

    putsUart0("XPSR \t:");
    putsUartInt(*stackNum, true);
    stackNum++;
    putsUart0("\r\n");

}

//------------------------------------------------------------------------------
//Fault ISRs


void mpuFaultISR(void){

    putsUart0("\r\n MPU Error detected \r\n");
    uint32_t val = 0;
    putsUart0("Process Name: ");
    putsUart0(tcb[taskCurrent].name);
    putsUart0("\r\n");
    putsUart0("Process ID :");
    putsUartInt((uint32_t)tcb[taskCurrent].pid, true);
    putsUart0("\r\n");

    putsUart0("Register Dump \r\n");
    putsUart0("MSP : ");
    val =  (uint32_t) get_MSP();
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("PSP : ");
    val =  (uint32_t) get_PSP();
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("NVIC_FAULT_STAT_R : ");
    val =  (uint32_t) NVIC_FAULT_STAT_R;
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("NVIC_MM_ADDR_R : ");
    val =  (uint32_t) NVIC_MM_ADDR_R;
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("NVIC_FAULT_ADDR_R : ");
    val =  (uint32_t) NVIC_FAULT_ADDR_R;
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("NVIC_FAULT_STAT_DERR : ");
    val =  (uint32_t) NVIC_FAULT_STAT_DERR;
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("NVIC_FAULT_STAT_IERR : ");
    val =  (uint32_t) NVIC_FAULT_STAT_IERR;
    putsUartInt(val, true);
    putsUart0("\r\n");


    uint32_t *stack;
    stack= get_PSP();

    printStack(stack);
    uint32_t *psp =  get_PSP();
        psp += 6;  //fetches the PC value of the stack
        uint32_t *pc = *psp;
        uint32_t *svcNum = *pc ;
        val = svcNum;
    putsUart0("\r\n Offending Instruction :");
    putsUartInt(val, true);
    putsUart0("\r\n");
    putsUart0("Address \t :");
    putsUartInt((uint32_t)pc, true);
    putsUart0("\r\n");

    tcb[taskCurrent].state = STATE_INVALID;
    NVIC_SYS_HND_CTRL_R &= ~(NVIC_SYS_HND_CTRL_MEMP);
    NVIC_INT_CTRL_R= NVIC_INT_CTRL_PEND_SV;


}                            // The MPU fault handler

void busFaultISR(void){
    putsUart0("\r\n Bus Error detected \r\n");

    putsUart0("Process Name: ");
    putsUart0(tcb[taskCurrent].name);
    putsUart0("\r\n");
    putsUart0("Process ID :");
    putsUartInt((uint32_t)tcb[taskCurrent].pid, true);
    putsUart0("\r\n");
    while(true);
}                            // The bus fault handler

void usageFaultISR(void){
    putsUart0("\r\n Usage Fault Error detected \r\n");

    putsUart0("Process Name: ");
    putsUart0(tcb[taskCurrent].name);
    putsUart0("\r\n");
    putsUart0("Process ID :");
    putsUartInt((uint32_t)tcb[taskCurrent].pid, true);
    putsUart0("\r\n");
    while(true);
}

void hardFaultISR(void){

    putsUart0("\r\n Hard Fault detected \r\n");
    uint32_t val = 0;
    putsUart0("Process Name: ");
    putsUart0(tcb[taskCurrent].name);
    putsUart0("\r\n");
    putsUart0("Process ID :");
    putsUartInt((uint32_t)tcb[taskCurrent].pid, true);
    putsUart0("\r\n");

    putsUart0("Register Dump \r\n");
    putsUart0("MSP : ");
    val =  (uint32_t) get_MSP();
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("PSP : ");
    val =  (uint32_t) get_PSP();
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("NVIC_FAULT_STAT_R : ");
    val =  (uint32_t) NVIC_FAULT_STAT_R;
    putsUartInt(val, true);
    putsUart0("\r\n");

    putsUart0("NVIC_FAULT_ADDR_R : ");
    val =  (uint32_t) NVIC_FAULT_ADDR_R;
    putsUartInt(val, true);
    putsUart0("\r\n");

    while(true);

}

void executePS(){
    __asm(" SVC #51");

}


void executeIpcs(){
    __asm("  SVC #52");

}

void executeSched(char *addrPtr){
    __asm(" SVC #54");

}

void executePI(char *addrPtr){
    __asm(" SVC #55");

}

void executePreempt(char *addrPtr){
    __asm(" SVC #56");

}


void printPid(char name[]){
    __asm(" SVC #53");


}

void getRestartPPID(char *addrPtr){
    __asm(" SVC #57");

}

uint8_t getCommandNum(char param_array[16]){

    if(my_strcmp(param_array, "ps"))
        return 0;

    if(my_strcmp(param_array, "ipcs"))
        return 1;

    if(my_strcmp(param_array, "kill"))
        return 2;

    if(my_strcmp(param_array, "reboot"))
        return 3;

    if(my_strcmp(param_array, "pidof"))
        return 4;

    if(my_strcmp(param_array, "sched"))
        return 5;

    if(my_strcmp(param_array, "pi"))
        return 6;

    if(my_strcmp(param_array, "preempt"))
        return 7;

    if(my_strcmp(param_array, "busFault"))
        return 8;


    return 255;

}

void checkCommand(char param_array[max_Arg][16], char instructdb[max_inst][max_inst_size], char temp_param[16]){
 uint8_t i =0;
//comlen = mystrlen(param_array[0]);

//Restore Thread Logic

if(param_array[1][0] == '&'){
    putsUart0("\r\n Searching and Restoring Thread. \r\n");

    getRestartPPID(param_array[0]);
    //restart thread call
    _fn pid = (_fn) get_R0();
    restartThread(pid);

}
else{
i = getCommandNum(param_array[0]);


switch (i){

case 0:
    executePS();
    break;

case 1:
    executeIpcs();
    break;

case 2:
    putsUart0("\r\n in kill command \r\n");
    i = mystrlen(param_array[1]);
    _fn fn = (_fn) my_atoi(param_array[1], i);
    destroyThread(fn);

    break;

case 3:
    __asm("  SVC  #255");

case 4:
    printPid(param_array[1]);

    break;

case 5:

    executeSched(param_array[1]);

    break;

case 6:
    executePI(param_array[1]);

    break;

case 7:
    executePreempt(param_array[1]);

    break;

case 8:
        waitMicrosecond(10000);
        NVIC_APINT_R = 0x04 | (0x05FA << 16);
        break;

default:
    putsUart0("\r\n Invalid command \r\n");
    break;

}

}//end else
}




// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs()
{
    uint8_t val =0;

    if(!PB0)
        val |=1;
    if(!PB1)
        val |=2;
    if(!PB2)
        val |=4;
    if(!PB3)
        val |=8;
    if(!PB4)
        val |=16;
    if(!PB5)
        val |=32;

    return val;
}

//-----------------------------------------------------------------------------
// YOUR UNIQUE CODE
// REQUIRED: add any custom code in this space
//-----------------------------------------------------------------------------

// ------------------------------------------------------------------------------
//  Task functions
// ------------------------------------------------------------------------------

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle()
{
    while(true)
    {
        ORANGE_LED = 1;
        waitMicrosecond(1000);
        ORANGE_LED = 0;
        yield();
    }
}

void flash4Hz()
{
    while(true)
    {
        GREEN_LED ^= 1;
        sleep(125);
    }
}

void oneshot()
{
    while(true)
    {
        wait(flashReq);
        YELLOW_LED = 1;
        sleep(1000);
        YELLOW_LED = 0;
    }
}

void partOfLengthyFn()
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn()
{
    uint16_t i;
    while(true)
    {
        wait(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        RED_LED ^= 1;
        post(resource);
    }
}

void readKeys()
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            YELLOW_LED ^= 1;
            RED_LED = 1;
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            RED_LED = 0;
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            destroyThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce()
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post((uint8_t)keyReleased);
    }
}

void errant()
{
 uint32_t* p = (uint32_t*)0x20007FFC;
     while(true)
     {
         while (readPbs() == 32)
         {
             *p = 0;
               }
         yield();
     }
}

void important()
{
    while(true)
    {
        wait(resource);
        BLUE_LED = 1;
        sleep(1000);
        BLUE_LED = 0;
        post(resource);
    }
}

// REQUIRED: add processing for the shell commands through the UART here
//           your solution should not use C library calls for strings, as the stack will be too large
void shell()
{   // code for shell used from 5313
    char input[Max_letter]; // Input buffer

    char instructdb[max_inst][max_inst_size]={"ps","ipcs","kill","reboot","pidof","sched","pi","preempt"};

    char param_array[max_Arg][16] = {0};
    char temp_param[16]="";

    uint8_t position[max_position];

    putsUart0("\r\n 6314: RTOS Shell Interface \r\n");


    while (true)
    {   putsUart0("\r\n>>");
        getsUart0(input,position,param_array);
        getposition(input,position,temp_param,param_array);
        checkCommand(param_array,instructdb,temp_param);
        yield(); // enable task switch at the end
    }
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    bool ok;

    // Initialize hardware
    initHw();
    initRtos();
    initUart0();
    // Power-up flash
    POWER_LED = 1;
    waitMicrosecond(250000);
    POWER_LED = 0;
    waitMicrosecond(250000);

    // Initialize semaphores
    createSemaphore(1,"keyPressed",keyPressed);
    createSemaphore(0,"keyReleased",keyReleased);
    createSemaphore(5,"flashReq",flashReq);
    createSemaphore(1,"resource",resource);

    // Add required idle process at lowest priority
    ok =  createThread(idle, "Idle", 15, 1024);

    // Add other processes
    ok &= createThread(lengthyFn, "LengthyFn", 12, 1024);
    ok &= createThread(flash4Hz, "Flash4Hz", 8, 1024);
    ok &= createThread(oneshot, "OneShot", 4, 1024);
    ok &= createThread(readKeys, "ReadKeys", 12, 1024);
    ok &= createThread(debounce, "Debounce", 12, 1024);
    ok &= createThread(important, "Important", 0, 1024);
    ok &= createThread(errant, "Errant", 12, 1024);
    ok &= createThread(shell, "Shell", 12, 1024);


    // Start up RTOS
    if (ok)
        startRtos(); // never returns
    else
        RED_LED = 1;

    return 0;
}

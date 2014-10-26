#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  
  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

void
trap(struct trapframe *tf)
{
/*  if(tf->trapno == T_PGFLT){
    // Check if stack expansion required	  
    if(rcr2()>=proc->stackTop-PGSIZE) {
	  cprintf("PAGE FAULT!! on address %x when STACKTOP=%x\n",rcr2(),proc->stackTop);
    } // Else regular old Page Fault! 
    else {
	  cprintf("SEGMENTATION FAULT!!\n");
    }
  }	 
*/	 
  if(tf->trapno == T_SYSCALL){
    if(proc->killed)
      exit();
    proc->tf = tf;
    syscall();
    if(proc->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpu->id == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpu->id, tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
    // Check if stack expansion required	 
    cprintf("addr=%x stacktop=%x heap end=%x\n",rcr2(), proc->stackTop, proc->sz); 
    //Extend stack by another page
    if(rcr2()>=proc->stackTop-PGSIZE && proc->stackTop > proc->sz+PGSIZE) { //proc->sz is the end of heap, so checking to ensure one guard page exists
      if((proc->tf->esp = allocuvm(proc->pgdir, proc->stackTop-PGSIZE, proc->stackTop)) == 0) { //FIXME How should sp be changed? 
        cprintf("Stack Full!\n");
	proc->killed = 1;
      }
      proc->stackTop-=PGSIZE;//keep track of the top of the stack
      cprintf("stackTop=%x\n",proc->stackTop);
    }
    // Else if Guard page check fails-> 
    else if(rcr2()>=proc->stackTop-PGSIZE && proc->stackTop <= proc->sz+PGSIZE) { //proc->sz is the end of heap, so checking to ensure one guard page exists
        cprintf("Stack Full!\n"); // Should this be segmentation fault too?
	proc->killed = 1;
    }// Else regular old Page Fault! 
    else {
      cprintf("SEGMENTATION FAULT\n");
      proc->killed = 1;
    }
    break;  
  default:
    if(proc == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpu->id, tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip, 
            rcr2());
    proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running 
  // until it gets to the regular system call return.)
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();
}

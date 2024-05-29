#ifndef __INTC_H__
#define __INTC_H__

void intc_init(void);
void intc_set_handler(int irq, void (*handler)(void));
void intc_set_priority(int irq, int prio);
void intc_set_pending(int irq);
void intc_clear_pending(int irq);
void intc_enable_irq(int irq);
void intc_disable_irq(int irq);

#endif

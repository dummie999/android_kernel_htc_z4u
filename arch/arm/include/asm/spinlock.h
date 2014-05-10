#ifndef __ASM_SPINLOCK_H
#define __ASM_SPINLOCK_H

#if __LINUX_ARM_ARCH__ < 6
#error SMP not supported on pre-ARMv6 CPUs
#endif

#include <asm/processor.h>

#define ALT_SMP(smp, up)					\
	"9998:	" smp "\n"					\
	"	.pushsection \".alt.smp.init\", \"a\"\n"	\
	"	.long	9998b\n"				\
	"	" up "\n"					\
	"	.popsection\n"

#ifdef CONFIG_THUMB2_KERNEL
#define SEV		ALT_SMP("sev.w", "nop.w")
#define WFE(cond)	ALT_SMP(		\
	"it " cond "\n\t"			\
	"wfe" cond ".n",			\
						\
	"nop.w"					\
)
#else
#define SEV		ALT_SMP("sev", "nop")
#define WFE(cond)	ALT_SMP("wfe" cond, "nop")
#endif

static inline void dsb_sev(void)
{
#if __LINUX_ARM_ARCH__ >= 7
	__asm__ __volatile__ (
		"dsb\n"
		SEV
	);
#else
	__asm__ __volatile__ (
		"mcr p15, 0, %0, c7, c10, 4\n"
		SEV
		: : "r" (0)
	);
#endif
}

#ifndef CONFIG_ARM_TICKET_LOCKS

#define arch_spin_is_locked(x)		((x)->lock != 0)
#define arch_spin_unlock_wait(lock) \
	do { while (arch_spin_is_locked(lock)) cpu_relax(); } while (0)

#define arch_spin_lock_flags(lock, flags) arch_spin_lock(lock)

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
	WFE("ne")
"	strexeq	%0, %2, [%1]\n"
"	teqeq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	smp_mb();

	__asm__ __volatile__(
"	str	%1, [%0]\n"
	:
	: "r" (&lock->lock), "r" (0)
	: "cc");

	dsb_sev();
}
#else

#define TICKET_SHIFT	16
#define TICKET_BITS	16
#define	TICKET_MASK	0xFFFF

#define arch_spin_lock_flags(lock, flags) arch_spin_lock(lock)

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned long tmp, ticket, next_ticket;

	
	__asm__ __volatile__(
"1:	ldrex	%[ticket], [%[lockaddr]]\n"
"	uadd16	%[next_ticket], %[ticket], %[val1]\n"
"	strex	%[tmp], %[next_ticket], [%[lockaddr]]\n"
"	teq	%[tmp], #0\n"
"	bne	1b\n"
"	uxth	%[ticket], %[ticket]\n"
"2:\n"
#ifdef CONFIG_CPU_32v6K
"	wfene\n"
#endif
"	ldr	%[tmp], [%[lockaddr]]\n"
"	cmp	%[ticket], %[tmp], lsr #16\n"
"	bne	2b"
	: [ticket]"=&r" (ticket), [tmp]"=&r" (tmp), [next_ticket]"=&r" (next_ticket)
	: [lockaddr]"r" (&lock->lock), [val1]"r" (1)
	: "cc");
	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned long tmp, ticket, next_ticket;

	
	__asm__ __volatile__(
"	ldrex	%[ticket], [%[lockaddr]]\n"
"	ror	%[tmp], %[ticket], #16\n"
"	eors	%[tmp], %[tmp], %[ticket]\n"
"	bne	1f\n"
"	uadd16	%[next_ticket], %[ticket], %[val1]\n"
"	strex	%[tmp], %[next_ticket], [%[lockaddr]]\n"
"1:"
	: [ticket]"=&r" (ticket), [tmp]"=&r" (tmp),
	  [next_ticket]"=&r" (next_ticket)
	: [lockaddr]"r" (&lock->lock), [val1]"r" (1)
	: "cc");
	if (!tmp)
		smp_mb();
	return !tmp;
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	unsigned long ticket, tmp;

	smp_mb();

	
	__asm__ __volatile__(
"1:	ldrex	%[ticket], [%[lockaddr]]\n"
"	uadd16	%[ticket], %[ticket], %[serving1]\n"
"	strex	%[tmp], %[ticket], [%[lockaddr]]\n"
"	teq	%[tmp], #0\n"
"	bne	1b"
	: [ticket]"=&r" (ticket), [tmp]"=&r" (tmp)
	: [lockaddr]"r" (&lock->lock), [serving1]"r" (0x00010000)
	: "cc");
	dsb_sev();
}

static inline void arch_spin_unlock_wait(arch_spinlock_t *lock)
{
	unsigned long ticket;

	
	__asm__ __volatile__(
#ifdef CONFIG_CPU_32v6K
"	cmpne	%[lockaddr], %[lockaddr]\n"
"1:	wfene\n"
#else
"1:\n"
#endif
"	ldr	%[ticket], [%[lockaddr]]\n"
"	eor	%[ticket], %[ticket], %[ticket], lsr #16\n"
"	uxth	%[ticket], %[ticket]\n"
"	cmp	%[ticket], #0\n"
"	bne	1b"
	: [ticket]"=&r" (ticket)
	: [lockaddr]"r" (&lock->lock)
	: "cc");
}

static inline int arch_spin_is_locked(arch_spinlock_t *lock)
{
	unsigned long tmp = ACCESS_ONCE(lock->lock);
	return (((tmp >> TICKET_SHIFT) ^ tmp) & TICKET_MASK) != 0;
}

static inline int arch_spin_is_contended(arch_spinlock_t *lock)
{
	unsigned long tmp = ACCESS_ONCE(lock->lock);
	return ((tmp - (tmp >> TICKET_SHIFT)) & TICKET_MASK) > 1;
}
#endif


static inline void arch_write_lock(arch_rwlock_t *rw)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
	WFE("ne")
"	strexeq	%0, %2, [%1]\n"
"	teq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

	smp_mb();
}

static inline int arch_write_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}

static inline void arch_write_unlock(arch_rwlock_t *rw)
{
	smp_mb();

	__asm__ __volatile__(
	"str	%1, [%0]\n"
	:
	: "r" (&rw->lock), "r" (0)
	: "cc");

	dsb_sev();
}

#define arch_write_can_lock(x)		((x)->lock == 0)

static inline void arch_read_lock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	adds	%0, %0, #1\n"
"	strexpl	%1, %0, [%2]\n"
	WFE("mi")
"	rsbpls	%0, %1, #0\n"
"	bmi	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
}

static inline void arch_read_unlock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	smp_mb();

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	sub	%0, %0, #1\n"
"	strex	%1, %0, [%2]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	if (tmp == 0)
		dsb_sev();
}

static inline int arch_read_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2 = 1;

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	adds	%0, %0, #1\n"
"	strexpl	%1, %0, [%2]\n"
	: "=&r" (tmp), "+r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
	return tmp2 == 0;
}

#define arch_read_can_lock(x)		((x)->lock < 0x80000000)

#define arch_read_lock_flags(lock, flags) arch_read_lock(lock)
#define arch_write_lock_flags(lock, flags) arch_write_lock(lock)

#define arch_spin_relax(lock)	cpu_relax()
#define arch_read_relax(lock)	cpu_relax()
#define arch_write_relax(lock)	cpu_relax()

#endif 

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>

static DEFINE_SPINLOCK(lock_a);
static DEFINE_SPINLOCK(lock_b);

static int __init deadlock_init(void)
{
    pr_info("deadlock_demo: loading module\n");

    /* Thread 1 order: A -> B */
    spin_lock(&lock_a);
    spin_lock(&lock_b);
    spin_unlock(&lock_b);
    spin_unlock(&lock_a);

    /* Thread 2 order: B -> A  <-- reversed, Lockdep detects this */
    spin_lock(&lock_b);
    spin_lock(&lock_a);   /* Lockdep will warn here */
    spin_unlock(&lock_a);
    spin_unlock(&lock_b);

    pr_info("deadlock_demo: done\n");
    return 0;
}

static void __exit deadlock_exit(void)
{
    pr_info("deadlock_demo: unloading module\n");
}

module_init(deadlock_init);
module_exit(deadlock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("Lockdep deadlock demonstration");

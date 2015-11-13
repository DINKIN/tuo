#2.6 kernel provides softirqs, tasklets and work queues as available types of bottom halves

# Work Queues #

http://www.linuxjournal.com/article/6916

Work queues are interesting for two main reasons. First, they are the simplest to use of all the bottom-half mechanisms. Second, they are the only bottom-half mechanism that runs in process context; thus, work queues often are the only option device driver writers have when their bottom half must sleep. In addition, the work queue mechanism is brand new, and new is cool.

Let's discuss the fact that work queues run in process context. This is in contrast to the other bottom-half mechanisms, which all run in interrupt context. Code running in interrupt context is unable to sleep, or block, because interrupt context does not have a backing process with which to reschedule. Therefore, because interrupt handlers are not associated with a process, there is nothing for the scheduler to put to sleep and, more importantly, nothing for the scheduler to wake up. Consequently, interrupt context cannot perform certain actions that can result in the kernel putting the current context to sleep, such as downing a semaphore, copying to or from user-space memory or non-atomically allocating memory. Because work queues run in process context (they are executed by kernel threads, as we shall see), they are fully capable of sleeping. The kernel schedules bottom halves running in work queues, in fact, the same as any other process on the system. As with any other kernel thread, work queues can sleep, invoke the scheduler and so on.

## The Work Queue Interface ##

The first step in using work queues is creating a work queue structure. The work queue structure is represented by struct work\_struct and defined in linux/workqueue.h. Thankfully, one of two different macros makes the job of creating a work queue structure easy. If you want to create your work queue structure statically (say, as a global variable), you can declare it directly with:

DECLARE\_WORK(name, function, data)

This macro creates a struct work\_struct and initializes it with the given work queue handler, function. Your work queue handler must match the following prototype:

void my\_workqueue\_handler(void **arg)**

The arg parameter is a pointer passed to your work queue handler by the kernel each time it is invoked. It is specified by the data parameter in the DECLARE\_WORKQUEUE() macro. By using a parameter, device drivers can use a single work queue handler for multiple work queues. The data parameter can be used to distinguish between work queues.

If you do not want to create your work queue structure directly but instead dynamically, you can do that too. If you have only indirect reference to the work queue structure, say, because you created it with kmalloc(), you can initialize it using:

INIT\_WORK(p, function, data)

In this case, p is a pointer to a work\_struct structure, function is the work queue handler and data is the lone argument the kernel passes to it on invocation.

Creating the work queue structure normally is done once—for example, in your driver's initialization routine. The kernel uses the work queue structure to keep track of the various work queues on the system. You need to keep track of the structure, because you will need it later.

## Your Work Queue Handler ##

Basically, your work queue handler can do whatever you want. It is your bottom half, after all. The only stipulation is that the handler's function fits the correct prototype. Because your work queue handler runs in process context, it can sleep if needed.

So you have a work queue data structure and a work queue handler—how do you schedule it to run? To queue a given work queue handler to run at the kernel's earliest possible convenience, invoke the following function, passing it your work queue structure:

int schedule\_work(struct work\_struct **work)**

This function returns nonzero if the work was successfully queued; on error, it returns zero. The function can be called from either process or interrupt context.

Sometimes, you may not want the scheduled work to run immediately, but only after a specified period has elapsed. In those situations, use:

int schedule\_delayed\_work(struct work\_struct **work,
> unsigned long delay)**

In this case, the work queue handler associated with the given work queue structure will not run for at least delay jiffies. For example, if you have a work queue structure named my\_work and you wish to delay its execution for five seconds, call:

schedule\_delayed\_work(&my\_work, 5\*HZ)


Normally, you would schedule your work queue handler from your interrupt handler, but nothing stops you from scheduling it from anywhere you want. In normal practice, the interrupt handler and the bottom half work together as a team. They each perform a specific share of the duties involved in processing a device's interrupt. The interrupt handler, as the top half of the solution, usually prepares the remaining work for the bottom half and then schedules the bottom half to run. You conceivably can use work queues for jobs other than bottom-half processing, however.

## Work Queue Management ##

When you queue work, it is executed when the worker thread next wakes up. Sometimes, you need to guarantee in your kernel code that your queued work has completed before continuing. This is especially important for modules, which need to ensure any pending bottom halves have executed before unloading. For these needs, the kernel provides a function to wait on all work pending for the worker thread:

void flush\_scheduled\_work(void)

Because this function waits on all pending work for the worker thread, it might take a relatively long time to complete. While waiting for the worker threads to finish executing all pending work, the call sleeps. Therefore, you must call this function only from process context. Do not call it unless you truly need to ensure that your scheduled work is executed and no longer pending.

This function does not flush any pending delayed work. If you scheduled the work with a delay, and the delay is not yet up, you need to cancel the delay before flushing the work queue:

int cancel\_delayed\_work(struct work\_struct **work)**

In addition, this function cancels the timer associated with the given work queue structure—other work queues are not affected. You can call cancel\_delayed\_work() only from process context because it may sleep. It returns nonzero if any outstanding work was canceled; otherwise, it returns zero.
Creating New Worker Threads

In rare cases, the default worker threads may be insufficient. Thankfully, the work queue interface allows you to create your own worker threads and use those to schedule your bottom-half work. To create new worker threads, invoke the function:

struct workqueue\_struct **create\_workqueue(const char**name)

For example, on system initialization, the kernel creates the default queues with:

keventd\_wq = create\_workqueue("events");

This function creates all of the per-processor worker threads. It returns a pointer to a struct workqueue\_struct, which is used to identify this work queue from other work queues (such as the default one). Once you create the worker thread, you can queue work in a fashion similar to how work is queued with the default worker thread:

int queue\_work(struct workqueue\_struct **wq,
> struct work\_struct**work)

Here, wq is a pointer to the specific work queue that you created using the call to create\_workqueue(), and work is a pointer to your work queue structure. Alternatively, you can schedule work with a delay:

int
queue\_delayed\_work(struct workqueue\_struct **wq,
> struct work\_struct**work,
> unsigned long delay)

This function works the same as queue\_work(), except it delays the queuing of the work for delay jiffies. These two functions are analogous to schedule\_work() and schedule\_delayed\_work(), except they queue the given work into the given work queue instead of the default one. Both functions return nonzero on success and zero on failure. Both functions may be called from both interrupt and process context.

Finally, you may flush a specific work queue with the function:

void flush\_workqueue(struct workqueue\_struct **wq)**

This function waits until all queued work on the wq work queue has completed before returning.

## Conclusion ##

The work queue interface has been a part of the kernel since 2.5.41. In that time, a large number of drivers and subsystems have made it their method of deferring work. But is it the right bottom half for you? If you need to run your bottom half in process context, a work queue is your only option. Furthermore, if you are considering creating a kernel thread, a work queue may be a better choice. But what if you do not need a bottom half that can sleep? In that case, you may find tasklets are a better choice. They also are easy to use, but they do not run in a kernel thread. Because they are not run in process context, no context switch overhead is associated with their execution; therefore, they may offer you less overhead.

## Work Queue Function Reference ##

Statically create a work queue structure:

DECLARE\_WORK(name, function, data)

Dynamically initialize a work queue structure:

INIT\_WORK(p, function, data)

Create a new worker thread:

struct workqueue\_struct
**create\_workqueue(const char**name)

Destroy a worker thread:

void
destroy\_workqueue(struct workqueue\_struct **wq)**

Queue work to a given worker thread:

int
queue\_work(struct workqueue\_struct **wq,
> struct work\_struct**work)

Queue work, after the given delay, to the given worker thread:

int
queue\_delayed\_work(struct workqueue\_struct **wq,
> struct work\_struct**work,
> unsigned long delay)

Wait on all pending work on the given worker thread:

void
flush\_workqueue(struct workqueue\_struct **wq)**

Schedule work to the default worker thread:

int
schedule\_work(struct work\_struct **work)**

Schedule work, after a given delay, to the default worker thread:

int
schedule\_delayed\_work(struct work\_struct **work,
> unsigned long delay)**

Wait on all pending work on the default worker thread:

void
flush\_scheduled\_work(void)

Cancel the given delayed work:

int
cancel\_delayed\_work(struct work\_struct 
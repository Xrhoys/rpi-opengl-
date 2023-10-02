/* date = July 14th 2023 1:09 am */

#ifndef LINUX_MAIN_H
#define LINUX_MAIN_H

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800
#define REFRESH_RATE 120
#define THREAD_WORK_COUNT 1
#define WORK_QUEUE_SIZE 2

#define internal static
#define global   static

typedef void (*thread_func_t)(void *arg);

internal DEBUG_CLOCK_GET_TIME(LinuxGetLastElapsed);
internal DEBUG_PLATFORM_FREE_FILE_MEMORY(LinuxFreeFile);
internal DEBUG_PLATFORM_WRITE_ENTIRE_FILE(LinuxWriteEntireFile);
internal DEBUG_PLATFORM_READ_ENTIRE_FILE(LinuxReadEntireFile);

enum linux_work_unit_status
{
	WORK_UNIT_STATUS_UNSET,
	WORK_UNIT_STATUS_LOCK,
	WORK_UNIT_STATUS_READY,
	WORK_UNIT_STATUS_PROCESSING,
	WORK_UNIT_STATUS_DONE, // NOTE(Ecy): this status block further use of the thread
	
	WORK_UNIT_STATUS_COUNT,
};

struct linux_work_unit
{
	pthread_t threadId;
	linux_work_unit_status status;
	
	thread_func_t funcPtr;
	void *param;
};

struct linux_work_queue
{
	linux_work_unit queue[WORK_QUEUE_SIZE];
};

inline linux_work_unit*
QueueThreadedWork(linux_work_queue *group, thread_func_t funcPtr, void *param)
{
	linux_work_unit *unit = nullptr;
	for(u32 index = 0;
		index < WORK_QUEUE_SIZE;
		++index)
	{
		unit = &group->queue[index];
		
		if(unit->status == WORK_UNIT_STATUS_UNSET)
		{
			fprintf(stderr, "queueing thread: %d\n", index);
			
			unit = &group->queue[index];
			unit->status = WORK_UNIT_STATUS_LOCK;
			unit->funcPtr = funcPtr;
			unit->param = param;
			unit->status = WORK_UNIT_STATUS_READY;
			
			break;
		}
		
		unit = nullptr;
	}
	
	return unit;
}

static void*
ProcessThreadWork(void *data)
{
	linux_work_queue *workGroup = (linux_work_queue*)data;
	
	while(1)
	{
		// Find work to do
		for(u32 index = 0;
			index < WORK_QUEUE_SIZE;
			++index)
		{
			linux_work_unit *unit = &workGroup->queue[index];
			
			switch(unit->status)
			{
				case WORK_UNIT_STATUS_READY:
				{				
					fprintf(stderr, "pop threaad %d, %d\n", unit->threadId, index);
					
					// ready to be processed
					Assert(unit->funcPtr);
					Assert(unit->param);
					
					unit->status = WORK_UNIT_STATUS_PROCESSING;
					unit->funcPtr(unit->param);
					unit->status = WORK_UNIT_STATUS_UNSET;
				}break;
				
				case WORK_UNIT_STATUS_PROCESSING:
				{
					// only used for logging/debug
				}break;
				
				case WORK_UNIT_STATUS_DONE:
				{
					// used to stall the thread
				}break;
				
				case WORK_UNIT_STATUS_UNSET:
				default:
				{
					// do nothing
				}break;
			}
			
		}
		
		// NOTE(Ecy): this could be needed
		//usleep(1);
	}
	
	
	return NULL;
}

#endif //LINUX_MAIN_H

#include "PTPowerManagement.h"
#include <assert.h>


#if __APPLE__

#include <IOKit/pwr_mgt/IOPMLib.h>


static IOPMAssertionID sAssertion;


static void DoStartPreventingSleep(void)
{
	// IOPM stuff was added in 10.6, so weak-linked for 10.5 compatibility.
	if (IOPMAssertionCreateWithName == NULL)  return;
	
	IOReturn status = IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep, kIOPMAssertionLevelOn, CFSTR("Rendering planet"), &sAssertion);
	if (status != kIOReturnSuccess)  sAssertion = kIOPMNullAssertionID;
}


static void DoStopPreventingSleep(void)
{
	// IOPM stuff was added in 10.6, so weak-linked for 10.5 compatibility.
	if (IOPMAssertionRelease == NULL)  return;
	
	if (sAssertion != kIOPMNullAssertionID)
	{
		IOPMAssertionRelease(sAssertion);
		sAssertion = kIOPMNullAssertionID;
	}
}

#else

static void DoStartPreventingSleep(void)
{
	
}


static void DoStopPreventingSleep(void)
{
	
}

#endif


static unsigned sPreventionCount;


void PTStartPreventingSleep(void)
{
	if (sPreventionCount++ == 0)
	{
		DoStartPreventingSleep();
	}
}


void PTStopPreventingSleep(void)
{
	assert(sPreventionCount != 0);
	
	if (--sPreventionCount == 0)
	{
		DoStopPreventingSleep();
	}
}


bool PTIsPreventingSleep(void)
{
	return sPreventionCount != 0;
}

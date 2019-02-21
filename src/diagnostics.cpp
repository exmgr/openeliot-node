#include "diagnostics.h"

namespace Diagnostics
{
	//
	// Private vars
	//
	/** Number of failed Calling Home procedures since last successfull */
	int call_home_failures = 0;

	/** Duration of last call time */
	int call_home_duration = 0;

	/******************************************************************************
	 * Accessors
	 *****************************************************************************/
	void set_call_home_failures(int val)
	{
		call_home_failures = val;
	}
	int get_call_home_failres()
	{
		return call_home_failures;
	}
}
#pragma once

class CPTCallback
{
public:
	virtual void UpdateProgress(long nProgress, long nTotal) = 0;
};
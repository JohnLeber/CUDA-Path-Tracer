#pragma once

class CCUDACallback
{
public:
	virtual void CUDAProgress(long nProgress, long nTotal) = 0;
};
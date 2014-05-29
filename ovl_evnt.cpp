
OVERLAPPED g_OvlStartStream[2];	// событие окончания всего составного буфера
HANDLE g_hBlockEndEvent[2];	// событие окончания блока

//***************************************************************************************
ULONG AllocMemory(PVOID* pBuf, ULONG blkSize, ULONG blkNum, ULONG isSysMem, ULONG dir, ULONG addr, int DmaChan)
{
	g_OvlStartStream[DmaChan].hEvent = CreateEvent (NULL, TRUE, TRUE, NULL); // начальное состояние Signaled
	TCHAR nameEvent[MAX_PATH];
	sprintf_s(nameEvent, _T("event_DmaChan_%d"), DmaChan);
	g_hBlockEndEvent[DmaChan] = CreateEvent(NULL, TRUE, TRUE, nameEvent); // начальное состояние Signaled

	g_pMemDescrip[DmaChan]->hBlockEndEvent = g_hBlockEndEvent[DmaChan];


//***************************************************************************************
ULONG FreeMemory(int DmaChan)
{
	if(g_hBlockEndEvent[DmaChan])
		CloseHandle(g_hBlockEndEvent[DmaChan]);
	if(g_OvlStartStream[DmaChan].hEvent)
		CloseHandle(g_OvlStartStream[DmaChan].hEvent);


//***************************************************************************************
ULONG StartDma(int IsCycling, int DmaChan)
{
		ResetEvent(g_OvlStartStream[DmaChan].hEvent); // сброс в состояние Non-Signaled перед стартом
		ResetEvent(g_hBlockEndEvent[DmaChan]); // сброс в состояние Non-Signaled перед стартом

		AMB_START_DMA_CHANNEL StartDescrip;
		StartDescrip.DmaChanNum = DmaChan;
		StartDescrip.IsCycling = IsCycling;

		ULONG   length;  

		if (!DeviceIoControl(
				g_hWDM, 
				IOCTL_AMB_START_MEMIO,
				&StartDescrip,
				sizeof(AMB_START_DMA_CHANNEL),
				NULL,
				0,
				&length,
				&(g_OvlStartStream[DmaChan]))) {
			return GetLastError();
		}


//***************************************************************************************
ULONG WaitBuffer(ULONG msTimeout, int DmaChan)
{
	if(g_pMemDescrip[DmaChan])
	{
		int Status = WaitForSingleObject(g_OvlStartStream[DmaChan].hEvent, msTimeout);
		if(Status == WAIT_TIMEOUT) 
			return TIMEOUT_ERROR;
		return 0;
	}
	return NON_MEMORY_ERROR;
}

//***************************************************************************************
ULONG WaitBlock(ULONG msTimeout, int DmaChan)
{
	if(g_pMemDescrip[DmaChan])
	{
		int Status = WaitForSingleObject(g_hBlockEndEvent[DmaChan], msTimeout);
		if(Status == WAIT_TIMEOUT) 
			return TIMEOUT_ERROR;
		else
			ResetEvent(g_hBlockEndEvent); // сброс в состояние Non-Signaled после завершения блока
		return 0;
	}
	return NON_MEMORY_ERROR;
}

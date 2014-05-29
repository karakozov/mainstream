
OVERLAPPED g_OvlStartStream[2];	// ������� ��������� ����� ���������� ������
HANDLE g_hBlockEndEvent[2];	// ������� ��������� �����

//***************************************************************************************
ULONG AllocMemory(PVOID* pBuf, ULONG blkSize, ULONG blkNum, ULONG isSysMem, ULONG dir, ULONG addr, int DmaChan)
{
	g_OvlStartStream[DmaChan].hEvent = CreateEvent (NULL, TRUE, TRUE, NULL); // ��������� ��������� Signaled
	TCHAR nameEvent[MAX_PATH];
	sprintf_s(nameEvent, _T("event_DmaChan_%d"), DmaChan);
	g_hBlockEndEvent[DmaChan] = CreateEvent(NULL, TRUE, TRUE, nameEvent); // ��������� ��������� Signaled

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
		ResetEvent(g_OvlStartStream[DmaChan].hEvent); // ����� � ��������� Non-Signaled ����� �������
		ResetEvent(g_hBlockEndEvent[DmaChan]); // ����� � ��������� Non-Signaled ����� �������

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
			ResetEvent(g_hBlockEndEvent); // ����� � ��������� Non-Signaled ����� ���������� �����
		return 0;
	}
	return NON_MEMORY_ERROR;
}

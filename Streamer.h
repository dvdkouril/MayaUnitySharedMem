#pragma once

#include <windows.h>
#include <map>
#include <maya/MGlobal.h>
#include <maya/MUiMessage.h>

#define BUF_SIZE 256

enum class StreamerState
{
	Running,
	Paused,
	Stopped
};

class Streamer
{
private:
	StreamerState				state;
	MCallbackId					mCallbackId;
	HANDLE						hCamInfoShMem;
	HANDLE						hMapFile;
	HANDLE						hSceneInfoShMem;
	HANDLE						hPdbMappingShMem;
	LPVOID						pCamInfo;
	LPVOID						pBuf;
	LPVOID						pSceneInfo;
	LPVOID						pPdbMappingShMem;
	std::map<std::string, int>	pdbIdMap;
	size_t						nextFreeInternalId;
	float						lastFrameTime;

public:
	Streamer(void);
	~Streamer(void);

	void update(const MString& panelName, void * data); // callback function
	
	void start();
	void pause();
	void stop();

private:
	bool initSharedMemory(size_t numberOfObjs);
	void freeSharedMemory();
	HANDLE createSharedMemory(char * name, size_t size);
	LPVOID createMemoryMapping(HANDLE handle);
};


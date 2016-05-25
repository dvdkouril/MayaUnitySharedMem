#pragma once

#include <maya/MPxLocatorNode.h>
#include <maya/MGlobal.h>
#include <maya/MArgList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>

#include <windows.h>
#include <conio.h>
#include <tchar.h>
#include <map>
#include <fstream>

#include "StreamingWindow.h"

class CustomLocator : MPxLocatorNode 
{
public:
	CustomLocator();
	virtual ~CustomLocator() { freeSharedMemory(); }

	virtual void			draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status) override;
	virtual MStatus			compute(const MPlug& plug, MDataBlock& data) override;
	virtual bool            isBounded() const;
	virtual MBoundingBox	boundingBox() const;

	// these two are not actually inherited functions, they could be named any other way because you specify them when calling MFnPlugin::registerNode (at least I think...)
	static  MStatus         initialize();
	static  void *          creator();

	//void					setWindowPointer(StreamingWindow *ptr) {this->windowPtr = ptr;} // is there a disadvantage of doing this instead of defining this in the .cpp?

	static const MTypeId typeId;
	static const MString typeName;

private:
	HANDLE hCamInfoShMem;
	HANDLE hMapFile;
	HANDLE hSceneInfoShMem;
	HANDLE hPdbMappingShMem;
	LPVOID pCamInfo;
	LPVOID pBuf;
	LPVOID pSceneInfo;
	LPVOID pPdbMappingShMem;

	//StreamingWindow *windowPtr; // this is not going to work actually

	std::map<std::string, int> pdbIdMap;
	size_t nextFreeInternalId;
	float lastFrameTime;

	//static MObject a_outFrameTime;

	bool initSharedMemory(size_t numberOfObjs);
	void freeSharedMemory();
	static MQuaternion rotationMayaToUnity(MQuaternion q);
	HANDLE createSharedMemory(char * name, size_t size);
	LPVOID createMemoryMapping(HANDLE handle);

	std::ofstream pdbIdsFile;
	// debug output
	std::ofstream debugOut;
};

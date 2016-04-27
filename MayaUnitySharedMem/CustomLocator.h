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

class CustomLocator : MPxLocatorNode 
{
public:
	//CustomLocator() : MPxLocatorNode() { initSharedMemory(); }
	CustomLocator();
	virtual ~CustomLocator() { freeSharedMemory(); debugOut.close(); }

	virtual void			draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);
	virtual bool            isBounded() const;
	virtual MBoundingBox	boundingBox() const;

	// these two are not actually inherited functions, they could be named any other way because you specify them when calling MFnPlugin::registerNode (at least I think...)
	static  MStatus         initialize();
	static  void *          creator();

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

	std::map<std::string, int> pdbIdMap;
	size_t nextFreeInternalId;

	bool initSharedMemory(size_t numberOfObjs);
	void freeSharedMemory();
	static MQuaternion rotationMayaToUnity(MQuaternion q);
	HANDLE createSharedMemory(char * name, size_t size);
	LPVOID createMemoryMapping(HANDLE handle);

	std::ofstream pdbIdsFile;
	// debug output
	std::ofstream debugOut;
};

const MTypeId CustomLocator::typeId(0x70000);
const MString CustomLocator::typeName("customLocator");
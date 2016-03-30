#pragma once

#include <maya/MPxLocatorNode.h>
#include <maya/MGlobal.h>
#include <maya/MArgList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>

#include <windows.h>
#include <conio.h>
#include <tchar.h>

class CustomLocator : MPxLocatorNode 
{
public:
	CustomLocator() : MPxLocatorNode() { initSharedMemory(); }
	virtual ~CustomLocator() { freeSharedMemory();  }

	virtual void			draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);
	virtual bool            isBounded() const;
	virtual MBoundingBox	boundingBox() const;

	static  MStatus         initialize();
	static  void *          creator();

	static const MTypeId typeId;
	static const MString typeName;
private:
	//TCHAR szName[] = TEXT("bla");
	//TCHAR * szName; // I don't really need to bother with this
	HANDLE hMapFile;
	LPCTSTR pBuf;
	bool initSharedMemory();
	void freeSharedMemory();
};

const MTypeId CustomLocator::typeId(0x70000);
const MString CustomLocator::typeName("customLocator");
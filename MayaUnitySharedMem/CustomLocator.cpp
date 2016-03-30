#include "CustomLocator.h"
#include <maya/MFnPlugin.h>

#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTransform.h>
#include <maya/MVector.h>
#include <maya/MSelectionList.h>
#include <string>
#include <sstream>
#include <iostream>

#define BUF_SIZE 256

MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, "David Kouril", "1.0", "Any");

	MStatus status = plugin.registerNode(CustomLocator::typeName,
		CustomLocator::typeId,
		CustomLocator::creator,
		CustomLocator::initialize,
		MPxNode::kLocatorNode); // is this why it didn't work before?????????

	if (!status)
	{
		status.perror("Failed to register customLocator\n");
		return status;
	}

	std::cout << "initializePlugin successful." << std::endl;
	
	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	MStatus status = plugin.deregisterNode(CustomLocator::typeId);
	if (!status)
	{
		status.perror("Failed to deregister customLocator\n");
		return status;
	}

	std::cout << "uninitializePlugin successful." << std::endl;
	return status;
}

/*
	Working draw method. Outputs just position of one cube (pCube1)
*/
//void CustomLocator::draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
//{
//	std::cout << "CustomLocator::draw" << std::endl;
//	// use CopyMemory here
//	MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
//	for (; !itTran.isDone(); itTran.next())
//	{
//		MObject obj = itTran.currentItem();
//
//		MFnDependencyNode nodeFn(obj);
//		MString nodeName = nodeFn.name();
//
//		// just looking for one specific instance of cube right now
//		if (!strcmp(nodeName.asChar(), "pCube1")) {
//			
//			MFnTransform fn(obj);
//			MVector translation = fn.getTranslation(MSpace::kObject);
//
//			std::ostringstream os;
//			os << translation.x << " " << translation.y << " " << translation.z << std::endl;
//			std::string s = os.str();
//
//			const char *msg = s.c_str();
//			CopyMemory((PVOID)pBuf, msg, (_tcslen(msg) * sizeof(TCHAR)));
//		}
//	}
//
//	// this is unnecessary (I hope)
//	view.beginGL();
//	view.endGL();
//}

// don't fucking forget to change this!!!
void CustomLocator::draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
{
	// "binary" writing

	/* steps:
	- go through scene, get selection list
	- get number of object to output
	- make an array of that size * 3 * 4 (3 floats - positions)
	- fill array by going through selection list
	- CopyMemory (in chunk, outside of the iteration loop)
	*/
	MSelectionList sl;

	// this loop just filters the scene and add objects I want to output into a selection list
	// I need to do this because I need to get number of objects so that I can allocate (local) array of that size
	// new, malloc is expensive
	MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
	for (; !itTran.isDone(); itTran.next())
	{
		MObject obj = itTran.currentItem();
		// get the name of the object
		MFnDependencyNode nodeFn(obj);
		MString nodeName = nodeFn.name();

		// filter out object I don't want to output
		if (strstr(nodeName.asChar(), "pCube") != NULL)
		{
			sl.add(obj); // add it into selection list
		}
	}

	//float output[sl.length()]; // oh yeah, I forgot can't do this

	for (int i = 0; i < sl.length(); ++i)
	{
		MObject obj;
		sl.getDependNode(i, obj);
		MFnTransform fn(obj);
		MVector translation = fn.getTranslation(MSpace::kObject);
	}
	MFnTransform fn(obj);
	MVector translation = fn.getTranslation(MSpace::kObject);
	float pos[3] = { translation.x, translation.y, translation.z };

	CopyMemory((PVOID)pBuf, msg, (_tcslen(msg) * sizeof(float))); // actually this should be 
	
}

bool CustomLocator::isBounded() const
{
	return true;
}

MBoundingBox CustomLocator::boundingBox() const
{
	MBoundingBox bbox;
	// simply expand the bounding box to contain the points used
	bbox.expand(MPoint(-0.5f, 0.0f, -0.5f));
	bbox.expand(MPoint(0.5f, 0.0f, -0.5f));
	bbox.expand(MPoint(0.5f, 0.0f, 0.5f));
	bbox.expand(MPoint(-0.5f, 0.0f, 0.5f));
	bbox.expand(MPoint(0.0f, -0.5f, 0.0f));
	bbox.expand(MPoint(0.0f, 0.5f, 0.0f));
	return bbox;
}

void * CustomLocator::creator()
{
	return new CustomLocator;
}

MStatus CustomLocator::initialize()
{
	std::cout << "CustomLocator::initialize" << std::endl;
	return MS::kSuccess;
}

// ==================================================== Shared Memory related functions
bool CustomLocator::initSharedMemory() 
{
	std::cout << "initSharedMemory()" << std::endl;
	TCHAR szName[] = TEXT("MyFileMappingObject");

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUF_SIZE, 
		szName);

	if (hMapFile == NULL)
	{
		DWORD err = GetLastError();
		_tprintf(TEXT("Could not create file mapping object: (%d)\n"), err);
		return false;
	}

	this->pBuf = (LPTSTR)MapViewOfFile(
		this->hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);

	if (pBuf == NULL)
	{
		DWORD err = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d) \n"), err);
		CloseHandle(hMapFile);
		return false;
	}

	return true;

}

void CustomLocator::freeSharedMemory()
{
	std::cout << "freeSharedMemory()" << std::endl;
	UnmapViewOfFile(this->pBuf);
	CloseHandle(this->hMapFile);
}
#include "CustomLocator.h"
#include <maya/MFnPlugin.h>

#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTransform.h>
#include <maya/MVector.h>
#include <maya/MQuaternion.h>
#include <maya/MEulerRotation.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include "startStreamingCommand.h"
#include "endStreamingCommand.h"

#define BUF_SIZE 256

// ------------------------------------------------------------------------------------- MAYA PLUGIN REQUIRED FUNCTIONS
// it might be better to move this somewhere else, but for now...

MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, "David Kouril", "1.0", "Any");

	MStatus status = plugin.registerNode(CustomLocator::typeName,
		CustomLocator::typeId,
		CustomLocator::creator,
		CustomLocator::initialize,
		MPxNode::kLocatorNode); // is this why it didn't work before????????? yep, looks like it

	// register startStreamingCommand command
	status = plugin.registerCommand("startM2CVStreaming", startStreamingCommand::creator);
	status = plugin.registerCommand("endM2CVStreaming", endStreamingCommand::creator);

	// TODO: add a custom menu with items: Start streaming, Stop streaming
	// yeah it looks like it works
	MGlobal::executeCommand("global string $gMainWindow");
	MGlobal::executeCommand("setParent $gMainWindow");
	MGlobal::executeCommand("menu -label \"MayaToCellVIEW\" mayaToCelViewMenu");

	MGlobal::executeCommand("setParent -menu mayaToCelViewMenu");
	MGlobal::executeCommand("menuItem -label \"Start Streaming\" -command \"startM2CVStreaming\" startStreamingItem");
	MGlobal::executeCommand("menuItem -label \"End Streaming\" -command \"endM2CVStreaming\" -enable false endStreamingItem");

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

	// TODO: delete custom menu
	MStatus status = plugin.deregisterNode(CustomLocator::typeId);
	status = plugin.deregisterCommand("startM2CVStreaming");
	status = plugin.deregisterCommand("endM2CVStreaming");

	MGlobal::executeCommand("global string $gMainWindow");
	MGlobal::executeCommand("setParent $gMainWindow");
	MGlobal::executeCommand("deleteUI \"mayaToCelViewMenu\""); // finally this works

	if (!status)
	{
		status.perror("Failed to deregister customLocator\n");
		return status;
	}

	std::cout << "uninitializePlugin successful." << std::endl;
	return status;
}

/*
	My utility function, probably shouldn't be here...
*/
MQuaternion CustomLocator::rotationMayaToUnity(MQuaternion q)
{
	MEulerRotation rotAng;
	rotAng = q.asEulerRotation();
	rotAng.x = -rotAng.x;
	rotAng.y = -rotAng.y;
	MEulerRotation reordered = rotAng.reorder(MEulerRotation::kZXY);
	MQuaternion res = reordered.asQuaternion();

	return res;
}

// ------------------------------------------------------------------------------------- MAYA PLUGIN REQUIRED FUNCTIONS end

void CustomLocator::draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
{
	std::vector<float> posMemOutArray; // array for positions (3 consecutive floats are single object's position)
	std::vector<float> rotMemOutArray; // array for rotations (4 consecutive floats are single object's rotation)
	std::vector<float> idMemOutArray; // I will see what the format of this will be... TODO, I am not using or outputting this right now
	std::vector<float> cameraParams;  // position (3 floats) | rotation (4 floats) | ?

	// MAYBE TODO: don't go over the whole scene but keep this information somewhere and update it when it changes.
	// The thing is that this actually gets called only when the viewport has changed (objects or camera)
	// so it's maybe already fine...
	int numberOfObjects = 0;
	MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
	for (; !itTran.isDone(); itTran.next())
	{
		MObject obj = itTran.currentItem();
		// get the name of the object
		MFnDependencyNode nodeFn(obj);
		MString nodeName = nodeFn.name();

		// just testing something - DELETE THIS
		MQuaternion test = CustomLocator::rotationMayaToUnity(MQuaternion(0.123, 0.654, 0.321, 1));

		// viewport camera
		if (strcmp(nodeName.asChar(), "persp") == 0)
		{
			MDagPath dagPath;
			MDagPath::getAPathTo(obj, dagPath);
			MFnTransform fn(dagPath);
			MVector camPos = fn.getTranslation(MSpace::kWorld);
			cameraParams.push_back((float)camPos.x);
			cameraParams.push_back((float)camPos.y);
			cameraParams.push_back((float)camPos.z);
			MQuaternion rot(0, 0, 0, 1);
			fn.getRotation(rot, MSpace::kTransform);
			MQuaternion res = CustomLocator::rotationMayaToUnity(rot);
			cameraParams.push_back((float)res.x);
			cameraParams.push_back((float)res.y);
			cameraParams.push_back((float)res.z);
			cameraParams.push_back((float)res.w);
		}

		// filter out object I don't want to output
		// TODO: design how will I parse the scene
		if (strstr(nodeName.asChar(), "pCube") != NULL) // this filtering of objects might be slow...
		{
			MDagPath dagPath;
			MDagPath::getAPathTo(obj, dagPath);
			MFnTransform fn(dagPath);
			MVector translation = fn.getTranslation(MSpace::kWorld);
			posMemOutArray.push_back((float)translation.x);
			posMemOutArray.push_back((float)translation.y);
			posMemOutArray.push_back((float)-translation.z); // inverted for Unity
			posMemOutArray.push_back((float)0); // 4th position coordinate for whatever reason

			MQuaternion rot(0, 0, 0, 1);
			fn.getRotation(rot, MSpace::kTransform);
			// TODO: Quaternion right-hand to left-hand conversion
			MQuaternion res = CustomLocator::rotationMayaToUnity(rot);
			// ----
			rotMemOutArray.push_back((float)res.x);
			rotMemOutArray.push_back((float)res.y);
			rotMemOutArray.push_back((float)res.z);
			rotMemOutArray.push_back((float)res.w);
			/*rotMemOutArray.push_back((float)rot.x);
			rotMemOutArray.push_back((float)rot.y);
			rotMemOutArray.push_back((float)rot.z);
			rotMemOutArray.push_back((float)rot.w);*/
			numberOfObjects += 1;
		}
	}
	size_t posArraySize = posMemOutArray.size();
	size_t rotArraySize = rotMemOutArray.size();
	float * posMemOutPtr = &posMemOutArray[0]; // people say you can do this
	float * rotMemOutPtr = &rotMemOutArray[0]; // pointer arithmetics
	size_t camInfoArraySize = cameraParams.size();
	float * camInfoPtr = &cameraParams[0];

	CopyMemory(pBuf, posMemOutPtr, (posArraySize * sizeof(float)));
	// changing the pointer location for rotation writing
	float * shMemPtr = (float*)pBuf; // I change type of pBuf to float* because I want to make it explicit that I will be jumping by 4 bytes
	shMemPtr = shMemPtr + posArraySize; // this looks like it works just how I wanted/expected
	CopyMemory(shMemPtr, rotMemOutPtr, (rotArraySize * sizeof(float)));
	// camera info
	CopyMemory(pCamInfo, camInfoPtr, (camInfoArraySize * sizeof(float)));
	// scene info
	CopyMemory(pSceneInfo, &numberOfObjects, sizeof(int));


	// I have to figure out how to transfer the information about number of objects
	// because as of now, I don't know where in the shared memory the positions end and rotations begin
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
/*
	TODO: 
			- Better error handling
*/
bool CustomLocator::initSharedMemory() 
{
	// ------------------------------------------------------ scene info shared memory initialization
	TCHAR sceneInfoShMemName[] = TEXT("MayaToUnitySceneInfoSharedMem");
	this->hSceneInfoShMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, sceneInfoShMemName); // memory size if a bit bigger than needed...

	if (hSceneInfoShMem == NULL)
	{
		DWORD err = GetLastError();
		_tprintf(TEXT("Could not create file mapping object: (%d)\n"), err);
		return false;
	}

	this->pSceneInfo = MapViewOfFile(this->hSceneInfoShMem, FILE_MAP_ALL_ACCESS, 0, 0, 0); // If I understand the documentation correctly, there could be 0

	if (pSceneInfo == NULL)
	{
		DWORD err = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d) \n"), err);
		CloseHandle(hSceneInfoShMem);
		return false;
	}

	// ------------------------------------------------------ camera info shared memory initialization
	TCHAR camShMemName[] = TEXT("MayaToUnityCameraInfoSharedMem");
	this->hCamInfoShMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, camShMemName);

	if (hCamInfoShMem == NULL)
	{
		DWORD err = GetLastError();
		_tprintf(TEXT("Could not create file mapping object: (%d)\n"), err);
		return false;
	}

	this->pCamInfo = MapViewOfFile(this->hCamInfoShMem, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE); // If I understand the documentation correctly, there could be 0

	if (pCamInfo == NULL)
	{
		DWORD err = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d) \n"), err);
		CloseHandle(hCamInfoShMem);
		return false;
	}

	// ------------------------------------------------------ main shared memory initialization
	std::cout << "initSharedMemory()" << std::endl;
	TCHAR szName[] = TEXT("MyFileMappingObject");
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,	BUF_SIZE, szName);

	if (hMapFile == NULL)
	{
		DWORD err = GetLastError();
		_tprintf(TEXT("Could not create file mapping object: (%d)\n"), err);
		return false;
	}

	this->pBuf = MapViewOfFile(this->hMapFile, FILE_MAP_ALL_ACCESS,	0, 0, BUF_SIZE); // If I understand the documentation correctly, there could be 0

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

	UnmapViewOfFile(this->pCamInfo);
	CloseHandle(this->hCamInfoShMem);

	UnmapViewOfFile(this->pSceneInfo);
	CloseHandle(this->hSceneInfoShMem);
}
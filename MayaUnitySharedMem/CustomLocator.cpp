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
#include <stdexcept>
#include <vector>

#include "startStreamingCommand.h"
#include "endStreamingCommand.h"

#define BUF_SIZE 256

/*
	Most important method of this class.
	Parses the scene looking for protein objects and outputs their positions, rotations and information about type int shared memory for Unity to read.
	Exploits MPxLocatorNode's draw method because it's the only class in Maya API (that I know of) that gets called everytime something changes in the viewport.
*/
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

		if (strstr(nodeName.asChar(), "pdbMolStruc_2RCJ") != NULL)
		{
			/*
			TODO: Parsing the PDB id from the name of the node and according to some hash table get an id of protein type that will be outputed
			*/
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
			// Quaternion right-hand to left-hand conversion
			MQuaternion res = CustomLocator::rotationMayaToUnity(rot);

			rotMemOutArray.push_back((float)res.x);
			rotMemOutArray.push_back((float)res.y);
			rotMemOutArray.push_back((float)res.z);
			rotMemOutArray.push_back((float)res.w);
			numberOfObjects += 1;
		}
		//// filter out object I don't want to output
		//// TODO: design how will I parse the scene
		//if (strstr(nodeName.asChar(), "pCube") != NULL) // this filtering of objects might be slow...
		//{
		//	MDagPath dagPath;
		//	MDagPath::getAPathTo(obj, dagPath);
		//	MFnTransform fn(dagPath);
		//	MVector translation = fn.getTranslation(MSpace::kWorld);
		//	posMemOutArray.push_back((float)translation.x);
		//	posMemOutArray.push_back((float)translation.y);
		//	posMemOutArray.push_back((float)-translation.z); // inverted for Unity
		//	posMemOutArray.push_back((float)0); // 4th position coordinate for whatever reason

		//	MQuaternion rot(0, 0, 0, 1);
		//	fn.getRotation(rot, MSpace::kTransform);
		//	// Quaternion right-hand to left-hand conversion
		//	MQuaternion res = CustomLocator::rotationMayaToUnity(rot);

		//	rotMemOutArray.push_back((float)res.x);
		//	rotMemOutArray.push_back((float)res.y);
		//	rotMemOutArray.push_back((float)res.z);
		//	rotMemOutArray.push_back((float)res.w);
		//	numberOfObjects += 1;
		//}
	}

	size_t posArraySize = posMemOutArray.size();
	size_t rotArraySize = rotMemOutArray.size();
	size_t camInfoArraySize = cameraParams.size();

	if ((posArraySize <= 0) || (rotArraySize <= 0) || (camInfoArraySize <= 0))
	{
		return;
	}

	float * posMemOutPtr = &posMemOutArray[0]; // people say you can do this
	float * rotMemOutPtr = &rotMemOutArray[0]; // pointer arithmetics
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

}



CustomLocator::CustomLocator() : MPxLocatorNode()
{
	// iterating through the scene to see for how many objects do I need to allocate memory
	int numberOfObjects = 0;
	MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
	for (; !itTran.isDone(); itTran.next())
	{
		MObject obj = itTran.currentItem();
		// get the name of the object
		MFnDependencyNode nodeFn(obj);
		MString nodeName = nodeFn.name();

		if (strstr(nodeName.asChar(), "pdbMolStruc_2RCJ") != NULL)
		{
			numberOfObjects += 1;
		}
	}

	if (!initSharedMemory(numberOfObjects))
	{ // if allocation of shared memory fails...
		throw std::bad_alloc();
	}
}

bool CustomLocator::isBounded() const
{
	return true; // by returning false I think this should make it to call the draw without culling it ever
	//return true;
}

MBoundingBox CustomLocator::boundingBox() const
{
	MBoundingBox bbox;
	// just some random bounding box
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
	try
	{
		return new CustomLocator; // CustomLocator constructor might throw an exception if shared memory allocation fails...
	} catch (const std::exception& error)
	{
		return nullptr;
	}
	//return new CustomLocator;
	//return nullptr; // testing
}

/*
	This function is called after loading of the plugin.
*/
MStatus CustomLocator::initialize()
{
	std::cout << "CustomLocator::initialize" << std::endl;
	return MS::kSuccess;
}

// ==================================================== Shared Memory related functions
/*
	TODO: 
			- figure out what size of memory to allocate (now it's BUF_SIZE for all of them)
			DONE:
			- Better error handling
*/
//bool CustomLocator::initSharedMemory() 
bool CustomLocator::initSharedMemory(size_t numberOfObjs) 
{
	// ------------------------------------------------------ scene info shared memory initialization
	TCHAR sceneInfoShMemName[] = TEXT("MayaToUnitySceneInfoSharedMem");
	this->hSceneInfoShMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, sceneInfoShMemName); // memory size if a bit bigger than needed...

	if (hSceneInfoShMem == NULL)
	{
		return false;
	}

	this->pSceneInfo = MapViewOfFile(this->hSceneInfoShMem, FILE_MAP_ALL_ACCESS, 0, 0, 0); // If I understand the documentation correctly, there could be 0

	if (pSceneInfo == NULL)
	{
		CloseHandle(hSceneInfoShMem);
		return false;
	}

	// ------------------------------------------------------ camera info shared memory initialization
	TCHAR camShMemName[] = TEXT("MayaToUnityCameraInfoSharedMem");
	this->hCamInfoShMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, camShMemName);

	if (hCamInfoShMem == NULL)
	{
		return false;
	}

	this->pCamInfo = MapViewOfFile(this->hCamInfoShMem, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE); // If I understand the documentation correctly, there could be 0

	if (pCamInfo == NULL)
	{
		CloseHandle(hCamInfoShMem);
		return false;
	}

	// ------------------------------------------------------ main shared memory initialization
	std::cout << "initSharedMemory()" << std::endl;
	TCHAR szName[] = TEXT("MyFileMappingObject");
	//hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,	BUF_SIZE, szName);
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,	BUF_SIZE, szName);

	if (hMapFile == NULL)
	{
		return false;
	}

	this->pBuf = MapViewOfFile(this->hMapFile, FILE_MAP_ALL_ACCESS,	0, 0, BUF_SIZE); // If I understand the documentation correctly, there could be 0

	if (pBuf == NULL)
	{
		CloseHandle(hMapFile);
		return false;
	}

	return true;
	//return false; // testing purposes

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

// ------------------------------------------------------------------------------------- 
// MAYA PLUGIN REQUIRED FUNCTIONS
// ------------------------------------------------------------------------------------- 
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

	// add a custom menu with items: Start streaming, Stop streaming
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

// ------------------------------------------------------------------------------------- MAYA PLUGIN REQUIRED FUNCTIONS end
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
#include <algorithm>

#include "startStreamingCommand.h"
#include "endStreamingCommand.h"

#define BUF_SIZE 256

double PCFreq = 0.0;
__int64 CounterStart = 0;

// counter from http://stackoverflow.com/questions/1739259/how-to-use-queryperformancecounter
// outputs number of ms
void StartCounter();
double GetCounter();

/*
	Most important method of this class.
	Parses the scene looking for protein objects and outputs their positions, rotations and information about type int shared memory for Unity to read.
	Exploits MPxLocatorNode's draw method because it's the only class in Maya API (that I know of) that gets called everytime something changes in the viewport.
*/
void CustomLocator::draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
{
	StartCounter();

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
		//MQuaternion test = CustomLocator::rotationMayaToUnity(MQuaternion(0.123, 0.654, 0.321, 1));

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
			//MQuaternion res = CustomLocator::rotationMayaToUnity(rot);
			cameraParams.push_back((float)rot.x);
			cameraParams.push_back((float)rot.y);
			cameraParams.push_back((float)rot.z);
			cameraParams.push_back((float)rot.w);
		}

		// TODO: if name contains "pdbMolStruc"..
		//if (strstr(nodeName.asChar(), "pdbMolStruc_2RCJ") != NULL)
		if (strstr(nodeName.asChar(), "pdbMolStruc_") != NULL)
		{
			// ... then extract what's after "pdbMolStruc_" and before another "_" (can be PDB id that is 4 chars but also something else
			std::string fullName(nodeName.asChar());
			std::string pdbString = fullName.substr(12, 4);
			std::transform(pdbString.begin(), pdbString.end(), pdbString.begin(), ::tolower);
			int proteinId = pdbIdMap[pdbString];
			

			MDagPath dagPath;
			MDagPath::getAPathTo(obj, dagPath);

			// position:
			MFnTransform fn(dagPath);
			MVector translation = fn.getTranslation(MSpace::kWorld);
			posMemOutArray.push_back((float)translation.x);
			posMemOutArray.push_back((float)translation.y);
			posMemOutArray.push_back((float)-translation.z); // inverted for Unity
			posMemOutArray.push_back((float)0); // 4th position coordinate for whatever reason

			// rotation:
			MQuaternion rot(0, 0, 0, 1);
			fn.getRotation(rot, MSpace::kTransform);
			//MQuaternion res = CustomLocator::rotationMayaToUnity(rot); // Quaternion right-hand to left-hand conversion
			rotMemOutArray.push_back((float)rot.x);
			rotMemOutArray.push_back((float)rot.y);
			rotMemOutArray.push_back((float)rot.z);
			rotMemOutArray.push_back((float)rot.w);

			// ids:
			idMemOutArray.push_back((float)proteinId); // id in unity scene <- this is the only important field!!!
			idMemOutArray.push_back((float)0); // state
			idMemOutArray.push_back((float)0); // i don't know
			idMemOutArray.push_back((float)0); // i don't know

			numberOfObjects += 1;
		}

		//std::cerr << GetCounter() << std::endl;
		//std::cout.flush();
		//debugOut << GetCounter() << std::endl;
	}

	size_t posArraySize = posMemOutArray.size();
	size_t rotArraySize = rotMemOutArray.size();
	size_t camInfoArraySize = cameraParams.size();
	size_t idMemOutArraySize = idMemOutArray.size();

	if ((posArraySize <= 0) || (rotArraySize <= 0) || (camInfoArraySize <= 0) || (idMemOutArraySize <= 0))
	{
		return;
	}

	float * posMemOutPtr = &posMemOutArray[0]; // people say you can do this
	float * rotMemOutPtr = &rotMemOutArray[0]; // pointer arithmetics
	float * camInfoPtr = &cameraParams[0];
	float * idMemOutPtr = &idMemOutArray[0];

	CopyMemory(pBuf, posMemOutPtr, (posArraySize * sizeof(float)));
	// changing the pointer location for rotation writing
	float * shMemPtr = (float*)pBuf; // I change type of pBuf to float* because I want to make it explicit that I will be jumping by 4 bytes
	shMemPtr = shMemPtr + posArraySize; // this looks like it works just how I wanted/expected
	CopyMemory(shMemPtr, rotMemOutPtr, (rotArraySize * sizeof(float)));
	shMemPtr = shMemPtr + rotArraySize;
	CopyMemory(shMemPtr, idMemOutPtr, (idMemOutArraySize * sizeof(float)));
	// camera info
	CopyMemory(pCamInfo, camInfoPtr, (camInfoArraySize * sizeof(float)));
	// scene info
	CopyMemory(pSceneInfo, &numberOfObjects, sizeof(int));

}



CustomLocator::CustomLocator() : MPxLocatorNode()
{
	nextFreeInternalId = 0;
	debugOut.open("C:\\Users\\dvdkouril\\Documents\\Visual Studio 2015\\Projects\\MayaUnitySharedMem\\output.txt");
	pdbIdsFile.open("C:\\Users\\dvdkouril\\Documents\\Visual Studio 2015\\Projects\\MayaUnitySharedMem\\pdbIdsFile.txt");

	if (!pdbIdsFile.is_open())
	{
		std::cerr << "There was a problem while opening pdbIdsFile.txt" << std::endl;
		throw std::bad_alloc();
	}

	// iterating through the scene to see for how many objects do I need to allocate memory
	// ... and also some other things
	// this is actually very similar to the iteration that I am doing in the "main" loop
	int numberOfObjects = 0;
	MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
	for (; !itTran.isDone(); itTran.next())
	{
		MObject obj = itTran.currentItem();
		MFnDependencyNode nodeFn(obj);
		MString nodeName = nodeFn.name();

		if (strstr(nodeName.asChar(), "pdbMolStruc_") != NULL)
		{
			numberOfObjects += 1;
			// ... then extract what's after "pdbMolStruc_" and before another "_" (can be PDB id that is 4 chars but also something else
			std::string fullName(nodeName.asChar());
			std::string pdbString = fullName.substr(12, 4);
			std::transform(pdbString.begin(), pdbString.end(), pdbString.begin(), ::tolower); // TODO: this might cause some problems in the future
			// TODO: figure out if it's not there already
			std::map<std::string, int>::iterator containsIt = pdbIdMap.find(pdbString);
			if (containsIt == pdbIdMap.end()) 
			{ // if this key is not in the map already...
				pdbIdMap[pdbString] = nextFreeInternalId;
				nextFreeInternalId += 1;
			}
			
		}
	}

	// put the info about 'pdbId -> internal id' link into a file that unity will read
	/*for (std::map<std::string, int>::iterator it = pdbIdMap.begin(); it != pdbIdMap.end(); ++it)
	{
		pdbIdsFile << it->first << " " << it->second << std::endl;
	}
	pdbIdsFile.close();*/
	std::stringstream ss;
	for (std::map<std::string, int>::iterator it = pdbIdMap.begin(); it != pdbIdMap.end(); ++it)
	{
		ss << it->first << " " << it->second << std::endl;
	}
	std::string toOutputStr = ss.str();

	if (!initSharedMemory(numberOfObjects))
	{ // if allocation of shared memory fails...
		throw std::bad_alloc();
	}

	CopyMemory(this->pPdbMappingShMem, toOutputStr.c_str(), sizeof(char) * toOutputStr.length());
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
		std::cerr << "Shared memory has not been properly allocated!" << std::endl;
		(void)error; // just to make the warning go away
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

HANDLE CustomLocator::createSharedMemory(char * name, size_t size)
{
	HANDLE memoryHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name);
	
	return memoryHandle;
}

LPVOID CustomLocator::createMemoryMapping(HANDLE handle)
{
	return MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
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

	// refactored:
	this->hSceneInfoShMem = createSharedMemory("MayaToUnitySceneInfoSharedMem", BUF_SIZE);
	this->hCamInfoShMem = createSharedMemory("MayaToUnityCameraInfoSharedMem", BUF_SIZE);
	DWORD memorySize = (DWORD)numberOfObjs * sizeof(float) * 4 * 3; // number of objects * sizeof(float) * 4 coordinates each * 3 (positions, rotations and ids)
	this->hMapFile = createSharedMemory("MyFileMappingObject", memorySize);
	this->hPdbMappingShMem = createSharedMemory("MayaToUnityPdbMappingSharedMem", BUF_SIZE);


	this->pSceneInfo = createMemoryMapping(this->hSceneInfoShMem);
	this->pCamInfo = createMemoryMapping(this->hCamInfoShMem);
	this->pBuf = createMemoryMapping(this->hMapFile);
	this->pPdbMappingShMem = createMemoryMapping(this->hPdbMappingShMem);

	// testing
	if (hSceneInfoShMem == NULL)
	{
		std::cerr << "Could not create file mapping object \"MayaToUnitySceneInfoSharedMem\": " << (int)GetLastError() << std::endl;
		return false;
	}

	if (pSceneInfo == NULL)
	{
		std::cerr << "Could not map view of file \"MayaToUnitySceneInfoSharedMem\": " << (int)GetLastError() << std::endl;
		CloseHandle(hSceneInfoShMem);
		return false;
	}

	if (hCamInfoShMem == NULL)
	{
		std::cerr << "Could not create file mapping object \"MayaToUnityCameraInfoSharedMem\": " << (int)GetLastError() << std::endl;
		return false;
	}

	if (pCamInfo == NULL)
	{
		std::cerr << "Could not map view of file \"MayaToUnityCameraInfoSharedMem\": " << (int)GetLastError() << std::endl;
		CloseHandle(hCamInfoShMem);
		return false;
	}

	if (hMapFile == NULL)
	{
		std::cerr << "Could not create file mapping object \"MyFileMappingObject\": " << (int)GetLastError() << std::endl;
		return false;
	}

	if (pBuf == NULL)
	{
		std::cerr << "Could not map view of file \"MyFileMappingObject\": " << (int)GetLastError() << std::endl;
		CloseHandle(hMapFile);
		return false;
	}

	if (hPdbMappingShMem == NULL)
	{
		std::cerr << "Could not create file mapping object \"MayaToUnityPdbMappingSharedMem\": " << (int)GetLastError() << std::endl;
		return false;
	}

	if (pPdbMappingShMem == NULL)
	{
		std::cerr << "Could not map view of file \"MayaToUnityPdbMappingSharedMem\": " << (int)GetLastError() << std::endl;
		CloseHandle(hPdbMappingShMem);
		return false;
	}

	return true;

	//TCHAR sceneInfoShMemName[] = TEXT("MayaToUnitySceneInfoSharedMem");
	//this->hSceneInfoShMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, sceneInfoShMemName); // memory size if a bit bigger than needed...

	//if (hSceneInfoShMem == NULL)
	//{
	//	std::cerr << "Could not create file mapping object \"MayaToUnitySceneInfoSharedMem\": " << (int)GetLastError() << std::endl;
	//	return false;
	//}

	//this->pSceneInfo = MapViewOfFile(this->hSceneInfoShMem, FILE_MAP_ALL_ACCESS, 0, 0, 0); // If I understand the documentation correctly, there could be 0

	//if (pSceneInfo == NULL)
	//{
	//	std::cerr << "Could not map view of file \"MayaToUnitySceneInfoSharedMem\": " << (int)GetLastError() << std::endl;
	//	CloseHandle(hSceneInfoShMem);
	//	return false;
	//}

	//// ------------------------------------------------------ camera info shared memory initialization
	//TCHAR camShMemName[] = TEXT("MayaToUnityCameraInfoSharedMem");
	//this->hCamInfoShMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, camShMemName);

	//if (hCamInfoShMem == NULL)
	//{
	//	std::cerr << "Could not create file mapping object \"MayaToUnityCameraInfoSharedMem\": " << (int)GetLastError() << std::endl;
	//	return false;
	//}

	//this->pCamInfo = MapViewOfFile(this->hCamInfoShMem, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE); // If I understand the documentation correctly, there could be 0

	//if (pCamInfo == NULL)
	//{
	//	std::cerr << "Could not map view of file \"MayaToUnityCameraInfoSharedMem\": " << (int)GetLastError() << std::endl;
	//	CloseHandle(hCamInfoShMem);
	//	return false;
	//}

	//// ------------------------------------------------------ main shared memory initialization
	//TCHAR szName[] = TEXT("MyFileMappingObject");
	////hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,	BUF_SIZE, szName);
	//DWORD memorySize = (DWORD)numberOfObjs * sizeof(float) * 4 * 3; // number of objects * sizeof(float) * 4 coordinates each * 3 (positions, rotations and ids)
	//hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, memorySize, szName);

	//if (hMapFile == NULL)
	//{
	//	std::cerr << "Could not create file mapping object \"MyFileMappingObject\": " << (int)GetLastError() << std::endl;
	//	return false;
	//}

	//this->pBuf = MapViewOfFile(this->hMapFile, FILE_MAP_ALL_ACCESS,	0, 0, BUF_SIZE); // If I understand the documentation correctly, there could be 0

	//if (pBuf == NULL)
	//{
	//	std::cerr << "Could not map view of file \"MyFileMappingObject\": " << (int)GetLastError() << std::endl;
	//	CloseHandle(hMapFile);
	//	return false;
	//}

	//return true;
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

	UnmapViewOfFile(this->pPdbMappingShMem);
	CloseHandle(this->hPdbMappingShMem);
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

void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		cout << "QueryPerformanceFrequency failed!\n";

	PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
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
#include <vector>
#include <algorithm>
#include <sstream>
#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MFnTransform.h>
#include <maya/MVector.h>
#include <maya/MQuaternion.h>


#include "Streamer.h"


Streamer::Streamer(void)
{
	state = StreamerState::Stopped; // Streaming is not started until doing so explicitly!

}

Streamer::~Streamer(void)
{
	stop();
}

/*
	What happens when you start the streaming:
	(1) you need to traverse the scene and figure out for how many objects to I need to allocate the shared memory
	(2) build up the pdbIdMap
	(3) init the memory
	(4) copy the pdbIdMap info into shared memory
*/
void Streamer::start()
{
	// (1)
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

	// (2)
	std::stringstream ss;
	for (std::map<std::string, int>::iterator it = pdbIdMap.begin(); it != pdbIdMap.end(); ++it)
	{
		ss << it->first << " " << it->second << std::endl;
	}
	std::string toOutputStr = ss.str();

	// (3)
	if (!initSharedMemory(numberOfObjects))
	{ // if allocation of shared memory fails...
		freeSharedMemory();
		return;
	}

	// (4)
	CopyMemory(this->pPdbMappingShMem, toOutputStr.c_str(), sizeof(char) * toOutputStr.length());

	state = StreamerState::Running;
}

/*
	This is not used yet really.
*/
void Streamer::pause()
{
	state = StreamerState::Paused;
}

void Streamer::stop()
{
	state = StreamerState::Stopped;

	freeSharedMemory();
}

void Streamer::update(const MString& panelName, void* data)
{
	if (this->state != StreamerState::Running)
	{
		return;
	}

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

	//Sleep(2000);

	/*this->lastFrameTime = GetCounter();
	frameTimesQueue.push(lastFrameTime);*/

	//QWidget* control = MQtUtil::findControl("frameTimeLabel");
	//if (control != NULL)
	//{
	//	double frameTimesSum = frameTimesQueue.getSum();
	//	int framesCount = frameTimesQueue.size();

	//	double averageFrameTime = frameTimesSum / framesCount;

	//	QLabel *label = qobject_cast<QLabel * >(control);
	//	//label->setText(QString::number(this->lastFrameTime) + "asdfa");
	//	label->setText(QString::number(averageFrameTime) + " ms (" + QString::number(1 / averageFrameTime) + " FPS)");
	//}
}

bool Streamer::initSharedMemory(size_t numberOfObjs)
{
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
}

HANDLE Streamer::createSharedMemory(char* name, size_t size)
{
	HANDLE memoryHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name);
	
	return memoryHandle;
}

LPVOID Streamer::createMemoryMapping(HANDLE handle)
{
	return MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

void Streamer::freeSharedMemory()
{
	UnmapViewOfFile(this->pBuf);
	CloseHandle(this->hMapFile);

	UnmapViewOfFile(this->pCamInfo);
	CloseHandle(this->hCamInfoShMem);

	UnmapViewOfFile(this->pSceneInfo);
	CloseHandle(this->hSceneInfoShMem);

	UnmapViewOfFile(this->pPdbMappingShMem);
	CloseHandle(this->hPdbMappingShMem);
}
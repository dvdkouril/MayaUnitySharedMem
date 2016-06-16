#include "CameraWatcherNode.h"

#include <windows.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFloatVector.h>

MTypeId CameraWatcherNode::id( 0x80029 );
MObject CameraWatcherNode::aSharedMemoryPointer;
MObject CameraWatcherNode::aPosition;
MObject CameraWatcherNode::aDirtyOutput;

CameraWatcherNode::CameraWatcherNode(void)
{
	
}

CameraWatcherNode::~CameraWatcherNode()
{
	
}



MStatus CameraWatcherNode::compute(const MPlug& plug, MDataBlock& data)
{
	std::cerr << "CameraWatcherNode::compute" << std::endl;

	void *pointer = data.inputValue(aSharedMemoryPointer).asAddr();

	if (pointer == 0) // in this case either pointer is not set yet or the streaming is stopped
	{
		return MStatus::kInvalidParameter;
	}

	std::vector<float> position;
	std::vector<float> rotation;

	MFloatVector pos = data.inputValue(aPosition).asFloatVector();
	position.push_back(pos.x);
	position.push_back(pos.y);
	position.push_back(pos.z);
	position.push_back(0);

	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);

	writeToMemory(position, rotation, pointer);

	MDataHandle hOutput = data.outputValue(aDirtyOutput);
	hOutput.setInt(0);

	data.setClean(plug);

	return MStatus::kSuccess;
}

void CameraWatcherNode::writeToMemory(std::vector<float> posMemOutArray, 
									   std::vector<float> rotMemOutArray, 
								       void* ptr)
{
	LPVOID pBuf = (LPVOID)ptr;
	std::cerr << "ptr in writeToMemory(hex): " << std::hex << ptr << std::endl;

	size_t posArraySize = posMemOutArray.size(); // this will always be 4
	size_t rotArraySize = rotMemOutArray.size(); // this will always be 4

	if ((posArraySize <= 0) || (rotArraySize <= 0))
	{
		return;
	}

	float * posMemOutPtr = &posMemOutArray[0]; // people say you can do this
	float * rotMemOutPtr = &rotMemOutArray[0]; // pointer arithmetics

	// TODO: maybe I could do this in a single CopyMemory call by appending all the vectors together...would that be more efficient than 3 CopeMemory calls?

	float * shMemPtr = (float*)pBuf; 
	shMemPtr = shMemPtr;			// jumping to a memory location that corresponds with the index of proteinWatcher node
	CopyMemory(shMemPtr, posMemOutPtr, posArraySize * sizeof(float));
	shMemPtr = shMemPtr + posArraySize;										// changing the pointer location for rotation writing		
	CopyMemory(shMemPtr, rotMemOutPtr, rotArraySize * sizeof(float));
}


void* CameraWatcherNode::creator()
{
	return new CameraWatcherNode;
}

MStatus CameraWatcherNode::initialize()
{
	MFnNumericAttribute nAttr;

	aPosition = nAttr.createPoint("PositionInput", "posIn");
	addAttribute(aPosition);

	aSharedMemoryPointer = nAttr.create("SharedMemoryPointer", "shMemPtr", MFnNumericData::kInt64);
	addAttribute(aSharedMemoryPointer);

	aDirtyOutput = nAttr.create("DirtyOutput", "drtOut", MFnNumericData::kInt);
	addAttribute(aDirtyOutput);

	attributeAffects(aPosition, aDirtyOutput);
	attributeAffects(aSharedMemoryPointer, aDirtyOutput);

	return MS::kSuccess;
}



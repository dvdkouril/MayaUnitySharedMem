#include "ProteinWatcherNode.h"

#include <maya/MNodeMessage.h>
#include <maya/MFloatVector.h>
#include <vector>
#include <iomanip>

MTypeId ProteinWatcherNode::id( 0x80028 );
MObject ProteinWatcherNode::aSharedMemoryPointer;
MObject ProteinWatcherNode::aPosition;
MObject	ProteinWatcherNode::aIndexOutput;

ProteinWatcherNode::ProteinWatcherNode(void)
{
}


ProteinWatcherNode::~ProteinWatcherNode(void)
{
}

void* ProteinWatcherNode::creator()
{
	std::cerr << "ProteinWatcherNode::creator" << std::endl;
	return new ProteinWatcherNode();
}

MStatus ProteinWatcherNode::compute(const MPlug& plug, MDataBlock& data)
{
	std::cerr << "ProteinWatcherNode::compute (" << this->name() << ")" << std::endl;

	//int pointer = data.inputValue(aSharedMemoryPointer).asInt();
	void *pointer = data.inputValue(aSharedMemoryPointer).asAddr();
	MInt64 longPointer = data.inputValue(aSharedMemoryPointer).asInt64();

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

	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);

	std::cerr << "Writing: " << position[0] << ", " << position[1] << ", " << position[2] << std::endl;

	writeToMemory(position, rotation, 0, pointer);

	MDataHandle hOutput = data.outputValue(aIndexOutput);
	hOutput.setFloat(0); // hard coded for now, the class should have it's own id as a member attribute

	data.setClean(plug);

	return MStatus::kSuccess;
}

void ProteinWatcherNode::writeToMemory(std::vector<float> posMemOutArray, 
								  std::vector<float> rotMemOutArray, 
								  int index,
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

	float * shMemPtr = (float*)pBuf; 
	shMemPtr = shMemPtr + index; // jumping to the right memory position
	//CopyMemory(pBuf, posMemOutPtr, posArraySize * sizeof(float));
	// changing the pointer location for rotation writing
	//shMemPtr = (float*)pBuf; // I change type of pBuf to float* because I want to make it explicit that I will be jumping by 4 bytes
	shMemPtr = shMemPtr + posArraySize; // this looks like it works just how I wanted/expected
	//CopyMemory(shMemPtr, rotMemOutPtr, rotArraySize * sizeof(float));
}

MStatus ProteinWatcherNode::initialize()
{
	MFnNumericAttribute nAttr;
	aPosition = nAttr.createPoint("PositionInput", "posIn");
	addAttribute(aPosition);

	aIndexOutput = nAttr.create("IndexOutput", "indxOut", MFnNumericData::kInt);
	addAttribute(aIndexOutput);

	attributeAffects(aPosition, aIndexOutput);
	
	// maybe attributeAffects also between memory pointer attribute and output .... but that's fine for now

	//aSharedMemoryPointer = nAttr.create("SharedMemoryPointer", "shMemPtr", MFnNumericData::kAddr);
	//aSharedMemoryPointer = nAttr.createAddr("SharedMemoryPointer", "shMemPtr");
	nAttr.setWritable(false);
	//nAttr.setHidden(true); // maybe I will not hide it so that I know it's there and just keep it unwritable
	aSharedMemoryPointer = nAttr.create("SharedMemoryPointer", "shMemPtr", MFnNumericData::kInt64); // I am such an idiot I had "asdfasdf" as a name...I wonder why it didn't find this attribute...
	addAttribute(aSharedMemoryPointer);

	//attributeAffects(aSharedMemoryPointer, aIndexOutput);

	return MStatus::kSuccess;
} 


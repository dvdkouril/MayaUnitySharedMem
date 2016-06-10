#include "ProteinWatcherNode.h"

#include <maya/MNodeMessage.h>
#include <vector>

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
	std::cerr << "ProteinWatcherNode::compute" << std::endl;

	int pointer = data.inputValue(aSharedMemoryPointer).asInt();

	if (pointer == 0) // in this case either pointer is not set yet or the streaming is stopped
	{
		return MStatus::kInvalidParameter;
	}

	std::vector<float> position;
	std::vector<float> rotation;
	writeToMemory(position, rotation, 0, pointer);

	MDataHandle hOutput = data.outputValue(aIndexOutput);
	hOutput.setFloat(0); // hard coded for now, the class should have it's own id as a member attribute

	data.setClean(plug);

	return MStatus::kSuccess;
}

void ProteinWatcherNode::writeToMemory(std::vector<float> posMemOutArray, 
								  std::vector<float> rotMemOutArray, 
								  int index,
								  int ptr)
{
	LPVOID pBuf = (LPVOID)ptr;

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
	CopyMemory(pBuf, posMemOutPtr, posArraySize * sizeof(float));
	// changing the pointer location for rotation writing
	//shMemPtr = (float*)pBuf; // I change type of pBuf to float* because I want to make it explicit that I will be jumping by 4 bytes
	shMemPtr = shMemPtr + posArraySize; // this looks like it works just how I wanted/expected
	CopyMemory(shMemPtr, rotMemOutPtr, rotArraySize * sizeof(float));
}

MStatus ProteinWatcherNode::initialize()
{
	MFnNumericAttribute nAttr;
	aPosition = nAttr.createPoint("PositionInput", "posIn");
	addAttribute(aPosition);

	aIndexOutput = nAttr.create("IndexOutput", "indxOut", MFnNumericData::kInt);
	addAttribute(aIndexOutput);

	attributeAffects(aPosition, aIndexOutput);

	return MStatus::kSuccess;
} 


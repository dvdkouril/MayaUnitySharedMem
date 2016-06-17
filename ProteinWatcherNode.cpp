#include "ProteinWatcherNode.h"

#include <maya/MFloatVector.h>
#include <vector>
#include <iomanip>

MTypeId ProteinWatcherNode::id( 0x80028 );
MObject ProteinWatcherNode::aSharedMemoryPointer;
MObject ProteinWatcherNode::aPosition;
MObject	ProteinWatcherNode::aIndex;
MObject ProteinWatcherNode::aDirtyOutput;

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

	void *pointer = data.inputValue(aSharedMemoryPointer).asAddr();
	//MInt64 longPointer = data.inputValue(aSharedMemoryPointer).asInt64();
	int index = data.inputValue(aIndex).asInt();

	if (pointer == 0) // in this case either pointer is not set yet or the streaming is stopped
	{
		return MStatus::kInvalidParameter;
	}

	std::vector<float> position;
	std::vector<float> rotation;
	std::vector<float> info;

	MFloatVector pos = data.inputValue(aPosition).asFloatVector();
	position.push_back(pos.x);
	position.push_back(pos.y);
	position.push_back(-pos.z); // inverted for Unity
	position.push_back(0);

	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);

	info.push_back(0);
	info.push_back(0);
	info.push_back(0);
	info.push_back(0);

	std::cerr << "Writing: " << position[0] << ", " << position[1] << ", " << position[2] << std::endl;

	writeToMemory(position, rotation, info, index, pointer);

	MDataHandle hOutput = data.outputValue(aDirtyOutput);
	hOutput.setInt(0); // hard coded for now, the class should have it's own id as a member attribute

	data.setClean(plug);

	return MStatus::kSuccess;
}

void ProteinWatcherNode::writeToMemory(std::vector<float> posMemOutArray, 
									   std::vector<float> rotMemOutArray, 
									   std::vector<float> infMemOutArray, 
									   int index,
								       void* ptr)
{
	LPVOID pBuf = (LPVOID)ptr;
	std::cerr << "ptr in writeToMemory(hex): " << std::hex << ptr << std::endl;

	size_t posArraySize = posMemOutArray.size(); // this will always be 4
	size_t rotArraySize = rotMemOutArray.size(); // this will always be 4
	size_t infArraySize = infMemOutArray.size(); // this will always be 4

	if ((posArraySize <= 0) || (rotArraySize <= 0))
	{
		return;
	}

	float * posMemOutPtr = &posMemOutArray[0]; // people say you can do this
	float * rotMemOutPtr = &rotMemOutArray[0]; // pointer arithmetics
	float * infMemOutPtr = &infMemOutArray[0]; // pointer arithmetics

	// TODO: maybe I could do this in a single CopyMemory call by appending all the vectors together...would that be more efficient than 3 CopeMemory calls?

	float * shMemPtr = (float*)pBuf; 
	shMemPtr = shMemPtr + index * (posArraySize + rotArraySize);			// jumping to a memory location that corresponds with the index of proteinWatcher node
	CopyMemory(shMemPtr, posMemOutPtr, posArraySize * sizeof(float));
	shMemPtr = shMemPtr + posArraySize;										// changing the pointer location for rotation writing		
	CopyMemory(shMemPtr, rotMemOutPtr, rotArraySize * sizeof(float));
	shMemPtr = shMemPtr + rotArraySize;
	CopyMemory(shMemPtr, infMemOutPtr, infArraySize * sizeof(float));
}

MStatus ProteinWatcherNode::initialize()
{
	std::cerr << "ProteinWatcherNode::initialize" << std::endl;

	MFnNumericAttribute nAttr;
	aPosition = nAttr.createPoint("PositionInput", "posIn");
	addAttribute(aPosition);

	aIndex = nAttr.create("IndexInput", "indxIn", MFnNumericData::kInt);
	addAttribute(aIndex);

	//nAttr.setHidden(true);
	aDirtyOutput = nAttr.create("DirtyOutput", "drtOut", MFnNumericData::kInt);
	addAttribute(aDirtyOutput);
	//nAttr.setWritable(false);
	//nAttr.setHidden(false);
	//nAttr.setHidden(true); // maybe I will not hide it so that I know it's there and just keep it unwritable
	aSharedMemoryPointer = nAttr.create("SharedMemoryPointer", "shMemPtr", MFnNumericData::kInt64); // I am such an idiot I had "asdfasdf" as a name...I wonder why it didn't find this attribute...
	addAttribute(aSharedMemoryPointer);

	attributeAffects(aSharedMemoryPointer, aDirtyOutput);
	attributeAffects(aPosition, aDirtyOutput);
	attributeAffects(aIndex, aDirtyOutput);

	return MStatus::kSuccess;
} 


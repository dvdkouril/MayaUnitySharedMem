#include "ProteinWatcherNode.h"

#include <maya/MFloatVector.h>
#include <maya/MAngle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <vector>
#include <iomanip>

MTypeId ProteinWatcherNode::id( 0x80028 );
MObject ProteinWatcherNode::aSharedMemoryPointer;
MObject ProteinWatcherNode::aPosition;
MObject ProteinWatcherNode::aRotationX;
MObject ProteinWatcherNode::aRotationY;
MObject ProteinWatcherNode::aRotationZ;
MObject ProteinWatcherNode::aRotationW;
MObject	ProteinWatcherNode::aIndex;
MObject ProteinWatcherNode::aNumberOfObjects;
MObject ProteinWatcherNode::aDirtyOutput;

ProteinWatcherNode::ProteinWatcherNode(void) : MPxLocatorNode()
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

void ProteinWatcherNode::draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
{
	std::cerr << "ProteinWatcherNode::draw (" << this->name() << ")" << std::endl;

	// get the data from position input attribute, I need to see if it's updated faster then how the compute method is called or if it's more or less the same
	MObject thisNode = thisMObject();
	MStatus stat;

	MPlug posPlug(thisNode, aPosition);
	MFloatPoint pos;
	stat = posPlug.child(0).getValue(pos.x);
	stat = posPlug.child(1).getValue(pos.y);
	stat = posPlug.child(2).getValue(pos.z);

	MPlug rotXPlug(thisNode, aRotationX);
	MPlug rotYPlug(thisNode, aRotationY);
	MPlug rotZPlug(thisNode, aRotationZ);
	MPlug rotWPlug(thisNode, aRotationW);
	float rotX;
	MStatus st = rotXPlug.getValue(rotX);
	if (st != MS::kSuccess)
	{
		std::cerr << "CANNOT GET PLUG VALUE!" << std::endl;
	}
	float rotY;
	st = rotXPlug.getValue(rotY);
	if (st != MS::kSuccess)
	{
		std::cerr << "CANNOT GET PLUG VALUE!" << std::endl;
	}
	float rotZ;
	st = rotXPlug.getValue(rotZ);
	if (st != MS::kSuccess)
	{
		std::cerr << "CANNOT GET PLUG VALUE!" << std::endl;
	}
	float rotW;
	st = rotXPlug.getValue(rotW);
	if (st != MS::kSuccess)
	{
		std::cerr << "CANNOT GET PLUG VALUE!" << std::endl;
	}
	/*float rotX = rotXPlug.asFloat();
	float rotY = rotYPlug.asFloat();
	float rotZ = rotZPlug.asFloat();
	float rotW = rotWPlug.asFloat();*/

	MPlug ptrPlug(thisNode, aSharedMemoryPointer);
	MInt64 intPtr;
	stat = ptrPlug.getValue(intPtr);
	void * ptr = (void*)intPtr;

	MPlug indexPlug(thisNode, aIndex);
	int index;
	stat = indexPlug.getValue(index);

	MPlug objNumPlug(thisNode, aNumberOfObjects);
	int numberOfObjects;
	stat = objNumPlug.getValue(numberOfObjects);

	// putting the data into intermediate containers
	std::vector<float> position;
	std::vector<float> rotation;
	std::vector<float> info;

	position.push_back(pos.x);
	position.push_back(pos.y);
	position.push_back(-pos.z);
	position.push_back(0);

	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);
	rotation.push_back(0);
	/*rotation.push_back(rotX);
	rotation.push_back(rotY);
	rotation.push_back(rotZ);
	rotation.push_back(rotW);*/

	info.push_back(0);
	info.push_back(0);
	info.push_back(0);
	info.push_back(0);

	writeToMemory(position, rotation, info, index, numberOfObjects, ptr);
}

bool ProteinWatcherNode::isBounded() const
{
	return false;
}

void ProteinWatcherNode::writeToMemory(std::vector<float> posMemOutArray, 
									   std::vector<float> rotMemOutArray, 
									   std::vector<float> infMemOutArray, 
									   int index,
									   int numberOfObjects,
								       void* ptr)
{
	LPVOID pBuf = (LPVOID)ptr;
	//std::cerr << "ptr in writeToMemory(hex): " << std::hex << ptr << std::endl;
	std::cerr << "Writing into memory:" << std::endl;
	std::cerr << posMemOutArray[0] << ", " << posMemOutArray[1] << ", " << posMemOutArray[2] << ", " << posMemOutArray[3] << std:: endl;
	std::cerr << rotMemOutArray[0] << ", " << rotMemOutArray[1] << ", " << rotMemOutArray[2] << ", " << rotMemOutArray[3] << std:: endl;
	std::cerr << infMemOutArray[0] << ", " << infMemOutArray[1] << ", " << infMemOutArray[2] << ", " << infMemOutArray[3] << std:: endl;

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

	//int numberOfObjects = 0; // TODO: !!!just for now, later this will be a parameter that will be set from outside!!!

	float * posPtr = (float*)pBuf + index*(posArraySize);
	CopyMemory(posPtr, posMemOutPtr, posArraySize * sizeof(float));

	float *rotPtr = (float*)pBuf + numberOfObjects * posArraySize + index * rotArraySize;
	CopyMemory(rotPtr, rotMemOutPtr, rotArraySize * sizeof(float));

	float *infPtr = (float*)pBuf + numberOfObjects * (posArraySize * rotArraySize) + index * infArraySize;
	CopyMemory(infPtr, infMemOutPtr, infArraySize * sizeof(float));

}

MStatus ProteinWatcherNode::initialize()
{
	std::cerr << "ProteinWatcherNode::initialize" << std::endl;

	MFnNumericAttribute nAttr;
	aPosition = nAttr.createPoint("PositionInput", "posIn");
	addAttribute(aPosition);

	aRotationX = nAttr.create("RotationX", "rotX", MFnNumericData::kFloat);
	addAttribute(aRotationX);
	aRotationY = nAttr.create("RotationY", "rotY", MFnNumericData::kFloat);
	addAttribute(aRotationY);
	aRotationZ = nAttr.create("RotationZ", "rotZ", MFnNumericData::kFloat);
	addAttribute(aRotationZ);
	aRotationW = nAttr.create("RotationW", "rotW", MFnNumericData::kFloat);
	addAttribute(aRotationW);
	//// THERE IS SOME PROBLEM HERE
	//MFnUnitAttribute uAttr;
	//aRotationX = uAttr.create("RotationX", "rotX", MFnUnitAttribute::kAngle);
	//addAttribute(aRotationX);
	//aRotationY = uAttr.create("RotationY", "rotY", MFnUnitAttribute::kAngle);
	//addAttribute(aRotationY);
	//aRotationZ = uAttr.create("RotationZ", "rotZ", MFnUnitAttribute::kAngle);
	//addAttribute(aRotationZ);
	//// ------------------------

	aIndex = nAttr.create("IndexInput", "indxIn", MFnNumericData::kInt);
	addAttribute(aIndex);

	aNumberOfObjects = nAttr.create("NumberOfObjects", "objNmbr", MFnNumericData::kInt);
	addAttribute(aNumberOfObjects);

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


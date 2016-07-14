#include "CameraWatcherNode.h"

#include <windows.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MAngle.h>
#include <maya/MQuaternion.h>
#include <maya/MEulerRotation.h>

MTypeId CameraWatcherNode::id( 0x80029 );
MObject CameraWatcherNode::aSharedMemoryPointer;
MObject CameraWatcherNode::aPosition;
MObject CameraWatcherNode::aRotationX;
MObject CameraWatcherNode::aRotationY;
MObject CameraWatcherNode::aRotationZ;
MObject CameraWatcherNode::aDirtyOutput;

CameraWatcherNode::CameraWatcherNode(void)
{
	
}

CameraWatcherNode::~CameraWatcherNode()
{
	
}

void CameraWatcherNode::draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
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
	//MPlug rotWPlug(thisNode, aRotationW);
	float rotX;
	MStatus st = rotXPlug.getValue(rotX);
	if (st != MS::kSuccess)
	{
		std::cerr << "CANNOT GET PLUG VALUE!" << std::endl;
	}
	float rotY;
	st = rotYPlug.getValue(rotY);
	if (st != MS::kSuccess)
	{
		std::cerr << "CANNOT GET PLUG VALUE!" << std::endl;
	}
	float rotZ;
	st = rotZPlug.getValue(rotZ);
	if (st != MS::kSuccess)
	{
		std::cerr << "CANNOT GET PLUG VALUE!" << std::endl;
	}

	MAngle rotationX(rotX, MAngle::kDegrees);
	MAngle rotationY(rotY, MAngle::kDegrees);
	MAngle rotationZ(rotZ, MAngle::kDegrees);

	MEulerRotation eulRotation(rotationX.asRadians(), rotationY.asRadians(), rotationZ.asRadians());
	MQuaternion quatRot = eulRotation.asQuaternion();

	MPlug ptrPlug(thisNode, aSharedMemoryPointer);
	MInt64 intPtr;
	stat = ptrPlug.getValue(intPtr);
	void * ptr = (void*)intPtr;

	std::vector<float> position;
	std::vector<float> rotation;
	std::vector<float> info;

	position.push_back(pos.x);
	position.push_back(pos.y);
	position.push_back(pos.z);

	rotation.push_back(quatRot.x);
	rotation.push_back(quatRot.y);
	rotation.push_back(quatRot.z);
	rotation.push_back(quatRot.w);

	writeToMemory(position, rotation, ptr);

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

bool CameraWatcherNode::isBounded() const
{
	return false;
}

MStatus CameraWatcherNode::initialize()
{
	MFnNumericAttribute nAttr;

	aPosition = nAttr.createPoint("PositionInput", "posIn");
	addAttribute(aPosition);

	aRotationX = nAttr.create("RotationInputX", "rotInX", MFnNumericData::kFloat);
	addAttribute(aRotationX);
	aRotationY = nAttr.create("RotationInputY", "rotInY", MFnNumericData::kFloat);
	addAttribute(aRotationY);
	aRotationZ = nAttr.create("RotationInputZ", "rotInZ", MFnNumericData::kFloat);
	addAttribute(aRotationZ);

	aSharedMemoryPointer = nAttr.create("SharedMemoryPointer", "shMemPtr", MFnNumericData::kInt64);
	addAttribute(aSharedMemoryPointer);

	aDirtyOutput = nAttr.create("DirtyOutput", "drtOut", MFnNumericData::kInt);
	addAttribute(aDirtyOutput);

	attributeAffects(aPosition, aDirtyOutput);
	attributeAffects(aSharedMemoryPointer, aDirtyOutput);
	attributeAffects(aRotationX, aDirtyOutput);
	attributeAffects(aRotationY, aDirtyOutput);
	attributeAffects(aRotationZ, aDirtyOutput);

	return MS::kSuccess;
}



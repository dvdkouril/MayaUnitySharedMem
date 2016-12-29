#pragma once

//#include <maya/MPxNode.h>
#include <maya/MPxLocatorNode.h>
#include <windows.h>
#include <vector>

class ProteinWatcherNode : public MPxLocatorNode
{
public:
								ProteinWatcherNode(void);
	virtual						~ProteinWatcherNode(void);

	//virtual MStatus				compute(const MPlug& plug, MDataBlock& data) override;
	virtual void				draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status) override;
	virtual bool				isBounded() const override;

	static void*				creator();
	static MStatus				initialize();
	
	static MTypeId				id;

	// ------------------------ in
	static MObject				aSharedMemoryPointer;
	static MObject				aPosition;
	static MObject				aRotationX;
	static MObject				aRotationY;
	static MObject				aRotationZ;
	//static MObject				aRotationW;
	static MObject				aIndex; // determines index of the Protein instance (and therefore where in the memory should the Watcher write it's changes
	static MObject				aNumberOfObjects;
	static MObject				aInternalTypeId; // type id which identifies what type of protein this instance is
	// ------------------------ out
	static MObject				aDirtyOutput; // Dummy plug for forcing an evaluation of this mode

private:
	void						writeToMemory(std::vector<float> posMemOutArray, 
											  std::vector<float> rotMemOutArray,
											  std::vector<float> infMemOutArray,
											  int index,
											  int numberOfObjects,
											  void* handle);

	bool						dataChanged(std::vector<float> pos, std::vector<float> rot);
	void						cacheData(std::vector<float> pos, std::vector<float> rot);

	std::vector<float>			lastPosition;
	std::vector<float>			lastRotation;
};


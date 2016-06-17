#pragma once

//#include <maya/MPxNode.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MFnNumericAttribute.h>
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
	//static MObject				aRotation; // TODO: might be a little trickier since I can't use .createPoint for this
	static MObject				aIndex; // determines index of the Protein instance (and therefore where in the memory should the Watcher write it's changes
	static MObject				aNumberOfObjects;
	// ------------------------ out
	static MObject				aDirtyOutput; // Dummy plug for forcing an evaluation of this mode

private:
	void						writeToMemory(std::vector<float> posMemOutArray, 
											  std::vector<float> rotMemOutArray,
											  std::vector<float> infMemOutArray,
											  int index,
											  void* handle);
};


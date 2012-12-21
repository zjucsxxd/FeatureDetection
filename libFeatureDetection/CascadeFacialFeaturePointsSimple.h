#pragma once
#include "VDetector.h"
#include "CascadeWvmOeSvmOe.h"

class CascadeFacialFeaturePointsSimple : public VDetector
{
public:
	CascadeFacialFeaturePointsSimple(void);
	~CascadeFacialFeaturePointsSimple(void);

	int initForImage(FdImage*);
	int detectOnImage(FdImage*);
	void setRoiInImage(Rect);	// Set the in-image ROI for all feature classifiers, and overwrite the values set by reading the config file.

	CascadeWvmOeSvmOe *reye;
	CascadeWvmOeSvmOe *leye;
	CascadeWvmOeSvmOe *nosetip;
	CascadeWvmOeSvmOe *lmouth;
	CascadeWvmOeSvmOe *rmouth;

	std::vector<FdPatch*> candidates;
};

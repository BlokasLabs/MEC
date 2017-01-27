
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MODEL__
#define __ML_MODEL__

#include "MLProperty.h"

// an MLModel is a kind of PropertySet that is also its own PropertyListener.
// Subclasses override doPropertyChangeAction to propagate any changes from Properties to the core logic.



//FIXME: call updateChangedProperties when appropriate
class MLModel : public MLPropertySet, public MLPropertyListener
{
public:
	MLModel();
	virtual ~MLModel();
private:
};

#endif // __ML_MODEL__


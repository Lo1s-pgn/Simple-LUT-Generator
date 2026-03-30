#ifndef LSP_LUT_GENERATOR_DESCRIBE_H
#define LSP_LUT_GENERATOR_DESCRIBE_H

#include "ofxsImageEffect.h"

// Called from factory describeInContext for Filter and General.
void describeLutGeneratorInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum p_Context);

#endif

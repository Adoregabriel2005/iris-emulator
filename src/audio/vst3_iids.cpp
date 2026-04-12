// vst3_iids.cpp — defines the static FUID members required by the linker.
// Must be compiled with INIT_CLASS_IID defined so DECLARE_CLASS_IID emits
// the actual const FUID definitions.

#define INIT_CLASS_IID
#include "vst3sdk/pluginterfaces/base/funknown.h"
#include "vst3sdk/pluginterfaces/base/ipluginbase.h"
#include "vst3sdk/pluginterfaces/vst/ivstcomponent.h"
#include "vst3sdk/pluginterfaces/vst/ivstaudioprocessor.h"
#include "vst3sdk/pluginterfaces/vst/ivsteditcontroller.h"
#include "vst3sdk/pluginterfaces/gui/iplugview.h"

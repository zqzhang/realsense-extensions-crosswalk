// Copyright 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REALSENSE_ENHANCED_PHOTOGRAPHY_WIN_MOTION_EFFECT_OBJECT_H_
#define REALSENSE_ENHANCED_PHOTOGRAPHY_WIN_MOTION_EFFECT_OBJECT_H_

#include <string>
#include <vector>

// This file is auto-generated by motion_effect.idl
#include "motion_effect.h" // NOLINT

#include "realsense/enhanced_photography/win/enhanced_photography_instance.h"
#include "third_party/libpxc/include/pxcenhancedphoto.h"
#include "third_party/libpxc/include/pxcimage.h"
#include "third_party/libpxc/include/pxcphoto.h"
#include "third_party/libpxc/include/pxcsession.h"

namespace realsense {
namespace enhanced_photography {

using xwalk::common::XWalkExtensionFunctionInfo;
using namespace jsapi::motion_effect; // NOLINT

class MotionEffectObject : public xwalk::common::BindingObject {
 public:
  explicit MotionEffectObject(EnhancedPhotographyInstance* instance,
                              PXCPhoto* photo);
  ~MotionEffectObject() override;

 private:
  void OnInitMotionEffect(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnApplyMotionEffect(scoped_ptr<XWalkExtensionFunctionInfo> info);

  EnhancedPhotographyInstance* instance_;
  PXCSession* session_;
  PXCEnhancedPhoto* ep_;
  PXCPhoto* photo_;

  scoped_ptr<uint8[]> binary_message_;
  size_t binary_message_size_;
};

}  // namespace enhanced_photography
}  // namespace realsense

#endif  // REALSENSE_ENHANCED_PHOTOGRAPHY_WIN_MOTION_EFFECT_OBJECT_H_
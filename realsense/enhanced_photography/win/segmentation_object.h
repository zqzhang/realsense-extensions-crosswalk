// Copyright 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REALSENSE_ENHANCED_PHOTOGRAPHY_WIN_SEGMENTATION_OBJECT_H_
#define REALSENSE_ENHANCED_PHOTOGRAPHY_WIN_SEGMENTATION_OBJECT_H_

// This file is auto-generated by segmentation.idl
#include "segmentation.h" // NOLINT

#include "realsense/enhanced_photography/win/enhanced_photography_instance.h"
#include "third_party/libpxc/include/pxcenhancedphoto.h"
#include "third_party/libpxc/include/pxcsession.h"
#include "third_party/libpxc/include/pxcphoto.h"

namespace realsense {
namespace enhanced_photography {

using xwalk::common::XWalkExtensionFunctionInfo;
using namespace jsapi::segmentation; // NOLINT

class SegmentationObject : public xwalk::common::BindingObject {
 public:
  explicit SegmentationObject(EnhancedPhotographyInstance* instance,
                              PXCPhoto* photo);
  ~SegmentationObject() override;

 private:
  void OnObjectSegment(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnRedo(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnRefineMask(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnUndo(scoped_ptr<XWalkExtensionFunctionInfo> info);

  bool CopyMaskImageToBinaryMessage(PXCImage* mask);

  EnhancedPhotographyInstance* instance_;
  PXCSession* session_;
  PXCEnhancedPhoto::Segmentation* segmentation_;
  PXCPhoto* photo_;

  scoped_ptr<uint8[]> binary_message_;
  size_t binary_message_size_;
};

}  // namespace enhanced_photography
}  // namespace realsense

#endif  // REALSENSE_ENHANCED_PHOTOGRAPHY_WIN_SEGMENTATION_OBJECT_H_

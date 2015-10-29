// Copyright 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REALSENSE_ENHANCED_PHOTOGRAPHY_DEPTH_PHOTO_OBJECT_H_
#define REALSENSE_ENHANCED_PHOTOGRAPHY_DEPTH_PHOTO_OBJECT_H_

#include "pxcsession.h"  // NOLINT
#include "pxcphoto.h" // NOLINT

// This file is auto-generated by enhanced_photography.idl
#include "depth_photo.h" // NOLINT
#include "realsense/enhanced_photography/win/enhanced_photography_instance.h"
#include "xwalk/common/event_target.h"

namespace realsense {
namespace enhanced_photography {

using xwalk::common::XWalkExtensionFunctionInfo;
using namespace jsapi::depth_photo; // NOLINT

class DepthPhotoObject : public xwalk::common::BindingObject {
 public:
  explicit DepthPhotoObject(EnhancedPhotographyInstance* instance);
  ~DepthPhotoObject() override;

  PXCPhoto* GetPhoto() { return photo_; }
  void DestroyPhoto();

 private:
  void OnLoadXDM(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnSaveXDM(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnQueryReferenceImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnQueryOriginalImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnQueryDepthImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnQueryRawDepthImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnSetReferenceImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnSetOriginalImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnSetDepthImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnSetRawDepthImage(scoped_ptr<XWalkExtensionFunctionInfo> info);
  void OnClone(scoped_ptr<XWalkExtensionFunctionInfo> info);

  bool CopyColorImage(PXCImage* pxcimage);
  bool CopyDepthImage(PXCImage* pxcimage);

  PXCSession* session_;
  PXCPhoto* photo_;
  EnhancedPhotographyInstance* instance_;
  scoped_ptr<uint8[]> binary_message_;
  size_t binary_message_size_;
};

}  // namespace enhanced_photography
}  // namespace realsense

#endif  // REALSENSE_ENHANCED_PHOTOGRAPHY_DEPTH_PHOTO_OBJECT_H_

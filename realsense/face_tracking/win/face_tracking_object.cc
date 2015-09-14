// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "realsense/face_tracking/win/face_tracking_object.h"

#include "pxcfaceconfiguration.h" //NOLINT

#include "base/bind.h"
#include "base/logging.h"
// This file is auto-generated by face_tracking.idl
#include "face_tracking.h" // NOLINT

namespace {

using NativeModeType = PXCFaceConfiguration::TrackingModeType;
using NativeStrategyType = PXCFaceConfiguration::TrackingStrategyType;
using JSModeType = realsense::jsapi::face_tracking::TrackingModeType;
using JSStrategyType = realsense::jsapi::face_tracking::TrackingStrategyType;
using JSFaceConfiguration = realsense::jsapi::face_tracking::FaceConfiguration;

NativeModeType MapTrackingMode(JSModeType params_mode) {
  NativeModeType mode;
  if (params_mode == JSModeType::TRACKING_MODE_TYPE_COLOR) {
    mode = NativeModeType::FACE_MODE_COLOR;
  } else if (params_mode == JSModeType::TRACKING_MODE_TYPE_COLOR_DEPTH) {
    mode = NativeModeType::FACE_MODE_COLOR_PLUS_DEPTH;
  } else {
    mode = NativeModeType::FACE_MODE_COLOR_PLUS_DEPTH;
  }

  return mode;
}

NativeStrategyType MapTrackingStrategy(JSStrategyType params_strategy) {
  NativeStrategyType strategy;
  switch (params_strategy) {
    case JSStrategyType::TRACKING_STRATEGY_TYPE_APPEARANCE_TIME:
      strategy = NativeStrategyType::STRATEGY_APPEARANCE_TIME;
      break;
    case JSStrategyType::TRACKING_STRATEGY_TYPE_CLOSEST_FARTHEST:
      strategy = NativeStrategyType::STRATEGY_CLOSEST_TO_FARTHEST;
      break;
    case JSStrategyType::TRACKING_STRATEGY_TYPE_FARTHEST_CLOSEST:
      strategy = NativeStrategyType::STRATEGY_FARTHEST_TO_CLOSEST;
      break;
    case JSStrategyType::TRACKING_STRATEGY_TYPE_LEFT_RIGHT:
      strategy = NativeStrategyType::STRATEGY_LEFT_TO_RIGHT;
      break;
    case JSStrategyType::TRACKING_STRATEGY_TYPE_RIGHT_LEFT:
      strategy = NativeStrategyType::STRATEGY_RIGHT_TO_LEFT;
      break;
    default:
      strategy = NativeStrategyType::STRATEGY_APPEARANCE_TIME;
      break;
  }

  return strategy;
}

void DisableAllFeatures(PXCFaceConfiguration* config) {
  config->detection.isEnabled = false;
  config->landmarks.isEnabled = false;
  config->pose.isEnabled = false;
  config->QueryExpressions()->DisableAllExpressions();
  config->QueryExpressions()->Disable();
  config->QueryRecognition()->Disable();
}

bool ApplyParamsConfig(
    PXCFaceModule* faceModule, const JSFaceConfiguration* params_config) {
  if (!params_config)
    return true;

  PXCFaceConfiguration* config = faceModule->CreateActiveConfiguration();
  if (!config) {
    return false;
  }

  DisableAllFeatures(config);

  if (params_config->enable_detection) {
    DLOG(INFO) << "ApplyParamsConfig: Enable detection "
        << *(params_config->enable_detection.get());
    config->detection.isEnabled = *(params_config->enable_detection.get());
  }
  if (params_config->enable_landmarks) {
    DLOG(INFO) << "ApplyParamsConfig: Enable landmarks "
        << *(params_config->enable_landmarks.get());
    config->landmarks.isEnabled = *(params_config->enable_landmarks.get());
  }
  if (params_config->max_faces) {
    DLOG(INFO) << "ApplyParamsConfig: max faces is "
        << *(params_config->max_faces.get());
    config->detection.maxTrackedFaces = *(params_config->max_faces.get());
  }

  if (params_config->mode != JSModeType::TRACKING_MODE_TYPE_NONE) {
    PXCFaceConfiguration::TrackingModeType mode =
        MapTrackingMode(params_config->mode);
    config->SetTrackingMode(mode);
    DLOG(INFO) << "ApplyParamsConfig: TrackingMode is " << mode;
  }

  if (params_config->strategy != JSStrategyType::TRACKING_STRATEGY_TYPE_NONE) {
    PXCFaceConfiguration::TrackingStrategyType strategy =
        MapTrackingStrategy(params_config->strategy);
    config->strategy = strategy;
    DLOG(INFO) << "ApplyParamsConfig: TrackingStrategy is " << strategy;
  }

  config->ApplyChanges();
  config->Release();
  return true;
}

}  // namespace

namespace realsense {
namespace face_tracking {

using namespace realsense::jsapi::face_tracking; // NOLINT
using namespace xwalk::common; // NOLINT

FaceTrackingObject::FaceTrackingObject()
    : state_(IDLE),
      on_processedsample_(false),
      on_error_(false),
      face_tracking_thread_("FaceTrackingPreviewThread"),
      message_loop_(base::MessageLoopProxy::current()),
      session_(NULL),
      sense_manager_(NULL),
      face_output_(NULL),
      latest_color_image_(NULL),
      latest_depth_image_(NULL),
      detection_enabled_(false),
      landmark_enabled_(false),
      num_of_landmark_points_(0),
      binary_message_size_(0) {
  handler_.Register("start",
                    base::Bind(&FaceTrackingObject::OnStart,
                               base::Unretained(this)));
  handler_.Register("stop",
                    base::Bind(&FaceTrackingObject::OnStop,
                               base::Unretained(this)));
  handler_.Register("getProcessedSample",
                    base::Bind(&FaceTrackingObject::OnGetProcessedSample,
                               base::Unretained(this)));
}

FaceTrackingObject::~FaceTrackingObject() {
  OnStop(NULL);
  DestroySessionInstance();
}

void FaceTrackingObject::StartEvent(const std::string& type) {
  if (type == std::string("processedsample")) {
    on_processedsample_ = true;
  } else if (type == std::string("error")) {
    on_error_ = true;
  }
}

void FaceTrackingObject::StopEvent(const std::string& type) {
  if (type == std::string("processedsample")) {
    on_processedsample_ = false;
  } else if (type == std::string("error")) {
    on_error_ = false;
  }
}

void FaceTrackingObject::OnStart(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (face_tracking_thread_.IsRunning()) {
    info->PostResult(
        Start::Results::Create(
            std::string(), std::string("Face tracking is already started")));
    return;  // Wrong state.
  }

  face_tracking_thread_.Start();

  face_tracking_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&FaceTrackingObject::OnCreateAndStartPipeline,
                 base::Unretained(this),
                 base::Passed(&info)));
}

void FaceTrackingObject::OnStop(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!face_tracking_thread_.IsRunning()) {
    if (info.get()) {
      info->PostResult(
          Stop::Results::Create(
              std::string(),
              std::string("Face tracking is not started yet")));
    }
    return;  // Wrong state.
  }

  face_tracking_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&FaceTrackingObject::OnStopAndDestroyPipeline,
                 base::Unretained(this),
                 base::Passed(&info)));

  face_tracking_thread_.Stop();
}

void FaceTrackingObject::OnGetProcessedSample(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  if (!face_tracking_thread_.IsRunning()) {
    ProcessedSample processed_sample;
    GetProcessedSample::Results::Create(
        processed_sample, std::string("Pipeline is not started"));
    return;
  }

  face_tracking_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&FaceTrackingObject::OnGetProcessedSampleOnPipeline,
                 base::Unretained(this),
                 base::Passed(&info)));
}

void FaceTrackingObject::OnCreateAndStartPipeline(
  scoped_ptr<XWalkExtensionFunctionInfo> info) {
  DCHECK_EQ(face_tracking_thread_.message_loop(), base::MessageLoop::current());
  DCHECK(state_ == IDLE);

  scoped_ptr<Start::Params> params(
      Start::Params::Create(*info->arguments()));
  if (!params) {
    info->PostResult(
        Start::Results::Create(
            std::string(), "Malformed parameters"));
    StopFaceTrackingThread();
    return;
  }

  // Create session.
  if (!CreateSessionInstance()) {
    info->PostResult(
        Start::Results::Create(
            std::string(), "Failed to create an SDK session"));
    StopFaceTrackingThread();
    return;
  }

  // Create sense manager.
  sense_manager_ = session_->CreateSenseManager();
  if (!sense_manager_) {
    info->PostResult(
        Start::Results::Create(
            std::string(), "Failed to create sense manager"));
    StopFaceTrackingThread();
    return;
  }

  // Enable face module in sense manager.
  PXCFaceModule* faceModule = NULL;
  if (sense_manager_->EnableFace() < PXC_STATUS_NO_ERROR
      || !(faceModule = sense_manager_->QueryFace())) {
    info->PostResult(
        Start::Results::Create(
            std::string(), "Failed to enable face module"));
    ReleaseResources();
    StopFaceTrackingThread();
    return;
  }

  // Apply face module configurations from JS side.
  if (!ApplyParamsConfig(faceModule, params->config.get())) {
    info->PostResult(
        Start::Results::Create(
            std::string(), "Failed to apply face configuration"));
    ReleaseResources();
    StopFaceTrackingThread();
    return;
  }

  // Init sense manager.
  if (sense_manager_->Init() < PXC_STATUS_NO_ERROR) {
    info->PostResult(
        Start::Results::Create(
            std::string(), "Failed to init sense manager"));
    ReleaseResources();
    StopFaceTrackingThread();
    return;
  }

  PXCFaceConfiguration* config = faceModule->CreateActiveConfiguration();
  if (config->GetTrackingMode() ==
      PXCFaceConfiguration::TrackingModeType::FACE_MODE_COLOR_PLUS_DEPTH) {
    // Quote from C++ SDK FaceTracking sample application.
    PXCCapture::DeviceInfo device_info;
    sense_manager_->QueryCaptureManager()->QueryDevice()
        ->QueryDeviceInfo(&device_info);

    if (device_info.model == PXCCapture::DEVICE_MODEL_DS4) {
      sense_manager_->QueryCaptureManager()->QueryDevice()
          ->SetDSLeftRightExposure(26);
      sense_manager_->QueryCaptureManager()->QueryDevice()
          ->SetDepthConfidenceThreshold(0);
    }
  }
  detection_enabled_ = config->detection.isEnabled;
  landmark_enabled_ = config->landmarks.isEnabled;
  num_of_landmark_points_ = config->landmarks.numLandmarks;
  DLOG(INFO) << "Number of landmark points: " << num_of_landmark_points_;

  config->Release();
  config = NULL;

  // Create face module output.
  face_output_ = faceModule->CreateOutput();

  // As we called EnableFace(),
  // SDK will enable the corresponding streams implicitly.
  // We create color/depth images according current stream profiles.
  if (!CreateProcessedSampleImages()) {
    info->PostResult(
        Start::Results::Create(
            std::string(), "Failed to create processed sample images"));
    ReleaseResources();
    StopFaceTrackingThread();
    return;
  }

  state_ = TRACKING;

  face_tracking_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&FaceTrackingObject::OnRunPipeline,
                 base::Unretained(this)));

  info->PostResult(
      Start::Results::Create(
          std::string("success"), std::string()));
}

void FaceTrackingObject::OnRunPipeline() {
  DCHECK_EQ(face_tracking_thread_.message_loop(), base::MessageLoop::current());
  if (state_ == IDLE) return;

  pxcStatus status = sense_manager_->AcquireFrame(true);
  if (status < PXC_STATUS_NO_ERROR) {
    DLOG(ERROR) << "AcquiredFrame failed: " << status;
    if (on_error_) {
      ErrorEvent event;
      event.status = "Fail to AcquireFrame. Stop.";
      scoped_ptr<base::ListValue> eventData(new base::ListValue);
      eventData->Append(event.ToValue().release());
      DispatchEvent("error", eventData.Pass());
    }

    ReleaseResources();
    StopFaceTrackingThread();
    state_ = IDLE;
    return;
  }

  face_output_->Update();
  PXCCapture::Sample* face_sample = sense_manager_->QueryFaceSample();
  if (face_sample) {
    // At least we should have color image!
    if (!face_sample->color) {
      if (on_error_) {
        ErrorEvent event;
        event.status = "Fail to query face sample";
        scoped_ptr<base::ListValue> eventData(new base::ListValue);
        eventData->Append(event.ToValue().release());
        DispatchEvent("error", eventData.Pass());
      }

      ReleaseResources();
      StopFaceTrackingThread();
      state_ = IDLE;
      return;
    }

    if (on_processedsample_) {
      latest_color_image_->CopyImage(face_sample->color);
      if (latest_depth_image_ && face_sample->depth) {
        latest_depth_image_->CopyImage(face_sample->depth);
      }
      DispatchEvent("processedsample");
    }
  } else {
    // face_sample is NULL means face module is paused
    // or error happened. Just ignore and continue next frame.
    DLOG(ERROR) << "QueryFaceSample() returned NULL";
  }

  sense_manager_->ReleaseFrame();

  face_tracking_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&FaceTrackingObject::OnRunPipeline,
                 base::Unretained(this)));
}

void FaceTrackingObject::OnStopAndDestroyPipeline(
    scoped_ptr<xwalk::common::XWalkExtensionFunctionInfo> info) {
  DCHECK_EQ(face_tracking_thread_.message_loop(), base::MessageLoop::current());

  state_ = IDLE;
  ReleaseResources();

  if (info.get()) {
    info->PostResult(
        Stop::Results::Create(
            std::string("Success"), std::string()));
  }
}

void FaceTrackingObject::OnGetProcessedSampleOnPipeline(
    scoped_ptr<XWalkExtensionFunctionInfo> info) {
  DCHECK_EQ(face_tracking_thread_.message_loop(), base::MessageLoop::current());

  bool fail = false;

  if (state_ != TRACKING) {
    ProcessedSample processed_sample;
    info->PostResult(
        GetProcessedSample::Results::Create(
            processed_sample,
            std::string("Is not tracking, no processed_sample")));
    return;
  }

  PXCImage* color = latest_color_image_;
  PXCImage* depth = latest_depth_image_;

  const size_t post_data_size = CalculateBinaryMessageSize();
  if (binary_message_size_ < post_data_size) {
    binary_message_.reset(new uint8[post_data_size]);
    binary_message_size_ = post_data_size;
  }

  size_t offset = 0;
  int* int_array = reinterpret_cast<int*>(binary_message_.get());
  // Fill ProcessedSample::color image.
  PXCImage::ImageInfo color_info = color->QueryInfo();
  int_array[1] = 1;  // 1 for PixelFormat::PIXEL_FORMAT_RGB32
  int_array[2] = color_info.width;
  int_array[3] = color_info.height;
  offset += 4 * sizeof(int);
  PXCImage::ImageData color_data;
  pxcStatus status = color->AcquireAccess(
      PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &color_data);
  uint8_t* uint8_array =
      reinterpret_cast<uint8_t*>(binary_message_.get() + offset);
  if (status >= PXC_STATUS_NO_ERROR) {
    int k = 0;
    uint8_t* rgb32 = reinterpret_cast<uint8_t*>(color_data.planes[0]);
    for (int y = 0; y < color_info.height; ++y) {
      for (int x = 0; x < color_info.width; ++x) {
        int i = (x + color_info.width * y) * 4;
        uint8_array[k++] = rgb32[i + 2];
        uint8_array[k++] = rgb32[i + 1];
        uint8_array[k++] = rgb32[i];
        uint8_array[k++] = rgb32[i + 3];
      }
    }
    offset += color_info.width * color_info.height * 4;
    color->ReleaseAccess(&color_data);
  } else {
    fail = true;
    DLOG(INFO) << "Failed to access color image data: " << status;
  }

  int_array = reinterpret_cast<int*>(binary_message_.get() + offset);
  // Fill ProcessedSample::depth image.
  if (!fail) {
    if (depth) {
      PXCImage::ImageInfo depth_info = depth->QueryInfo();
      int_array[0] = 2;  // 2 for PixelFormat::PIXEL_FORMAT_DEPTH
      int_array[1] = depth_info.width;
      int_array[2] = depth_info.height;
      offset += 3 * sizeof(int);
      PXCImage::ImageData depth_data;
      status = depth->AcquireAccess(
          PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_DEPTH, &depth_data);
      uint16_t* uint16_array = reinterpret_cast<uint16_t*>(
          binary_message_.get() + offset);
      if (status >= PXC_STATUS_NO_ERROR) {
        int k = 0;
        for (int y = 0; y < depth_info.height; ++y) {
          for (int x = 0; x < depth_info.width; ++x) {
            uint16_t* depth16 =
                reinterpret_cast<uint16_t*>(
                    depth_data.planes[0] + depth_data.pitches[0] * y);
            uint16_array[k++] = depth16[x];
          }
        }
        offset += depth_info.width * depth_info.height * 2;
        depth->ReleaseAccess(&depth_data);
      } else {
        fail = true;
        DLOG(INFO) << "Failed to access depth image data: " << status;
      }
    } else {
      // If no depth stream, only set depth width and height as 0.
      int_array[0] = 2;
      int_array[1] = 0;
      int_array[2] = 0;
      offset += 3 * sizeof(int);
    }
  }

  // Fill ProcessedSample::faceResults data.
  if (!fail) {
    const int num_of_faces = face_output_->QueryNumberOfDetectedFaces();

    int_array = reinterpret_cast<int*>(binary_message_.get() + offset);
    int_array[0] = num_of_faces;
    int_array[1] = detection_enabled_ ? 1 : 0;
    int_array[2] = landmark_enabled_ ? 1 : 0;
    offset += 3 * sizeof(int);

    for (int i = 0; i < num_of_faces; i++) {
      PXCFaceData::Face* trackedFace = face_output_->QueryFaceByIndex(i);

      if (detection_enabled_) {
        const PXCFaceData::DetectionData* detectionData =
            trackedFace->QueryDetection();
        // Fill Face::detection data.
        if (detectionData) {
          int_array = reinterpret_cast<int*>(binary_message_.get() + offset);
          PXCRectI32 rectangle;
          if (detectionData->QueryBoundingRect(&rectangle)) {
            DLOG(INFO) << "Traced face No." << i << ": "
                << rectangle.x << ", " << rectangle.y << ", "
                << rectangle.w << ", " << rectangle.h;
            int_array[0] = rectangle.x;
            int_array[1] = rectangle.y;
            int_array[2] = rectangle.w;
            int_array[3] = rectangle.h;
          }
          offset += 4 * sizeof(int);

          pxcF32 avgDepth;
          if (detectionData->QueryFaceAverageDepth(&avgDepth)) {
            *(reinterpret_cast<float*>(binary_message_.get() + offset)) =
                avgDepth;
          }
          offset += sizeof(float);
        } else {
          // In case of no detection data.
          memset(binary_message_.get() + offset,
                 0,
                 4 * sizeof(int) + sizeof(float));
          offset += (4 * sizeof(int) + sizeof(float));
        }
      }

      if (landmark_enabled_) {
        const PXCFaceData::LandmarksData* landmarkData =
            trackedFace->QueryLandmarks();
        // Fill Face::landmark data.
        if (landmarkData) {
          const int num_of_points = landmarkData->QueryNumPoints();
          DCHECK(num_of_points == num_of_landmark_points_);
          *(reinterpret_cast<int*>(binary_message_.get() + offset)) =
              num_of_points;
          offset += sizeof(int);

          PXCFaceData::LandmarkPoint landmark_point;
          for (int j = 0; j < num_of_points; j++) {
            int_array = reinterpret_cast<int*>(binary_message_.get() + offset);
            landmarkData->QueryPoint(j, &landmark_point);

            DCHECK(landmark_point.source.index == j);
            int_array[0] = landmark_point.source.alias;
            int_array[1] = landmark_point.confidenceImage;
            int_array[2] = landmark_point.confidenceWorld;
            offset += 3 * sizeof(int);

            float* float_array =
                reinterpret_cast<float*>(binary_message_.get() + offset);
            float_array[0] = landmark_point.world.x;
            float_array[1] = landmark_point.world.y;
            float_array[2] = landmark_point.world.z;
            float_array[3] = landmark_point.image.x;
            float_array[4] = landmark_point.image.y;
            offset += 5 * sizeof(float);
          }
        } else {
          // No landmark data for this face.
          *(reinterpret_cast<int*>(binary_message_.get() + offset)) = 0;
          offset += sizeof(int);
        }
      }
    }
  }

  if (fail) {
    ProcessedSample processed_sample;
    info->PostResult(
        GetProcessedSample::Results::Create(
            processed_sample,
            std::string("Failed to prepare processed_sample")));
  } else {
    DCHECK(offset <= post_data_size);
    scoped_ptr<base::ListValue> result(new base::ListValue());
    result->Append(base::BinaryValue::CreateWithCopiedBuffer(
        reinterpret_cast<const char*>(binary_message_.get()),
        offset));
    info->PostResult(result.Pass());
  }
}

bool FaceTrackingObject::CreateSessionInstance() {
  if (session_) {
    return true;
  }

  session_ = PXCSession::CreateInstance();
  if (!session_) {
    return false;
  }
  return true;
}

void FaceTrackingObject::DestroySessionInstance() {
  if (session_) {
    session_->Release();
    session_ = NULL;
  }
}

bool FaceTrackingObject::CreateProcessedSampleImages() {
  DCHECK_EQ(face_tracking_thread_.message_loop(), base::MessageLoop::current());
  DCHECK(!latest_color_image_);
  DCHECK(!latest_depth_image_);

  PXCCapture::Device::StreamProfileSet profiles = {};
  sense_manager_->QueryCaptureManager()->QueryDevice()
      ->QueryStreamProfileSet(&profiles);

  // color image.
  if (profiles.color.imageInfo.format) {
    // TODO(leonhsl): Currently we assume color stream is PIXEL_FORMAT_RGB32.
    DCHECK(profiles.color.imageInfo.format == PXCImage::PIXEL_FORMAT_RGB32);
    DLOG(INFO) << "color.imageInfo: width is " << profiles.color.imageInfo.width
        << ", height is " << profiles.color.imageInfo.height
        << ", format is "
        << PXCImage::PixelFormatToString(profiles.color.imageInfo.format);
    PXCImage::ImageInfo image_info = profiles.color.imageInfo;
    image_info.format = PXCImage::PIXEL_FORMAT_RGB32;
    latest_color_image_ = sense_manager_->QuerySession()
        ->CreateImage(&image_info);
  }
  // depth image.
  if (profiles.depth.imageInfo.format) {
    // TODO(leonhsl): Currently we assume depth stream is PIXEL_FORMAT_DEPTH.
    DCHECK(profiles.depth.imageInfo.format == PXCImage::PIXEL_FORMAT_DEPTH);
    DLOG(INFO) << "depth.imageInfo: width is " << profiles.depth.imageInfo.width
        << ", height is " << profiles.depth.imageInfo.height
        << ", format is "
        << PXCImage::PixelFormatToString(profiles.depth.imageInfo.format);
    PXCImage::ImageInfo image_info = profiles.depth.imageInfo;
    image_info.format = PXCImage::PIXEL_FORMAT_DEPTH;
    latest_depth_image_ = sense_manager_->QuerySession()
        ->CreateImage(&image_info);
  }

  // At least should have color image!
  return latest_color_image_ != NULL;
}

void FaceTrackingObject::ReleaseResources() {
  DCHECK_EQ(face_tracking_thread_.message_loop(), base::MessageLoop::current());

  binary_message_.reset();
  binary_message_size_ = 0;

  if (latest_color_image_) {
    latest_color_image_->Release();
    latest_color_image_ = NULL;
  }
  if (latest_depth_image_) {
    latest_depth_image_->Release();
    latest_depth_image_ = NULL;
  }
  if (face_output_) {
    face_output_->Release();
    face_output_ = NULL;
  }
  if (sense_manager_) {
    sense_manager_->Close();
    sense_manager_->Release();
    sense_manager_ = NULL;
  }
}

void FaceTrackingObject::StopFaceTrackingThread() {
  message_loop_->PostTask(
      FROM_HERE,
      base::Bind(&FaceTrackingObject::OnStopFaceTrackingThread,
                 base::Unretained(this)));
}

void FaceTrackingObject::OnStopFaceTrackingThread() {
  if (face_tracking_thread_.IsRunning()) {
    face_tracking_thread_.Stop();
  }
}

// binary message: call_id (int32),
// color format (int32), width (int32), height (int32), data (int8 buffer),
// depth format (int32), width (int32), height (int32), data (int16 buffer),
// number of faces (int32),
// detection data available (int32),
// landmark data available (int32),
// face data array:
//     array element: rect x, y, w, h (int32), avgDepth (float32),
//                    landmark data: number of landmark points (int32),
//                                   landmark point data array:
//                                      array element:
//                                               type (int32),
//                                               image confidence (int32),
//                                               world confidence (int32),
//                                               world point x, y, z (float32),
//                                               image point x, y (float32),
size_t FaceTrackingObject::CalculateBinaryMessageSize() {
  const int image_header_size = 3 * sizeof(int);  // format, width, height
  PXCImage::ImageInfo color_info = latest_color_image_->QueryInfo();
  const int color_image_size = color_info.width * color_info.height * 4;

  int depth_image_size = 0;
  if (latest_depth_image_) {
    PXCImage::ImageInfo depth_info = latest_depth_image_->QueryInfo();
    depth_image_size = depth_info.width * depth_info.height * 2;
  }

  const int num_of_faces = face_output_->QueryNumberOfDetectedFaces();
  int one_face_size = 0;
  if (detection_enabled_) {
    one_face_size += (4 * sizeof(int) + sizeof(float));
  }
  if (landmark_enabled_) {
    int landmark_size = 0;
    // size for "number of landmark points"
    landmark_size += sizeof(int);
    // size for "one landmark point data"
    const int landmark_point_size = 3 * sizeof(int) + 5 * sizeof(float);
    landmark_size += num_of_landmark_points_ * landmark_point_size;

    one_face_size += landmark_size;
  }

  const int message_size =
      // call_id
      sizeof(int)
      // color image
      + image_header_size + color_image_size
      // depth image
      + image_header_size + depth_image_size
      // faceResults
      + 3 * sizeof(int) + num_of_faces * one_face_size;
  return message_size;
}

}  // namespace face_tracking
}  // namespace realsense

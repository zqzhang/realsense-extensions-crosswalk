// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var FaceTracking = function(object_id) {
  common.BindingObject.call(this, common.getUniqueId());
  common.EventTarget.call(this);

  if (object_id == undefined)
    internal.postMessage('faceTrackingConstructor', [this._id]);

  function wrapProcessedSampleReturns(data) {
    // ProcessedSample layout:
    // color format (int32), width (int32), height (int32), data (int8 buffer),
    // depth format (int32), width (int32), height (int32), data (int16 buffer),
    // FaceResults

    // FaceResults layout:
    // number of faces (int32),
    // detection data available (int32),
    // landmark data available (int32),
    // Face Array

    // Face layout:
    // detection data: rect x, y, w, h (int32), avgDepth (float32),
    // landmark data: number of landmark points (int32),
    //                landmark point array
    //                    landmark point layout:
    //                    type (int32),
    //                    image confidence (int32),
    //                    world confidence (int32),
    //                    world point x, y, z (float32),
    //                    image point x, y (float32)
    var int32_array = new Int32Array(data, 0, 4);
    // color format
    var color_format = '';
    if (int32_array[1] == 1) {
      color_format = 'RGB32';
    } else if (int32_array[1] == 2) {
      color_format = 'DEPTH';
    }
    // color width, height
    var color_width = int32_array[2];
    var color_height = int32_array[3];
    // color data
    var offset = 4 * 4; // 4 int32(4 bytes)
    var color_data =
        new Uint8Array(data, offset, color_width * color_height * 4);
    offset = offset + color_width * color_height * 4;

    int32_array = new Int32Array(data, offset, 3);
    // depth format
    var depth_format = '';
    if (int32_array[0] == 1) {
      depth_format = 'RGB32';
    } else if (int32_array[0] == 2) {
      depth_format = 'DEPTH';
    }
    // depth width, height
    var depth_width = int32_array[1];
    var depth_height = int32_array[2];
    // depth data
    var offset = offset + 3 * 4; // 3 int32(4 bytes)
    var depth_data =
        new Uint16Array(data, offset, depth_width * depth_height);
    offset = offset + depth_width * depth_height * 2;

    var face_array = [];
    int32_array = new Int32Array(data, offset, 3);
    // number of faces
    var num_of_faces = int32_array[0];
    var detection_enabled = int32_array[1] > 0 ? true : false;
    var landmark_enabled = int32_array[2] > 0 ? true : false;
    offset = offset + 3 * 4; // 3 int32(4 bytes)

    for (var i = 0; i < num_of_faces; ++i) {
      int32_array = new Int32Array(data, offset, 4);

      var detection_value = undefined;
      var landmark_value = undefined;
      if (detection_enabled) {
        var rect_value = {
          x: int32_array[0],
          y: int32_array[1],
          w: int32_array[2],
          h: int32_array[3],
        };
        offset = offset + 4 * 4; // 4 int32(4 bytes)
        var float32_array = new Float32Array(data, offset, 1);
        offset = offset + 4; // 1 float32(4 bytes)
        detection_value = {
          boundingRect: rect_value,
          avgDepth: float32_array[0]
        };
      }
      if (landmark_enabled) {
        var landmark_points_array = [];
        int32_array = new Int32Array(data, offset, 1);
        var num_of_points = int32_array[0];
        offset = offset + 4; // 1 int32(4 bytes)

        for (var i = 0; i < num_of_points; ++i) {
          int32_array = new Int32Array(data, offset, 3);
          offset = offset + 3 * 4; // 3 int32(4 bytes)
          var float32_array = new Float32Array(data, offset, 5);
          offset = offset + 5 * 4; // 5 float32(4 bytes)

          var landmark_point = {
            type: int32_array[0],
            confidenceImage: int32_array[1],
            confidenceWorld: int32_array[2],
            coordinateWorld: { x: float32_array[0], y: float32_array[1], z: float32_array[2] },
            coordinateImage: { x: float32_array[3], y: float32_array[4] },
          };
          landmark_points_array.push(landmark_point);
        }
        landmark_value = { points: landmark_points_array };
      }

      var face = {
        detection: detection_value,
        landmark: landmark_value
      };
      face_array.push(face);
    }

    var color_image_value = {
      format: color_format,
      width: color_width,
      height: color_height,
      data: color_data
    };
    var depth_image_value = undefined;
    if (depth_width > 0 && depth_height > 0) {
      depth_image_value = {
        format: depth_format,
        width: depth_width,
        height: depth_height,
        data: depth_data
      };
    }
    var face_results_value = {
      faces: face_array
    };
    return {
      color: color_image_value,
      depth: depth_image_value,
      faceResults: face_results_value
    };
  };

  this._addMethodWithPromise('start', Promise);
  this._addMethodWithPromise('stop', Promise);
  this._addMethodWithPromise('getProcessedSample', Promise, null, wrapProcessedSampleReturns);

  this._addEvent('error');
  this._addEvent('processedsample');
};

FaceTracking.prototype = new common.EventTargetPrototype();
FaceTracking.prototype.constructor = FaceTracking;

exports = new FaceTracking();

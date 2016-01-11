var statusElement = document.getElementById('status');
var startButton = document.getElementById('start');
var stopButton = document.getElementById('stop');
var takePhotoButton = document.getElementById('takePhoto');
var loadPhoto = document.getElementById('loadPhoto');

var previewCanvas = document.getElementById('preview');
var imageCanvas = document.getElementById('image');

var motionEffect, photoCapture, XDMUtils;
var previewContext, previewData, imageContext, imageData;

var width = 1920, height = 1080;
var yaw = 0.0, pitch = 0.0, roll = 0.0, zoom = 0.0;
var right = 0.0, up = 0.0, forward = 0.0;

var hasImage = false;
var isInitialized = false;

function outputRightUpdate(value) {
  right = parseInt(value) * 0.2;
  doMothionEffect();
}

function outputUpUpdate(value) {
  up = parseInt(value) * 0.2;
  doMothionEffect();
}

function outputforwardUpdate(value) {
  forward = parseInt(value) * 0.2;
  doMothionEffect();
}

function outputYawUpdate(value) {
  yaw = parseInt(value) * 0.2;
  doMothionEffect();
}

function outputPitchUpdate(value) {
  pitch = parseInt(value) * 0.2;
  doMothionEffect();
}

function outputRollUpdate(value) {
  roll = parseInt(value) * 0.2;
  doMothionEffect();
}

function outputZoomUpdate(value) {
  zoom = parseInt(value) * 0.2 * 0.2;
  doMothionEffect();
}

function doMothionEffect() {
  if (!hasImage || !isInitialized || !motionEffect)
    return;

  motionEffect.apply({ horizontal: right, vertical: up, distance: forward },
                     { pitch: pitch, yaw: yaw, roll: roll },
                     zoom).then(
      function(image) {
        statusElement.innerHTML = 'Finished MotionEffects';
        imageData.data.set(image.data);
        imageContext.putImageData(imageData, 0, 0);
      },
      function(e) { statusElement.innerHTML = e; });
}

function main() {
  photoCapture = realsense.DepthEnabledPhotography.PhotoCapture;
  XDMUtils = realsense.DepthEnabledPhotography.XDMUtils;

  previewContext = previewCanvas.getContext('2d');
  imageContext = imageCanvas.getContext('2d');
  previewData = previewContext.createImageData(width, height);

  var gettingImage = false;
  photoCapture.onpreview = function(e) {
    if (gettingImage)
      return;
    gettingImage = true;
    photoCapture.getPreviewImage().then(
        function(image) {
          previewData.data.set(image.data);
          previewContext.putImageData(previewData, 0, 0);
          gettingImage = false;
        }, function() { });
  };

  photoCapture.onerror = function(e) {
    statusElement.innerHTML = 'Status Info : onerror: ' + e.status;
  };

  startButton.onclick = function(e) {
    statusElement.innerHTML = 'Status Info : Start: ';
    gettingImage = false;
    photoCapture.startPreview({
      colorWidth: 1920,
      colorHeight: 1080,
      depthWidth: 480,
      depthHeight: 360,
      framerate: 30}).then(
        function(e) { statusElement.innerHTML += e; },
        function(e) { statusElement.innerHTML += e; });
  };

  takePhotoButton.onclick = function(e) {
    photoCapture.takePhoto().then(
        function(photo) {
          photo.queryContainerImage().then(
              function(image) {
                imageData =
                    imageContext.createImageData(image.width, image.height);
                statusElement.innerHTML = 'Take photo sucessfully';
                imageData.data.set(image.data);
                imageContext.putImageData(imageData, 0, 0);
                hasImage = true;

                if (!motionEffect) {
                  motionEffect = new realsense.DepthEnabledPhotography.MotionEffect();
                }
                motionEffect.init(photo).then(
                    function() {
                      isInitialized = true;
                      doMothionEffect();
                    },
                    function(e) { statusElement.innerHTML = e });
              },
              function(e) { statusElement.innerHTML = e; });
        },
        function(e) { statusElement.innerHTML = e; });
  };

  loadPhoto.addEventListener('change', function(e) {
    var file = loadPhoto.files[0];
    XDMUtils.isXDM(file).then(
        function(success) {
          if (success) {
            XDMUtils.loadXDM(file).then(
                function(photo) {
                  photo.queryContainerImage().then(
                      function(image) {
                        imageContext.clearRect(0, 0, width, height);
                        imageData = imageContext.createImageData(image.width, image.height);
                        statusElement.innerHTML = 'Load successfully';
                        imageData.data.set(image.data);
                        imageContext.putImageData(imageData, 0, 0);
                        hasImage = true;
                        if (!motionEffect) {
                          motionEffect =
                              new realsense.DepthEnabledPhotography.MotionEffect();
                        }
                        motionEffect.init(photo).then(
                            function() {
                              isInitialized = true;
                              doMothionEffect();
                            },
                            function(e) { statusElement.innerHTML = e });
                      },
                      function(e) { statusElement.innerHTML = e; });
                },
                function(e) { statusElement.innerHTML = e; });
          } else {
            statusElement.innerHTML = 'This is not a XDM file. Load failed.';
          }
        },
        function(e) { statusElement.innerHTML = e; });
  });

  stopButton.onclick = function(e) {
    statusElement.innerHTML = 'Status Info : Stop: ';
    photoCapture.stopPreview().then(function(e) { statusElement.innerHTML += e; },
                                    function(e) { statusElement.innerHTML += e; });
  };
}

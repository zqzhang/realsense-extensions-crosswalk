// MODULE_NAME will be defined here.
var child_process = require('child_process');
var fs = require('fs');
var OS = require('os');
var Path = require('path');

var RS_RUNTIME_URL_BASE =
    'http://registrationcenter-download.intel.com/akdlm/irc_nas/8516/';
var RS_RUNTIME_URL = RS_RUNTIME_URL_BASE +
    'intel_rs_sdk_runtime_websetup_8.0.24.6528.exe';
var RS_LICENSE_URL = RS_RUNTIME_URL_BASE +
    'Intel%20RealSense%20SDK%20RT%20EULA.rtf';
var FeatureNameMap = {
  'RS_R200_DEP': 'epv',
  'RS_R200_SP': 'scene_perception',
  'RS_R200_Face': 'face3d',
  'RS_F200_Hand': 'hand'
};

function RsRuntimePackagingHooks(app, configId, extraArgs, sharedState) {
  this._app = app;
  this._sharedState = sharedState;
  this._output = app.output;
  this._util = app.util;
  this._hooksTempPath = app.rootPath + Path.sep + 'hooksTemp';
  this._util.ShellJS.mkdir(this._hooksTempPath);

  this._moduleNamesFile = this._hooksTempPath + Path.sep + 'RS_MODULE_NAMES';
  this._postTaskLockFile = this._hooksTempPath + Path.sep + 'POST_TASK_LOCK';
}

RsRuntimePackagingHooks.prototype.prePackage = function(platform, callback) {
  this._app.output.info('Hooks: prePackage ' + MODULE_NAME + ' ' + platform);
  if (process.env.NO_RS_RUNTIME_HOOKS == 1 || process.env.NO_RS_RUNTIME_HOOKS == true) {
    this._app.output.warning(
        'Skip RealSense runtime packaging hooks, because NO_RS_RUNTIME_HOOKS was set.');
    callback(0);
    return;
  }
  fs.appendFileSync(this._moduleNamesFile, MODULE_NAME + ';');
  if (callback instanceof Function)
    callback(0);
};

RsRuntimePackagingHooks.prototype.postPackage = function(platform, callback) {
  this._app.output.info('Hooks: postPackage ' + MODULE_NAME + ' ' + platform);
  if (process.env.NO_RS_RUNTIME_HOOKS == 1 || process.env.NO_RS_RUNTIME_HOOKS == true) {
    this._app.output.warning(
        'Skip RealSense runtime packaging hooks, because NO_RS_RUNTIME_HOOKS was set.');
    callback(0);
    return;
  }
  // Get the lock to do post work.
  if (this._util.ShellJS.test('-f', this._postTaskLockFile)) {
    callback(0);
    return;
  }
  fs.closeSync(fs.openSync(this._postTaskLockFile, 'w'));

  // Get the generated wxs file.
  var wxsFiles = this._util.ShellJS.ls(Path.join(this._app.rootPath, '*.wxs'));
  if (wxsFiles.length < 1) {
    this._output.error('Error: no .wxs file found in ' + this._app.rootPath);
    callback(1);
    return;
  }
  // Currently, we pick up the first wxs file.
  var wxsFile = wxsFiles[0];

  // Get the needed modules, registerred by prePackage phase.
  var modules = fs.readFileSync(this._moduleNamesFile, 'utf8');
  var length = modules.length;
  if (modules[length - 1] == ';') {
    modules = modules.substr(0, length - 1);
  }
  modules = modules.split(';');

  // Try to get the runtime installer.
  var runtimeFile = null;
  if (process.env.RS_RUNTIME_WEB_INSTALLER) {
    if (this._util.ShellJS.test('-f', process.env.RS_RUNTIME_WEB_INSTALLER)) {
      runtimeFile = process.env.RS_RUNTIME_WEB_INSTALLER;
    }
  }
  if (process.env.RS_RUNTIME_OFFLINE_INSTALLER) {
    // TODO(Donna):
    // Create right pre-bundle command line according to the modules
    // Pre-bundle the installer, so that we can just include the needed modules.
    // Bundle the customized runtime with Application MSI with correct
    // command line options.
  }
  // If no environment variables we can use to can runtime installer, we need to
  // downdown the web installer from website and then bundle it with App MSI.
  function bundleCbk(success) {
    if (callback instanceof Function) callback(success ? 0 : 1);
  }
  var tryTimes = 10;
  this.downloadFromUrl(RS_LICENSE_URL, '.', tryTimes, function(licenseFile, errorMsg) {
    if (!this._util.ShellJS.test('-f', licenseFile)) {
      this._output.warning('Failed to download license file, ' + errorMsg);
    }
    if (this._util.ShellJS.test('-f', runtimeFile)) {
      this.bundleThemAll(wxsFile, runtimeFile, licenseFile, modules, bundleCbk);
      return;
    }
    this.downloadFromUrl(RS_RUNTIME_URL, '.', tryTimes, function(runtimeFile, errorMsg) {
      if (this._util.ShellJS.test('-f', runtimeFile)) {
        this.bundleThemAll(wxsFile, runtimeFile, licenseFile, modules, bundleCbk);
      } else {
        this._output.error('Failed to download runtime installer, ' + errorMsg);
        callback(1);
      }
    }.bind(this));
  }.bind(this));
};

// Return name like:
// 'intel_rs_sdk_runtime_websetup_7.0.23.6161.exe'
function buildFileName(url) {
  return Path.basename(url);
  /*
     var name = 'intel_rs_sdk_runtime_';
     if (!offline)
     name = name + 'websetup_';
     return name + version + '.exe';
     */
}

// To be used for cmd line arguments.
function InQuotes(arg) {
  return '\"' + arg + '\"';
}

RsRuntimePackagingHooks.prototype.runWix = function(basename, wxsPath, options, callback) {
  var candle = 'candle ' + options + ' ' + wxsPath;
  this._output.info('Running "' + candle + '"');
  var child = child_process.exec(candle);

  child.stdout.on('data', function(data) {
    this.onData(data);
  }.bind(this));

  child.stderr.on('data', function(data) {
    this._output.warning(data);
  }.bind(this));

  child.on('exit', function(code, signal) {
    if (code) {
      this._output.error('Unhandled error ' + code);
      callback(false);
    } else {
      this.runWixLight(basename, options, callback);
    }
    return;
  }.bind(this));
};

RsRuntimePackagingHooks.prototype.runWixLight = function(basename, options, callback) {

  var light = 'light ' + options + ' ' + basename + '.wixobj';
  this._output.info('Running "' + light + '"');
  var child = child_process.exec(light);

  child.stdout.on('data', function(data) {
    this.onData(data);
  }.bind(this));

  child.stderr.on('data', function(data) {
    this._output.warning(data);
  }.bind(this));

  child.on('exit', function(code, signal) {
    if (code) {
      this._output.error('Unhandled error ' + code);
    }
    callback(code === 0);
    return;
  }.bind(this));
};

RsRuntimePackagingHooks.prototype.onData = function(data) {
};

// This callback of this function:
// function ([Boolean] success) {}
RsRuntimePackagingHooks.prototype.bundleThemAll =
    function(wxsFile, runtimeFile, licenseFile, modules, callback) {
  this._output.info('Create a bundle with following files:');
  this._output.info('wxsFile:' + wxsFile);
  this._output.info('runtimeFile:' + runtimeFile);
  this._output.info('licenseFile:' + licenseFile);
  this._output.info('modules:' + modules);
  var DOMParser = this._util.XmlDom.DOMParser;
  var XMLSerializer = this._util.XmlDom.XMLSerializer;

  var buf = fs.readFileSync(wxsFile, {'encoding': 'utf8'});
  var doc = new DOMParser().parseFromString(buf);

  var product = doc.getElementsByTagName('Product');
  var featureEle = doc.getElementsByTagName('Feature');
  if (product.length < 1 || featureEle.length < 1) {
    this._output.error('Failed to get [Product] or [Feature] element of:' + wxsFile);
    if (callback instanceof Function)
      callback(false);
  }
  product = product[0];
  featureEle = featureEle[0];
  var version = product.getAttribute('Version');

  // Insert the customAction in the wxs file.
  var appRootRef = doc.createElement('DirectoryRef');
  appRootRef.setAttribute('Id', 'ApplicationRootFolder');
  product.appendChild(appRootRef);

  var rtFolder = doc.createElement('Directory');
  rtFolder.setAttribute('Id', 'RSSDKRuntimeFolder');
  rtFolder.setAttribute('Name', 'rssdk_runtime');
  appRootRef.appendChild(rtFolder);

  var fileKey = Path.basename(runtimeFile);
  var component = doc.createElement('Component');
  component.setAttribute('Id', fileKey);
  component.setAttribute('Guid', this._util.NodeUuid.v1());
  component.setAttribute('Win64', 'yes');
  rtFolder.appendChild(component);

  var fileEle = doc.createElement('File');
  fileEle.setAttribute('Id', fileKey);
  fileEle.setAttribute('Source', runtimeFile);
  component.appendChild(fileEle);

  // Add component referring item.
  var componentRef = doc.createElement('ComponentRef');
  componentRef.setAttribute('Id', fileKey);
  featureEle.appendChild(componentRef);

  var rtCmdLine = getRuntimeCmdOptions(modules);
  var actionId = 'InstallRSSDKRuntime';
  var customAction = doc.createElement('CustomAction');
  customAction.setAttribute('Id', actionId);
  customAction.setAttribute('FileKey', fileKey);
  customAction.setAttribute('ExeCommand', rtCmdLine);
  customAction.setAttribute('Execute', 'deferred');
  customAction.setAttribute('Return', 'ignore');
  product.appendChild(customAction);

  var installSeq = doc.createElement('InstallExecuteSequence');
  product.appendChild(installSeq);

  var customEle = doc.createElement('Custom');
  customEle.setAttribute('Action', actionId);
  customEle.setAttribute('Before', 'InstallFinalize');
  var cData = doc.createCDATASection('NOT REMOVE');
  customEle.appendChild(cData);
  // TODO: add cdata?
  installSeq.appendChild(customEle);

  // Add UI hint.
  var ui = doc.createElement('UI');
  product.appendChild(ui);

  var processText = doc.createElement('ProgressText');
  processText.setAttribute('Action', actionId);
  var textNode = doc.createTextNode('Installing RS SDK runtime');
  processText.appendChild(textNode);
  ui.appendChild(processText);

  // Use build-in Wix_minimal dialog to show EULA.
  var uiRef = doc.createElement('UIRef');
  uiRef.setAttribute('Id', 'WixUI_Minimal');
  product.appendChild(uiRef);

  // Delete 'Property:ARPNOMODIFY' setting,
  // because it is conflict with 'WixUI_Minimal'.
  var pArray = doc.getElementsByTagName('Property');
  var length = pArray.length;
  var p;
  for (var i = 0; i < length; i++) {
    p = pArray[i];
    if (p.getAttribute('Id') == 'ARPNOMODIFY')
      product.removeChild(p);
  }

  // Add EULA.
  var eula = doc.createElement('WixVariable');
  eula.setAttribute('Id', 'WixUILicenseRtf');
  eula.setAttribute('Value', licenseFile);
  product.appendChild(eula);

  var xmlStr = new XMLSerializer().serializeToString(doc);
  var basename = this._app.manifest.packageId + '_with_rssdk_runtime_' + version;
  var wxsPath = Path.join(this._app.rootPath, basename + '.wxs');
  fs.writeFileSync(wxsPath, xmlStr);
  var wixOptions = '-v -ext WixUIExtension';
  this.runWix(InQuotes(basename), wxsPath, wixOptions, function(success) {
    if (success) {
      var generatedFile = Path.resolve(basename + '.msi');
      if (!this._util.ShellJS.test('-f', generatedFile)) {
        this._output.error('Bundle installer could not be found ' + generatedFile);
        success = false;
      } else {
        this._output.highlight('Installer including RealSense runtime: ' + generatedFile);
      }

      // Only delete on success, for debugging reasons.
      this._util.ShellJS.rm('-f', basename + '.wixobj');
      this._util.ShellJS.rm('-f', basename + '.wixpdb');
    }
    if (callback instanceof Function)
      callback(success);
  }.bind(this));
};

function getRuntimeCmdOptions(modules) {
  var features = ' --passive --acceptlicense=yes --fnone=all --finstall=';
  modules.forEach(function(m, i) {
    if (FeatureNameMap.hasOwnProperty(m)) {
      features += FeatureNameMap[m];
      features += ',';
    }
  });
  features += 'core,vs_rt_2012';
  return features;
}


/**
 * Download file from a URL, checks for already existing file,
 * and returns it in case.
 * @param {String} url runtime URL string
 * @param {String} defaultPath Directory to download to if not already exists
 * @param {Number} tryTimes Max try times if download failed.
 * @param {downloadFinishedCb} callback callback function
 * @throws {FileCreationFailed} If download file could not be written.
 */
RsRuntimePackagingHooks.prototype.downloadFromUrl = function(url, defaultPath, tryTimes, callback) {
  // Namespaces

  var fileName = buildFileName(url);

  // Check for existing download in defaultPath, parent dir, and cache dir if set
  var handler = new this._util.DownloadHandler(defaultPath, fileName);
  var localDirs = [defaultPath, ''];
  if (process.env.RS_RUNTIME_CACHE_DIR)
    localDirs.push(process.env.RS_RUNTIME_CACHE_DIR);
  var localPath = handler.findLocally(localDirs);
  if (localPath) {
    this._output.info('Using cached', localPath);
    callback(Path.resolve(localPath));
    return;
  }

  // Download
  var label = 'Downloading ' + url;
  var indicator = this._output.createFiniteProgress(label);

  var stream = handler.createStream();
  var downloader = new this._util.Downloader(url, stream);
  downloader.progress = function(progress) {
    indicator.update(progress);
  };
  downloader.get(function(errormsg) {
    indicator.done('');

    if (errormsg) {
      if (--tryTimes > 0) {
        this.downloadFromUrl(url, defaultPath, tryTimes, callback);
        return;
      }
      callback(null, errormsg);
    } else {
      var finishedPath = handler.finish(process.env.RS_RUNTIME_CACHE_DIR);
      callback(Path.resolve(finishedPath));
    }
  }.bind(this));
};

module.exports = RsRuntimePackagingHooks;

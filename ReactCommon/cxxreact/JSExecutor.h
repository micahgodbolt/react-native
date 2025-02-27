// Copyright (c) Facebook, Inc. and its affiliates.

// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <memory>
#include <string>
#include <cassert>

#include <cxxreact/ModuleRegistry.h>
#include <cxxreact/NativeModule.h>
#include <folly/dynamic.h>

#ifndef RN_EXPORT
#define RN_EXPORT __attribute__((visibility("default")))
#endif

namespace facebook {
namespace react {

class JSBigString;
class ExecutorDelegate;
class JSExecutor;
class JSModulesUnbundle;
class MessageQueueThread;
class RAMBundleRegistry;
struct InstanceCallback;
struct JSEConfigParams;

class ExecutorDelegateFactory {
public:
  virtual std::unique_ptr<ExecutorDelegate> createExecutorDelegate(
    std::shared_ptr<ModuleRegistry> registry,
    std::shared_ptr<InstanceCallback> callback) = 0;
  virtual ~ExecutorDelegateFactory() {}
};

// This interface describes the delegate interface required by
// Executor implementations to call from JS into native code.
class ExecutorDelegate {
 public:
  virtual ~ExecutorDelegate() {}

  virtual std::shared_ptr<ModuleRegistry> getModuleRegistry() = 0;

  virtual void callNativeModules(
    JSExecutor& executor, folly::dynamic&& calls, bool isEndOfBatch) = 0;
  virtual MethodCallResult callSerializableNativeHook(
    JSExecutor& executor, unsigned int moduleId, unsigned int methodId, folly::dynamic&& args) = 0;

  virtual bool isBatchActive() = 0;
};

using NativeExtensionsProvider = std::function<folly::dynamic(const std::string&)>;

class JSExecutorFactory {
public:
  virtual std::unique_ptr<JSExecutor> createJSExecutor(
    std::shared_ptr<ExecutorDelegate> delegate,
    std::shared_ptr<MessageQueueThread> jsQueue) = 0;

  virtual std::unique_ptr<JSExecutor> createJSExecutor(
    std::shared_ptr<ExecutorDelegate> delegate,
    std::shared_ptr<MessageQueueThread> jsQueue,
    std::shared_ptr<JSEConfigParams> /*jseConfigParams*/) {
      return createJSExecutor(std::move(delegate),std::move(jsQueue));
  }

  virtual ~JSExecutorFactory() {}
};

class RN_EXPORT JSExecutor {
public:
  /**
   * Execute an application script bundle in the JS context.
   */
  virtual void loadApplicationScript(std::unique_ptr<const JSBigString> script,
                                     uint64_t scriptVersion, // TODO(OSS Candidate ISS#2710739)
                                     std::string sourceURL,
                                     std::string&& bytecodeFileName) = 0; // TODO(OSS Candidate ISS#2710739)

  /**
   * Add an application "RAM" bundle registry
   */
  virtual void setBundleRegistry(std::unique_ptr<RAMBundleRegistry> bundleRegistry) = 0;

  /**
   * Register a file path for an additional "RAM" bundle
   */
  virtual void registerBundle(uint32_t bundleId, const std::string& bundlePath) = 0;

  /**
   * Executes BatchedBridge.callFunctionReturnFlushedQueue with the module ID,
   * method ID and optional additional arguments in JS. The executor is responsible
   * for using Bridge->callNativeModules to invoke any necessary native modules methods.
   */
  virtual void callFunction(const std::string& moduleId, const std::string& methodId, const folly::dynamic& arguments) = 0;

  /**
   * Executes BatchedBridge.invokeCallbackAndReturnFlushedQueue with the cbID,
   * and optional additional arguments in JS and returns the next queue. The executor
   * is responsible for using Bridge->callNativeModules to invoke any necessary
   * native modules methods.
   */
  virtual void invokeCallback(const double callbackId, const folly::dynamic& arguments) = 0;

  virtual void setGlobalVariable(std::string propName, std::unique_ptr<const JSBigString> jsonValue) = 0;

  virtual void* getJavaScriptContext() {
    return nullptr;
  }

  /**
   * Returns whether or not the underlying executor supports debugging via the
   * Chrome remote debugging protocol.
   */
  virtual bool isInspectable() {
    return false;
  }

  /**
   * The description is displayed in the dev menu, if there is one in
   * this build.  There is a default, but if this method returns a
   * non-empty string, it will be used instead.
   */
  virtual std::string getDescription() = 0;

  virtual void handleMemoryPressure(__unused int pressureLevel) {}

  /**
   * Returns the current peak memory usage due to the JavaScript
   * execution environment in bytes. If the JavaScript execution
   * environment does not track this information, return -1.
   */
  virtual int64_t getPeakJsMemoryUsage() const noexcept { // ISS
    return -1;
  }

  virtual void destroy() {}
  virtual ~JSExecutor() {}

  virtual void flush() {}

  static std::string getSyntheticBundlePath(
      uint32_t bundleId,
      const std::string& bundlePath);
};

} }

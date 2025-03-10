#include "internal.hh"

using namespace SSC::android;

namespace SSC::android {
  Runtime::Runtime (JNIEnv* env, jobject self, String rootDirectory)
    : SSC::Core()
  {
    this->env = env;
    this->self = env->NewGlobalRef(self);
    this->pointer = reinterpret_cast<jlong>(this);
    this->rootDirectory = rootDirectory;
  }

  bool Runtime::isPermissionAllowed (const String& name) const {
    static const auto config = SSC::getUserConfig();
    const auto permission = String("permissions_allow_") + replace(name, "-", "_");

    // `true` by default
    if (!config.contains(permission)) {
      return true;
    }

    return config.at(permission) != "false";
  }

  Runtime::~Runtime () {
    this->env->DeleteGlobalRef(this->self);
  }
}

extern "C" {
  jlong external(Runtime, alloc)(
    JNIEnv *env,
    jobject self,
    jstring rootDirectory
  ) {
    auto runtime = new Runtime(env, self, StringWrap(env, rootDirectory).str());

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return 0;
    }

    return runtime->pointer;
  }

  jboolean external(Runtime, dealloc)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    delete runtime;
    return true;
  }

  jboolean external(Runtime, isDebugEnabled) (
    JNIEnv* env,
    jobject self
  ) {
    return SSC::isDebugEnabled();
  }

  jboolean external(Runtime, startEventLoop)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->runEventLoop();
    return true;
  }

  jboolean external(Runtime, stopEventLoop)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->stopEventLoop();
    return true;
  }

  jboolean external(Runtime, startTimers)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->startTimers();
    return true;
  }

  jboolean external(Runtime, stopTimers)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->stopTimers();
    return true;
  }

  jboolean external(Runtime, pause)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->pauseAllPeers();
    runtime->stopTimers();
    runtime->stopEventLoop();

    return true;
  }

  jboolean external(Runtime, resume)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->runEventLoop();
    runtime->resumeAllPeers();

    return true;
  }

  jboolean external(Runtime, isPermissionAllowed)(
    JNIEnv *env,
    jobject self,
    jstring permission
  ) {
    auto runtime = Runtime::from(env, self);
    auto name = StringWrap(env, permission).str();

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    return runtime->isPermissionAllowed(name);
  }

  jboolean external(Runtime, setIsEmulator)(
    JNIEnv *env,
    jobject self,
    jboolean value
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->isEmulator = value;
    return true;
  }
}

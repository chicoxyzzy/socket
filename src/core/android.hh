#ifndef SSC_ANDROID_H
#define SSC_ANDROID_H

#ifndef __ANDROID__
#  define __ANDROID__
#endif

#ifndef ANDROID
#  define ANDROID 1
#endif

#ifndef DEBUG
#  define DEBUG 0
#endif

#ifndef SETTINGS
#  define SETTINGS ""
#endif

#ifndef VERSION
#  define VERSION ""
#endif

#ifndef VERSION_HASH
#  define VERSION_HASH ""
#endif

// defined before includes below
#if DEBUG
#define debug(format, ...)                                                     \
  {                                                                            \
    __android_log_print(                                                       \
      ANDROID_LOG_DEBUG, __FUNCTION__, format, ##__VA_ARGS__                   \
    );                                                                         \
  }
#else
#define debug(format, ...)
#endif

/**
 * Java Native Interface
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp276
 */
#include <jni.h>

// libc
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// c++
#include <any>
#include <map>
#include <queue>
#include <semaphore>
#include <string>

// android
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

// SSC
#include "../window/options.hh"
#include "runtime-preload.hh"
#include "common.hh"
#include "core.hh"

typedef std::string NativeCoreSequence;
typedef uint64_t NativeID;
typedef NativeID NativeCallbackID;
typedef NativeID NativeCoreID;

typedef std::map<std::string, std::string> AppConfig;
typedef std::binary_semaphore BinarySemaphore; // aka `std::counting_semaphore<1>`
typedef std::map<std::string, std::string> EnvironmentVariables;
typedef std::recursive_mutex Mutex;
typedef std::lock_guard<Mutex> Lock;
typedef std::thread Thread;

template <typename T> using Queue = std::queue<T>;
template <int K> using Semaphore = std::counting_semaphore<K>;

// Forward declaration
class NativeCallbackRef;
class NativeCore;
class NativeFileSystem;
class NativeUDP;

/**
 * A file that should contain the contents `OK`. It is used during init
 * time verificatino checks.
 */
#define SSC_VITAL_CHECK_OK_FILE "__ssc_vital_check_ok_file__.txt"

/**
 * Defined by the Socket SDK preprocessor
 */
#define PACKAGE_NAME __BUNDLE_IDENTIFIER__

/**
 * Creates a named native package export for the configured bundle suitable for
 * for definition only.
 * @param name The name of the package function to export
 */
#define exports(namespace, name)                                               \
  JNIEXPORT JNICALL Java___BUNDLE_IDENTIFIER___##namespace##_##name

/**
 * Gets the `NativeCore` binding class from the JNI environment.
 */
#define GetNativeCoreBindingClass(env) env->GetObjectClass(self)

/**
 * Gets `NativeCore` instance pointer from JNI environment.
 */
#define GetNativeCoreFromEnvironment(env)                                      \
  ({                                                                           \
    auto Class = GetNativeCoreBindingClass(env);                               \
    auto pointerField = env->GetFieldID(Class, "pointer", "J");                \
    auto core = (NativeCore *) env->GetLongField(self, pointerField);          \
    if (!core) { debug("NativeCore failed to load in environment"); }          \
    (core);                                                                    \
  })

/**
 * Gets the JNI `Exception` class from environment.
 */
#define GetExceptionClass(env)                                                 \
  ({                                                                           \
    auto Exception = env->FindClass("java/lang/Exception");                    \
    (Exception);                                                               \
  })

/**
 * Calls method on `NativeCore` instance in environment.
 */
#define CallNativeCoreMethodFromEnvironment(                                   \
  env, object, method, signature, ...                                          \
)                                                                              \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    auto _value = env->CallObjectMethod(object, _id, ##__VA_ARGS__);           \
    (_value);                                                                  \
  })

/**
 * Calls `callback(id, data)` method on `NativeCore` instance in environment.
 */
#define CallNativeCoreVoidMethodFromEnvironment(                               \
  env, object, method, signature, ...                                          \
)                                                                              \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    env->CallVoidMethod(object, _id, ##__VA_ARGS__);                           \
  })


/**
 * Converts a `jstring` to an ID type
 */
#define GetNativeCoreIDFromJString(env, string)                                \
  (NativeCoreID) std::stoull(NativeString(env, string).str())

/**
 * @TODO
 */
#define EvaluateJavaScriptInEnvironment(env, object, source)                   \
  CallNativeCoreVoidMethodFromEnvironment(                                     \
    env, object, "evaluateJavascript", "(Ljava/lang/String;)V", source         \
  );

/**
 * Generic `Exception` throw helper
 */
#define Throw(env, E)                                                          \
  ({                                                                           \
    env->ThrowNew(GetExceptionClass(env), E);                                  \
    (void) 0;                                                                  \
  });

/**
 * Translate a libuv error to a message suitable for `Throw(...)`
 */
#define UVException(code) uv_strerror(code)

/**
 * Errors thrown from the JNI/NDK bindings
 */
#define AssetManagerIsNotReachableException                                    \
  "AssetManager is not reachable through binding"
#define ExceptionCheckException "ExceptionCheck"
#define JavaScriptPreloadSourceNotInitializedException                         \
  "JavaScript preload source is not initialized"
#define NativeCoreJavaVMNotInitializedException                                \
  "NativeCore JavaVM is not initialized"
#define NativeCoreNotInitializedException "NativeCore is not initialized"
#define NativeCoreRefsNotInitializedException                                  \
  "NativeCore refs are not initialized"
#define RootDirectoryIsNotReachableException                                   \
  "Root directory in file system is not reachable through binding"
#define UVLoopNotInitializedException "UVLoop is not initialized"

/**
 * A container for a JNI string (jstring).
 */
class NativeString {
  JNIEnv *env;
  jstring ref;
  size_t length;
  const char *string;
  jboolean needsRelease;

  public:
  /**
   * `NativeString` class constructors.
   */
  NativeString (JNIEnv *env);
  NativeString (const NativeString &copy);
  NativeString (JNIEnv *env, jstring ref);
  NativeString (JNIEnv *env, std::string string);
  NativeString (JNIEnv *env, const char *string);
  ~NativeString ();

  /**
   * @TODO
   */
  const NativeString &
  operator= (const NativeString &string) {
    *this = string;
    this->needsRelease = false;
    return *this;
  }

  /**
   * Various ways to set the internal value of a `NativeString` instance.
   */
  void Set (std::string string);
  void Set (const char *string);
  void Set (jstring ref);

  /**
   * Releases memory back to the JavaVM is needed. This is called
   * internally by the `NativeString` destructor.
   */
  void Release ();

  /**
   * Various ways to convert a `NativeString` to other string representations.
   */
  const char * c_str ();
  const std::string str ();
  const jstring jstr ();

  /**
   * Returns the computed size of internal string representation.
   */
  const size_t
  size ();
};

/**
 * A structured collection of global/local JNI references
 * for continued persistence in `NativeCore`.
 */
class NativeCoreRefs {
  JNIEnv *env;

  public:
  jobject core;

  NativeCoreRefs (JNIEnv *env) {
    this->env = env;
  }

  void Release ();
};

/**
 * A container for a callback pointer and JVM global ref
 */
class NativeCallbackRef {
  public:
  jobject ref = nullptr;
  NativeCore *core = nullptr;
  NativeCallbackID id = 0;
  std::string name;
  std::string signature;

  NativeCallbackRef () {};
  NativeCallbackRef (
    NativeCore *core,
    NativeCallbackID id,
    std::string name,
    std::string signature
  );

  ~NativeCallbackRef ();

  template <typename ...Args> void Call (Args ...args);
};

class NativeRequestContext {
  public:
  NativeCoreSequence seq = "";
  NativeCoreID id = 0;
  NativeCore *core = nullptr;
  NativeCallbackRef *callback = nullptr;

  ~NativeRequestContext () {
    if (this->callback != nullptr) {
      delete this->callback;
    }
  }

  void Send (std::string data, SSC::Post post) const;
  void Send (
    NativeCoreSequence seq,
    std::string data,
    SSC::Post post
  ) const;

  void Finalize (
    NativeCoreSequence seq,
    std::string data,
    SSC::Post post
  ) const;
  void Finalize (NativeCoreSequence seq, std::string data) const;
  void Finalize (std::string data, SSC::Post) const;
  void Finalize (std::string data) const;
  void Finalize (SSC::Post post) const;
};

class NativeFileSystem {
  NativeCore *core;
  JNIEnv *env;

  public:

  typedef std::map<std::string, std::string> Constants;

  NativeFileSystem (JNIEnv *env, NativeCore *core);

  static const Constants GetConstants () {
    return static_cast<Constants>(SSC::Core::getFSConstantsMap());
  }

  static const std::string GetEncodedConstants () {
    using SSC::encodeURIComponent;

    auto constants = GetConstants();
    std::stringstream stream;

    for (auto const &tuple : constants) {
      auto key = tuple.first;
      auto value = tuple.second;

      stream
        << encodeURIComponent(key) << "="
        << encodeURIComponent(value) << "&";
    }

    return stream.str();
  }

  void Access (
    NativeCoreSequence seq,
    std::string path,
    int mode,
    NativeCallbackID callback
  ) const;

  void Chmod (
    NativeCoreSequence seq,
    std::string path,
    int mode,
    NativeCallbackID callback
  ) const;

  void Close (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void FStat (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void Open (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string path,
    int flags,
    int mode,
    NativeCallbackID callback
  ) const;

  void Read (
    NativeCoreSequence seq,
    NativeCoreID id,
    int len,
    int offset,
    NativeCallbackID callback
  ) const;

  void Stat (
    NativeCoreSequence seq,
    std::string path,
    NativeCallbackID callback
  ) const;

  void Write (
    NativeCoreSequence seq,
    NativeCoreID id,
    char *bytes,
    int16_t size,
    int16_t offset,
    NativeCallbackID callback
  ) const;
};

/**
 * An interface for core UDP APIs
 */
class NativeUDP {
  NativeCore *core;
  JNIEnv *env;

  public:

  NativeUDP (JNIEnv *env, NativeCore *core);

  void Bind (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string address,
    int port,
    bool reuseAddr,
    NativeCallbackID callback
  ) const;

  void Close (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void Connect (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string address,
    int port,
    NativeCallbackID callback
  ) const;

  void Disconnect (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void GetPeerName (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void GetSockName (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void GetState (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void ReadStart (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void ReadStop (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void Send (
    NativeCoreSequence seq,
    NativeCoreID id,
    char *data,
    int16_t size,
    std::string address,
    int port,
    bool ephemeral,
    NativeCallbackID callback
  ) const;
};

/**
 * An extended `SSC::Core` class for Android NDK/JNI
 * imeplementation.
 */
class NativeCore : public SSC::Core {
  // special floating variable that points to `refs.core`
  jobject self;

  // JNI
  JavaVM *jvm;
  JNIEnv *env;
  int jniVersion = 0;

  // Native
  NativeCoreRefs refs;
  NativeString rootDirectory;

  // webkti webview
  std::string javaScriptPreloadSource;

  public:
  // thread
  Semaphore<64> semaphore;
  Mutex mutex;

  // window options
  SSC::WindowOptions windowOptions;

  // application
  AppConfig config;
  EnvironmentVariables environmentVariables;

  struct JNIEnvAttachment {
    JNIEnv *env = nullptr;
    JavaVM *jvm = nullptr;
    int status = 0;
    int version = 0;
    bool attached = false;

    JNIEnvAttachment () {}
    JNIEnvAttachment (NativeCore *core) {
      auto jvm = core->GetJavaVM();
      auto version = core->GetJNIVersion();
      this->Attach(jvm, version);
    }

    JNIEnvAttachment (JavaVM *jvm, int version) {
      this->Attach(jvm, version);
    }

    ~JNIEnvAttachment () {
      this->Detach();
    }

    void Attach (JavaVM *jvm, int version) {
      this->jvm = jvm;
      this->version = version;

      if (jvm != nullptr) {
        this->status = this->jvm->GetEnv((void **) &this->env, this->version);

        if (this->status == JNI_EDETACHED) {
          this->attached = this->jvm->AttachCurrentThread(&this->env, 0);
        }
      }
    }

    void Detach () {
      auto jvm = this->jvm;
      auto attached = this->attached;

      if (this->HasException()) {
        this->PrintException();
      }

      this->env = nullptr;
      this->jvm = nullptr;
      this->status = 0;
      this->attached = false;

      if (attached && jvm != nullptr) {
        jvm->DetachCurrentThread();
      }
    }

    inline bool HasException () {
      return this->env != nullptr && this->env->ExceptionCheck();
    }

    inline void PrintException () {
      if (this->env != nullptr) {
        this->env->ExceptionDescribe();
      }
    }
  };

  NativeCore (JNIEnv *env, jobject core);
  ~NativeCore ();

  jboolean ConfigureEnvironment ();
  jboolean ConfigureWebViewWindow ();

  NativeRequestContext * CreateRequestContext (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void EvaluateJavaScript (std::string js);
  void EvaluateJavaScript (const char *js);

  void * GetPointer () const;
  JavaVM * GetJavaVM () const;
  AppConfig & GetAppConfig ();

  const NativeString GetAppConfigValue (const char *key) const;
  const NativeString GetAppConfigValue (std::string key) const;
  const EnvironmentVariables & GetEnvironmentVariables () const;

  const NativeFileSystem GetNativeFileSystem () const;
  const NativeString & GetRootDirectory () const;
  const NativeCoreRefs & GetRefs () const;

  AAssetManager * GetAssetManager () const;

  const int GetJNIVersion () const;

  const std::string GetNetworkInterfaces () const;
  const char * GetJavaScriptPreloadSource () const;

  void BufferSize (
    NativeCoreSequence seq,
    NativeCoreID id,
    int size,
    int buffer,
    NativeCallbackID callback
  );

  void DNSLookup (
    NativeCoreSequence seq,
    std::string hostname,
    int family,
    NativeCallbackID callback
  ) const;

  inline void Wait () {
    this->semaphore.acquire();
  }

  inline void Signal () {
    this->semaphore.release();
  }
};
#endif

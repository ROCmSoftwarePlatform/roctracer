#ifndef SRC_CORE_LOADER_H_
#define SRC_CORE_LOADER_H_

#include <atomic>
#include <mutex>
#include <dlfcn.h>

#define LOADER_INSTANTIATE() \
  std::atomic<roctracer::HipLoader*> roctracer::HipLoader::instance_{}; \
  std::atomic<roctracer::HccLoader*> roctracer::HccLoader::instance_{}; \
  std::atomic<roctracer::KfdLoader*> roctracer::KfdLoader::instance_{}; \
  std::atomic<roctracer::RocTxLoader*> roctracer::RocTxLoader::instance_{}; \
  roctracer::Loader::mutex_t roctracer::Loader::mutex_;

namespace roctracer {

// Base runtime loader class
class Loader {
  public:
  typedef std::mutex mutex_t;

  template <class fun_t>
  fun_t* GetFun(const char* fun_name) { return (fun_t*) dlsym(handle_, fun_name); }

  protected:
  Loader(const char* lib_name) {
    handle_ = dlopen(lib_name, RTLD_LAZY|RTLD_NOLOAD);
    if (handle_ == NULL) {
      fprintf(stderr, "roctracer: Loading '%s' failed, %s\n", lib_name, dlerror());
      abort();
    }
  }

  ~Loader() {
    if (handle_ != NULL) dlclose(handle_);
  }

  protected:
  static mutex_t mutex_;

  private:
  void* handle_;
};

// HIP runtime library loader class
class HipLoader : protected Loader {
  public:
  typedef decltype(hipRegisterApiCallback) RegisterApiCallback_t;
  typedef decltype(hipRemoveApiCallback) RemoveApiCallback_t;
  typedef decltype(hipRegisterActivityCallback) RegisterActivityCallback_t;
  typedef decltype(hipRemoveActivityCallback) RemoveActivityCallback_t;
  typedef decltype(hipKernelNameRef) KernelNameRef_t;
  typedef decltype(hipApiName) ApiName_t;

  static HipLoader& Instance() {
    HipLoader* obj = instance_.load(std::memory_order_acquire);
    if (obj == NULL) {
      std::lock_guard<mutex_t> lck(mutex_);
      if (instance_.load(std::memory_order_relaxed) == NULL) {
        obj = new HipLoader();
        instance_.store(obj, std::memory_order_release);
      }
    }
    return *instance_;
  }

  RegisterApiCallback_t* RegisterApiCallback;
  RemoveApiCallback_t* RemoveApiCallback;
  RegisterActivityCallback_t* RegisterActivityCallback;
  RemoveActivityCallback_t* RemoveActivityCallback;
  KernelNameRef_t* KernelNameRef;
  ApiName_t* ApiName;

  protected:
  HipLoader() : Loader("libhip_hcc.so") {
    RegisterApiCallback = GetFun<RegisterApiCallback_t>("hipRegisterApiCallback");
    RemoveApiCallback = GetFun<RemoveApiCallback_t>("hipRemoveApiCallback");
    RegisterActivityCallback = GetFun<RegisterActivityCallback_t>("hipRegisterActivityCallback");
    RemoveActivityCallback = GetFun<RemoveActivityCallback_t>("hipRemoveActivityCallback");
    KernelNameRef = GetFun<KernelNameRef_t>("hipKernelNameRef");
    ApiName = GetFun<ApiName_t>("hipApiName");
  }

  private:
  static std::atomic<HipLoader*> instance_;
};

// HCC runtime library loader class
class HccLoader : protected Loader {
  public:
  typedef std::mutex mutex_t;

  typedef decltype(Kalmar::CLAMP::InitActivityCallback) InitActivityCallback_t;
  typedef decltype(Kalmar::CLAMP::EnableActivityCallback) EnableActivityCallback_t;
  typedef decltype(Kalmar::CLAMP::GetCmdName) GetCmdName_t;

  static HccLoader* GetRef() { return instance_; }

  static HccLoader& Instance() {
    HccLoader* obj = instance_.load(std::memory_order_acquire);
    if (obj == NULL) {
      std::lock_guard<mutex_t> lck(mutex_);
      if (instance_.load(std::memory_order_relaxed) == NULL) {
        obj = new HccLoader();
        instance_.store(obj, std::memory_order_release);
      }
    }
    return *obj;
  }

  InitActivityCallback_t* InitActivityCallback;
  EnableActivityCallback_t* EnableActivityCallback;
  GetCmdName_t* GetCmdName;

  protected:
  HccLoader() : Loader("libmcwamp_hsa.so") {
    // Kalmar::CLAMP::InitActivityCallback
    InitActivityCallback = GetFun<InitActivityCallback_t>("InitActivityCallbackImpl");
    // Kalmar::CLAMP::EnableActivityIdCallback
    EnableActivityCallback = GetFun<EnableActivityCallback_t>("EnableActivityCallbackImpl");
    // Kalmar::CLAMP::GetCmdName
    GetCmdName = GetFun<GetCmdName_t>("GetCmdNameImpl");
  }

  private:
  static std::atomic<HccLoader*> instance_;
};

// KFD runtime library loader class
class KfdLoader : protected Loader {
  public:
  typedef bool (RegisterApiCallback_t)(uint32_t op, void* callback, void* arg);
  typedef bool (RemoveApiCallback_t)(uint32_t op);

  static KfdLoader& Instance() {
    KfdLoader* obj = instance_.load(std::memory_order_acquire);
    if (obj == NULL) {
      std::lock_guard<mutex_t> lck(mutex_);
      if (instance_.load(std::memory_order_relaxed) == NULL) {
        obj = new KfdLoader();
        instance_.store(obj, std::memory_order_release);
      }
    }
    return *instance_;
  }

  RegisterApiCallback_t* RegisterApiCallback;
  RemoveApiCallback_t* RemoveApiCallback;

  protected:
  KfdLoader() : Loader("libkfdwrapper64.so") {
    RegisterApiCallback = GetFun<RegisterApiCallback_t>("RegisterApiCallback");
    RemoveApiCallback = GetFun<RemoveApiCallback_t>("RemoveApiCallback");
  }

  private:
  static std::atomic<KfdLoader*> instance_;
};

// rocTX runtime library loader class
class RocTxLoader : protected Loader {
  public:
  typedef bool (RegisterApiCallback_t)(uint32_t op, void* callback, void* arg);
  typedef bool (RemoveApiCallback_t)(uint32_t op);

  static RocTxLoader& Instance() {
    RocTxLoader* obj = instance_.load(std::memory_order_acquire);
    if (obj == NULL) {
      std::lock_guard<mutex_t> lck(mutex_);
      if (instance_.load(std::memory_order_relaxed) == NULL) {
        obj = new RocTxLoader();
        instance_.store(obj, std::memory_order_release);
      }
    }
    return *instance_;
  }

  RegisterApiCallback_t* RegisterApiCallback;
  RemoveApiCallback_t* RemoveApiCallback;

  protected:
  RocTxLoader() : Loader("libroctx64.so") {
    RegisterApiCallback = GetFun<RegisterApiCallback_t>("RegisterApiCallback");
    RemoveApiCallback = GetFun<RemoveApiCallback_t>("RemoveApiCallback");
  }

  private:
  static std::atomic<RocTxLoader*> instance_;
};

} // namespace roctracer

#endif // SRC_CORE_LOADER_H_

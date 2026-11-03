#include <RED4ext/NativeTypes.hpp>

#ifdef __APPLE__
#include <RED4ext/ResourceLoader.hpp>

// LTBF R41: the game's LoadResRef<T> (hash 1157708450) has no standalone macOS copy (R41b section 3). It builds a default loading
// request for the path and issues it; the macOS SDK's ResourceLoader::LoadAsync<T>(path) does the same through the mapped
// IssueLoadingRequestByPath (Mac 0x21B7130). LTBF's calls are asynchronous (sync = false), which is the default request.
template <typename T>
RED4ext::ResourceToken<T> *LoadResRef(RED4ext::ResourcePath *aPath, RED4ext::SharedPtr<RED4ext::ResourceToken<T>> *aWrapper,
                                      bool aSync) {
  (void)aSync;
  auto loader = RED4ext::ResourceLoader::Get();
  if (!loader || !aPath || !aWrapper)
    return nullptr;
  *aWrapper = loader->template LoadAsync<T>(*aPath);
  return aWrapper->instance;
}
#else
template <typename T>
RED4ext::UniversalRelocFunc<RED4ext::ResourceToken<T> *(*)(
    RED4ext::ResourcePath *, 
    RED4ext::SharedPtr<RED4ext::ResourceToken<T>> *wrapper, 
    bool sync)>
        LoadResRef(1157708450);
#endif

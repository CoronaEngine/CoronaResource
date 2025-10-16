#pragma once

// 迁移版 ResourceManager：从 Src/Core/IO 移至 Utility/ResourceManager
// 对外暴露统一头 <ResourceManager.h>

#include <cabbage_concurrent/concurrent.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "IResource.h"
#include "IResourceLoader.h"
#include "ResourceTypes.h"

namespace Corona {
class ResourceManager {
   public:
    using LoadCallback = std::function<void(const ResourceId&, std::shared_ptr<IResource>)>;  // nullptr 表示失败

    ResourceManager();
    ~ResourceManager();

    void register_loader(std::shared_ptr<IResourceLoader> loader);
    void unregister_loader(std::shared_ptr<IResourceLoader> loader);

    std::shared_ptr<IResource> load(const ResourceId& id);

    template <typename T>
    std::shared_ptr<T> load_typed(const ResourceId& id) {
        auto r = load(id);
        return std::static_pointer_cast<T>(r);
    }

    std::future<std::shared_ptr<IResource>> load_async(const ResourceId& id);
    void load_async(const ResourceId& id, LoadCallback cb);

    std::shared_ptr<IResource> load_once(const ResourceId& id);
    std::future<std::shared_ptr<IResource>> load_once_async(const ResourceId& id);
    void load_once_async(const ResourceId& id, LoadCallback cb);

    void preload(const std::vector<ResourceId>& ids);
    void wait();

    void clear();
    bool contains(const ResourceId& id) const;

   private:
    std::shared_ptr<IResource> load_internal(const ResourceId& id);
    std::shared_ptr<IResourceLoader> find_loader(const ResourceId& id);
    void schedule_task(std::function<void()> task);

   private:
    Cabbage::Concurrent::ConcurrentHashMap<ResourceId, std::shared_ptr<IResource>, ResourceIdHash> cache_;
    Cabbage::Concurrent::ConcurrentHashMap<ResourceId, std::shared_ptr<std::mutex>, ResourceIdHash> locks_;
    mutable std::shared_mutex loaders_mutex_;
    std::vector<std::shared_ptr<IResourceLoader>> loaders_;
    Cabbage::Concurrent::ThreadPool task_pool_;
    std::atomic<std::size_t> pending_tasks_{0};
    mutable std::mutex wait_mutex_;
    std::condition_variable wait_cv_;
};
}  // namespace Corona

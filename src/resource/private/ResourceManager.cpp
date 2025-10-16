#include <ResourceManager.h>
#include <corona_logger.h>

#include <algorithm>
#include <exception>
#include <mutex>


namespace Corona {
ResourceManager::ResourceManager() = default;
ResourceManager::~ResourceManager() {
    wait();
    task_pool_.shutdown();
}

void ResourceManager::schedule_task(std::function<void()> task) {
    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
    try {
        task_pool_.submit_detached([this, task = std::move(task)]() mutable {
            try {
                if (task) {
                    task();
                }
            } catch (const std::exception& e) {
                CE_LOG_ERROR("ResourceManager async task threw exception: {}", e.what());
            } catch (...) {
                CE_LOG_ERROR("ResourceManager async task threw unknown exception");
            }

            if (pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::lock_guard lk(wait_mutex_);
                wait_cv_.notify_all();
            }
        });
    } catch (...) {
        pending_tasks_.fetch_sub(1, std::memory_order_acq_rel);
        throw;
    }
}

void ResourceManager::register_loader(std::shared_ptr<IResourceLoader> loader) {
    if (!loader) {
        return;
    }
    std::unique_lock lk(loaders_mutex_);
    loaders_.push_back(std::move(loader));
}

void ResourceManager::unregister_loader(std::shared_ptr<IResourceLoader> loader) {
    std::unique_lock lk(loaders_mutex_);
    loaders_.erase(std::remove(loaders_.begin(), loaders_.end(), loader), loaders_.end());
}

std::shared_ptr<IResource> ResourceManager::load_internal(const ResourceId& id) {
    if (auto cached = cache_.find(id)) {
        return *cached;
    }

    std::shared_ptr<std::mutex> mtx;
    if (auto existingLock = locks_.find(id)) {
        mtx = *existingLock;
    } else {
        auto created = std::make_shared<std::mutex>();
        if (locks_.insert(id, created)) {
            mtx = std::move(created);
        } else {
            auto retryLock = locks_.find(id);
            mtx = retryLock ? *retryLock : created;
        }
    }

    std::scoped_lock lk(*mtx);
    if (auto cachedAfterLock = cache_.find(id)) {
        return *cachedAfterLock;
    }

    auto loader = find_loader(id);
    if (!loader) {
        CE_LOG_ERROR("No loader for type='{}' path='{}'", id.type, id.path);
        return nullptr;
    }

    auto res = loader->load(id);
    if (!res) {
        CE_LOG_ERROR("Load failed for type='{}' path='{}'", id.type, id.path);
        return nullptr;
    }

    cache_.insert(id, res);
    return res;
}

std::shared_ptr<IResourceLoader> ResourceManager::find_loader(const ResourceId& id) {
    std::shared_lock lk(loaders_mutex_);
    for (auto& l : loaders_) {
        if (l && l->supports(id)) {
            return l;
        }
    }
    return nullptr;
}

std::shared_ptr<IResource> ResourceManager::load(const ResourceId& id) {
    return load_internal(id);
}

std::shared_ptr<IResource> ResourceManager::load_once(const ResourceId& id) {
    auto loader = find_loader(id);
    if (!loader) {
        CE_LOG_ERROR("No loader for type='{}' path='{}' (load_once)", id.type, id.path);
        return nullptr;
    }
    auto res = loader->load(id);
    if (!res) {
        CE_LOG_ERROR("load_once failed for type='{}' path='{}'", id.type, id.path);
    }
    return res;
}

std::future<std::shared_ptr<IResource>> ResourceManager::load_async(const ResourceId& id) {
    auto prom = std::make_shared<std::promise<std::shared_ptr<IResource>>>();
    auto fut = prom->get_future();
    schedule_task([this, id, prom] {
        prom->set_value(this->load_internal(id));
    });
    return fut;
}

std::future<std::shared_ptr<IResource>> ResourceManager::load_once_async(const ResourceId& id) {
    auto prom = std::make_shared<std::promise<std::shared_ptr<IResource>>>();
    auto fut = prom->get_future();
    schedule_task([this, id, prom] {
        prom->set_value(this->load_once(id));
    });
    return fut;
}

void ResourceManager::load_async(const ResourceId& id, LoadCallback cb) {
    schedule_task([this, id, cb] {
        auto res = load_internal(id);
        if (cb) {
            cb(id, res);
        }
    });
}

void ResourceManager::load_once_async(const ResourceId& id, LoadCallback cb) {
    schedule_task([this, id, cb] {
        auto res = load_once(id);
        if (cb) {
            cb(id, res);
        }
    });
}

void ResourceManager::preload(const std::vector<ResourceId>& ids) {
    for (const auto& rid : ids) {
        schedule_task([this, rid]() {
            (void)load_internal(rid);
        });
    }
}

void ResourceManager::wait() {
    std::unique_lock lk(wait_mutex_);
    wait_cv_.wait(lk, [this]() { return pending_tasks_.load(std::memory_order_acquire) == 0; });
}

void ResourceManager::clear() {
    cache_.clear();
    locks_.clear();
}

bool ResourceManager::contains(const ResourceId& id) const {
    return cache_.find(id).has_value();
}
}  // namespace Corona

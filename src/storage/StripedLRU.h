#ifndef AFINA_STORAGE_STRIPED_LRU_H
#define AFINA_STORAGE_STRIPED_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <afina/Storage.h>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class StripedLRU : public Afina::Storage {
public: 

	StripedLRU(size_t stripe_count, size_t max_shard_size);
	
   	~StripedLRU() {};
    
    static std::unique_ptr<StripedLRU> BuildStripedLRU(size_t memory_limit, size_t stripe_count);

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    std::vector <SimpleLRU> _shard;
    std::vector <std::mutex> _mtx_shard;
    std::hash <std::string> _hash;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_LRU_H

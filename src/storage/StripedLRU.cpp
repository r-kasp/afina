#include "StripedLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {


std::unique_ptr<StripedLRU> StripedLRU::BuildStripedLRU(size_t memory_limit, size_t stripe_count) {
 	size_t stripe_limit = memory_limit / stripe_count;
 	if (stripe_limit < 1UL * 1024 * 1024)
 	{
 		throw std::runtime_error("Wrong parametres\n");
 	}
 	return std::unique_ptr<StripedLRU>(new StripedLRU(stripe_limit, stripe_count));
}


StripedLRU::StripedLRU(size_t stripe_count, size_t max_shard_size) : _mtx_shard(stripe_count)
{	
	for (size_t i = 0; i < stripe_count; i++)
	{
		_shard.emplace_back(max_shard_size);
	}
}

// See MapBasedGlobalLockImpl.h
bool StripedLRU::Put(const std::string &key, const std::string &value) 
{
	int ind = (int)(_hash(key) % _shard.size());
	std::unique_lock<std::mutex> _lock(_mtx_shard[ind]);
	return _shard[ind].Put(key, value);
}

// See MapBasedGlobalLockImpl.h
bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
	size_t ind = _hash(key) % _shard.size();
	std::unique_lock<std::mutex> _lock(_mtx_shard[ind]);
	return _shard[ind].PutIfAbsent(key, value);
}


// See MapBasedGlobalLockImpl.h
bool StripedLRU::Set(const std::string &key, const std::string &value)
{
	size_t ind = _hash(key) % _shard.size();
	std::unique_lock<std::mutex> _lock(_mtx_shard[ind]);
	return _shard[ind].Set(key, value);
}


// See MapBasedGlobalLockImpl.h
bool StripedLRU::Delete(const std::string &key) 
{ 
	size_t ind = _hash(key) % _shard.size();
	std::unique_lock<std::mutex> _lock(_mtx_shard[ind]);
	return _shard[ind].Delete(key);
}

// See MapBasedGlobalLockImpl.h
bool StripedLRU::Get(const std::string &key, std::string &value) 
{
	size_t ind = _hash(key) % _shard.size();
	std::unique_lock<std::mutex> _lock(_mtx_shard[ind]);
	return _shard[ind].Get(key, value);
}

} // namespace Backend
} // namespace Afina

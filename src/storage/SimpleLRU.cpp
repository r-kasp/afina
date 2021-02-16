#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) 
{
	std::reference_wrapper<const std::string> adress_key = key;
	//element in cache now
	if (_lru_index.find(adress_key) != _lru_index.end())
	{
		Delete(key);
		PutIfAbsent(key, value);
	} 
	else
	{
		while (_lru_index.size() > 0 && _cur_size + key.size() + value.size() > _max_size)
		{
			DeleteTail();
		}
		PutIfAbsent(key, value);
	}
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
	lru_node *buf = new lru_node(key, value, nullptr);
	std::reference_wrapper<lru_node> adress_buf = *buf;
	std::reference_wrapper<const std::string> adress_key = buf->key;
	if (_lru_index.find(adress_key) != _lru_index.end())
	{
		delete buf;
		return false;
	}
	if (_lru_head == nullptr)
	{
		buf->next = std::move(_lru_head);
		_lru_head = std::unique_ptr<lru_node>(buf);
		_lru_index.insert({adress_key, adress_buf});
		_lru_tail = buf;
	}
	else
	{
		_lru_head->prev = buf;
		buf->next = std::move(_lru_head);
		_lru_head.reset(buf);
		_lru_index.insert({adress_key, adress_buf});
	}
	_cur_size += key.size() + value.size();
	return true;	
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
	std::reference_wrapper<const std::string> adress_key = key;
	if (_lru_index.find(adress_key) == _lru_index.end())
		return false;
	_lru_index.find(adress_key)->second.get().value = value;
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) 
{ 
	std::reference_wrapper<const std::string> adress_key = key;
	if (_lru_index.find(adress_key) == _lru_index.end())
		return false;
	_cur_size -= (key.size() - _lru_index.find(adress_key)->second.get().value.size());
	_lru_index.erase(adress_key);
	DeleteFromList(key);
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) 
{
	std::reference_wrapper<const std::string> adress_key = key;
	if (_lru_index.find(adress_key) == _lru_index.end())
			return false;
	value = _lru_index.find(adress_key)->second.get().value;
	return true;
}


bool SimpleLRU::DeleteFromList(const std::string &key) 
{ 
	std::reference_wrapper<const std::string> adress_key = key;
	if (_lru_index.find(adress_key) == _lru_index.end())
		return false;
	lru_node *node = &(_lru_index.find(adress_key)->second.get());
	if (node->prev == nullptr)
	{
		if (node->next == nullptr)
		{
			_lru_head.reset();
			_lru_tail = nullptr;
		}
		else
		{
			node->next->prev = nullptr;
			_lru_head = std::move(std::unique_ptr<lru_node>(node));
		}
	}
	else
	{
		if (node == _lru_tail)
		{
			node->prev->next = nullptr;
			_lru_tail = node->prev;
		}
		else
		{
			node->prev->next = std::move(node->next);
			node->next->prev = node->prev;
		}
	}
	return true;
}


bool SimpleLRU::DeleteTail()
{
	if (_lru_tail == nullptr)
		return false;
	_cur_size -= (_lru_tail->key.size() + _lru_index.find(_lru_tail->key)->second.get().value.size());
	_lru_index.erase(_lru_tail->key);
	if (_lru_tail->prev != nullptr)
	{
		_lru_tail = _lru_tail->prev;
		_lru_tail->next.reset();
	}
	return true;
}

} // namespace Backend
} // namespace Afina

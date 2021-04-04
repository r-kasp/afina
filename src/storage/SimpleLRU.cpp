#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) 
{
	if (key.size()+value.size() > _max_size) 
		return false;
	auto it = _lru_index.find(key);	
	if (it != _lru_index.end())
	{
		MoveToHead(&(it->second.get()));
		_cur_size -= it->second.get().value.size();
		reserve_mem(value.size());
		_cur_size += value.size();
		it->second.get().value = value;
	} 
	else
	{
		_PutIfAbsent(key, value);
	}
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
	if (key.size() + value.size() > _max_size)
		return false;
	auto it = _lru_index.find(key);
	if (it != _lru_index.end())
	{
		return false;
	}
	reserve_mem(key.size() + value.size());
	_cur_size += key.size() + value.size();
	lru_node *buf = new lru_node(key, value, nullptr);
	std::reference_wrapper<lru_node> adress_buf = *buf;
	std::reference_wrapper<const std::string> adress_key = buf->key;
	AddToHead(buf);
	_lru_index.insert({adress_key, adress_buf});
	return true;	
}


bool SimpleLRU::_PutIfAbsent(const std::string &key, const std::string &value)
{
	if (key.size() + value.size() > _max_size)
		return false;
	reserve_mem(key.size() + value.size());
	_cur_size += key.size() + value.size();
	lru_node *buf = new lru_node(key, value, nullptr);
	std::reference_wrapper<lru_node> adress_buf = *buf;
	std::reference_wrapper<const std::string> adress_key = buf->key;
	AddToHead(buf);
	_lru_index.insert({adress_key, adress_buf});
	return true;	
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
	if (key.size() + value.size() > _max_size)
		return false;
	auto it = _lru_index.find(key);
	if (it == _lru_index.end())
		return false;
	_cur_size -= it->second.get().value.size();
	MoveToHead(&(it->second.get()));
	reserve_mem(value.size());
	_cur_size += value.size();
	it->second.get().value = value;
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) 
{ 
	auto it = _lru_index.find(key);
	std::reference_wrapper<const std::string> adress_key = key;
	if (it == _lru_index.end())
		return false;
	_cur_size -= (key.size() - it->second.get().value.size());
	lru_node *node = &it->second.get();
	_lru_index.erase(key);
	if (node->prev == nullptr)
	{
		if (node->next == nullptr)
		{
			_lru_head = nullptr;
			_lru_tail = nullptr;
		}
		else
		{
			node->next->prev = nullptr;
			_lru_head = std::move(node->next);
		}
	}
	else
	{	
		if (node == _lru_tail)
		{
			_lru_tail = node->prev;
			_lru_tail->next = nullptr;
		}
		else
		{
			node->prev->next = std::move(node->next);
			node->next->prev = node->prev;
		}
	}
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) 
{
	auto it = _lru_index.find(key);
	if (it == _lru_index.end())
		return false;
	value = it->second.get().value;
	MoveToHead(&(it->second.get()));
	return true;
}


bool SimpleLRU::DeleteTail()
{
	if (_lru_tail == nullptr)
		return false;
	_cur_size -= (_lru_tail->key.size() + _lru_tail->value.size());
	_lru_index.erase(_lru_tail->key);
	if (_lru_tail->prev != nullptr)
	{
		_lru_tail = _lru_tail->prev;
		_lru_tail->next.reset();
	}
	return true;
}

void SimpleLRU::AddToHead(lru_node *node)
{
	if (_lru_head == nullptr)
	{
		node->next = nullptr;
		_lru_head = std::unique_ptr<lru_node>(node);
		_lru_tail = node;
	}
	else
	{
		_lru_head->prev = node;
		node->next = std::move(_lru_head);
		_lru_head.reset(node);
	}
}

void SimpleLRU::MoveToHead(lru_node *node)
{
	if (node->prev != nullptr)
	{
		if (node == _lru_tail)
		{
			_lru_tail = node->prev;
			std::unique_ptr<lru_node> buf = std::move(node->prev->next);
			node->prev->next = std::move(nullptr);
			_lru_head->prev = node;
			node->next = std::move(_lru_head);
			buf->prev = nullptr;
			_lru_head = std::move(buf);			
		}
		else
		{
			std::unique_ptr<lru_node> buf = std::move(node->prev->next);
			node->next->prev = node->prev;
			node->prev->next = std::move(node->next);
			_lru_head->prev = node;
			node->next = std::move(_lru_head);
			buf->prev = nullptr;
			_lru_head = std::move(buf);
		}
	}
}

void SimpleLRU::reserve_mem(int delta)
{
	while (_lru_index.size() > 0 && _cur_size + delta > _max_size)
	{
		DeleteTail();
	}
}

} // namespace Backend
} // namespace Afina

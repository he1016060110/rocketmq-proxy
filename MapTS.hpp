#ifndef ROCKETMQ_PROXY_MAPTS_H
#define ROCKETMQ_PROXY_MAPTS_H

#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

template<class Key, class T>
class MapTS
{
private:
    std::map<Key, T> the_map;
    mutable boost::mutex the_mutex;
    boost::condition_variable the_condition_variable;
public:
    void insert(const Key &inputKey, const T &inputValue) {
        boost::mutex::scoped_lock lock(the_mutex);
        the_map.insert(std::pair<Key, T>(inputKey, inputValue));
        lock.unlock();
        the_condition_variable.notify_all();
    }

    void insert_or_update(const Key &inputKey, const T &inputValue)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        typename std::map<Key, T>::iterator it;
        it = the_map.find(inputKey);
        if(the_map.end() == it) {
            the_map.insert(std::pair<Key, T>(inputKey, inputValue));
        } else {
            the_map[inputKey] = inputValue;
        }

        lock.unlock();
        the_condition_variable.notify_all();
    }

    bool empty() const {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_map.empty();
    }

    bool try_get(const Key &inputKey, T &outputValue) {
        boost::mutex::scoped_lock lock(the_mutex);

        typename std::map<Key, T>::iterator it;
        it = the_map.find(inputKey);

        if(the_map.end() == it) {
            return false;
        }

        outputValue = it->second;
        return true;
    }

    void wait_and_get(const Key &inputKey, T &outputValue) {
        boost::mutex::scoped_lock lock(the_mutex);

        typename std::map<Key, T>::iterator it;

        while(the_map.end() == (it = the_map.find(inputKey))) {
            the_condition_variable.wait(lock);
        }

        outputValue = it->second;
    }

    void wait_next_insert() {
        boost::mutex::scoped_lock lock(the_mutex);
        the_condition_variable.wait(lock);
    }

    void erase(const Key &inputKey) {
        boost::mutex::scoped_lock lock(the_mutex);
        the_map.erase(inputKey);
    }


    size_t size() const {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_map.size();
    }

    void getAllKeys(std::vector<Key> &keys)
    {
      keys.empty();
      keys.reserve(the_map.size());
      boost::mutex::scoped_lock lock(the_mutex);
      for(auto const& imap: the_map)
        keys.push_back(imap.first);
    }
};

#endif //ROCKETMQ_PROXY_MAPTS_H


/**
 * lru_replacer.h
 *
 * Functionality: The buffer pool manager must maintain a LRU list to collect
 * all the pages that are unpinned and ready to be swapped. The simplest way to
 * implement LRU is a FIFO queue, but remember to dequeue or enqueue pages when
 * a page changes from unpinned to pinned, or vice-versa.
 */

#pragma once

#include "buffer/replacer.h"
#include "hash/extendible_hash.h"
#include <mutex>
#include <map>
using namespace std;
namespace scudb {

    template <typename T> class LRUReplacer : public Replacer<T> {
    public:
        // do not change public interface
        LRUReplacer();

        ~LRUReplacer();

        void Insert(const T &value);

        bool Victim(T &value);

        bool Erase(const T &value);

        size_t Size();

    private:
        // add your member variables here
        struct Node{
            Node() = default;
            T data;
            explicit Node(T data, Node *p = nullptr) : data(data), pre(p) {}
            shared_ptr<Node> pre;
            shared_ptr<Node> next;
        };
        shared_ptr<Node> head;
        shared_ptr<Node> tail;
        map<T, shared_ptr<Node>> list;
        std::mutex mutex;
        size_t size;
    };

} // namespace scudb
#include <list>

#include "hash/extendible_hash.h"
#include "page/page.h"
using namespace std;
namespace scudb {
/*
 *  Bucket Class
 */
    template <typename K, typename V>
    Bucket<K, V>::Bucket(int LocalDepth) {
        this->localDepth = LocalDepth;
    }

    template <typename K, typename V>
    int Bucket<K, V>::GetLocalDepth() {
        return this->localDepth;
    }

    template <typename K, typename V>
    map<K,V> Bucket<K,V>::GetValues() {
        return this->values;
    }

    template <typename K, typename V>
    bool Bucket<K,V>::find(const K &key, V &value) {
        if(values.find(key) != values.end()){
            value = values[key];
            return true;
        }
        return false;
    }

    template <typename K, typename V>
    bool Bucket<K,V>::remove(const K &key) {
        if(values.find(key) == values.end()){
            return false;
        }
        values.erase(key);
        return true;
    }

    template <typename K, typename V>
    size_t Bucket<K,V>::GetValuesSize() {
        return this->values.size();
    }

    template <typename K, typename V>
    void Bucket<K,V>::insert(const K &key, const V &value){
        if(values.find(key) != values.end()){
            return;
        }
        values[key] = value;
    };

    template <typename K, typename V>
    void Bucket<K,V>::insert(pair<K, V> pair) {
        if(values.find(pair.first) != values.end()){
            return;
        }
        values.insert(pair);
    }

/*
 * constructor
 * array_size: fixed array size for each bucket
 */
    template <typename K, typename V>
    ExtendibleHash<K, V>::ExtendibleHash() {
        this->globalDepth = 0;
        this->bucketSize = 0;
        this->bucketTable.push_back(make_shared<Bucket<K,V>>(0));
    }

    template <typename K, typename V>
    ExtendibleHash<K, V>::ExtendibleHash(size_t size) {
        this->globalDepth = 0;
        this->bucketSize = size;
        this->bucketTable.push_back(make_shared<Bucket<K,V>>(0));
    }

/*
 * helper function to calculate the hashing address of input key
 */
    template <typename K, typename V>
    size_t ExtendibleHash<K, V>::HashKey(const K &key) {
        return hash<K>{}(key);
    }

/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
    template <typename K, typename V>
    int ExtendibleHash<K, V>::GetGlobalDepth() const {
        return this->globalDepth;
    }

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
    template <typename K, typename V>
    int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {
        return bucketTable[bucket_id]->GetLocalDepth();
    }

/*
 * helper function to return current number of bucket in hash table
 */
    template <typename K, typename V>
    int ExtendibleHash<K, V>::GetNumBuckets() const {
        return this->bucketNums;
    }

/*
 * lookup function to find value associate with input key
 */
    template <typename K, typename V>
    bool ExtendibleHash<K, V>::Find(const K &key, V &value) {
        lock_guard<std::mutex> guard(mutex);
        int bucket_id = GetTableIndex(key);
        if(bucketTable[bucket_id] == nullptr){
            return false;
        }
        return bucketTable[bucket_id]->find(key, value);
    }

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
    template <typename K, typename V>
    bool ExtendibleHash<K, V>::Remove(const K &key) {
        lock_guard<std::mutex> guard(mutex);
        int bucket_id = GetTableIndex(key);
        if(bucketTable[bucket_id] == nullptr){
            return false;
        }
        return bucketTable[bucket_id]->remove(key);
    }

/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * 当出现溢流时，拆分并重新分配桶，必要时增加
 * global depth
 */
    template <typename K, typename V>
    void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
        lock_guard<std::mutex> guard(mutex);
        int bucket_id = GetTableIndex(key);
        shared_ptr<Bucket<K,V>> TargetBucket = bucketTable[bucket_id];
        while(TargetBucket->GetValuesSize() == this->bucketSize) {
            //
            if (TargetBucket->GetLocalDepth() == this->globalDepth) {
                size_t length = bucketTable.size();
                for (size_t i = 0; i < length; i++) {
                    bucketTable.push_back(bucketTable[i]);
                }
                globalDepth++;
            }
            int mask = 1 << TargetBucket->GetLocalDepth();
            shared_ptr<Bucket<K, V>> ZeroBucket = make_shared<Bucket<K, V>>(TargetBucket->GetLocalDepth() + 1);
            shared_ptr<Bucket<K, V>> OneBucket = make_shared<Bucket<K, V>>(TargetBucket->GetLocalDepth() + 1);
            //
            for (auto pair: TargetBucket->GetValues()) {
                size_t hashkey = HashKey(pair.first);
                if (hashkey & mask) {
                    OneBucket->insert(pair);
                } else {
                    ZeroBucket->insert(pair);
                }
            }

            for (size_t i = 0; i < bucketTable.size(); i++) {
                if (bucketTable[i] == TargetBucket) {
                    if (i & mask) {
                        bucketTable[i] = OneBucket;
                    } else {
                        bucketTable[i] = ZeroBucket;
                    }
                }
            }

            bucket_id = GetTableIndex(key);
            TargetBucket = bucketTable[bucket_id];
        }
        TargetBucket->insert(key, value);
    }

    template <typename K, typename V>
    int ExtendibleHash<K, V>::GetTableIndex(const K &key) {
        return HashKey(key) & ((1 << this->globalDepth) - 1);
    }

    template class ExtendibleHash<page_id_t, Page *>;
    template class ExtendibleHash<Page *, list<Page *>::iterator>;
// test purpose
    template class ExtendibleHash<int, string>;
    template class ExtendibleHash<int, list<int>::iterator>;
    template class ExtendibleHash<int, int>;
} // namespace scudb
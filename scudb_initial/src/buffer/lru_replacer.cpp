/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"
using namespace std;

namespace scudb {

    template <typename T> LRUReplacer<T>::LRUReplacer() {this->size = 0;}

    template <typename T> LRUReplacer<T>::~LRUReplacer() {}

/*
 * Insert value into LRU 
 * 在LRU中插入值
 */
    template <typename T> void LRUReplacer<T>::Insert(const T &value) {
        Erase(value);
        mutex.lock();
        shared_ptr<Node> InsertNode = make_shared<Node>(value);
        InsertNode->next = head;

        if (head != nullptr) {
            head->pre = InsertNode;
        }
        head = InsertNode;
        if (tail == nullptr) {
            tail = InsertNode;
        }
        list[InsertNode->data] = InsertNode;
        size++;
        mutex.unlock();
    }

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 * 如果LRU非空，则将头成员从LRU弹出到参数“value”，并返回true。
 * 如果LRU为空，则返回false
 */
    template <typename T> bool LRUReplacer<T>::Victim(T &value) {
        mutex.lock();
//        如果LRU为空，则返回false
        if(size == 0){
            mutex.unlock();
            return false;
        }
//        如果LRU非空，则将头成员从LRU弹出到参数“value”，并返回true。
//      special  Situation
        if(head == tail){
            value = head->data;
            head = nullptr;
            tail = nullptr;
            list.erase(value);
            size--;
            mutex.unlock();
            return true;
        }
        //
        value = tail->data;
        shared_ptr<Node> Node = tail;
        Node->pre->next = nullptr;
        tail = Node->pre;
        Node->pre = nullptr;
        list.erase(value);
        size--;
        mutex.unlock();
        return true;
    }

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 * 从LRU中删除值。
 * 如果删除成功，则返回true，否则返回false
 */
    template <typename T> bool LRUReplacer<T>::Erase(const T &value) {
        mutex.lock();
        shared_ptr<Node> aim = nullptr;
        if(list.find(value) == list.end()){
            mutex.unlock();
            return false;
        }
        aim = list.find(value)->second;
        //
        if(aim == head && aim == tail){
            head = nullptr;
            tail = nullptr;
        }
        if(aim == head){
            aim->next->pre = nullptr;
            head = aim->next;
        }
        if(aim == tail){
            aim->pre->next = nullptr;
            tail = aim->pre;
        }else{
            aim->pre->next = aim->next;
            aim->next->pre = aim->pre;
        }
        aim->pre = nullptr;
        aim->next = nullptr;
        list.erase(value);
        size--;
        mutex.unlock();
        return true;
    }

    template <typename T> size_t LRUReplacer<T>::Size() { return size; }

    template class LRUReplacer<Page *>;
// test only
    template class LRUReplacer<int>;

} // namespace scudb
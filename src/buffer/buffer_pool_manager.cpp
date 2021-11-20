#include "buffer/buffer_pool_manager.h"
using namespace std;
namespace scudb {

/*
 * BufferPoolManager Constructor
 * When log_manager is nullptr, logging is disabled (for test purpose)
 * WARNING: Do Not Edit This Function
 */
    BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, LogManager *log_manager)
            : pool_size_(pool_size), disk_manager_(disk_manager),
              log_manager_(log_manager) {
        // a consecutive memory space for buffer pool
        pages_ = new Page[pool_size_];
        page_table_ = new ExtendibleHash<page_id_t, Page *>(BUCKET_SIZE);
        replacer_ = new LRUReplacer<Page *>;
        free_list_ = new std::list<Page *>;

        // put all the pages into free list
        for (size_t i = 0; i < pool_size_; ++i) {
            free_list_->push_back(&pages_[i]);
        }
    }

/*
 * BufferPoolManager Deconstructor
 * WARNING: Do Not Edit This Function
 */
    BufferPoolManager::~BufferPoolManager() {
        delete[] pages_;
        delete page_table_;
        delete replacer_;
        delete free_list_;
    }

/**
 * 1. search hash table.
 *  1.1 if exist, pin the page and return immediately
 *  1.2 if no exist, find a replacement entry from either free list or lru
 *      replacer. (NOTE: always find from free list first)
 * 2. If the entry chosen for replacement is dirty, write it back to disk.
 * 3. Delete the entry for the old page from the hash table and insert an
 * entry for the new page.
 * 4. Update page metadata, read page content from disk file and return page
 * pointer
 */
// 获取页面
    Page *BufferPoolManager::FetchPage(page_id_t page_id) {
        assert(page_id != INVALID_PAGE_ID);
        lock_guard<std::mutex> guard(latch_);
        Page *aimPage = nullptr;
        if(page_table_->Find(page_id, aimPage)){
            aimPage->pin_count_++;
            replacer_->Erase(aimPage);
            return aimPage;
        }else{
            if(!free_list_->empty()){
                aimPage = free_list_->front();
                free_list_->pop_front();
            }else{
                if(!replacer_->Victim(aimPage)){
                    return nullptr;
                }
            }
        }
        assert(aimPage->pin_count_ == 0);
        if(aimPage->is_dirty_){
            disk_manager_->WritePage(aimPage->page_id_, aimPage->GetData());
        }
        page_table_->Remove(aimPage->page_id_);
        page_table_->Insert(page_id, aimPage);
        aimPage->page_id_ = page_id;
        aimPage->is_dirty_ = false;
        aimPage->pin_count_ = 1;
        disk_manager_->ReadPage(page_id, aimPage->GetData());
        return aimPage;
    }

/*
 * Implementation of unpin page
 * if pin_count>0, decrement it and if it becomes zero, put it back to
 * replacer if pin_count<=0 before this call, return false. is_dirty: set the
 * dirty flag of this page
 */
//    unpin页面的实现
    bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
        lock_guard<std::mutex> guard(latch_);

        Page *aimPage = nullptr;
        if(!page_table_->Find(page_id, aimPage)){
            return false;
        }
        if(aimPage->pin_count_ > 0){

            if(--aimPage->pin_count_ == 0){
                replacer_->Insert(aimPage);
            }
        }else{
            return false;
        }
        if(is_dirty){
            aimPage->is_dirty_ = true;
        }
        return true;
    }

/*
 * Used to flush a particular page of the buffer pool to disk. Should call the
 * write_page method of the disk manager
 * if page is not found in page table, return false
 * NOTE: make sure page_id != INVALID_PAGE_ID
 */
//    将缓冲池的特定页刷新到磁盘
    bool BufferPoolManager::FlushPage(page_id_t page_id) {
        lock_guard<std::mutex> guard(latch_);

//        make sure page_id != INVALID_PAGE_ID
        if(page_id == INVALID_PAGE_ID){
            return false;
        }

        Page *aimPage = nullptr;
        if(page_table_->Find(page_id, aimPage)){
            disk_manager_->WritePage(page_id, aimPage->GetData());
            return true;
        }
        return false;
    }

/**
 * User should call this method for deleting a page. This routine will call
 * disk manager to deallocate the page. First, if page is found within page
 * table, buffer pool manager should be reponsible for removing this entry out
 * of page table, reseting page metadata and adding back to free list. Second,
 * call disk manager's DeallocatePage() method to delete from disk file. If
 * the page is found within page table, but pin_count != 0, return false
 */
//    删除页面
//    首先，如果在页面中找到页面表中，缓冲池管理器应负责删除此项重新设置页面元数据并添加回自由列表。
//    第二调用磁盘管理器的DeallocatePage（）方法从磁盘文件中删除。如果该页在页表中找到，但pin_count！=0，返回false
    bool BufferPoolManager::DeletePage(page_id_t page_id) {
        std::lock_guard<std::mutex> guard(latch_);

        Page *aimPage = nullptr;
        if(page_table_->Find(page_id, aimPage)){
            page_table_->Remove(page_id);
            aimPage->page_id_ = INVALID_PAGE_ID;
            aimPage->is_dirty_ = false;
            replacer_->Erase(aimPage);
            disk_manager_->DeallocatePage(page_id);
            free_list_->push_back(aimPage);
            return true;
        }
        return false;
    }

/**
 * User should call this method if needs to create a new page. This routine
 * will call disk manager to allocate a page.
 * Buffer pool manager should be responsible to choose a victim page either
 * from free list or lru replacer(NOTE: always choose from free list first),
 * update new page's metadata, zero out memory and add corresponding entry
 * into page table. return nullptr if all the pages in pool are pinned
 */
//    如果需要创建新页面，用户应调用此方法。此例程将调用磁盘管理器来分配页面。
//    缓冲池管理器应负责从空闲列表或lru替换器中选择受害者页面
//    （注意：始终首先从空闲列表中选择），更新新页面的元数据，清空内存，并将相应条目添加到页面表中。如果池中的所有页面都已固定，则返回nullptr
//    创建新页面
    Page *BufferPoolManager::NewPage(page_id_t &page_id) {
        std::lock_guard<std::mutex> guard(latch_);

        Page *newPage = nullptr;
        if(!free_list_->empty()){
            newPage = free_list_->front();
            free_list_->pop_front();
        }else{
            if(!replacer_->Victim(newPage)){
                return nullptr;
            }
        }
        page_id = disk_manager_->AllocatePage();
        if(newPage->is_dirty_){
            disk_manager_->WritePage(newPage->page_id_, newPage->GetData());
        }
        page_table_->Remove(newPage->page_id_);
        page_table_->Insert(page_id, newPage);
        newPage->page_id_ = page_id;
        newPage->is_dirty_ = false;
        newPage->pin_count_ = 1;
        newPage->ResetMemory();
        return newPage;
    }
} // namespace scudb
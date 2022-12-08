#ifndef S_LRU_FILELIST_H
#define S_LRU_FILELIST_H

#include <memory>

#include <wrench-dev.h>

class LRU_FileList {

public:
    /**
     * @brief Touch a file to update its last access time
     * 
     * @param file 
     */
    void touchFile(wrench::DataFile  *file) {
        // If the file is new, then it's easy
        if (this->indexed_files.find(file) == this->indexed_files.end()) {
            this->indexed_files[file] = 0;
            this->lru_list.insert(this->lru_list.begin(), file);
            return;
        }

        // if the file is not new, then we need to do a bit of work
        auto current_index = indexed_files[file];
        this->lru_list.erase(this->lru_list.begin() + current_index);
        this->indexed_files[file] = 0;
        this->lru_list.insert(this->lru_list.begin(), file);
    }

    /**
     * @brief Identify the file touched last from file collection, 
     * which shall be evicted according to LRU policy
     * 
     * @return std::shared_ptr<wrench::DataFile> 
     */
    std::shared_ptr<wrench::DataFile> removeLRUFile() {
        auto file = this->lru_list.back();
        this->lru_list.pop_back();
        this->indexed_files.erase(file);
        return wrench::Simulation::getFileByID(file->getID());
    }

    /**
     * @brief Checks whether a file is in the LRU list
     * @param file : a data file
     * @return true if the file is there, false otherwise
     */
    bool hasFile(std::shared_ptr<wrench::DataFile> file) {
        return (this->indexed_files.find(file.get()) != this->indexed_files.end());
    }


private:
    // File collection mapped to index
    std::map<wrench::DataFile *, ssize_t> indexed_files;
    // Ordered list of files in file collection -- front is most recently used.
    std::vector<wrench::DataFile *> lru_list;

};

#endif //S_LRU_FILELIST_H

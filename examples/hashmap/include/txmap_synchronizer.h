#pragma once

#include <runnable.h>
#include <iostream>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <unordered_map>

extern "C" {
        #include "fuzzy_log.h"
}

typedef struct txmap_record {
        uint64_t key;
        uint64_t value;
        uint64_t version;  
} txmap_record;

typedef struct txmap_set {
        uint32_t num_entry;
        txmap_record* set;
        ~txmap_set() { delete set; }
} txmap_set;


// Synchronizer class which eagerly synchronize with fuzzylog 
class TXMapSynchronizer : public Runnable {
private:
        std::queue<std::pair<std::condition_variable*, std::atomic_bool*>>      m_pending_queue;
        std::queue<std::pair<std::condition_variable*, std::atomic_bool*>>      m_current_queue;
        std::mutex                                                              m_queue_mtx;

        DAGHandle*                                                              m_fuzzylog_client;
        struct colors*                                                          m_interesting_colors;
        char                                                                    m_read_buf[DELOS_MAX_DATA_SIZE];

        std::atomic_bool                                                        m_running;
        std::unordered_map<uint64_t, uint64_t>                                  m_local_map;    // Hack: let's store version into value 
        std::mutex                                                              m_local_map_mtx;

        bool                                                                    m_replication;
        
public:
        TXMapSynchronizer(std::vector<std::string>* log_addr, std::vector<ColorID>& interesting_colors, bool replication);
        ~TXMapSynchronizer() {}
        virtual void run();
        virtual void join();
        static void* bootstrap(void *arg);
        void Execute(); 

        void enqueue_get(std::condition_variable* cv, std::atomic_bool* cv_spurious_wake_up);
        void swap_queue();
        std::mutex* get_local_map_lock();

        uint64_t get(uint64_t key);
        void put(uint64_t key, uint64_t value);

        // Txn validation
        void deserialize_commit_record(char *buf, size_t size, txmap_set *rset, txmap_set *wset);
        void validate_txn(txmap_set *rset, txmap_set *wset);
        uint64_t get_latest_key_version(uint64_t key);
        void update_map(txmap_set *wset);
};

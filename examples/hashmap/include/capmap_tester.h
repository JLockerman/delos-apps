#pragma once

#include <pthread.h>
#include <capmap_workload.h>
#include <tester.h>
#include <iostream>

// CAPMapTester class which should be unaware of AtomicMap 
class CAPMapTester : public Tester {
public:
        Context*                        m_context;
        CAPMap*                         m_map;  // FIXME: hacky way for calling fuzzylog api...
        Txn**                           m_txns;
        uint32_t                        m_num_txns;
        bool                            m_async;
        uint32_t                        m_window_size;
        uint32_t                        m_txn_rate;
        std::atomic<bool>*              m_flag;
        // Number of executed operations
        std::atomic<uint64_t>           m_num_executed; 
        
public:
        CAPMapTester(Context* context, CAPMap* map, std::atomic<bool>* flag, Txn** txns, uint32_t num_txns, bool async, uint32_t window_size, uint32_t txn_rate): Tester() {
                this->m_context = context;
                this->m_map = map;
                this->m_flag = flag;
                this->m_txns = txns;
                this->m_num_txns = num_txns;
                this->m_async = async;
                this->m_window_size = window_size;
                this->m_txn_rate = txn_rate;
                this->m_num_executed = 0;
        }
        ~CAPMapTester();
        virtual void run();
        virtual void join();
        static void* bootstrap(void *arg);
        pthread_t* get_pthread_id();
        void ExecuteProtocol1(); 
        void ExecuteProtocol2(std::string& role); 
        uint64_t get_num_executed();
        uint32_t try_get_completed();
};

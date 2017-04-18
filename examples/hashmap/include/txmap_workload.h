#pragma once

#include <txmap.h>
#include <request.h>
#include <txmap_config.h>

class read_write_txn: public Txn {
private:
        TXMap*                                  m_map;
        uint64_t                                m_local_key_range_start;
        uint64_t                                m_local_key_range_end;
        uint64_t                                m_remote_key_range_start;
        uint64_t                                m_remote_key_range_end;
        struct colors*                          m_color;
        Context*                                m_context;
public:
        read_write_txn(TXMap* map, struct colors* color, uint64_t local_key_range_start, uint64_t local_key_range_end, uint64_t remote_key_range_start, uint64_t remote_key_range_end, Context* context) {
                this->m_map = map;
                this->m_color = color;
                this->m_local_key_range_start = local_key_range_start;
                this->m_local_key_range_end = local_key_range_end;
                this->m_remote_key_range_start = remote_key_range_start;
                this->m_remote_key_range_end = remote_key_range_end;
                this->m_context = context;
        }
        ~read_write_txn(){}
        virtual void Run();
        virtual write_id AsyncRun();
        virtual write_id AsyncRemoteRun();
        virtual write_id AsyncStronglyConsistentRun();
        virtual write_id AsyncWeaklyConsistentRun();
};

class txmap_workload_generator {
private:
        Context*                                m_context;
        TXMap*                                  m_map;
        vector<workload_config>*                m_workload;
public:
        txmap_workload_generator(Context* context, TXMap* map, vector<workload_config>* workload) {
                this->m_context = context;
                this->m_map = map;
                this->m_workload = workload;
        }
        Txn** Gen();
};
#pragma once

#include <request.h>
#include <txmap.h>
#include <txmap_config.h>

class read_write_txn: public Txn {
private:
        TXMap*                                  m_map;
        struct colors*                          m_color;
        Context*                                m_context;
        optype                                  m_op_type;
public:
        read_write_txn(TXMap* map, struct colors* color, Context* context, optype op_type) {
                this->m_map = map;
                this->m_color = color;
                this->m_context = context;
                this->m_op_type = op_type;
        }
        ~read_write_txn(){}
        virtual void Run();
        virtual write_id AsyncRun();
        virtual write_id AsyncRemoteRun();
        virtual write_id AsyncStronglyConsistentRun();
        virtual write_id AsyncWeaklyConsistentRun();
        virtual optype op_type() {
                return m_op_type;
        }
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

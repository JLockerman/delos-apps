#include <atomicmap_tester.h>
#include <cstring>
#include <cassert>

AtomicMapTester::~AtomicMapTester() {
        for (size_t i = 0; i < m_num_txns; i++) {
                ycsb_insert* txn = dynamic_cast<ycsb_insert*>(m_txns[i]);
                delete txn;
        }
        delete m_txns;
}

void AtomicMapTester::run() {
        int err;
        err = pthread_create(&m_thread, NULL, bootstrap, this);
        assert(err == 0);
}

void AtomicMapTester::join() {
        pthread_join(m_thread, NULL);
}

void* AtomicMapTester::bootstrap(void *arg) {
        AtomicMapTester *worker = static_cast<AtomicMapTester*>(arg);
        worker->Execute();
        return NULL;
}

pthread_t* AtomicMapTester::get_pthread_id() {
        return &m_thread;
}

uint32_t AtomicMapTester::try_get_completed() {
        uint32_t num_completed = 0;
        while (true) {
                write_id wid = m_map->try_wait_for_any_put();
                //std::cout << "try_wait:" << nwid.id.p1 << std::endl; 
                if (wid_equality{}(wid, WRITE_ID_NIL))
                        break; 
                m_context->inc_num_executed();
                num_completed += 1;
                // TODO: manage map for issued request                
        }
        return num_completed;
}

void AtomicMapTester::Execute() {
        uint32_t i;
        uint32_t num_pending = 0; 

        if (m_async) {
                for (i = 0; *m_flag; i++, i = i % m_num_txns) {
                        // try get completed
                        num_pending -= try_get_completed();
                        if (!is_duration_based_run() && is_all_executed()) break;
                                                
                        // issue
                        if (num_pending == m_window_size) {
                                write_id wid = m_map->wait_for_any_put();
                                if (wid_equality{}(wid, WRITE_ID_NIL)) { 
                                        std::cout << "no pending writes. terminate" << std::endl; 
                                        break;
                                }
                                num_pending -= 1;
                                m_context->inc_num_executed();
                                if (!is_duration_based_run() && is_all_executed()) break;
                        }

                        m_txns[i]->AsyncRun();
                        if (m_txns[i]->op_type() == Txn::optype::PUT) {
                                num_pending += 1;
                        }
                }
                m_map->wait_for_all_puts();
                m_context->set_finished();

                std::cout << "total executed: " << m_context->get_num_executed() << std::endl;

        } else {
                // Repeat request while flag = true
                for (i = 0; *m_flag; i++, i = i % m_num_txns) {
                        if (!is_duration_based_run() && is_all_executed()) break;
                        m_txns[i]->Run();
                }
                m_context->set_finished();

                std::cout << "total executed: " << m_context->get_num_executed() << std::endl;
        }
}

uint64_t AtomicMapTester::get_num_executed() {
        return m_context->get_num_executed();
}

bool AtomicMapTester::is_duration_based_run() {
        return m_expt_duration > 0;
}

bool AtomicMapTester::is_all_executed() {
        return m_context->get_num_executed() == m_num_txns;
}

from fabric.api import run, local, sudo
from fabric.context_managers import env, cd
import settings

def start_fuzzylog():
    sudo('supervisorctl start fuzzylog_server')

def stop_fuzzylog():
    sudo('supervisorctl stop fuzzylog_server')

def ls_test():
    run('ls')

def update_fuzzymap_binary():
    with cd('~/fuzzylog/delos-apps/examples/hashmap'):
        run('git pull')
        run('make clean')
        run('make')

def update_fuzzylog_binary(commit=None):
    with cd('~/fuzzylog/delos-rust/'):
        if commit:
                run('git reset --hard ' + commit)
        else:
                run('git pull')

    with cd('~/fuzzylog/delos-rust/examples/c_linking'): 
        run('make clean')
        run('make')

    with cd('~/fuzzylog/delos-rust/servers/tcp_server'):
        run('cargo clean') 
        run('cargo build --release')

def kill_fuzzylog():
    run('killall delos_tcp_server')

def fuzzylog_proc(port, maxthreads, index_in_group, total_num_servers_in_group, up=None, down=None):
    if up == "None":
        up = None

    if down == "None":
        down = None

    with cd('~/fuzzylog/delos-rust/servers/tcp_server'):
        cmd = 'cargo run --release %s -- -ig %s:%s' % (port, index_in_group, total_num_servers_in_group)
        if maxthreads != '-1':
            cmd += ' -w %s' % maxthreads
        if up:
            cmd += ' -up %s' % up
        if down:
            cmd += ' -dwn %s' % down
        run(cmd)

def clean_fuzzymap():
    with cd('~/fuzzylog/delos-apps/examples/hashmap'):
        run('rm *.txt')

def atomicmap_proc(log_addr, exp_range, exp_duration, client_id, workload, async, window_size, replication):
    with cd('~/fuzzylog/delos-apps/examples/hashmap'):
        args = 'build/atomicmap '
        args += '--log_addr=' + str(log_addr) + ' '
        args += '--expt_range=' + str(exp_range) + ' '
        args += '--expt_duration=' + str(exp_duration) + ' '
        args += '--client_id=' + str(client_id) + ' '
        args += '--workload=' + str(workload) + ' '
        if async == "True":
            args += '--async '
            args += '--window_size=' + str(window_size)
        if replication == "True":
            args += ' --replication'
        run(args)

def capmap_proc(log_addr, exp_range, exp_duration, num_clients, client_id, workload, txn_rate, async, window_size, protocol, role, replication):
    with cd('~/fuzzylog/delos-apps/examples/hashmap'):
        args = 'build/capmap '
        args += '--log_addr=' + str(log_addr) + ' '
        args += '--expt_range=' + str(exp_range) + ' '
        args += '--expt_duration=' + str(exp_duration) + ' '
        args += '--num_clients=' + str(num_clients) + ' '
        args += '--client_id=' + str(client_id) + ' '
        args += '--workload=' + str(workload) + ' '
        if int(txn_rate) > 0: 
            args += '--txn_rate=' + str(txn_rate) + ' '
        if async == "True":
            args += '--async '
            args += '--window_size=' + str(window_size)
        args += ' --protocol=' + str(protocol)
        if role == "primary" or role == "secondary":
            args += ' --role=' + str(role)
        if replication == "True":
            args += ' --replication'
        run(args)

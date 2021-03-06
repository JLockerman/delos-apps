#include <or_set.h>
#include <or_set_tester.h>
#include <chrono>
#include <workload_generator.h>
#include <config.h>
#include <tuple>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <iostream>
#include <fstream>
#include <or_set_getter.h>


void write_throughput(config cfg, const std::vector<double> &results)
{
	std::ofstream result_file;
	result_file.open(std::to_string(cfg.server_id) + ".txt", std::ios::trunc | std::ios::out);
	for (auto v : results) {
		result_file << v << "\n";
	}
	result_file.close();
}

void write_gets_per_snapshot(config cfg, const std::vector<uint64_t> &results)
{
	std::ofstream result_file;
	result_file.open("gets_per_snapshot.txt", std::ios::trunc | std::ios::out);
	for (auto v : results)
		result_file << v << "\n";
	result_file.close();
}

void write_latency(config cfg, const std::vector<tester_request*> &latencies)
{
	std::ofstream latency_file;
	latency_file.open(std::to_string(cfg.server_id) + "_latency.txt", std::ios::trunc | std::ios::out);
	for (auto rq : latencies) {
		if (rq->_executed == false)
			break;
		std::chrono::duration<double, std::milli> rq_duration = rq->_end_time - rq->_start_time;
		latency_file << rq_duration.count() << "\n";
	}
	latency_file.close();
}

void write_put_output(config cfg, const std::vector<double> &results, const std::vector<tester_request*> latencies)
{
	write_throughput(cfg, results);
	write_latency(cfg, latencies);
}

void write_get_output(config cfg, const std::vector<double> &throughput, const std::vector<uint64_t> &gets_snapshot)
{
	write_throughput(cfg, throughput);
	write_gets_per_snapshot(cfg, gets_snapshot);
}

void gen_input(uint64_t range, uint64_t num_inputs, std::vector<tester_request*> &output)
{
	workload_generator gen(range);
	uint64_t i;

	std::vector<uint64_t> seen_keys;
	for (i = 0; i < num_inputs; ++i) {
		auto rq = static_cast<or_set_rq*>(malloc(sizeof(or_set_rq)));
		rq->_key = gen.gen_next();
		if (i == 0 || rand() % 2 == 0) {
			rq->_opcode = or_set::log_opcode::ADD;
			rq->_key = gen.gen_next();
			seen_keys.push_back(rq->_key);
		} else {
			rq->_opcode = or_set::log_opcode::REMOVE;
			auto idx = rand() % seen_keys.size();
			rq->_key = seen_keys[idx];
		}
		auto temp = reinterpret_cast<tester_request*>(rq);
		temp->_executed = false;
		output.push_back(temp);
	}
}

void wait_signal(config cfg)
{
	size_t buf_sz;
	ColorSpec c;
	auto num_received = 0;

	uint64_t sig_color[1];
	sig_color[0] = cfg.num_clients + 1;
	c.local_chain = sig_color[0];
	c.remote_chains = sig_color;
	c.num_remote_chains = 1;
	buf_sz = 1;

	/* Server ips for handle. */
	ServerSpec servers;
	size_t num_servers = cfg.log_addr.size();
	assert(num_servers > 0);
	const char *server_ips[num_servers];
	for (auto i = 0; i < num_servers; ++i) {
		server_ips[i] = cfg.log_addr[i].c_str();
	}
	servers.head_ips = (char **)&*server_ips;
	servers.tail_ips = NULL;
	servers.num_ips = num_servers;

	auto handle = new_fuzzylog_instance(servers, c, NULL);
	fuzzylog_append(handle, NULL, 0, &c, 1);
	while (num_received < cfg.num_clients) {
		fuzzylog_sync(handle, [](void *num_received, const char *, size_t) {
			*(int *)num_received += 1;
		}, &num_received);
	}
	fuzzylog_close(handle);
}

void run_putter(config cfg, std::vector<tester_request*> &inputs, std::vector<double> &throughput_samples)
{
	/* Colors for DAG Handle */
	ColorSpec c;
	c.num_remote_chains = cfg.num_clients;
	c.remote_chains = new uint64_t[cfg.num_clients];
	for (auto i = 0; i < cfg.num_clients; ++i)
		c.remote_chains[i] = (uint8_t)(i+1);
	c.local_chain = (uint8_t)(cfg.server_id + 1);

	/* Local appends */
	// struct colors local_c;
	// local_c.numcolors = 1;
	// local_c.mycolors = new ColorID[1];
	// local_c.mycolors[0] = (uint8_t)(cfg.server_id + 1);


	/* For playing back remote appends */
	// std::set<uint8_t> remote_color_set;
	// for (auto i = 0; i < cfg.num_clients; ++i)
	// 	if (i != cfg.server_id)
	// 		remote_color_set.insert(i+1);

	// struct colors remote_colors;
	// remote_colors.numcolors = cfg.num_clients-1;
	// remote_colors.mycolors = new ColorID[cfg.num_clients-1];
	// auto i = 0;
	// std::cerr << "Remote colors:\n";
	// for (auto c : remote_color_set) {
	// 	remote_colors.mycolors[i] = c;
	// 	std::cerr << (uint64_t)c << "\n";
	// 	i += 1;
	// }

	/* Server ips for handle. */
	ServerSpec servers;
	servers.num_ips = cfg.log_addr.size();
	assert(servers.num_ips > 0);
	char *server_ips[servers.num_ips];
	std::cerr << "Num servers: " << servers.num_ips << "\n";
	std::cerr << "Server list:\n";
	for (auto i = 0; i < servers.num_ips; ++i) {
		server_ips[i] = (char *)cfg.log_addr[i].c_str();
		std::cerr << server_ips[i] << "\n";
	}
	servers.head_ips = server_ips;
	servers.tail_ips = NULL;

	auto handle = new_fuzzylog_instance(servers, c, NULL);
	gen_input(cfg.expt_range, cfg.num_rqs, inputs);
	wait_signal(cfg);

	auto orset = new or_set(handle, c, cfg.server_id, cfg.sync_duration);
	auto tester = new or_set_tester(cfg.window_sz, orset, handle);

	std::cerr << "Worker " << (uint64_t)cfg.server_id << " initialized!\n";

	void do_run_fix_throughput(const std::vector<tester_request*> &requests, std::vector<double> &samples,
				 int interval,
				 int duration,
				 double low_throughput,
				 double high_throughput,
				 double spike_start,
	 			 	double spike_duration);

	tester->do_run_fix_throughput(inputs, throughput_samples, cfg.sample_interval, cfg.expt_duration, cfg.low_throughput, cfg.high_throughput, cfg.spike_start, cfg.spike_duration);
	fuzzylog_close(handle);
}

void run_getter(config cfg, std::vector<double> &throughput_samples, std::vector<uint64_t> &gets_per_snapshot)
{
	assert(cfg.writer == false);

	/* Colors for DAG Handle */
	ColorSpec c;
	c.num_remote_chains = cfg.num_clients;
	c.remote_chains = new uint64_t[cfg.num_clients];
	std::cerr << "Num colors: " << cfg.num_clients << "\n";
	for (auto i = 0; i < cfg.num_clients; ++i) {
		c.remote_chains[i] = (uint8_t)(i+1);
		std::cerr << "Color " << c.remote_chains[i] << "\n";
	}

	/* Server ips for handle. */
	ServerSpec servers;
	servers.num_ips = cfg.log_addr.size();
	assert(servers.num_ips > 0);
	char *server_ips[servers.num_ips];
	std::cerr << "Num servers: " << servers.num_ips << "\n";
	std::cerr << "Server list:\n";
	for (auto i = 0; i < servers.num_ips; ++i) {
		server_ips[i] = (char *)cfg.log_addr[i].c_str();
		std::cerr << server_ips[i] << "\n";
	}
	servers.head_ips = server_ips;
	servers.tail_ips = NULL;

	auto handle = new_fuzzylog_instance(servers, c, NULL);

	//wait_signal(cfg);

	auto orset = new or_set(handle, c, cfg.server_id, cfg.sync_duration);
	auto getter = new or_set_getter(orset);

	std::cerr << "Getter initialized!\n";
	getter->run(throughput_samples, gets_per_snapshot, cfg.sample_interval, cfg.expt_duration);
	fuzzylog_close(handle);

}

void validate_puts(config cfg, const std::vector<tester_request*> &inputs, const std::vector<double> throughput_samples)
{
	auto input_count = 0;
	for (auto t : inputs) {
		if (t->_executed == false)
			break;
		input_count += 1;
	}

	auto throughput_count = 0.0;
	for (auto sample : throughput_samples) {
		throughput_count += sample;
	}

	std::cerr << "Input count: " << input_count << "\n";
	std::cerr << "Throughput count: " << throughput_count << "\n";
}


void do_put_experiment(config cfg)
{
	std::vector<tester_request*> inputs;
	std::vector<double> throughput_samples;
	run_putter(cfg, inputs, throughput_samples);
	validate_puts(cfg, inputs, throughput_samples);
	write_put_output(cfg, throughput_samples, inputs);
}

void do_get_experiment(config cfg)
{
	std::vector<double> throughput_samples;
	std::vector<uint64_t> gets_per_snapshot;
	run_getter(cfg, throughput_samples, gets_per_snapshot);
	write_get_output(cfg, throughput_samples, gets_per_snapshot);
}

int main(int argc, char **argv)
{
	std::vector<uint64_t> results;
	config_parser cfg_prser;
	config cfg = cfg_prser.get_config(argc, argv);

	if (cfg.writer == true)
		do_put_experiment(cfg);
	else
		do_get_experiment(cfg);
	return 0;
}

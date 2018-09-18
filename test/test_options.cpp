//
// Created by ringo on 17.09.18.
//

#include "test_options.hpp"
int debug,
	kdebugflag,
	holdoff,
	idle_time_limit,
	maxconnect,
	log_to_fd,
	maxfail,
	connect_delay,
	req_unit,
	child_wait,
	privileged_option,
	option_priority,
	phase;
bool nodetach,
	updetach,
	persist,
	holdoff_specified,
	log_default,
	tune_kernel,
	demand,
	dump_options,
	dryrun,
	devnam_fixed,
	multilink,
	master_detach;
char *progname,
	*bundle_name,
	req_ifname[32],
	hostname[64],
	*option_source,
	linkname[4096];
struct protent *protocols[] = {nullptr};
struct userenv *userenv_list;
using namespace rng0;
typedef int((rng0::Options::*m_func))(const char *, rng0::Option *, int);
struct channel *the_channel;
char *current_option;

int main()
{
	the_channel = new channel;
	progname = new char[1024];
	bundle_name = new char[1024];
	current_option = new char[512];
	option_source = new char[512];
	auto opts = Options::get_instance();
	opts->fill_options();
	char *qwe = new char[512];
	sprintf(qwe, "jaja=qwerty");
	auto opt = opts->find_option("set");
	opt->process(qwe, &qwe);

	return 0;
}
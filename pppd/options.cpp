//
// Created by ringo on 13.09.18.
//

#include "options.hpp"

using namespace rng0;

static int user_setenv(char **);
static int user_unsetenv(char **);
static void user_unsetprint(option_t *, printer_func, void *);
static void user_setprint(option_t *opt, printer_func printer, void *arg);
static int showversion(char **);
static int showhelp(char **);
static int setlogfile(char **);
static int setdomain(char **argv);

/*
 * option_error - print a message about an error in an option.
 * The message is logged, and also sent to
 * stderr if phase == PHASE_INITIALIZE.
 */
void
option_error __V((const char *fmt, ...))
{
	va_list args;
	char buf[1024];

#if defined(__STDC__)
	va_start(args, fmt);
#else
	char *fmt;
    va_start(args);
    fmt = va_arg(args, char *);
#endif
	vslprintf(buf, sizeof(buf), (char *)fmt, args);
	va_end(args);
	if (phase == PHASE_INITIALIZE)
		fprintf(stderr, "%s: %s\n", progname, buf);
	syslog(LOG_ERR, "%s", buf);
}


/*
 * Structure to store extra lists of options.
 */
struct option_list {
	option_t *options;
	struct option_list *next;
};

static struct option_list *extra_options = nullptr;

/* fd opened for log file */
static int logfile_fd = -1;
/* name of log file */
static char logfile_name[MAXPATHLEN];

/* metric of the default route to set over the PPP link */
constexpr int	dfl_route_metric = -1;
static const char *usage_string =
"pppd version %s\n\
Usage: %s [ options ], where options are:\n\
	<device>	Communicate over the named device\n\
	<speed>		Set the baud rate to <speed>\n\
	<loc>:<rem>	Set the local and/or remote interface IP\n\
			addresses.  Either one may be omitted.\n\
	asyncmap <n>	Set the desired async map to hex <n>\n\
	auth		Require authentication from peer\n\
        connect <p>     Invoke shell command <p> to set up the serial line\n\
	crtscts		Use hardware RTS/CTS flow control\n\
	defaultroute	Add default route through interface\n\
	file <f>	Take options from file <f>\n\
	modem		Use modem control lines\n\
	mru <n>		Set MRU value to <n> for negotiation\n\
See pppd(8) for more options.\n\
";

/*
 * Valid arguments.
 */
static option_t general_options[] = {
		{"debug", o_int, (char *)&debug,
		 "Increase debugging level", OPT_INC | OPT_NOARG | 1},
		{"-d", o_int, &debug,
		 "Increase debugging level",
		 OPT_ALIAS | OPT_INC | OPT_NOARG | 1},

		{"kdebug", o_int, &kdebugflag,
		 "Set kernel driver debug level", OPT_PRIO},

		{"nodetach", o_bool, &nodetach,
		 "Don't detach from controlling tty", OPT_PRIO | 1},
		{"-detach", o_bool, &nodetach,
		 "Don't detach from controlling tty", OPT_ALIAS | OPT_PRIOSUB | 1},

#ifdef SYSTEMD
{ "up_sdnotify", o_bool, &up_sdnotify,
	  "Notify systemd once link is up (implies nodetach)",
	  OPT_PRIOSUB | OPT_A2COPY | 1, &nodetach },
#endif
		{"updetach", o_bool, &updetach,
		 "Detach from controlling tty once link is up",
		 OPT_PRIOSUB | OPT_A2CLR | 1, &nodetach},

		{"master_detach", o_bool, &master_detach,
		 "Detach when we're multilink master but have no link", 1},

		{"holdoff", o_int, &holdoff,
		 "Set time in seconds before retrying connection",
		 OPT_PRIO, &holdoff_specified},

		{"idle", o_int, &idle_time_limit,
		 "Set time in seconds before disconnecting idle link", OPT_PRIO},

		{"maxconnect", o_int, &maxconnect,
		 "Set connection time limit",
		 OPT_PRIO | OPT_LLIMIT | OPT_NOINCR | OPT_ZEROINF},

		{"domain", o_special, (void *) setdomain,
		 "Add given domain name to hostname",
		 OPT_PRIO | OPT_PRIV | OPT_A2STRVAL, Options::get_instance()->get_domain()},

		{"file", o_special, (void *) Options::readfile,
		 "Take options from a file", OPT_NOPRINT},
		{"call", o_special, (void *) Options::callfile,
		 "Take options from a privileged file", OPT_NOPRINT},

		{"persist", o_bool, &persist,
		 "Keep on reopening connection after close", OPT_PRIO | 1},
		{"nopersist", o_bool, &persist,
		 "Turn off persist option", OPT_PRIOSUB},

		{"demand", o_bool, &demand,
		 "Dial on demand", OPT_INITONLY | 1, &persist},

		{"--version", o_special_noarg, (void *) showversion,
		 "Show version number"},
		{"--help", o_special_noarg, (void *) showhelp,
		 "Show brief listing of options"},
		{"-h", o_special_noarg, (void *) showhelp,
		 "Show brief listing of options", OPT_ALIAS},

		{"logfile", o_special, (void *) setlogfile,
		 "Append log messages to this file",
		 OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, &logfile_name},
		{"logfd", o_int, &log_to_fd,
		 "Send log messages to this file descriptor",
		 OPT_PRIOSUB | OPT_A2CLR, &log_default},
		{"nolog", o_int, &log_to_fd,
		 "Don't send log messages to any file",
		 OPT_PRIOSUB | OPT_NOARG | OPT_VAL(-1)},
		{"nologfd", o_int, &log_to_fd,
		 "Don't send log messages to any file descriptor",
		 OPT_PRIOSUB | OPT_ALIAS | OPT_NOARG | OPT_VAL(-1)},

		{"linkname", o_string, linkname,
		 "Set logical name for link",
		 OPT_PRIO | OPT_PRIV | OPT_STATIC, nullptr, MAXPATHLEN},

		{"maxfail", o_int, &maxfail,
		 "Maximum number of unsuccessful connection attempts to allow",
		 OPT_PRIO},

		{"ktune", o_bool, &tune_kernel,
		 "Alter kernel settings as necessary", OPT_PRIO | 1},
		{"noktune", o_bool, &tune_kernel,
		 "Don't alter kernel settings", OPT_PRIOSUB},

		{"connect-delay", o_int, &connect_delay,
		 "Maximum time (in ms) to wait after connect script finishes",
		 OPT_PRIO},

		{"unit", o_int, &req_unit,
		 "PPP interface unit number to use if possible",
		 OPT_PRIO | OPT_LLIMIT, 0, 0},

		{"ifname", o_string, req_ifname,
		 "Set PPP interface name",
		 OPT_PRIO | OPT_PRIV | OPT_STATIC, nullptr, MAXIFNAMELEN},

		{"dump", o_bool, &dump_options,
		 "Print out option values after parsing all options", 1},
		{"dryrun", o_bool, &dryrun,
		 "Stop after parsing, printing, and checking options", 1},

		{"child-timeout", o_int, &child_wait,
		 "Number of seconds to wait for child processes at exit",
		 OPT_PRIO},

		{"set", o_special, (void *) user_setenv,
		 "Set user environment variable",
		 OPT_A2PRINTER | OPT_NOPRINT, (void *) user_setprint},
		{"unset", o_special, (void *) user_unsetenv,
		 "Unset user environment variable",
		 OPT_A2PRINTER | OPT_NOPRINT, (void *) user_unsetprint},

		{"defaultroute-metric", o_int, (void *)&dfl_route_metric,
		 "Metric to use for the default route (Linux only; -1 for default behavior)",
		 OPT_PRIV | OPT_LLIMIT | OPT_INITONLY, nullptr, 0, -1},

#ifdef HAVE_MULTILINK
		{"multilink", o_bool, &multilink,
		 "Enable multilink operation", OPT_PRIO | 1},
		{"mp", o_bool, &multilink,
		 "Enable multilink operation", OPT_PRIOSUB | OPT_ALIAS | 1},
		{"nomultilink", o_bool, &multilink,
		 "Disable multilink operation", OPT_PRIOSUB | 0},
		{"nomp", o_bool, &multilink,
		 "Disable multilink operation", OPT_PRIOSUB | OPT_ALIAS | 0},

		{"bundle", o_string, &bundle_name,
		 "Bundle name for multilink", OPT_PRIO},
#endif /* HAVE_MULTILINK */

#ifdef PLUGIN
{ "plugin", o_special, (void *)loadplugin,
	  "Load a plug-in module into pppd", OPT_PRIV | OPT_A2LIST },
#endif

#ifdef PPP_FILTER
{ "pass-filter", o_special, setpassfilter,
	  "set filter for packets to pass", OPT_PRIO },

	{ "active-filter", o_special, setactivefilter,
	  "set filter for active pkts", OPT_PRIO },
#endif

#ifdef MAXOCTETS
{ "maxoctets", o_int, &maxoctets,
	  "Set connection traffic limit",
	  OPT_PRIO | OPT_LLIMIT | OPT_NOINCR | OPT_ZEROINF },
	{ "mo", o_int, &maxoctets,
	  "Set connection traffic limit",
	  OPT_ALIAS | OPT_PRIO | OPT_LLIMIT | OPT_NOINCR | OPT_ZEROINF },
	{ "mo-direction", o_special, setmodir,
	  "Set direction for limit traffic (sum,in,out,max)" },
	{ "mo-timeout", o_int, &maxoctets_timeout,
	  "Check for traffic limit every N seconds", OPT_PRIO | OPT_LLIMIT | 1 },
#endif

		{nullptr}
};

Options::Options()
{
//	this->fill_options();
}

void Options::fill_options()
{
	option_t general_options[] = {
		{"debug", o_int, (void *)&debug,
		 "Increase debugging level", OPT_INC | OPT_NOARG | 1},

		{"-d", o_int, &debug,
		 "Increase debugging level",
		 OPT_ALIAS | OPT_INC | OPT_NOARG | 1},

		{"kdebug", o_int, &kdebugflag,
		 "Set kernel driver debug level", OPT_PRIO},

		{"nodetach", o_bool, &nodetach,
		 "Don't detach from controlling tty", OPT_PRIO | 1},
		{"-detach", o_bool, &nodetach,
		 "Don't detach from controlling tty", OPT_ALIAS | OPT_PRIOSUB | 1},

		{"updetach", o_bool, (void *)&updetach,
		 "Detach from controlling tty once link is up",
		 OPT_PRIOSUB | OPT_A2CLR | 1, (void *)&nodetach},

		{"master_detach", o_bool, &master_detach,
		 "Detach when we're multilink master but have no link", 1},

		{"holdoff", o_int, &holdoff,
		 "Set time in seconds before retrying connection",
		 OPT_PRIO, &holdoff_specified},

		{"idle", o_int, &idle_time_limit,
		 "Set time in seconds before disconnecting idle link", OPT_PRIO},

		{"maxconnect", o_int, &maxconnect,
		 "Set connection time limit",
		 OPT_PRIO | OPT_LLIMIT | OPT_NOINCR | OPT_ZEROINF},

		{"domain", o_special, (void *) setdomain,
		 "Add given domain name to hostname",
		 OPT_PRIO | OPT_PRIV | OPT_A2STRVAL, Options::get_instance()->get_domain()},

		{"file", o_special, (void *) Options::readfile,
		 "Take options from a file", OPT_NOPRINT},
		{"call", o_special, (void *) Options::callfile,
		 "Take options from a privileged file", OPT_NOPRINT},

		{"persist", o_bool, &persist,
		 "Keep on reopening connection after close", OPT_PRIO | 1},
		{"nopersist", o_bool, &persist,
		 "Turn off persist option", OPT_PRIOSUB},

		{"demand", o_bool, &demand,
		 "Dial on demand", OPT_INITONLY | 1, &persist},

		{"--version", o_special_noarg, (void *) showversion,
		 "Show version number"},
		{"--help", o_special_noarg, (void *) showhelp,
		 "Show brief listing of options"},
		{"-h", o_special_noarg, (void *) showhelp,
		 "Show brief listing of options", OPT_ALIAS},

		{"logfile", o_special, (void *) setlogfile,
		 "Append log messages to this file",
		 OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, &logfile_name},
		{"logfd", o_int, &log_to_fd,
		 "Send log messages to this file descriptor",
		 OPT_PRIOSUB | OPT_A2CLR, &log_default},
		{"nolog", o_int, &log_to_fd,
		 "Don't send log messages to any file",
		 OPT_PRIOSUB | OPT_NOARG | OPT_VAL(-1)},
		{"nologfd", o_int, &log_to_fd,
		 "Don't send log messages to any file descriptor",
		 OPT_PRIOSUB | OPT_ALIAS | OPT_NOARG | OPT_VAL(-1)},

		{"linkname", o_string, linkname,
		 "Set logical name for link",
		 OPT_PRIO | OPT_PRIV | OPT_STATIC, nullptr, MAXPATHLEN},

		{"maxfail", o_int, &maxfail,
		 "Maximum number of unsuccessful connection attempts to allow",
		 OPT_PRIO},

		{"ktune", o_bool, &tune_kernel,
		 "Alter kernel settings as necessary", OPT_PRIO | 1},
		{"noktune", o_bool, &tune_kernel,
		 "Don't alter kernel settings", OPT_PRIOSUB},

		{"connect-delay", o_int, &connect_delay,
		 "Maximum time (in ms) to wait after connect script finishes",
		 OPT_PRIO},

		{"unit", o_int, &req_unit,
		 "PPP interface unit number to use if possible",
		 OPT_PRIO | OPT_LLIMIT, 0, 0},

		{"ifname", o_string, req_ifname,
		 "Set PPP interface name",
		 OPT_PRIO | OPT_PRIV | OPT_STATIC, nullptr, MAXIFNAMELEN},

		{"dump", o_bool, &dump_options,
		 "Print out option values after parsing all options", 1},
		{"dryrun", o_bool, &dryrun,
		 "Stop after parsing, printing, and checking options", 1},

		{"child-timeout", o_int, &child_wait,
		 "Number of seconds to wait for child processes at exit",
		 OPT_PRIO},

		{"set", o_special, (void *) user_setenv,
		 "Set user environment variable",
		 OPT_A2PRINTER | OPT_NOPRINT, (void *) user_setprint},
		{"unset", o_special, (void *) user_unsetenv,
		 "Unset user environment variable",
		 OPT_A2PRINTER | OPT_NOPRINT, (void *) user_unsetprint},

		{"defaultroute-metric", o_int, (void *)&dfl_route_metric,
		 "Metric to use for the default route (Linux only; -1 for default behavior)",
		 OPT_PRIV | OPT_LLIMIT | OPT_INITONLY, nullptr, 0, -1},
	};
	for (auto _opt : general_options) {
		this->__general_options.push_back(new Option(_opt));

	}
#ifdef SYSTEMD
	option_t systemd_opts[] = {
		{ "up_sdnotify", o_bool, &up_sdnotify,
		  "Notify systemd once link is up (implies nodetach)",
		  OPT_PRIOSUB | OPT_A2COPY | 1, &nodetach },

	};
	for (auto _opt : systemd_opts) {
		this->__general_options.push_back(new Option(_opt));

	}
#endif

#ifdef HAVE_MULTILINK
	option_t ml_options[] = {
			{"multilink",   o_bool,   &multilink,
					"Enable multilink operation",  OPT_PRIO | 1},
			{"mp",          o_bool,   &multilink,
					"Enable multilink operation",  OPT_PRIOSUB | OPT_ALIAS | 1},
			{"nomultilink", o_bool,   &multilink,
					"Disable multilink operation", OPT_PRIOSUB | 0},
			{"nomp",        o_bool,   &multilink,
					"Disable multilink operation", OPT_PRIOSUB | OPT_ALIAS | 0},

			{"bundle",      o_string, &bundle_name,
					"Bundle name for multilink",   OPT_PRIO},
	};
	for (auto _opt : ml_options) {
		this->__general_options.push_back(new Option(_opt));

	}
#endif /* HAVE_MULTILINK */

#ifdef PLUGIN
	option_t plugin_opts[] = {
		{ "plugin", o_special, (void *)loadplugin,
		  "Load a plug-in module into pppd", OPT_PRIV | OPT_A2LIST },
	};
	for (auto _opt : plugin_opts) {
		this->__general_options.push_back(new Option(_opt));

	}
#endif

#ifdef PPP_FILTER
	option_t filter_opts[] = {
		{ "pass-filter", o_special, setpassfilter,
		  "set filter for packets to pass", OPT_PRIO },

		{ "active-filter", o_special, setactivefilter,
		  "set filter for active pkts", OPT_PRIO },
	};
	for (auto _opt : filter_opts) {
		this->__general_options.push_back(new Option(_opt));

	}
#endif

#ifdef MAXOCTETS
	option_t maxoctets_opts[] = {
		{ "maxoctets", o_int, &maxoctets,
		  "Set connection traffic limit",
		  OPT_PRIO | OPT_LLIMIT | OPT_NOINCR | OPT_ZEROINF },
		{ "mo", o_int, &maxoctets,
		  "Set connection traffic limit",
		  OPT_ALIAS | OPT_PRIO | OPT_LLIMIT | OPT_NOINCR | OPT_ZEROINF },
		{ "mo-direction", o_special, setmodir,
		  "Set direction for limit traffic (sum,in,out,max)" },
		{ "mo-timeout", o_int, &maxoctets_timeout,
		  "Check for traffic limit every N seconds", OPT_PRIO | OPT_LLIMIT | 1 },

	};
	for (auto _opt : maxoctets_opts) {
		this->__general_options.push_back(new Option(_opt));

	}
#endif
}

/*
 * find_option - scan the option lists for the various protocols
 * looking for an entry with the given name.
 * This could be optimized by using a hash table.
 */
Option *Options::find_option(const char *name)
{
	option_t *opt;
	int i, dowild;

	for (dowild = 0; dowild <= 1; ++dowild) {
		for (auto _opt : this->__general_options) {
			if (this->match_option(name, _opt, dowild)) {
				return _opt;
			}
		}
		for (auto _opt : this->__auth_options) {
			if (this->match_option(name, _opt, dowild)) {
				return _opt;
			}
		}

		for (auto _opt : this->__extra_options) {
			if (this->match_option(name, _opt, dowild)) {
				return _opt;
			}
		}


		for (opt = the_channel->options; opt->name != nullptr; ++opt) {
			auto new_opt = new Option(opt);
			if (match_option(name, new_opt, dowild)) {
				return new_opt;
			} else {
				delete new_opt;
			}
		}

		for (i = 0; protocols[i] != nullptr; ++i)
			if ((opt = protocols[i]->options) != nullptr)
				for (; opt->name != nullptr; ++opt) {
					auto new_opt = new Option(opt);
					if (match_option(name, new_opt, dowild)) {
						return new_opt;
					} else {
						delete new_opt;
					}
				}
	}
	return nullptr;
}

int Options::match_option(const char *name, rng0::Option *opt, int dowild)
{
	int (*match) __P((char *, char **, int));

	if (dowild != (opt->type == o_wild))
		return 0;
	if (!dowild)
		return strcmp(name, opt->name) == 0;
	match = (int (*) __P((char *, char **, int))) opt->addr;
	return (*match)((char *)name, nullptr, 0);
}

option_t *Options::curopt = nullptr;
Options *Options::m_instance = nullptr;
Options *Options::get_instance()
{
	if (!Options::m_instance) {
		Options::m_instance = new Options();
	}
	return m_instance;
}

/*
 * options_from_file - Read a string of options from a file,
 * and interpret them.
 */
int Options::options_from_file(char *filename, int must_exist, int check_prot, int priv)
{
	FILE *f;
	int i, newline, ret, err;
	Option *opt;
	int oldpriv, n;
	char *oldsource;
	uid_t euid;
	char *argv[MAXARGS];
	char args[MAXARGS][MAXWORDLEN];
	char cmd[MAXWORDLEN];

	euid = geteuid();
	if (check_prot && seteuid(getuid()) == -1) {
		option_error((char *) "unable to drop privileges to open %s: %m", filename);
		return 0;
	}
	f = fopen(filename, "r");
	err = errno;
	if (check_prot && seteuid(euid) == -1)
		fatal((char *) "unable to regain privileges");
	if (f == nullptr) {
		errno = err;
		if (!must_exist) {
			if (err != ENOENT && err != ENOTDIR)
				warn((char *) "Warning: can't open options file %s: %m", filename);
			return 1;
		}
		option_error((char *) "Can't open options file %s: %m", filename);
		return 0;
	}

	oldpriv = privileged_option;
	privileged_option = priv;
	oldsource = option_source;
	option_source = strdup(filename);
	if (option_source == nullptr)
		option_source = (char *) "file";
	ret = 0;
	while (getword(f, cmd, &newline, filename)) {
		opt = find_option(cmd);
		if (opt == nullptr) {
			option_error((char *) "In file %s: unrecognized option '%s'",
			             filename, cmd);
			goto err;
		}
		n = opt->arguments_number();
		for (i = 0; i < n; ++i) {
			if (!getword(f, args[i], &newline, filename)) {
				option_error(
						(char *) "In file %s: too few parameters for option '%s'",
						filename, cmd);
				goto err;
			}
			argv[i] = args[i];
		}
		if (!opt->process(cmd, argv)) {
			goto err;
		}
	}
	ret = 1;

	err:
	fclose(f);
	privileged_option = oldpriv;
	option_source = oldsource;
	return ret;
}

/*
 * process_option - process one new-style option.
 */
int Options::process_option(Option *opt, char *cmd, char **argv)
{
	return opt->process(cmd, argv);
}

int Options::readfile(char **argv)
{
	return get_instance()->options_from_file(*argv, 1, 1, privileged_option);
}

int Options::callfile(char **argv)
{
	std::string path = *argv;
	if (path.find("..") != std::string::npos || path.find('/') != std::string::npos) {
		option_error("call option value may not contain .. or start with /");
		return 0;
	}
	path = _PATH_PEERFILES + path;
	return get_instance()->options_from_file(path, true, true, 1);
}

int Option::process(char *cmd, char **argv)
{
	int (*parser)(char **);
	int (*wildp)(char *, char **, int);
	const char *optopt = (this->type == o_wild)? "": " option";
	int prio = option_priority;
	auto mainopt = this;

	current_option = (char *)this->name;
	if ((this->flags & OPT_PRIVFIX) && privileged_option) {
		prio += OPRIO_ROOT;
	}
	while (mainopt->flags & OPT_PRIOSUB) {
		--mainopt;
	}
	if (mainopt->flags & OPT_PRIO) {
		if (prio < mainopt->priority) {
			/* new value doesn't override old */
			if (prio == OPRIO_CMDLINE && mainopt->priority > OPRIO_ROOT) {
				option_error("%s%s set in %s cannot be overridden\n",
				             this->name, optopt, mainopt->source);
				return 0;
			}
			return 1;
		}
		if (prio > OPRIO_ROOT && mainopt->priority == OPRIO_CMDLINE) {
			warn((char *) "%s%s from %s overrides command line",
			     this->name, optopt, option_source);
		}
	}

	if (!this->check_flags(optopt)) {
		return 0;
	}

	switch (this->type) {
		case o_bool:
			this->process_bool();
			break;

		case o_int:
			if (!this->process_int(argv)) {
				return 0;
			}
			break;

		case o_uint32:
			if (!this->process_number(argv)) {
				return 0;
			}
			break;

		case o_string:
			process_string(argv);
			break;

		case o_special_noarg:
		case o_special:
			parser = (int (*) __P((char **))) this->addr;
			Options::curopt = this;
			if (!(*parser)(argv))
				return 0;
			if (this->flags & OPT_A2LIST) {
				struct option_value *ovp, *pp;

				ovp = new option_value;
				strcpy(ovp->value, *argv);
				ovp->source = option_source;
				ovp->next = nullptr;
				if (this->addr2 == nullptr) {
					this->addr2 = ovp;
				} else {
					for (pp = static_cast<option_value *>(this->addr2); pp->next != nullptr; pp = pp->next) {
						;
					}
					pp->next = ovp;
				}
			}
			break;

		case o_wild:
			wildp = (int (*) __P((char *, char **, int))) this->addr;
			if (!(*wildp)(cmd, argv, 1))
				return 0;
			break;
	}

	/*
	 * If addr2 wasn't used by any flag (OPT_A2COPY, etc.) but is set,
	 * treat it as a bool and set/clear it based on the OPT_A2CLR bit.
	 */
	if (this->addr2 && (this->flags & (OPT_A2COPY|OPT_ENABLE
	                                 |OPT_A2PRINTER|OPT_A2STRVAL|OPT_A2LIST|OPT_A2OR)) == 0)
		*(bool *)(this->addr2) = !(this->flags & OPT_A2CLR);

	mainopt->source = option_source;
	mainopt->priority = (short) prio;
	mainopt->winner = (short)(this - mainopt);

	return 1;
}

bool Option::check_flags(const char *optopt) const
{
	bool failed = false;
	if ((flags & OPT_INITONLY) && phase != PHASE_INITIALIZE) {
		option_error("%s%s cannot be changed after initialization",
		             name, optopt);
		failed = true;
	}
	if ((flags & OPT_PRIV) && !privileged_option) {
		option_error("using the %s%s requires root privilege",
		             name, optopt);
		failed = true;
	}
	if ((flags & OPT_ENABLE) && *(bool *)(addr2) == 0) {
		option_error("%s%s is disabled", name, optopt);
		failed = true;
	}
	if ((flags & OPT_DEVEQUIV) && devnam_fixed) {
		option_error("the %s%s may not be changed in %s",
		             name, optopt, option_source);
		failed = true;
	}
	return !failed;
}

void Option::process_string(char **argv) const
{
	char *sv;
	if (flags & OPT_STATIC) {
		strlcpy((char *)(addr), *argv, (size_t) upper_limit);
	} else {
		char **optptr = (char **)(addr);
		sv = strdup(*argv);
		if (sv == nullptr)
			novm((char *)"option argument");
		if (*optptr)
			free(*optptr);
		*optptr = sv;
	}
}

int Option::arguments_number()
{
	return (this->type == o_bool || this->type == o_special_noarg
	        || (this->flags & OPT_NOARG)) ? 0 : 1;
}

int Option::parse_number(char *str, u_int32_t *valp, int base)
{
	char *ptr;

	*valp = (u_int32_t)strtoul(str, &ptr, base);
	if (ptr == str) {
		option_error("invalid numeric parameter '%s' for %s option",
		             str, current_option);
		return 0;
	}
	return 1;
}

int Option::parse_int(char *str, int *valp)
{
	u_int32_t v;
	if (!parse_number(str, &v, 0))
		return 0;
	*valp = (int) v;
	return 1;
}

bool Option::process_number(char **argv)
{
	u_int32_t vv;
	if (this->flags & OPT_NOARG) {
		vv = this->flags & OPT_VALUE;
		if (vv & 0x80)
			vv |= 0xffffff00U;

	} else if (!parse_number(*argv, &vv, 16)) {
		return false;
	}
	if (this->flags & OPT_OR)
		vv |= *(u_int32_t *)(this->addr);
	*(u_int32_t *)(this->addr) = vv;
	if (this->addr2 && (this->flags & OPT_A2COPY))
		*(u_int32_t *)(this->addr2) = vv;
	return true;
}

bool Option::process_bool()
{
	u_int32_t v;
	v = this->flags & OPT_VALUE;
	*(bool *)(this->addr) = v == 1;
	if (this->addr2 && (this->flags & OPT_A2COPY))
		*(bool *)(this->addr2) = v == 1;
	else if (this->addr2 && (this->flags & OPT_A2CLR))
		*(bool *)(this->addr2) = false;
	else if (this->addr2 && (this->flags & OPT_A2CLRB))
		*(u_char *)(this->addr2) &= ~v;
	else if (this->addr2 && (this->flags & OPT_A2OR))
		*(u_char *)(this->addr2) |= v;
	return true;
}

bool Option::process_int(char **argv)
{
	int iv, a;
	iv = 0;
	if ((this->flags & OPT_NOARG) == 0) {
		if (!parse_int(*argv, &iv))
			return false;
		if ((((this->flags & OPT_LLIMIT) && iv < this->lower_limit)
		     || ((this->flags & OPT_ULIMIT) && iv > this->upper_limit))
		    && !((this->flags & OPT_ZEROOK && iv == 0))) {
			const char *zok = (this->flags & OPT_ZEROOK)? " zero or": "";
			switch (this->flags & OPT_LIMITS) {
				case OPT_LLIMIT:
					option_error("%s value must be%s >= %d",
					             this->name, zok, this->lower_limit);
					break;
				case OPT_ULIMIT:
					option_error("%s value must be%s <= %d",
					             this->name, zok, this->upper_limit);
					break;
				case OPT_LIMITS:
					option_error("%s value must be%s between %d and %d",
					             this->name, zok, this->lower_limit, this->upper_limit);
					break;
				default:
					break;
			}
			return false;
		}
	}
	a = this->flags & OPT_VALUE;
	if (a >= 128)
		a -= 256;		/* sign extend */
	iv += a;
	if (this->flags & OPT_INC)
		iv += *(int *)(this->addr);
	if ((this->flags & OPT_NOINCR) && !privileged_option) {
		int oldv = *(int *)(this->addr);
		if ((this->flags & OPT_ZEROINF) ?
		    (oldv != 0 && (iv == 0 || iv > oldv)) : (iv > oldv)) {
			option_error("%s value cannot be increased", this->name);
			return false;
		}
	}
	*(int *)(this->addr) = iv;
	if (this->addr2 && (this->flags & OPT_A2COPY))
		*(int *)(this->addr2) = iv;
	return true;
}

/*
 * Set an environment variable specified by the user.
 */
static int
user_setenv(char **argv)
{
	char *arg = argv[0];
	char *eqp;
	struct userenv *uep, **insp;

	if ((eqp = strchr(arg, '=')) == nullptr) {
		option_error((char *) "missing = in name=value: %s", arg);
		return 0;
	}
	if (eqp == arg) {
		option_error((char *) "missing variable name: %s", arg);
		return 0;
	}
	for (uep = userenv_list; uep != nullptr; uep = uep->ue_next) {
		size_t nlen = strlen(uep->ue_name);
		if (nlen == (eqp - arg) &&
		    strncmp(arg, uep->ue_name, nlen) == 0)
			break;
	}
	/* Ignore attempts by unprivileged users to override privileged sources */
	if (uep != nullptr && !privileged_option && uep->ue_priv)
		return 1;
	/* The name never changes, so allocate it with the structure */
	if (uep == nullptr) {
		uep = new userenv;
		strncpy(uep->ue_name, arg, eqp - arg);
		uep->ue_name[eqp - arg] = '\0';
		uep->ue_next = nullptr;
		insp = &userenv_list;
		while (*insp != nullptr)
			insp = &(*insp)->ue_next;
		*insp = uep;
	} else {
		struct userenv *uep2;
		for (uep2 = userenv_list; uep2 != nullptr; uep2 = uep2->ue_next) {
			if (uep2 != uep && !uep2->ue_isset)
				break;
		}
		if (uep2 == nullptr && !uep->ue_isset)
			Options::get_instance()->find_option("unset")->flags |= OPT_NOPRINT;
		free(uep->ue_value);
	}
	uep->ue_isset = true;
	uep->ue_priv = privileged_option == 1;
	uep->ue_source = option_source;
	uep->ue_value = strdup(eqp + 1);
	Options::curopt->flags &= ~OPT_NOPRINT;
	return 1;
}



static void user_setprint(option_t *opt, printer_func printer, void *arg)
{
	struct userenv *uep = nullptr, *uepnext;

	uepnext = userenv_list;
	while (uepnext != nullptr && !uepnext->ue_isset)
		uepnext = uepnext->ue_next;
	while (uepnext != nullptr) {
		uep = uepnext;
		uepnext = uep->ue_next;
		while (uepnext != nullptr && !uepnext->ue_isset)
			uepnext = uepnext->ue_next;
		(*printer)(arg, (char *) "%s=%s", uep->ue_name, uep->ue_value);
		if (uepnext != nullptr)
			(*printer)(arg, (char *) "\t\t# (from %s)\n%s ", uep->ue_source, opt->name);
		else
			opt->source = uep->ue_source;
	}
}

static void user_unsetprint(option_t *opt, printer_func printer, void *arg)
{
	struct userenv *uep, *uepnext;

	uepnext = userenv_list;
	while (uepnext != nullptr && uepnext->ue_isset)
		uepnext = uepnext->ue_next;
	while (uepnext != nullptr) {
		uep = uepnext;
		uepnext = uep->ue_next;
		while (uepnext != nullptr && uepnext->ue_isset)
			uepnext = uepnext->ue_next;
		(*printer)(arg, (char *)"%s", uep->ue_name);
		if (uepnext != nullptr)
			(*printer)(arg, (char *)"\t\t# (from %s)\n%s ", uep->ue_source, opt->name);
		else
			opt->source = uep->ue_source;
	}
}

static int user_unsetenv(char **argv)
{
	struct userenv *uep, **insp;
	char *arg = argv[0];

	if (strchr(arg, '=') != nullptr) {
		option_error((char *) "unexpected = in name: %s", arg);
		return 0;
	}
	if (*arg == '\0') {
		option_error((char *) "missing variable name for unset");
		return 0;
	}
	for (uep = userenv_list; uep != nullptr; uep = uep->ue_next) {
		if (strcmp(arg, uep->ue_name) == 0)
			break;
	}
	/* Ignore attempts by unprivileged users to override privileged sources */
	if (uep != nullptr && !privileged_option && uep->ue_priv)
		return 1;
	/* The name never changes, so allocate it with the structure */
	if (uep == nullptr) {
		uep = new userenv;
		strcpy(uep->ue_name, arg);
		uep->ue_next = nullptr;
		insp = &userenv_list;
		while (*insp != nullptr)
			insp = &(*insp)->ue_next;
		*insp = uep;
	} else {
		struct userenv *uep2;
		for (uep2 = userenv_list; uep2 != nullptr; uep2 = uep2->ue_next) {
			if (uep2 != uep && uep2->ue_isset)
				break;
		}
		if (uep2 == nullptr && uep->ue_isset)
			Options::get_instance()->find_option("set")->flags |= OPT_NOPRINT;
		free(uep->ue_value);
	}
	uep->ue_isset = false;
	uep->ue_priv = privileged_option == 1;
	uep->ue_source = option_source;
	uep->ue_value = nullptr;
	Options::curopt->flags &= ~OPT_NOPRINT;
	return 1;
}

/*
 * showversion - print out the version number and exit.
 */
static int
showversion(char **)
{
	if (phase == PHASE_INITIALIZE) {
		fprintf(stderr, "pppd version %s\n", VERSION);
		exit(0);
	}
	return 0;
}

/*
 * usage - print out a message telling how to use the program.
 */
static void usage()
{
	if (phase == PHASE_INITIALIZE) {
		fprintf(stderr, usage_string, VERSION, progname);
	}
}

/*
 * showhelp - print out usage message and exit.
 */
static int showhelp(char **)
{
	if (phase == PHASE_INITIALIZE) {
		usage();
		exit(0);
	}
	return 0;
}
static int setlogfile(char **argv)
{
	int fd, err;
	uid_t euid;

	euid = geteuid();
	if (!privileged_option && seteuid(getuid()) == -1) {
		option_error("unable to drop permissions to open %s: %m", *argv);
		return 0;
	}
	fd = open(*argv, O_WRONLY | O_APPEND | O_CREAT | O_EXCL, 0644);
	if (fd < 0 && errno == EEXIST)
		fd = open(*argv, O_WRONLY | O_APPEND);
	err = errno;
	if (!privileged_option && seteuid(euid) == -1)
		fatal((char *)"unable to regain privileges: %m");
	if (fd < 0) {
		errno = err;
		option_error("Can't open log file %s: %m", *argv);
		return 0;
	}
	strlcpy(logfile_name, *argv, sizeof(logfile_name));
	if (logfile_fd >= 0)
		close(logfile_fd);
	logfile_fd = fd;
	log_to_fd = fd;
	log_default = false;
	return 1;
}

/*
 * setdomain - Set domain name to append to hostname
 */
static int
setdomain(char **argv)
{
	auto options = Options::get_instance();
	gethostname(hostname, MAXNAMELEN);
	if (**argv != 0) {
		if (**argv != '.')
			strncat(hostname, ".", MAXNAMELEN - strlen(hostname));
		//domain = hostname + strlen(hostname);
		options->set_domain(hostname + strlen(hostname));
		strncat(hostname, *argv, MAXNAMELEN - strlen(hostname));
	}
	hostname[MAXNAMELEN - 1] = 0;
	return 1;
}

//
// Created by ringo on 13.09.18.
//

#ifndef PPP_OPTIONS_HPP
#define PPP_OPTIONS_HPP

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <pwd.h>

#include <string>
#include <list>

#include "pppd.hpp"
#include "pathnames.h"

extern option_t auth_options[];

namespace rng0 {
	class Option;
	struct option_value {
		struct option_value *next;
		const char *source;
		char value[1];
	};
	/*
	 * setdomain - Set domain name to append to hostname
	 */
//	static int setdomain(char **argv);
//	static void user_unsetenv(char **arg);
//	static void user_setprint(option_t *opt, printer_func printer, void *arg);
//	static int n_arguments(option_t *);



	class Options {
	public:
		static Options *get_instance();
		/* pointer to option being processed */
		static option_t *curopt;
		static int process_option(Option *opt, char *cmd, char **argv);
		int options_from_file(char *filename, int must_exist, int check_prot, int priv);
		int options_from_file(std::string &filename, bool must_exist, bool check_prot, int priv) {
			return options_from_file((char *)filename.c_str(), (int)must_exist, (int)check_prot, priv);
		}
		void set_domain(char *) {};
		char *get_domain() { return nullptr; };

		Option *find_option(const char *);
		int match_option(const char *name, Option *opt, int dowild);


		/*
		 * readfile - take commands from a file.
		 */
		static int readfile(char **argv);

		void fill_options();

		/*
		 * callfile - take commands from /etc/ppp/peers/<name>.
		 * Name may not contain /../, start with / or ../, or end in /..
		 */
		static int callfile(char **argv);
	private:
		Options();
		static Options *m_instance;
		std::list<Option *> __general_options;
		std::list<Option *> __auth_options;
		std::list<Option *> __extra_options;
	};

	class Option:option_t {
	protected:
		/*
		 * parse_number - parse an unsigned numeric parameter for an option.
		 */
		static int parse_number(char *str, u_int32_t *valp, int base);
		/*
		 * parse_int - like parse_number, but valp is int *,
		 * the base is assumed to be 0, and *valp is not changed
		 * if there is an error.
		 */
		static int parse_int(char *str, int *valp);
	public:
		Option(option_t &other) : option_t()
		{
			name = other.name;
			type = other.type;
			description = other.description;
			flags = other.flags;
			addr = other.addr;
			addr2 = other.addr2;
			upper_limit = other.upper_limit;
			lower_limit = other.lower_limit;
			source = other.source;
			priority = other.priority;
			winner = other.winner;
		}
		Option(Option &other) : option_t()
		{
			name = other.name;
			type = other.type;
			description = other.description;
			flags = other.flags;
			addr = other.addr;
			addr2 = other.addr2;
			upper_limit = other.upper_limit;
			lower_limit = other.lower_limit;
			source = other.source;
			priority = other.priority;
			winner = other.winner;
		}
		Option(const char *name, opt_type opt_type, void *addr, const char *description, u_int flags = 0, void *ptr = nullptr, int upper_limit = 0, int lower_Limit = 0):
			option_t()
		{
			this->name = name;
			this->type = opt_type;
			this->addr = addr;
			this->description = description;
			this->flags = flags;
			this->addr2 = ptr;
			this->upper_limit = upper_limit;
			this->lower_limit = lower_Limit;
		}
		explicit Option(option_t *opt):option_t() {
			if (opt) {
				name = opt->name;
				type = opt->type;
				description = opt->description;
				flags = opt->flags;
				addr = opt->addr;
				addr2 = opt->addr2;
				upper_limit = opt->upper_limit;
				lower_limit = opt->lower_limit;
				source = opt->source;
				priority = opt->priority;
				winner = opt->winner;
			}
		}
		const char	*name = nullptr;		/* name of the option */
		enum opt_type type;
		void	*addr = nullptr;
		const char	*description = nullptr;
		unsigned int flags;
		void	*addr2 = nullptr;
		int	upper_limit;
		int	lower_limit;
		const char *source = nullptr;
		short int priority = 0;
		short int winner = 0;

		int process(char *cmd, char **argv);
		/*
		 * n_arguments - tell how many arguments an option takes
         */
		int arguments_number();

		void process_string(char **argv) const;

		bool process_number(char **argv);

		bool process_bool();

		bool process_int(char **argv);

		bool check_flags(const char *optopt) const;
	};

	template<typename T>
	class TypedOption:Option {
		typedef T owner_type;
		T *owner = nullptr;
		void (owner_type ::*handler)(char **) = nullptr;
	};

}


#endif //PPP_OPTIONS_HPP

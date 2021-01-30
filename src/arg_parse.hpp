#pragma once

#include <cassert>
#include <string.h>

#include <string>
#include <vector>
#include <tuple>
#include <iostream>
#include <algorithm>

#if !defined(GIT_HASH)
	#define GIT_HASH UNKNOWN
#endif

#if !defined(GIT_DATE)
	#define GIT_DATE UNKNOWN
#endif

// Stringizing, see https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
#define xstr(s) mstr(s)
#define mstr(s) #s

class ArgParse {
public:
    enum Type { FLAG, BOOL, INT, FLOAT, STRING };
    enum Level { GENERAL, EXPERT };

	std::string get_selected_subprogram() const noexcept
	{
		assert(!subprograms.empty());
		return selected_subprogram;
	}

private:

    typedef struct
    {
        // char short_name;
        std::string name;
        Type type;
        Level level;
        bool is_required;
        bool is_set;
        std::string desc;
        std::string value;
    } Arg;

    char type_str[5][7] = {"", "BOOL", "INT", "FLOAT", "STRING"};

    std::string program_name;
    const std::string git_hash = xstr(GIT_HASH);
    const std::string git_date = xstr(GIT_DATE);
    std::string desc_;
    std::vector<Arg> args;
    std::vector<Arg> pos_args;
    std::vector<std::tuple<std::string, std::string> > subprograms;
	std::string selected_subprogram = "";

	bool allow_more_pos_arguments = false;
    uint16_t longest_opt_arg = 0; // for printing help page

    std::vector<Arg>::iterator find(std::vector<Arg> & v, const std::string & name) const
    {
        auto it = v.begin();
        while (it != v.end())
        {
            if (it->name == name)
                return it;
            ++it;
        }
        return it;
    }

    std::vector<Arg>::const_iterator find(const std::vector<Arg> & v, const std::string & name) const
    {
        auto it = v.begin();
        while (it != v.end())
        {
            if (it->name == name)
                return it;
            ++it;
        }
        return it;
    }

    void error_message(std::string && msg) const
    {
        std::cerr << "\033[1;31m" << "ERROR: " << msg << "\033[0m" << '\n'
                  << "Run `" << program_name << " --help` for more information!\n";
    }

public:

    explicit ArgParse(std::string && program_name, std::string && desc)
    {
        desc_ = desc;
		this->program_name = program_name;
    }

    void parse_args(int argc, char **argv) noexcept
    {
        if (argc == 1)
        {
            std::cout << "Run `" << program_name << " --help` for more information!\n";
        }

        size_t pos_arg_i = 0;
        for (int i = 1; i < argc; ++i)
        {
            if (strlen(argv[i]) >= 2 && argv[i][0] == '-')
            {
                if (pos_arg_i > 0)
                {
                    error_message("Option " + std::string(argv[i]) + " has to be specified before the positional arguments!");
                    exit(-1);
                }

                if (argv[i][1] == '-') // --option
                {
                    const std::string arg_name = argv[i] + 2;

                    if (arg_name == "help")
                    {
                        print_help();
                        exit(0);
                    }

                    auto arg = find(args, arg_name);
                    if (arg != args.end())
                    {
                        if (i + 1 >= argc && arg->type != FLAG)
                        {
                            error_message("Option --" + arg_name + " needs a value!");
                            exit(-1);
                        }

                        arg->is_set = true;

                        if (arg->type != FLAG)
                        {
                            arg->value = argv[i + 1];
                            ++i; // don't read value in next iteration
                        }
                    }
                    else
                    {
                        error_message("Option --" + arg_name + " does not exist!");
                        exit(-1);
                    }
                }
                else // -o
                {
                    error_message(std::string(argv[i]) + " is not a valid option. Options start with two dashes -- !");
                    exit(-1);
                }
            }
            else
            {
				if (!subprograms.empty())
				{
                    selected_subprogram = argv[i];
				    if (std::find_if(subprograms.begin(), subprograms.end(), [this](const std::tuple<std::string, std::string> & t){ return std::get<0>(t) == selected_subprogram; }) == subprograms.end())
                    {
                        error_message(selected_subprogram + " is not a valid tool name!");
                        exit(-1);
                    }
					break; // this will ignore everything else that will follow
				}
                if (pos_arg_i >= pos_args.size())
                {
                    if (allow_more_pos_arguments)
                    {
                        pos_args.emplace_back();
                    }
                    else
                    {
                        error_message("More positional arguments passed than allowed!");
                        exit(-1);
                    }
                }
                pos_args[pos_arg_i].is_set = true;
                pos_args[pos_arg_i].value = argv[i];
                ++pos_arg_i;
            }
        }
    }

    void check_args() const noexcept
    {
        // check whether the required positional arguments are given
        for (size_t pos_arg_i = 0; pos_arg_i < pos_args.size() && pos_args[pos_arg_i].is_required; ++pos_arg_i)
        {
            if (!pos_args[pos_arg_i].is_set)
            {
                error_message("Positional argument for '" + pos_args[pos_arg_i].name + "' missing!");
                exit(-1);
            }

            // if (argc == 1) // no arguments/options passed
            // {
            //     print_help();
            //     exit(0);
            // }
        }

        // check whether the required arguments are set
        for (const auto & a : args)
        {
            if (a.is_required && !a.is_set)
            {
                error_message("Option --" + a.name + " required!");
                exit(-1);
            }
        }
    }

    void add_option(std::string && name, /*const char short_name,*/ const Type type, std::string && description, const Level level, const bool required)
    {
#if defined _DEBUG
        if (find(args, name) != args.end()) // option already exists
        {
            error_message("Option '" + name + "' cannot be defined twice!");
            exit(-1);
        }
#endif

        args.emplace_back(Arg({name, /*short_name,*/ type, level, required, false, description, ""}));

        uint16_t length_help_opt = name.size() + 2 + strlen(type_str[type]) + 1;
        if (length_help_opt > longest_opt_arg) // +2 account for -- prefix
            longest_opt_arg = length_help_opt;
    }

    void set_subprograms(const std::vector<std::tuple<std::string, std::string> > & subprograms)
    {
        // if (name.size() + 2 > longest_opt_arg) // account for < and > prefix/suffix
        //     longest_opt_arg = name.size() + 2;

		this->subprograms = subprograms;

        // pos_args.emplace_back(Arg({name, type, Level::GENERAL, required, false, description, ""}));
    }

    void add_positional_argument(std::string && name, const Type type, std::string && description, const bool required, const bool allow_more_pos_arguments = false)
    {
#if defined _DEBUG
        if (find(pos_args, name) != pos_args.end()) // option already exists
        {
            error_message("Argument '" + name + "' cannot be defined twice!");
            exit(-1);
        }

        if (pos_args.size() > 0 && !pos_args.back().is_required && required) // option already existed
        {
            error_message("Cannot create the required positional argument '" + name + "' after an optional one!");
            exit(-1);
        }
#endif

        assert(!this->allow_more_pos_arguments); // allow_more_pos_arguments can only be true for the last positional argument
        this->allow_more_pos_arguments = allow_more_pos_arguments;

        if (name.size() + 2 > longest_opt_arg) // account for < and > prefix/suffix
            longest_opt_arg = name.size() + 2;

        pos_args.emplace_back(Arg({name, type, Level::GENERAL, required, false, description, ""}));
    }

    bool is_set(std::string && s) const
    {
        auto it = find(args, s);
        assert(it != args.end());

        return it->is_set;
    }

    std::string get_string(std::string && s) const noexcept
    {
        auto it = find(args, s);
        assert(it != args.end());

        return it->value;
    }

    int64_t get_int(std::string && s) const
    {
        auto it = find(args, s);
        assert(it != args.end());

        try
        {
            return std::stoll(it->value);
        }
        catch (const std::exception& ia)
        {
            error_message("Option --" + s + " needs an integer value!");
            exit(-1);
        }
    }

    bool get_bool(std::string && s) const
    {
        auto it = find(args, s);
        assert(it != args.end());

        std::string val = it->value;
        std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c){ return std::tolower(c); });

        return it->value == "1" || it->value == "true" || it->value == "one";
    }

    double get_float(std::string && s) const
    {
        auto it = find(args, s);
        assert(it != args.end());

        try
        {
            return std::stof(it->value);
        }
        catch (const std::exception& e)
        {
            error_message("Option --" + s + " needs a float value!");
            exit(-1);
        }
    }

    bool is_set_positional(std::string && s) const
    {
        auto it = find(pos_args, s);
        assert(it != args.end());

        return it->is_set;
    }

    std::string get_positional_argument(std::string && s) const
    {
        auto it = find(pos_args, s);
        assert(it != pos_args.end());

        return it->value;
    }

    std::string get_positional_argument(size_t index) const noexcept
    {
        assert(index < pos_args.size());

        return pos_args[index].value;
    }

    size_t positional_argument_size() const noexcept
    {
        return pos_args.size();
    }

	void print_desc(const std::string & desc, uint16_t padding) const noexcept
	{
		uint32_t pos = 0;
		const uint16_t space_for_desc = 85 - 4 - longest_opt_arg;

		while (pos < desc.size())
		{
			if (pos > 0)
				std::cout << '\n' << std::string(longest_opt_arg + 4, ' ');
			else
				std::cout << std::string(padding, ' ');

			uint16_t last_space_pos = pos + space_for_desc;
			while (last_space_pos > pos && last_space_pos < desc.size() && desc[last_space_pos] != ' ')
				--last_space_pos;
			if (last_space_pos == pos)
				last_space_pos = pos + space_for_desc; // just split in the middle of a word.

			std::cout << desc.substr(pos, last_space_pos - pos);
			pos = last_space_pos;
			if (desc[last_space_pos] == ' ')
				++pos;
		}
	}

    void print_help() const
    {
        std::cout << "\033[1m";
        std::cout << "PhyloCSF++ v1.0.0 (build date: " << git_date << ", git commit: " << git_hash << ")\n\n";
        std::cout << "\033[0m";
		std::cout << desc_ << "\n\n";

        std::cout << "Usage: " << program_name << " [OPTIONS]";

		if (!subprograms.empty())
		{
	        std::cout << " <tool>";
		}

        for (const auto & pa : pos_args)
        {
			assert(subprograms.empty());
            if (pa.is_required)
                std::cout << " <" << pa.name << ">";
            else
                std::cout << " [<" << pa.name << ">]";
        }

        if (allow_more_pos_arguments)
            std::cout << "...";

        std::cout << "\n\n";

		if (!subprograms.empty())
		{
	        std::cout << "Tools:\n\n";
	        for (const auto & p : subprograms)
	        {
				const std::string & name = std::get<0>(p);
				const std::string & desc = std::get<1>(p);
	            std::cout << "  " << name;
				print_desc(desc, longest_opt_arg - name.size() + 2);
				std::cout << "\n\n";
	        }
	        std::cout << '\n';
		}

		if (!pos_args.empty())
		{
	        std::cout << "Arguments:\n\n";
	        for (const auto & pa : pos_args)
	        {
	            std::cout << "  <" << pa.name << '>';
				print_desc(pa.desc, longest_opt_arg - pa.name.size());
				std::cout << "\n\n";
	        }
		}

        std::cout << "Options:\n\n";
        for (const auto & a : args)
        {
            if (a.level == GENERAL)
			{
                std::cout << "  --" << a.name << ' ' << type_str[a.type];
				print_desc(a.desc, longest_opt_arg - a.name.size() - strlen(type_str[a.type]) - 1);
				std::cout << '\n';
			}
        }
        std::cout << '\n';

        // std::cout << "Expert options:\n\n";
        // for (const auto & a : args)
        // {
        //     if (a.level == EXPERT)
        //         std::cout << "  --" << a.name << ' ' << type_str[a.type] << std::string(longest_opt_arg - a.name.size() - strlen(type_str[a.type]) - 1, ' ') << a.desc << '\n';
        // }
    }
};

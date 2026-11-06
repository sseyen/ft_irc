#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <vector>

class Command
{
	private:
		std::string _command;		// uppercased, for dispatch
		std::string _rawCommand;	// as sent by the client, for replies
		std::vector<std::string> _params;

	public:
		static const size_t MAX_LINE_SIZE = 510;   // 512 minus \r\n
		static const size_t MAX_PARAMS = 15;

		Command();

		// line: one line with its terminator already removed.
		// Returns false for lines carrying no command (empty, spaces, prefix only)
		bool parse(const std::string& raw);

		const std::string& getCommand() const;
		const std::string& getRawCommand() const;
		const std::vector<std::string>& getParams() const;
		size_t getParamCount() const;
		const std::string& getParam(size_t i) const;

};

#endif


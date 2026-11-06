#include "Command.hpp"
#include <iostream>

Command::Command()
{

}

//PARSING/////////////////////////////////////////////////////

static void skipSpaces(const std::string& s, size_t& i)
{
    while (i < s.size() && s[i] == ' ')
        ++i;
}

static std::string nextWord(const std::string& s, size_t& i)
{
    size_t start = i;
    while (i < s.size() && s[i] != ' ')
        ++i;
    return s.substr(start, i - start);
}

bool Command::parse(const std::string& rawLine)
{
	if (rawLine.size() > MAX_LINE_SIZE)
		return std::cerr << "line over byte limit long\n", false;

	std::string line = rawLine;
    size_t i = 0;

    if (!line.empty() && line[0] == ':')
        nextWord(line, i);

    skipSpaces(line, i);
    std::string command = nextWord(line, i);
    if (command.empty())
        return false;
    this->_rawCommand = command;
    for (size_t k = 0; k < command.size(); ++k)
        command[k] = std::toupper(static_cast<unsigned char>(command[k]));

    std::vector<std::string> params;
    for (;;)
    {
        skipSpaces(line, i);
        if (i >= line.size())
            break;
        if (line[i] == ':' || params.size() == MAX_PARAMS - 1)
        {
            if (line[i] == ':')
                ++i;
            params.push_back(line.substr(i));
            break;
        }
        params.push_back(nextWord(line, i));
    }

    this->_command = command;
    this->_params = params;
    return true;
}

///////////////////////////////////////////////////////

const std::string& Command::getCommand() const
{
	return this->_command;
}

const std::string& Command::getRawCommand() const
{
	return this->_rawCommand;
}

const std::vector<std::string>& Command::getParams() const
{
	return this->_params;
}

size_t Command::getParamCount() const
{
	return this->_params.size();
}

const std::string& Command::getParam(size_t i) const
{
    static const std::string empty;
    return i < this->_params.size() ? this->_params[i] : empty;
}

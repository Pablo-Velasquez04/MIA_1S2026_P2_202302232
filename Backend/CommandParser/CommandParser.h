//CommandParser/CommandParser.h
#pragma once
#include <string>
#include <map>

bool hasUnclosedQuote(const std::string& line);
std::string processLine(const std::string& rawLine);
std::string processScript(const std::string& script);
std::map<std::string, std::string> parseParams(const std::string& line);
std::string cmdReset();
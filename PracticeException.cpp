#include "PracticeException.h"
#include <sstream>

PracticeException::PracticeException(int line, const char* file) noexcept
	:
	line(line),
	file(file) {
}

const char* PracticeException::what() const noexcept {
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* PracticeException::GetType() const noexcept {
	return "Practice Exception";
}

int PracticeException::GetLine() const noexcept {
	return line;
}

const std::string& PracticeException::GetFile() const noexcept {
	return file;
}

std::string PracticeException::GetOriginString() const noexcept {
	std::ostringstream oss;
	oss << "[File] " << file << std::endl
		<< "[Line] " << line;
	return oss.str();
}



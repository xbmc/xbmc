#include "UpdateDialogAscii.h"

#include "ProcessUtils.h"
#include "StringUtils.h"

const char* introMessage = 
 "Mendeley Updater (ASCII-art edition)\n"
 "====================================\n"
 "\n"
 "We have a nice graphical interface for the Mendeley Updater, but unfortunately\n"
 "we can't show it to you :(\n"
 "\n"
 "You can fix this by installing the GTK 2 libraries.\n\n"
 "Installing Updates...\n";

void UpdateDialogAscii::init()
{
	const char* path = "/tmp/update-progress";
	m_output.open(path);
	m_output << introMessage;

	std::string command = "xterm";
	std::list<std::string> args;
	args.push_back("-hold");
	args.push_back("-T");
	args.push_back("Mendeley Updater");
	args.push_back("-e");
	args.push_back("tail");
	args.push_back("-n+1");
	args.push_back("-f");
	args.push_back(path);

	ProcessUtils::runAsync(command,args);
}

void UpdateDialogAscii::updateError(const std::string& errorMessage)
{
	m_mutex.lock();
	m_output << "\nThere was a problem installing the update: " << errorMessage << std::endl;
	m_mutex.unlock();
}

void UpdateDialogAscii::updateProgress(int percentage)
{
	m_mutex.lock();
	m_output << "Update Progress: " << intToStr(percentage) << '%' << std::endl;
	m_mutex.unlock();
}

void UpdateDialogAscii::updateFinished()
{
	m_mutex.lock();
	m_output << "\nUpdate Finished.  You can now restart Mendeley Desktop." << std::endl;
	m_mutex.unlock();
}



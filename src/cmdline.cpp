
//==============================================================================
//
//     cmdline.cpp
//
//============================================================================
//  Copyright (C) Guilaume Plante 2020 <cybercastor@icloud.com>
//==============================================================================


#include "stdafx.h"
#include <cmdline.h>

CmdLineUtil* CmdLineUtil::instance = nullptr;

#define APP_NAME		"svcm"
#define APP_DESC		"Windows Service Manipulator"
#define EXECUTABLE_NAME	"svcm.exe"
#define APP_VERSION		"v1.0"
#define APP_COPYRIGHT	"Copyright(C) 2000 - 2021 Guillaume Plante"
#define APP_CONTACT		"codecastor - codecastor.github.io"


CmdLineUtil* CmdLineUtil::getInstance()
{
	if (instance == nullptr)
	{
		instance = new CmdLineUtil();
	}

	return instance;
}


void CmdLineUtil::initializeCmdlineParser(int argc,  char **argv)
{
	inputParser.reset(argc, argv); 
}

void CmdLineUtil::printTitle()
{
	std::cout << std::endl;
	std::cout << APP_NAME << " " << APP_VERSION << " - " << APP_DESC << std::endl;
	std::cout << APP_COPYRIGHT << std::endl;
	std::cout << APP_CONTACT << std::endl;
	std::cout << std::endl;
}

void CmdLineUtil::printDescription()
{
	std::cout << "Description:" << std::endl;
	std::cout << "PLauncher executes a program locally with different privileges. It is mosly used to execute an app in admin mode." << std::endl;
	/*
		Usage: plauncher [-e][-u user[-p psswd]]   cmd[arguments]
		- a         Separate processors on which the application can run with
		commas where 1 is the lowest numbered CPU.For example,
		to run the application on CPU 2 and CPU 4, enter:
	"-a 2,4"
		- c         Copy the specified program to the remote system for
		execution.If you omit this option the application
		must be in the system path on the remote system.
		- d         Don't wait for process to terminate (non-interactive).
		- e         Does not load the specified account's profile.
		- f         Copy the specified program even if the file already
		exists on the remote system.
		- i         Run the program so that it interacts with the desktop of the
		specified session on the remote system.If no session is
		specified the process runs in the console session.
		- h         If the target system is Vista or higher, has the process
		run with the account's elevated token, if available.
		- l         Run process as limited user(strips the Administrators group
			and allows only privileges assigned to the Users group).
		On Windows Vista the process runs with Low Integrity.
		- n         Specifies timeout in seconds connecting to remote computers.
		- p         Specifies optional password for user name.If you omit this
		you will be prompted to enter a hidden password.
		- r         Specifies the name of the remote service to create or interact.
		with.
		- s         Run the remote process in the System account.
		- u         Specifies optional user name for login to remote
		computer.
		- v         Copy the specified file only if it has a higher version number
		or is newer on than the one on the remote system.
		- w         Set the working directory of the process(relative to
			remote computer).
		- x         Display the UI on the Winlogon secure desktop(local system
			only).
		- arm       Specifies the remote computer is of ARM architecture.
		- priority  Specifies - low, -belownormal, -abovenormal, -high or
		-realtime to run the process at a different priority.Use
		- background to run at low memory and I / O priority on Vista.
		computer   Direct PsExec to run the application on the remote
		computer or computers specified.If you omit the computer
		name PsExec runs the application on the local system,
		and if you specify a wildcard(\\*), PsExec runs the
		command on all computers in the current domain.
		@file      PsExec will execute the command on each of the computers listed
		in the file.
		cmd            Name of application to execute.
		arguments  Arguments to pass(note that file paths must be
			absolute paths on the target system).
		- accepteula This flag suppresses the display of the license dialog.
		- nobanner   Do not display the startup banner and copyright message.

		You can enclose applications that have spaces in their name with
		quotation marks e.g.psexec \\marklap "c:\long name app.exe".
		Input is only passed to the remote system when you press the enter
		key, and typing Ctrl - C terminates the remote process.

		If you omit a user name the process will run in the context of your
		account on the remote system, but will not have access to network
		resources(because it is impersonating).Specify a valid user name
		in the Domain\User syntax if the remote process requires access
		to network resources or to run in a different account.Note that
		the password and command is encrypted in transit to the remote system.

		Error codes returned by PsExec are specific to the applications you
		execute, not PsExec.*/
}

void CmdLineUtil::printSyntax()
{
	std::cout << "Syntax:" << std::endl;
	std::cout << " " << EXECUTABLE_NAME << " [command] [options]" << std::endl << std::endl;
	std::cout << " [-h --help]\n\tShow this help." << std::endl;
	std::cout << " [-v --version]\n\tShows installed codemeter sdk version." << std::endl;
	std::cout << " [-l --list] <options> <filters>\n\tList available license servers on the network." << std::endl;
}


void CmdLineUtil::printExamples()
{
	std::cout << "Examples:" << std::endl;
	std::cout << " " << EXECUTABLE_NAME << " --server-list\n\tDefault: list all licence network servers, remote and that have a FSB connected. Others are useless." << std::endl;
	std::cout << " " << EXECUTABLE_NAME << " --server-list --firm 6000994\n\tlist all licence servers that have the Bodycad licence." << std::endl;
	std::cout << " " << EXECUTABLE_NAME << " --server-list --local\n\tlist all licence servers including the local one." << std::endl;
	std::cout << " " << EXECUTABLE_NAME << " --server-list --empty\n\tlist all licence servers including servers with no FSB or licences." << std::endl;
	std::cout << " " << EXECUTABLE_NAME << " --run-tests\n\tExecute all the required tests for continuous integration." << std::endl << std::endl;
}

void CmdLineUtil::printUsage()
{
	printTitle();
	printSyntax();
}

void CmdLineUtil::errorCommands()
{
	printTitle();
	std::cout << "Error: no command given." << std::endl;
	std::cout << "use -h or --help for help." << std::endl << std::endl;
}
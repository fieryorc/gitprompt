gitprompt
=========

(Windows) Suite of tools to display git status on the command prompt quickly. 
This repository contains two C++ (Win32) executables. 

GitPromptCache.exe
------------------

   This is the long running cache service that caches the git status information. 
It also listens to file system change, and invalidates the cache. Also opens a named
pipe so clients can talk to it.

GitPromptClient.exe
-------------------

   This is a client program that is responsible for starting the cache service if it is not
running already. Once service is started, client talks to the service to get the status information to display.
 The output is dipslayed in the following format:

 (branch) [+A, -D, ~M]

where A - Added count, D - Deleted count, M - Modified count.

BUILDING
========

Clone the repository with submodules
   
   git clone --recursive https://github.com/fieryorc/gitprompt.git

Install cmake
Install python (latest 2.x.x) 

cd ext
md build
cd build
cmake ..\libgit2
cmake --build .

Alternatively you can open the libgit2.sln in visual studio to build. 
By default cmake builds debug version. To build Release version, you need
to open visual studio and change the configuration.

Once libgit2 is built, You can open the gitprompt.sln in the root folder
to build the gitprompt executables.


RUNNING
=======

You can run Debug\GitPromptClient.exe or Release\GitPromptClient.exe from the
directory you want to show the git status for. This will print the status of
the repository.

Note:
This also starts GitPromptCache.exe process if not running already.


CMDER 
-----
* Make sure GitPromptClient.exe, GitPromptCache.exe and git2.dll are in the same folder and also part of %path%.
* Overwrite the %cmder_root%\config\git.lua file with install\cmder\git.lua file.


POWERSHELL
----------
Work in progress



   



gitprompt
=========

(Windows) Suite of tools to display git status on the command prompt quickly. 
This repository contains two C++ (Win32) executables. 

Download the latest binaries for Windows here: https://github.com/fieryorc/gitprompt/releases

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

* Clone the repository with submodules
   *git clone --recursive https://github.com/fieryorc/gitprompt.git
* Install cmake (latest 3.x.x)
* Install python (latest 2.x.x) (Python 3.x.x will result in build error with libgit2.)
* Run the following to build libgit2
```Batchfile
cd ext
md build && cd build
cmake ..\libgit2
:: Choose one of the following (or both if you intend to compile both release and debug)
cmake --build . --config Debug
cmake --build . --config Release
```

Alternatively you can open the libgit2.sln in visual studio to build.
Once libgit2 is built successfully, You can open the gitprompt.sln in the root folder
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
GitPrompt can be used to replace the default prompt that displays the branch and clean/dirty status.
GitPrompt is much faster since it returns the cached restult most of the time.

* Make sure GitPromptClient.exe, GitPromptCache.exe and git2.dll are in the same folder and also part of %path%.
* Overwrite the %cmder_root%\config\git.lua file with install\cmder\git.lua file.


POWERSHELL
----------
Work in progress



   



# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/centos/project/MCRedis

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/centos/project/MCRedis

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/usr/local/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/local/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/centos/project/MCRedis/CMakeFiles /home/centos/project/MCRedis/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/centos/project/MCRedis/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named MCRedis

# Build rule for target.
MCRedis: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 MCRedis
.PHONY : MCRedis

# fast build rule for target.
MCRedis/fast:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/build
.PHONY : MCRedis/fast

src/Connection.o: src/Connection.cpp.o

.PHONY : src/Connection.o

# target to build an object file
src/Connection.cpp.o:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/Connection.cpp.o
.PHONY : src/Connection.cpp.o

src/Connection.i: src/Connection.cpp.i

.PHONY : src/Connection.i

# target to preprocess a source file
src/Connection.cpp.i:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/Connection.cpp.i
.PHONY : src/Connection.cpp.i

src/Connection.s: src/Connection.cpp.s

.PHONY : src/Connection.s

# target to generate assembly for a file
src/Connection.cpp.s:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/Connection.cpp.s
.PHONY : src/Connection.cpp.s

src/MiddleWare.o: src/MiddleWare.cpp.o

.PHONY : src/MiddleWare.o

# target to build an object file
src/MiddleWare.cpp.o:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/MiddleWare.cpp.o
.PHONY : src/MiddleWare.cpp.o

src/MiddleWare.i: src/MiddleWare.cpp.i

.PHONY : src/MiddleWare.i

# target to preprocess a source file
src/MiddleWare.cpp.i:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/MiddleWare.cpp.i
.PHONY : src/MiddleWare.cpp.i

src/MiddleWare.s: src/MiddleWare.cpp.s

.PHONY : src/MiddleWare.s

# target to generate assembly for a file
src/MiddleWare.cpp.s:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/MiddleWare.cpp.s
.PHONY : src/MiddleWare.cpp.s

src/Reply.o: src/Reply.cpp.o

.PHONY : src/Reply.o

# target to build an object file
src/Reply.cpp.o:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/Reply.cpp.o
.PHONY : src/Reply.cpp.o

src/Reply.i: src/Reply.cpp.i

.PHONY : src/Reply.i

# target to preprocess a source file
src/Reply.cpp.i:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/Reply.cpp.i
.PHONY : src/Reply.cpp.i

src/Reply.s: src/Reply.cpp.s

.PHONY : src/Reply.s

# target to generate assembly for a file
src/Reply.cpp.s:
	$(MAKE) -f CMakeFiles/MCRedis.dir/build.make CMakeFiles/MCRedis.dir/src/Reply.cpp.s
.PHONY : src/Reply.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... MCRedis"
	@echo "... src/Connection.o"
	@echo "... src/Connection.i"
	@echo "... src/Connection.s"
	@echo "... src/MiddleWare.o"
	@echo "... src/MiddleWare.i"
	@echo "... src/MiddleWare.s"
	@echo "... src/Reply.o"
	@echo "... src/Reply.i"
	@echo "... src/Reply.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system


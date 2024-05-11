#include "mcmini/real_world/process/fork_process_source.hpp"

#include <fcntl.h>
#include <libgen.h>
#include <sys/personality.h>
#include <sys/types.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>

#include "mcmini/common/shm_config.h"
#include "mcmini/defines.h"
#include "mcmini/misc/extensions/unique_ptr.hpp"
#include "mcmini/real_world/mailbox/runner_mailbox.h"
#include "mcmini/real_world/process/local_linux_process.hpp"
#include "mcmini/real_world/process/template_process.h"
#include "mcmini/signal.hpp"

using namespace real_world;
using namespace extensions;

std::unique_ptr<shared_memory_region> fork_process_source::rw_region = nullptr;

void fork_process_source::initialize_shared_memory() {
  const std::string shm_file_name = "/mcmini-" + std::string(getenv("USER")) +
                                    "-" + std::to_string((long)getpid());
  rw_region = make_unique<shared_memory_region>(shm_file_name, shm_size);

  volatile runner_mailbox* mbp = rw_region->as_array_of<runner_mailbox>();

  // TODO: This should be a configurable parameter perhaps...
  const int max_total_threads = MAX_TOTAL_THREADS_IN_PROGRAM;
  for (int i = 0; i < max_total_threads; i++) mc_runner_mailbox_init(mbp + i);
}

fork_process_source::fork_process_source(std::string target_program)
    : target_program(std::move(target_program)) {
  static std::once_flag shm_once_flag;
  std::call_once(shm_once_flag, initialize_shared_memory);
}

std::unique_ptr<process> fork_process_source::make_new_process() {
  // 1. Set up phase (LD_PRELOAD, binary sempahores, template process creation)
  setup_ld_preload();
  reset_binary_semaphores_for_new_process();
  if (!has_template_process_alive()) make_new_template_process();

  // 2. Check if the current template process has previously exited; if so, it
  // would have delivered a `SIGCHLD` to this process. By default this signal is
  // ignored, but McMini explicitly captures it (see `signal_tracker`).
  if (signal_tracker::instance().try_consume_signal(SIGCHLD)) {
    this->template_pid = fork_process_source::no_template;
    if (waitpid(this->template_pid, nullptr, 0) == -1) {
      throw process_source::process_creation_exception(
          "Failed to create a cleanup zombied child process (waitpid(2) "
          "returned -1): " +
          std::string(strerror(errno)));
    }
    throw process_source::process_creation_exception(
        "Failed to create a new process (template process died)");
  }

  // 3. If the current template process is alive, tell it to spawn a new
  // process and then wait for it to successfully call `fork(2)` to tell us
  // about its new child.
  const volatile template_process_t* tstruct =
      rw_region->as<template_process_t>();

  if (sem_post((sem_t*)&tstruct->libmcmini_sem) != 0) {
    throw process_source::process_creation_exception(
        "The template process (" + std::to_string(template_pid) +
        ") was not synchronized with correctly: " +
        std::string(strerror(errno)));
  }

  if (sem_wait((sem_t*)&tstruct->mcmini_process_sem) != 0) {
    throw process_source::process_creation_exception(
        "The template process (" + std::to_string(template_pid) +
        ") was not synchronized with correctly: " +
        std::string(strerror(errno)));
  }

  return extensions::make_unique<local_linux_process>(tstruct->cpid,
                                                      *rw_region);
}

void fork_process_source::make_new_template_process() {
  // Reset first. If an exception is raised in subsequent steps, we don't want
  // to erroneously think that there is a template process when indeed there
  // isn't one.
  this->template_pid = fork_process_source::no_template;

  {
    const volatile template_process_t* tstruct =
        rw_region->as<template_process_t>();
    sem_init((sem_t*)&tstruct->mcmini_process_sem, SEM_FLAG_SHARED, 0);
    sem_init((sem_t*)&tstruct->libmcmini_sem, SEM_FLAG_SHARED, 0);
  }

  int pipefd[2];
  if (pipe(pipefd) == -1) {
    throw std::runtime_error("Failed to open pipe(2): " +
                             std::string(strerror(errno)));
  }

  errno = 0;
  pid_t child_pid = fork();
  if (child_pid == -1) {
    // fork(2) failed
    throw process_source::process_creation_exception(
        "Failed to create a new process (fork(2) failed): " +
        std::string(strerror(errno)));
  } else if (child_pid == 0) {
    // ******************
    // Child process case
    // ******************
    close(pipefd[0]);
    fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);

    // `const_cast<>` is needed to call the C-functions here. A new/delete
    // or malloc/free _could be_ needed, we'd need to check the man page. As
    // long as the char * is not actually modified, this is OK and the best way
    // to interface with the C library routines
    char* args[] = {const_cast<char*>(this->target_program.c_str()),
                    NULL /*TODO: Add additional arguments here if needed */};
    setenv("libmcmini-template-loop", "1", 1);
    personality(ADDR_NO_RANDOMIZE);
    execvp(this->target_program.c_str(), args);
    unsetenv("libmcmini-template-loop");

    // If `execvp()` fails, we signal the error to the parent process by writing
    // into the pipe.
    int err = errno;
    write(pipefd[1], &err, sizeof(err));
    close(pipefd[1]);

    // @note: We invoke `quick_exit()` here to ensure that C++ static
    // objects are NOT destroyed. `std::exit()` will invoke the destructors
    // of such static objects. This is only intended to happen exactly once
    // however; bad things likely would happen to a program which called the
    // destructor on an object that already cleaned up its resources.
    //
    // We must remember that this child is in a completely separate process with
    // a completely separate address space, but the shared resources that the
    // McMini process holds onto will also (inadvertantly) be shared with the
    // child. To get C++ to play nicely, this is how we do it.
    std::quick_exit(EXIT_FAILURE);
    // ******************
    // Child process case
    // ******************

  } else {
    // *******************
    // Parent process case
    // *******************
    close(pipefd[1]);  // Close write end

    int err = 0;
    if (read(pipefd[0], &err, sizeof(err)) > 0) {
      // waitpid() ensures that the child's resources are properly reacquired.
      if (waitpid(child_pid, nullptr, 0) == -1) {
        throw process_source::process_creation_exception(
            "Failed to create a cleanup zombied child process (waitpid(2) "
            "returned -1): " +
            std::string(strerror(errno)));
      }
      throw process_source::process_creation_exception(
          "Failed to create a new process (execvp(2) failed): " +
          std::string(strerror(err)));
    }
    close(pipefd[0]);

    // *******************
    // Parent process case
    // *******************
    this->template_pid = child_pid;
  }
}

void fork_process_source::setup_ld_preload() {
  char buf[1000];
  buf[sizeof(buf) - 1] = '\0';
  snprintf(buf, sizeof buf, "%s:%s/libmcmini.so",
           (getenv("LD_PRELOAD") ? getenv("LD_PRELOAD") : ""),
           dirname(const_cast<char*>(this->target_program.c_str())));
  setenv("LD_PRELOAD", buf, 1);
}

void fork_process_source::reset_binary_semaphores_for_new_process() {
  // Reinitialize the region for the new process, as the contents of the
  // memory are dirtied from the last process which used the same memory and
  // exited arbitrarily (i.e. in such a way as to leave data in the shared
  // memory).
  //
  // INVARIANT: Only one `local_linux_process` is in existence at any given
  // time.
  volatile runner_mailbox* mbp =
      rw_region->as_array_of<runner_mailbox>(0, THREAD_SHM_OFFSET);
  const int max_total_threads = MAX_TOTAL_THREADS_IN_PROGRAM;
  for (int i = 0; i < max_total_threads; i++) {
    mc_runner_mailbox_destroy(mbp + i);
    mc_runner_mailbox_init(mbp + i);
  }
}
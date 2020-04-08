/* Internal interfaces for the Windows code
   Copyright (C) 1995-2020 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef NAT_WINDOWS_NAT_H
#define NAT_WINDOWS_NAT_H

#include <windows.h>
#include <vector>

#include "target/waitstatus.h"

namespace windows_nat
{

/* Thread information structure used to track extra information about
   each thread.  */
struct windows_thread_info
{
  windows_thread_info (DWORD tid_, HANDLE h_, CORE_ADDR tlb)
    : tid (tid_),
      h (h_),
      thread_local_base (tlb)
  {
  }

  ~windows_thread_info ();

  DISABLE_COPY_AND_ASSIGN (windows_thread_info);

  /* Ensure that this thread has been suspended.  */
  void suspend ();

  /* Resume the thread if it has been suspended.  */
  void resume ();

  /* The Win32 thread identifier.  */
  DWORD tid;

  /* The handle to the thread.  */
  HANDLE h;

  /* Thread Information Block address.  */
  CORE_ADDR thread_local_base;

  /* This keeps track of whether SuspendThread was called on this
     thread.  -1 means there was a failure or that the thread was
     explicitly not suspended, 1 means it was called, and 0 means it
     was not.  */
  int suspended = 0;

#ifdef _WIN32_WCE
  /* The context as retrieved right after suspending the thread. */
  CONTEXT base_context {};
#endif

  /* The context of the thread, including any manipulations.  */
  union
  {
    CONTEXT context {};
#ifdef __x86_64__
    WOW64_CONTEXT wow64_context;
#endif
  };

  /* Whether debug registers changed since we last set CONTEXT back to
     the thread.  */
  bool debug_registers_changed = false;

  /* Nonzero if CONTEXT is invalidated and must be re-read from the
     inferior thread.  */
  bool reload_context = false;

  /* True if this thread is currently stopped at a software
     breakpoint.  This is used to offset the PC when needed.  */
  bool stopped_at_software_breakpoint = false;

  /* The name of the thread, allocated by xmalloc.  */
  gdb::unique_xmalloc_ptr<char> name;
};


/* Possible values to pass to 'thread_rec'.  */
enum thread_disposition_type
{
  /* Do not invalidate the thread's context, and do not suspend the
     thread.  */
  DONT_INVALIDATE_CONTEXT,
  /* Invalidate the context, but do not suspend the thread.  */
  DONT_SUSPEND,
  /* Invalidate the context and suspend the thread.  */
  INVALIDATE_CONTEXT
};

/* Find a thread record given a thread id.  THREAD_DISPOSITION
   controls whether the thread is suspended, and whether the context
   is invalidated.

   This function must be supplied by the embedding application.  */
extern windows_thread_info *thread_rec (ptid_t ptid,
					thread_disposition_type disposition);


/* Handle OUTPUT_DEBUG_STRING_EVENT from child process.  Updates
   OURSTATUS and returns the thread id if this represents a thread
   change (this is specific to Cygwin), otherwise 0.

   Cygwin prepends its messages with a "cygwin:".  Interpret this as
   a Cygwin signal.  Otherwise just print the string as a warning.

   This function must be supplied by the embedding application.  */
extern int handle_output_debug_string (struct target_waitstatus *ourstatus);

/* Currently executing process */
extern HANDLE current_process_handle;
extern DWORD current_process_id;
extern DWORD main_thread_id;
extern enum gdb_signal last_sig;

/* The current debug event from WaitForDebugEvent or from a pending
   stop.  */
extern DEBUG_EVENT current_event;

/* The most recent event from WaitForDebugEvent.  Unlike
   current_event, this is guaranteed never to come from a pending
   stop.  This is important because only data from the most recent
   event from WaitForDebugEvent can be used when calling
   ContinueDebugEvent.  */
extern DEBUG_EVENT last_wait_event;

/* Info on currently selected thread */
extern windows_thread_info *current_windows_thread;

/* The ID of the thread for which we anticipate a stop event.
   Normally this is -1, meaning we'll accept an event in any
   thread.  */
extern DWORD desired_stop_thread_id;

/* A single pending stop.  See "pending_stops" for more
   information.  */
struct pending_stop
{
  /* The thread id.  */
  DWORD thread_id;

  /* The target waitstatus we computed.  */
  target_waitstatus status;

  /* The event.  A few fields of this can be referenced after a stop,
     and it seemed simplest to store the entire event.  */
  DEBUG_EVENT event;
};

/* A vector of pending stops.  Sometimes, Windows will report a stop
   on a thread that has been ostensibly suspended.  We believe what
   happens here is that two threads hit a breakpoint simultaneously,
   and the Windows kernel queues the stop events.  However, this can
   result in the strange effect of trying to single step thread A --
   leaving all other threads suspended -- and then seeing a stop in
   thread B.  To handle this scenario, we queue all such "pending"
   stops here, and then process them once the step has completed.  See
   PR gdb/22992.  */
extern std::vector<pending_stop> pending_stops;

/* Contents of $_siginfo */
extern EXCEPTION_RECORD siginfo_er;

/* Return the name of the DLL referenced by H at ADDRESS.  UNICODE
   determines what sort of string is read from the inferior.  Returns
   the name of the DLL, or NULL on error.  If a name is returned, it
   is stored in a static buffer which is valid until the next call to
   get_image_name.  */
extern const char *get_image_name (HANDLE h, void *address, int unicode);

}

#endif
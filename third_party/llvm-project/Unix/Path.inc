//===- llvm/Support/Unix/Path.inc - Unix Path Implementation ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the Unix specific implementation of the Path API.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only generic UNIX code that
//===          is guaranteed to work on *all* UNIX variants.
//===----------------------------------------------------------------------===//

#include "Unix.h"
#include <limits.h>
#include <stdio.h>
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include <dirent.h>
#include <pwd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/attr.h>
#include <copyfile.h>
#elif defined(__DragonFly__)
#include <sys/mount.h>
#endif

// Both stdio.h and cstdio are included via different paths and
// stdcxx's cstdio doesn't include stdio.h, so it doesn't #undef the macros
// either.
#undef ferror
#undef feof

// For GNU Hurd
#if defined(__GNU__) && !defined(PATH_MAX)
# define PATH_MAX 4096
# define MAXPATHLEN 4096
#endif

#include <sys/types.h>
#if !defined(__APPLE__) && !defined(__OpenBSD__) && !defined(__FreeBSD__) &&   \
    !defined(__linux__) && !defined(__FreeBSD_kernel__) && !defined(_AIX)
#include <sys/statvfs.h>
#define STATVFS statvfs
#define FSTATVFS fstatvfs
#define STATVFS_F_FRSIZE(vfs) vfs.f_frsize
#else
#if defined(__OpenBSD__) || defined(__FreeBSD__)
#include <sys/mount.h>
#include <sys/param.h>
#elif defined(__linux__)
#if defined(HAVE_LINUX_MAGIC_H)
#include <linux/magic.h>
#else
#if defined(HAVE_LINUX_NFS_FS_H)
#include <linux/nfs_fs.h>
#endif
#if defined(HAVE_LINUX_SMB_H)
#include <linux/smb.h>
#endif
#endif
#include <sys/vfs.h>
#elif defined(_AIX)
#include <sys/statfs.h>

// <sys/vmount.h> depends on `uint` to be a typedef from <sys/types.h> to
// `uint_t`; however, <sys/types.h> does not always declare `uint`. We provide
// the typedef prior to including <sys/vmount.h> to work around this issue.
typedef uint_t uint;
#include <sys/vmount.h>
#else
#include <sys/mount.h>
#endif
#define STATVFS statfs
#define FSTATVFS fstatfs
#define STATVFS_F_FRSIZE(vfs) static_cast<uint64_t>(vfs.f_bsize)
#endif

#if defined(__NetBSD__) || defined(__DragonFly__) || defined(__GNU__)
#define STATVFS_F_FLAG(vfs) (vfs).f_flag
#else
#define STATVFS_F_FLAG(vfs) (vfs).f_flags
#endif

using namespace llvm;

namespace llvm {
namespace sys  {
namespace fs {

const file_t kInvalidFile = -1;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) ||     \
    defined(__minix) || defined(__FreeBSD_kernel__) || defined(__linux__) ||   \
    defined(__CYGWIN__) || defined(__DragonFly__) || defined(_AIX) || defined(__GNU__)
static int
test_dir(char ret[PATH_MAX], const char *dir, const char *bin)
{
  struct stat sb;
  char fullpath[PATH_MAX];

  int chars = snprintf(fullpath, PATH_MAX, "%s/%s", dir, bin);
  // We cannot write PATH_MAX characters because the string will be terminated
  // with a null character. Fail if truncation happened.
  if (chars >= PATH_MAX)
    return 1;
  if (!realpath(fullpath, ret))
    return 1;
  if (stat(fullpath, &sb) != 0)
    return 1;

  return 0;
}

static char *
getprogpath(char ret[PATH_MAX], const char *bin)
{
  /* First approach: absolute path. */
  if (bin[0] == '/') {
    if (test_dir(ret, "/", bin) == 0)
      return ret;
    return nullptr;
  }

  /* Second approach: relative path. */
  if (strchr(bin, '/')) {
    char cwd[PATH_MAX];
    if (!getcwd(cwd, PATH_MAX))
      return nullptr;
    if (test_dir(ret, cwd, bin) == 0)
      return ret;
    return nullptr;
  }

  /* Third approach: $PATH */
  char *pv;
  if ((pv = getenv("PATH")) == nullptr)
    return nullptr;
  char *s = strdup(pv);
  if (!s)
    return nullptr;
  char *state;
  for (char *t = strtok_r(s, ":", &state); t != nullptr;
       t = strtok_r(nullptr, ":", &state)) {
    if (test_dir(ret, t, bin) == 0) {
      free(s);
      return ret;
    }
  }
  free(s);
  return nullptr;
}
#endif // __FreeBSD__ || __NetBSD__ || __FreeBSD_kernel__

/// GetMainExecutable - Return the path to the main executable, given the
/// value of argv[0] from program startup.
std::string getMainExecutable(const char *argv0, void *MainAddr) {
#if defined(__APPLE__)
  // On OS X the executable path is saved to the stack by dyld. Reading it
  // from there is much faster than calling dladdr, especially for large
  // binaries with symbols.
  char exe_path[MAXPATHLEN];
  uint32_t size = sizeof(exe_path);
  if (_NSGetExecutablePath(exe_path, &size) == 0) {
    char link_path[MAXPATHLEN];
    if (realpath(exe_path, link_path))
      return link_path;
  }
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) ||   \
    defined(__minix) || defined(__DragonFly__) ||                              \
    defined(__FreeBSD_kernel__) || defined(_AIX)
  const char *curproc = "/proc/curproc/file";
  char exe_path[PATH_MAX];
  // /proc is not mounted by default under FreeBSD, but gives more accurate
  // information than argv[0] when it is.
  if (sys::fs::exists(curproc)) {
    ssize_t len = readlink(curproc, exe_path, sizeof(exe_path));
    if (len > 0) {
      // Null terminate the string for realpath. readlink never null
      // terminates its output.
      len = std::min(len, ssize_t(sizeof(exe_path) - 1));
      exe_path[len] = '\0';
      return exe_path;
    }
  }
  // If we don't have procfs mounted, fall back to argv[0]
  if (getprogpath(exe_path, argv0) != NULL)
    return exe_path;
#elif defined(__linux__) || defined(__CYGWIN__) || defined(__gnu_hurd__)
  char exe_path[MAXPATHLEN];
  const char *aPath = "/proc/self/exe";
  if (sys::fs::exists(aPath)) {
    // /proc is not always mounted under Linux (chroot for example).
    ssize_t len = readlink(aPath, exe_path, sizeof(exe_path));
    if (len < 0)
      return "";

    // Null terminate the string for realpath. readlink never null
    // terminates its output.
    len = std::min(len, ssize_t(sizeof(exe_path) - 1));
    exe_path[len] = '\0';

    // On Linux, /proc/self/exe always looks through symlinks. However, on
    // GNU/Hurd, /proc/self/exe is a symlink to the path that was used to start
    // the program, and not the eventual binary file. Therefore, call realpath
    // so this behaves the same on all platforms.
#if _POSIX_VERSION >= 200112 || defined(__GLIBC__)
    if (char *real_path = realpath(exe_path, NULL)) {
      std::string ret = std::string(real_path);
      free(real_path);
      return ret;
    }
#else
    char real_path[MAXPATHLEN];
    if (realpath(exe_path, real_path))
      return std::string(real_path);
#endif
  }
  // Fall back to the classical detection.
  if (getprogpath(exe_path, argv0))
    return exe_path;
#elif defined(HAVE_DLFCN_H) && defined(HAVE_DLADDR)
  // Use dladdr to get executable path if available.
  Dl_info DLInfo;
  int err = dladdr(MainAddr, &DLInfo);
  if (err == 0)
    return "";

  // If the filename is a symlink, we need to resolve and return the location of
  // the actual executable.
  char link_path[MAXPATHLEN];
  if (realpath(DLInfo.dli_fname, link_path))
    return link_path;
#else
#error GetMainExecutable is not implemented on this host yet.
#endif
  return "";
}

TimePoint<> basic_file_status::getLastAccessedTime() const {
  return toTimePoint(fs_st_atime, fs_st_atime_nsec);
}

TimePoint<> basic_file_status::getLastModificationTime() const {
  return toTimePoint(fs_st_mtime, fs_st_mtime_nsec);
}

UniqueID file_status::getUniqueID() const {
  return UniqueID(fs_st_dev, fs_st_ino);
}

uint32_t file_status::getLinkCount() const {
  return fs_st_nlinks;
}

ErrorOr<space_info> disk_space(const Twine &Path) {
  struct STATVFS Vfs;
  if (::STATVFS(const_cast<char *>(Path.str().c_str()), &Vfs))
    return std::error_code(errno, std::generic_category());
  auto FrSize = STATVFS_F_FRSIZE(Vfs);
  space_info SpaceInfo;
  SpaceInfo.capacity = static_cast<uint64_t>(Vfs.f_blocks) * FrSize;
  SpaceInfo.free = static_cast<uint64_t>(Vfs.f_bfree) * FrSize;
  SpaceInfo.available = static_cast<uint64_t>(Vfs.f_bavail) * FrSize;
  return SpaceInfo;
}

std::error_code current_path(SmallVectorImpl<char> &result) {
  result.clear();

  const char *pwd = ::getenv("PWD");
  llvm::sys::fs::file_status PWDStatus, DotStatus;
  if (pwd && llvm::sys::path::is_absolute(pwd) &&
      !llvm::sys::fs::status(pwd, PWDStatus) &&
      !llvm::sys::fs::status(".", DotStatus) &&
      PWDStatus.getUniqueID() == DotStatus.getUniqueID()) {
    result.append(pwd, pwd + strlen(pwd));
    return std::error_code();
  }

#ifdef MAXPATHLEN
  result.reserve(MAXPATHLEN);
#else
// For GNU Hurd
  result.re
#include "utilsFileUtils.h"
#include "utilsStream.h"
#include "utilsLog.h"
#include "utilsInternational.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>

namespace geo
{
// -----------------------------------------------------------------------------
//  FileUtils implementation
//
//  仅支持 Linux（POSIX）平台。

const char *line_separator ()
{
  return "\n";
}

static std::string trimmed_part (const std::string &part)
{
  const char *cp = part.c_str ();
  while (*cp == '/') {
    ++cp;
  }
  return std::string (cp);
}

static bool is_part_with_separator (const std::string &part)
{
  return part.c_str ()[0] == '/';
}

std::vector<std::string> split_path (const std::string &p, bool keep_last)
{
  std::vector<std::string> parts;

  bool first = true;

  const char *cp = p.c_str ();
  while (*cp) {

    const char *cp0 = cp;
    bool any = false;
    while (*cp && (!any || *cp != '/')) {
      if (*cp != '/') {
        any = true;
      } else {
        cp0 = cp;
      }
      //  backslash escape
      if (*cp == '\\' && cp[1]) {
        ++cp;
      }
      ++cp;
    }

    if (any || first || keep_last) {
      first = false;
      parts.push_back (std::string (cp0, 0, cp - cp0));
    }

  }

  return parts;
}

static std::vector<std::string> split_filename (const std::string &fn)
{
  std::vector<std::string> parts;

  const char *cp = fn.c_str ();

  while (*cp) {

    const char *cp0 = cp;
    ++cp;
    while (*cp && *cp != '.') {
      //  backslash escaping
      if (*cp == '\\' && cp[1]) {
        ++cp;
      }
      ++cp;
    }

    parts.push_back (std::string (cp0, 0, cp - cp0));
    if (*cp) {
      ++cp;
    }

  }

  return parts;
}

std::string normalize_path (const std::string &s)
{
  return geo::join (geo::split_path (s), "");
}

std::string combine_path (const std::string &p1, const std::string &p2, bool always_join)
{
  if (! always_join && p2.empty ()) {
    return p1;
  } else {
    return p1 + "/" + p2;
  }
}

std::string dirname (const std::string &s)
{
  std::vector<std::string> parts = split_path (s, true /*keep last part*/);
  if (parts.size () > 0) {
    parts.pop_back ();
  }

  if (parts.empty ()) {
    return is_part_with_separator (s) ? "" : ".";
  } else {
    return geo::join (parts, "");
  }
}

std::string filename (const std::string &s)
{
  std::vector<std::string> parts = split_path (s, true /*keep last part*/);
  if (parts.size () > 0) {
    return trimmed_part (parts.back ());
  } else {
    return std::string ();
  }
}

std::string basename (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 0) {
    return fnp.front ();
  } else {
    return std::string ();
  }
}

std::string extension (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 0) {
    fnp.erase (fnp.begin ());
  }
  return geo::join (fnp, ".");
}

std::string extension_last (const std::string &s)
{
  std::vector<std::string> fnp = split_filename (filename (s));
  if (fnp.size () > 1) {
    return fnp.back ();
  } else {
    return std::string ();
  }
}

bool
is_parent_path (const std::string &parent, const std::string &path)
{
  if (! file_exists (parent)) {
    //  If the parent path does not exist, we always return false. This cannot be a parent.
    return false;
  }

  std::vector<std::string> parts = split_path (absolute_file_path (path));

  while (! parts.empty ()) {
    if (is_same_file (parent, geo::join (parts, ""))) {
      return true;
    } else {
      parts.pop_back ();
    }
  }

  //  We did not find a match - now maybe the parent is root
  return (is_same_file (parent, geo::combine_path (geo::join (parts, ""), "", true /*always add slash*/)));
}

std::vector<std::string> dir_entries (const std::string &s, bool with_files, bool with_dirs, bool without_dotfiles)
{
  std::vector<std::string> ee;

  DIR *h = opendir (geo::to_local (s).c_str ());
  if (h) {

    struct dirent *d;
    while ((d = readdir (h)) != NULL) {

      std::string e = geo::to_string_from_local (d->d_name);
      if (e.empty () || e == "." || e == "..") {
        continue;
      }

      bool is_dir = (d->d_type == DT_DIR);
      if ((e[0] != '.' || !without_dotfiles) && ((is_dir && with_dirs) || (!is_dir && with_files))) {
        ee.push_back (e);
      }

    }

    closedir (h);

  }

  return ee;
}

bool mkdir (const std::string &path)
{
  return ::mkdir (geo::to_local (path).c_str (), 0777) == 0;
}

bool mkpath (const std::string &p)
{
  std::vector<std::string> parts = split_path (absolute_file_path (p));

  size_t i = 0;
  std::string front;

  while (i < parts.size ()) {
    front += parts[i++];
    if (! file_exists (front)) {
      if (! mkdir (front)) {
        geo::error << tr ("Unable to create directory: ") << front;
        return false;
      }
    }
  }

  return true;
}

bool rename_file (const std::string &path, const std::string &new_name)
{
  //  resolve relative names in new_name
  std::string new_path = new_name;
  if (! geo::is_absolute (new_path)) {
    new_path = geo::combine_path (geo::dirname (path), new_name);
  }

  return rename (geo::to_local (path).c_str (), geo::to_local (new_path).c_str ()) == 0;
}

bool rm_file (const std::string &path)
{
  return unlink (geo::to_local (path).c_str ()) == 0;
}

bool rm_dir (const std::string &path)
{
  return rmdir (geo::to_local (path).c_str ()) == 0;
}

bool rm_dir_recursive (const std::string &p)
{
  std::vector<std::string> entries;
  std::string path = geo::absolute_file_path (p);

  if (! geo::file_exists (path)) {
    //  already gone.
    return true;
  }

  entries = dir_entries (path, false /*without_files*/, true /*with_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    if (! rm_dir_recursive (geo::combine_path (path, *e))) {
      return false;
    }
  }

  entries = dir_entries (path, true /*with_files*/, false /*without_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    std::string tc = geo::combine_path (path, *e);
    if (! rm_file (tc)) {
      geo::error << tr ("Unable to remove file: ") << tc;
      return false;
    }
  }

  if (! rm_dir (path)) {
    geo::error << tr ("Unable to remove directory: ") << path;
    return false;
  }

  return true;
}

bool
cp_dir_recursive (const std::string &source, const std::string &target)
{
  std::vector<std::string> entries;
  std::string path = geo::absolute_file_path (source);
  std::string path_to = geo::absolute_file_path (target);

  entries = dir_entries (path, false /*without_files*/, true /*with_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    std::string tc = geo::combine_path (path_to, *e);
    if (! mkpath (tc)) {
      geo::error << tr ("Unable to create target directory: ") << tc;
      return false;
    }
    if (! cp_dir_recursive (geo::combine_path (path, *e), tc)) {
      return false;
    }
  }

  entries = dir_entries (path, true /*with_files*/, false /*without_dirs*/);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {

    //  TODO: leave symlinks symlinks?

    //  copy the files
    try {

      geo::OutputFile os_file (geo::combine_path (path_to, *e));
      geo::OutputStream os (os_file);
      geo::InputFile is_file (geo::combine_path (path, *e));
      geo::InputStream is (is_file);
      is.copy_to (os);

    } catch (geo::Exception &ex) {
      geo::error << tr ("Unable to copy file ") << geo::combine_path (path_to, *e) << tr (" to ") << geo::combine_path (path, *e)
                << tr ("(Error ") << ex.msg () << ")";
      return false;
    }

  }

  return true;
}

std::string absolute_path (const std::string &s)
{
  std::vector<std::string> parts = split_path (absolute_file_path (s));
  if (parts.size () > 0) {
    parts.pop_back ();
  }

  return geo::join (parts, "");
}

std::string current_dir ()
{
  char *cwd = getcwd (NULL, 0);
  if (cwd == NULL) {
    return std::string ();
  } else {
    std::string cwds (geo::to_string_from_local (cwd));
    free (cwd);
    return cwds;
  }
}

static std::pair<std::string, bool> absolute_path_of_existing (const std::string &s)
{
  char *fp = realpath (geo::to_local (s).c_str (), NULL);
  if (fp == NULL) {
    return std::make_pair (std::string (), false);
  } else {
    std::string fps (geo::to_string_from_local (fp));
    free (fp);
    return std::make_pair (fps, true);
  }
}

bool is_absolute (const std::string &s)
{
  std::vector<std::string> parts = split_path (s);
  if (! parts.empty ()) {
    return is_part_with_separator (parts.front ());
  } else {
    return false;
  }
}

std::string absolute_file_path (const std::string &s)
{
  std::vector<std::string> parts = split_path (s);
  if (parts.empty ()) {
    return current_dir ();
  }

  std::pair<std::string, bool> known_part;
  std::vector<std::string> unknown_parts;

  while (! parts.empty ()) {
    known_part = absolute_path_of_existing (geo::join (parts, ""));
    if (! known_part.second) {
      unknown_parts.push_back (parts.back ());
      parts.pop_back ();
    } else {
      break;
    }
  }

  std::reverse (unknown_parts.begin (), unknown_parts.end ());

  if (! known_part.second) {

    //  the top-level component is unknown. This can mean:
    //  1.) the path is already absolute, but the top-level entry does not exist
    //  2.) the path is relative, but the entry does not exist

    tl_assert (! unknown_parts.empty ());
    if (is_part_with_separator (unknown_parts.front ())) {

      //  case 1: return the full path as absolute
      return s;

    } else {

      //  case 2: try to root on current working dir
      return combine_path (current_dir (), geo::join (unknown_parts, ""));

    }

  } else {
    return combine_path (known_part.first, geo::join (unknown_parts, ""));
  }
}

typedef struct stat stat_struct;

static int stat_func (const std::string &s, stat_struct &st)
{
  return stat (geo::to_local (s).c_str (), &st);
}

bool file_exists (const std::string &p)
{
  stat_struct st;
  return stat_func (p, st) == 0;
}

bool is_writable (const std::string &p)
{
  stat_struct st;
  return stat_func (p, st) == 0 && (st.st_mode & S_IWUSR) != 0;
}

bool is_readable (const std::string &p)
{
  stat_struct st;
  return stat_func (p, st) == 0 && (st.st_mode & S_IRUSR) != 0;
}

bool is_dir (const std::string &p)
{
  stat_struct st;
  if (stat_func (p, st) != 0) {
    return false;
  } else {
    return !S_ISREG (st.st_mode);
  }
}

std::string relative_path (const std::string &base, const std::string &p)
{
  std::vector<std::string> rem;
  std::vector<std::string> parts = split_path (p);

  while (! parts.empty ()) {

    if (is_same_file (base, geo::join (parts, ""))) {

      //  combine the remaining path
      std::reverse (rem.begin (), rem.end ());
      if (! rem.empty ()) {
        rem[0] = geo::trimmed_part (rem.front ());
      }
      return geo::join (rem, "");

    }

    rem.push_back (parts.back ());
    parts.pop_back ();

  }

  return p;
}

bool is_same_file (const std::string &a, const std::string &b)
{
  if (geo::normalize_path (a) == geo::normalize_path (b)) {
    return true;
  }

  stat_struct sta, stb;
  if (stat_func (a, sta) != 0 || stat_func (b, stb) != 0) {
    return false;
  }

  return sta.st_dev == stb.st_dev && sta.st_ino == stb.st_ino;
}

static std::string
get_inst_path_internal ()
{
  std::string pf = geo::sprintf ("/proc/%d/exe", getpid ());
  if (geo::file_exists (pf)) {
    return geo::absolute_path (pf);
  }

  return std::string ();
}

std::string
get_inst_path ()
{
  static std::string s_inst_path;
  if (s_inst_path.empty ()) {
    s_inst_path = get_inst_path_internal ();
  }
  return s_inst_path;
}

std::string
get_module_path (void *addr)
{
  Dl_info info = { };
  if (dladdr (addr, &info)) {
    return geo::absolute_file_path (geo::to_string_from_local (info.dli_fname));
  } else {
    geo::warn << geo::to_string (tr ("Unable to get path of db library (as basis for loading db_plugins)"));
    return std::string ();
  }
}

}

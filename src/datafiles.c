/*
 * Copyright 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "datafiles.h"

static const gchar*
get_directory_prefix_for_type (DatafileType type)
{
  switch (type)
    {
      case DATAFILE_TYPE_SOUND:
        return "sounds";

      default:
        g_critical("unknown type");
        return "";
    }
}

static gchar*
test_directory_for_file(const char* dir, DatafileType type, const char* basename)
{
  gchar* filename = g_build_filename(dir,
                                     GETTEXT_PACKAGE,
                                     get_directory_prefix_for_type(type),
                                     basename,
                                     NULL);

  g_debug("looking for \"%s\" at \"%s\"", basename, filename);
  if (g_file_test(filename, G_FILE_TEST_EXISTS))
    return filename;

  g_free(filename);
  return NULL;
}

gchar*
datafile_find(DatafileType type, const char * basename)
{
  gchar * filename;
  const gchar * user_data_dir;
  const gchar * const * system_data_dirs;
  gsize i;

  user_data_dir = g_get_user_data_dir();
  if ((filename = test_directory_for_file(user_data_dir, type, basename)))
    return filename;
  
  system_data_dirs = g_get_system_data_dirs();
  for (i=0; system_data_dirs && system_data_dirs[i]; ++i)
    if ((filename = test_directory_for_file(system_data_dirs[i], type, basename)))
      return filename;

  return NULL;
}

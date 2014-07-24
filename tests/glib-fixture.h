/*
 * Copyright 2014 Canonical Ltd.
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

#include <map>

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include <gtest/gtest.h>

#include <locale.h> // setlocale()

class GlibFixture : public ::testing::Test
{
  private:

    GLogFunc realLogHandler;

    std::map<GLogLevelFlags,size_t> expected_log;
    std::map<GLogLevelFlags,std::vector<std::string>> log;

    void test_log_counts()
    {
      const GLogLevelFlags levels_to_test[] = { G_LOG_LEVEL_ERROR,
                                                G_LOG_LEVEL_CRITICAL,
                                                G_LOG_LEVEL_MESSAGE,
                                                G_LOG_LEVEL_WARNING };

      for(const auto& level : levels_to_test)
      {
        const auto& v = log[level];
        const auto n = v.size();

        EXPECT_EQ(expected_log[level], n);

        if (expected_log[level] != n)
            for (size_t i=0; i<n; ++i)
                g_message("%d %s", (n+1), v[i].c_str());
      }

      expected_log.clear();
      log.clear();
    }

    static void default_log_handler(const gchar    * log_domain,
                                    GLogLevelFlags   log_level,
                                    const gchar    * message,
                                    gpointer         self)
    {
      auto tmp = g_strdup_printf ("%s:%d \"%s\"", log_domain, (int)log_level, message);
      static_cast<GlibFixture*>(self)->log[log_level].push_back(tmp);
      g_free(tmp);
    }

  protected:

    void increment_expected_errors(GLogLevelFlags level, size_t n=1)
    {
      expected_log[level] += n;
    }

    virtual void SetUp()
    {
      setlocale(LC_ALL, "C.UTF-8");

      loop = g_main_loop_new(nullptr, false);

      g_log_set_default_handler(default_log_handler, this);

      g_unsetenv("DISPLAY");
    }

    virtual void TearDown()
    {
      test_log_counts();

      g_log_set_default_handler(realLogHandler, this);

      g_clear_pointer(&loop, g_main_loop_unref);
    }

  private:

    static gboolean
    wait_for_signal__timeout(gpointer name)
    {
      g_error("%s: timed out waiting for signal '%s'", G_STRLOC, (char*)name);
      return G_SOURCE_REMOVE;
    }

    static gboolean
    wait_msec__timeout(gpointer loop)
    {
      g_main_loop_quit(static_cast<GMainLoop*>(loop));
      return G_SOURCE_CONTINUE;
    }

  protected:

    /* convenience func to loop while waiting for a GObject's signal */
    void wait_for_signal(gpointer o, const gchar * signal, const guint timeout_seconds=5)
    {
      // wait for the signal or for timeout, whichever comes first
      const auto handler_id = g_signal_connect_swapped(o, signal,
                                                       G_CALLBACK(g_main_loop_quit),
                                                       loop);
      const auto timeout_id = g_timeout_add_seconds(timeout_seconds,
                                                    wait_for_signal__timeout,
                                                    loop);
      g_main_loop_run(loop);
      g_source_remove(timeout_id);
      g_signal_handler_disconnect(o, handler_id);
    }

    /* convenience func to loop for N msec */
    void wait_msec(guint msec=50)
    {
      const auto id = g_timeout_add(msec, wait_msec__timeout, loop);
      g_main_loop_run(loop);
      g_source_remove(id);
    }

    GMainLoop * loop;
};

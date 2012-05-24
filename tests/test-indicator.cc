/*
Copyright 2012 Canonical Ltd.

Authors:
    Charles Kerr <charles.kerr@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>

#include "device.h"
#include "indicator-power.h"

/***
****
***/

namespace
{
  void ensure_glib_initialized ()
  {
    static bool initialized = false;

    if (G_UNLIKELY(!initialized))
    {
      initialized = true;
      g_type_init();
    }
  }
}

/***
****
***/

class IndicatorTest : public ::testing::Test
{
  protected:

    virtual void SetUp()
    {
      ensure_glib_initialized ();
      g_setenv( "GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, TRUE);
      g_message ("GSETTINGS_SCHEMA_DIR is %s", g_getenv("GSETTINGS_SCHEMA_DIR"));
    }

    virtual void TearDown()
    {
    }
};

/***
****
***/

TEST_F(IndicatorTest, GObjectNew)
{
  ensure_glib_initialized ();

  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (IS_INDICATOR_POWER(o));
  g_object_unref (o);
}

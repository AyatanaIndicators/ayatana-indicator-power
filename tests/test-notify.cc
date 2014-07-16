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

#include "device.h"
#include "service.h"

#include <gtest/gtest.h>

#include <gio/gio.h>

class NotifyTest : public ::testing::Test
{
  private:

    typedef ::testing::Test super;

  protected:

    virtual void SetUp()
    {
      super::SetUp();
    }

    virtual void TearDown()
    {
      super::TearDown();
    }
};

/***
****
***/

// mock device provider

// send notifications of a device going down from 50% to 3% by 1% increments

// popup should appear exactly twice: once at 10%, once at 5%

TEST_F(NotifyTest, HelloWorld)
{
}


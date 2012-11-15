'''apport package hook for indicator-power

(c) 2011 Canonical Ltd.
Author: Ken VanDine <ken.vandine@ubuntu.com>
'''

from apport.hookutils import *
from os import path

def add_info(report):
    report['UPowerDump'] = command_output(['upower', '-d'])
    return report

#!/usr/bin/env python3
# Copyright (C) 2013-2025  CEA, EDF
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

import sys
import os
import  subprocess


def Chercheversion(salomePath):
    home=os.path.expanduser("~")
    commande=os.path.abspath(os.path.join(home,salomePath,'salome'))
    argus="--version"
    p = subprocess.Popen([commande,argus], stdout=subprocess.PIPE)
    (output, err) = p.communicate()
    version=' '.join(output.decode().split())
    return version


if __name__ == "__main__":
      print(Chercheversion("/local00/home/A96028/Appli"))


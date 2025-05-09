# -*- coding: iso-8859-1 -*-
# Copyright (C) 2011-2025  CEA, EDF
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
# Author : Guillaume Boulant (EDF) 
#

def runSpadderPlugin(context):
    from salome.smesh.spadder.gui import plugindialog
    from salome.kernel.uiexception import UiException
    try:
        dialog=plugindialog.getDialog()
    except UiException as err:
        from qtsalome import QMessageBox
        QMessageBox.critical(None,"An error occurs during PADDER configuration",
                             err.getUIMessage())
        return
    
    dialog.update()    
    dialog.show()


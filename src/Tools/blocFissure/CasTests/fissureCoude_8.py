# -*- coding: utf-8 -*-
# Copyright (C) 2014-2024  CEA, EDF
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
"""Fissure dans un coude - ASCOU16"""

import os

from blocFissure.gmu.fissureCoude  import fissureCoude

class fissureCoude_8(fissureCoude):
  """cas test ASCOU16"""

  nomProbleme = "fissureCoude_8"
  geomParams = dict()
  meshParams = dict()
  shapeFissureParams = dict()
  maillageFissureParams = dict()
  referencesMaillageFissure = dict()

# ---------------------------------------------------------------------------
  def setParamGeometrieSaine(self):
    """
    Paramètres géométriques du tuyau coudé sain:
    angleCoude
    r_cintr
    l_tube_p1
    l_tube_p2
    epais
    de
    """
    self.geomParams = dict(angleCoude = 90,
                           r_cintr    = 2290,
                           l_tube_p1  = 3200,
                           l_tube_p2  = 3200,
                           epais      = 30.5,
                           de         = 762)

  # ---------------------------------------------------------------------------
  def setParamMaillageSain(self):
    self.meshParams = dict(n_long_p1    = 8,
                           n_ep         = 2,
                           n_long_coude = 20,
                           n_circ_g     = 20,
                           n_circ_d     = 20,
                           n_long_p2    = 8)

  # ---------------------------------------------------------------------------

  def setParamShapeFissure(self):
    """
    paramètres de la fissure pour le tuyau coude
    profondeur  : 0 < profondeur <= épaisseur
    rayonPipe   : rayon du pipe correspondant au maillage rayonnant
    lenSegPipe  : longueur des mailles rayonnantes le long du fond de fissure (= rayonPipe par défaut)
    azimut      : entre 0 et 360°
    alpha       : 0 < alpha < angleCoude
    longueur    : <=2*profondeur ==> force une fissure elliptique (longueur/profondeur = grand axe/petit axe).
    orientation : 0° : longitudinale, 90° : circonférentielle, autre : uniquement fissures elliptiques
    lgInfluence : distance autour de la shape de fissure a remailler (si 0, pris égal à profondeur. A ajuster selon le maillage)
    elliptique  : True : fissure elliptique (longueur/profondeur = grand axe/petit axe); False : fissure longue (fond de fissure de profondeur constante, demi-cercles aux extrémites)
    pointIn_x   : optionnel coordonnées x d'un point dans le solide, pas trop loin du centre du fond de fissure (idem y,z)
    externe     : True : fissure face externe, False : fissure face interne
    """
    self.shapeFissureParams = dict(profondeur  = 8,
                                   rayonPipe   = 1,
                                   lenSegPipe  = 1.5,
                                   azimut      = 180,
                                   alpha       = 45,
                                   longueur    = 48,
                                   orientation = 0,
                                   lgInfluence = 30,
                                   elliptique  = True,
                                   externe     = False)

  # ---------------------------------------------------------------------------

  def setParamMaillageFissure(self):
    """
    Paramètres du maillage de la fissure pour le tuyau coudé
    Voir également setParamShapeFissure, paramètres rayonPipe et lenSegPipe.
    nbSegRad = nombre de couronnes
    nbSegCercle = nombre de secteurs
    areteFaceFissure = taille cible de l'arête des triangles en face de fissure.
    """
    self.maillageFissureParams = dict(nomRep        = os.curdir,
                                      nomFicSain    = self.nomProbleme,
                                      nomFicFissure = self.nomProbleme + "_fissure",
                                      nbsegRad      = 4,
                                      nbsegCercle   = 16,
                                      areteFaceFissure = 5)

  # ---------------------------------------------------------------------------
  def setReferencesMaillageFissure(self):
    from salome.smesh import smeshBuilder
    if smeshBuilder.NETGEN_VERSION_MAJOR < 6:
      self.referencesMaillageFissure = dict( \
                                             Entity_Quad_Quadrangle = 4572, \
                                             Entity_Quad_Hexa = 5128, \
                                             Entity_Node = 43015, \
                                             Entity_Quad_Edge = 648, \
                                             Entity_Quad_Triangle = 1282, \
                                             Entity_Quad_Tetra = 9146, \
                                             Entity_Quad_Pyramid = 768, \
                                             Entity_Quad_Penta = 752 \
                                           )
    else:
      self.referencesMaillageFissure = dict( \
                                             Entity_Quad_Quadrangle = 4572, \
                                             Entity_Quad_Hexa = 5128, \
                                             Entity_Node = 43443, \
                                             Entity_Quad_Edge = 648, \
                                             Entity_Quad_Triangle = 1332, \
                                             Entity_Quad_Tetra = 9431, \
                                             Entity_Quad_Pyramid = 768, \
                                             Entity_Quad_Penta = 752 \
                                           )

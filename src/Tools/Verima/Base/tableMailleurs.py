from PyQt4 import QtSql, QtCore
from tableDeBase import TableDeBase

class TableMailleurs (TableDeBase):
	def __init__(self):
            TableDeBase.__init__(self,"Mailleurs")
            self.cols=" (nomMailleur) "
            self.setField(("id","nomMailleur"))
            self.setTypeField(("int","str"),('id'))

        def createSqlTable(self):
            query=QtSql.QSqlQuery()
            print "Creation de TableMailleurs", query.exec_("create table Mailleurs(id integer primary key autoincrement, nomMailleur varchar(40));")

        def dejaRemplie(self):
            texteQuery="select * from  Mailleurs where nomMailleur='Blsurf+Ghs3D';"
            maQuery=QtSql.QSqlQuery()
            maQuery.exec_(texteQuery)
            nb=0
            while(maQuery.next()): nb=nb+1
            return nb

        def remplit(self):
            if self.dejaRemplie() :
               print "Table Mailleurs deja initialisee"
               return
            self.insereLigneAutoId(('Blsurf+Ghs3D',))
            self.insereLigneAutoId(('Tetra',))

        def insereLigneAutoId(self,valeurs):
          # difficulte a construire le texte avec une seule valeur
          texteQuery='insert into  Mailleurs (nomMailleur) values ("'+ str(valeurs[0])+ '");'
          maQuery=QtSql.QSqlQuery()
          print texteQuery, " " , maQuery.exec_(texteQuery)

        def getTous(self):
            l1=[]
            l2=[]
            texteQuery="select * from  Mailleurs;"
            maQuery=QtSql.QSqlQuery()
            maQuery.exec_(texteQuery)
            while(maQuery.next()): 
                 l1.append( maQuery.value(0).toInt()[0])
                 l2.append( maQuery.value(1).toString())
            return l1,l2

/*
	Header de la classe du dialogue de l'etat des ports pour les cles USB

	(C)David.Romeuf@univ-lyon1.fr 2006 by David Romeuf
*/


#ifndef DIALOGETATSCLES_H
#define DIALOGETATSCLES_H

#include "DialogCleUSB.h"

class DialogEtatsCles : public DialogCleUSB
{
  Q_OBJECT

public:
  DialogEtatsCles(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
  ~DialogEtatsCles();
  /*$PUBLIC_FUNCTIONS$*/

public slots:
  /*$PUBLIC_SLOTS$*/

protected:
  /*$PROTECTED_FUNCTIONS$*/

protected slots:
  /*$PROTECTED_SLOTS$*/

};

#endif


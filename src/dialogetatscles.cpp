/*
	Module de la classe du dialogue de l'etat des ports pour les cles USB
	
	(C)David.Romeuf@univ-lyon1.fr 2006 by David Romeuf
*/

#include <iostream>
#include "dialogetatscles.h"

// Constructeur du dialogue
//
DialogEtatsCles::DialogEtatsCles(QWidget* parent, const char* name, bool modal, WFlags fl) : DialogCleUSB(parent,name, modal,fl)
{
}

// Destructeur du dialogue
//
DialogEtatsCles::~DialogEtatsCles()
{
}

/*$SPECIALIZATION$*/

#include "dialogetatscles.moc"

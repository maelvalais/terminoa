/* HEADER DU MODULE DE LA CLASSE DE LA FENETRE PRINCIPALE DE L'APPLICATION TerminOA

   LOGICIEL TERMINAL RESEAU DE VISUALISATION D'IMAGES ET RECUPERATION DE DONNEES PAR PERIPHERIQUE USB

  (C)David.Romeuf@univ-lyon1.fr 30/01/2006 par David Romeuf
*/


#ifndef _TERMINOA_H_
#define _TERMINOA_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Inclusions Qt et KDE
//
#include <kmainwindow.h>
#include <qevent.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qstatusbar.h>
#include <qstring.h>
#include <qthread.h>
#include <qtimer.h>
#include <qvbox.h>
#include "dialogetatscles.h"
#include "WidgetZoneImage.h"

// Inclusions C++
//
#include "pointscomm.h"

// Definitions
//
#define TAILLE_X_BASE_TERMINOA	922
#define TAILLE_Y_BASE_TERMINOA	535

#define TAILLE_BOUTON_MAXI_X	32
#define TAILLE_BOUTON_MAXI_Y	32

#define FICHIER_ECRAN_LANCEMENT	"TerminOA_Lancement.png"
#define FICHIER_SAUV_CONSIGNES	"TerminOA_Consignes.dat"

#define TAILLE_MINI_CHAINE		255

#define LONGUEUR_ID_ALEA_TERMINOA		26

#define NB_PORTS_USB				4
#define REPERTOIRE_DIFFUSION				"/CLIMSO-OA-PicDuMidi"
#define FICHIER_TRACE_DIFFUSION_TERMINOA	"TerminOADateDiffusion"

#define TAILLE_BUFFER_COPIER_FICHIER		2048

#define MIN_CSB		0
#define MAX_CSB		999999
#define MIN_CSH		0
#define MAX_CSH		999999
#define MIN_CPUILUT	20
#define MAX_CPUILUT	200

#define ID_CUSTOM_EVENT_TERMINOA_ETAT_USB		30000
#define ID_CUSTOM_EVENT_TERMINOA_REPBARPROGUSB	30001
#define ID_CUSTOM_EVENT_TERMINOA_AFFLOGUSB		30002
#define ID_CUSTOM_EVENT_TERMINOA_AFFORDUSB		30003
#define ID_CUSTOM_EVENT_TERMINOA_CHRGAFFIMGCOUR	30004
#define ID_CUSTOM_EVENT_TERMINOA_MODEIC			30005
#define ID_CUSTOM_EVENT_TERMINOA_MODEDIAPO		30006
#define ID_CUSTOM_EVENT_TERMINOA_ECHLIN			30007
#define ID_CUSTOM_EVENT_TERMINOA_ECHLOG			30008
#define ID_CUSTOM_EVENT_TERMINOA_PALNB			30009
#define ID_CUSTOM_EVENT_TERMINOA_PALHALPHA		30010
#define ID_CUSTOM_EVENT_TERMINOA_PALNOB			30011
#define ID_CUSTOM_EVENT_TERMINOA_PALNVB			30012
#define ID_CUSTOM_EVENT_TERMINOA_PALNBB			30013
#define ID_CUSTOM_EVENT_TERMINOA_Z0				30014
#define ID_CUSTOM_EVENT_TERMINOA_Z1				30015
#define ID_CUSTOM_EVENT_TERMINOA_Z2				30016
#define ID_CUSTOM_EVENT_TERMINOA_Z3				30017
#define ID_CUSTOM_EVENT_TERMINOA_Z4				30018
#define ID_CUSTOM_EVENT_TERMINOA_Z5				30019
#define ID_CUSTOM_EVENT_TERMINOA_Z6				30020
#define ID_CUSTOM_EVENT_TERMINOA_Z7				30021
#define ID_CUSTOM_EVENT_TERMINOA_Z8				30022
#define ID_CUSTOM_EVENT_TERMINOA_Z9				30023
#define ID_CUSTOM_EVENT_TERMINOA_CSB			30024
#define ID_CUSTOM_EVENT_TERMINOA_CSH			30025
#define ID_CUSTOM_EVENT_TERMINOA_CPUILUT		30026
#define ID_CUSTOM_EVENT_TERMINOA_QUIT			30027


// Enumerations des etats possible du terminal
//
enum EtatsTerminal
{
	EtatLancement,
	EtatVisualisationCourante,
	EtatDiaporama
};

// Enumeration des etats des ports USB
//
enum EtatsPortsUSB
{
	AttenteIntroduction,
	CleMontee,
	CleCopieDocuments,
	CleDocumentsCopies
};


// Predeclaration des classes pour imbrication de pointeurs entre processus
//
class TerminOA;
class ProcessusLegerCopieDocuments;
class ProcessusLegerServeurCommandes;
class ProcessusLegerServeurDonnees;


// Classe de definition du thread de copie des documents sur cle USB dans l'application TerminOA
//
class ProcessusLegerCopieDocuments : public QThread
{
private:
	int n;						// Numero du processus leger
	QString CheminRepTerminal;	// Chemin vers le repertoire de base du terminal
	QString CheminDocuments;	// Chemin vers le repertoire des documents a transferer
	QString CheminCleUSB;		// Chemin vers le point de montage de la cle USB
	
public:
	TerminOA *FPTerminOA;		// Pointeur sur la fenetre principale de l'application
	
	ProcessusLegerCopieDocuments();	// Constructeur du processus leger de copie des documents sur cle USB
	
	void ParamNumProcessusLeger(int i);		// Fonction de parametrage du numero du processus leger
	void ParamCheminRepTerminal(QString chemin);	// Fonction de parametrage du chemin vers le repertoire de base du terminal
	void ParamCheminDocuments(QString chemin);	// Fonction de parametrage du chemin vers le repertoire des documents a transferer
	void ParamCheminCleUSB(QString chemin);		// Fonction de parametrage du chemin vers le point de montage de la cle USB
	
	virtual void run();		// Surcharge de la methode run() qui contient le code d'execution du thread
};


// Classe de definition du thread du serveur chiffre monoclient d'attente des commandes par le reseau
//
class ProcessusLegerServeurCommandes : public QThread
{
private:
	int DrapeauDemandeTerminaison;	// Variable drapeau de demande de terminaison propre du processus

public:
	TerminOA *FPTerminOA;								// Pointeur sur la fenetre principale de l'application
	ProcessusLegerServeurDonnees *threadCanalDonnees;	// Pointeur sur le thread du canal des donnees
	
	// Pointeur vers un objet Serveur chiffre mono client
	//
	PointCommServeurChiffreMonoClient *Serveur;
	
	// Constructeur du thread
	//
	ProcessusLegerServeurCommandes(TerminOA *papp,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet,int pTimeoutNegoTLSSSL,void (*pFnHandlerSIGPIPE)(int),const char *MdpClePriveeServeur,char *BuffStockMdpClePriveeServeur,int (*pFnMotDePasseClePriveeChiffree)(char*, int, int, void*),const char *pCheminCertificatCA,const char *pCheminCertificatServeur,const char *pCheminClePriveeServeur,const char *pParametresDH,const char *pListeChiffreurs);
	
	// Destructeur du thread
	//
	virtual ~ProcessusLegerServeurCommandes();
	
	virtual void run();				// Surcharge de la methode run() qui contient le code d'execution du thread
	
	int CopierDernierRecuDansFITSCourant(void);	// Copier le dernier fichier recu dans le fichier FITS courant
	
	void DemandeTerminaison(void);			// Fonction de positionnement de la demande de terminaison propre du processus leger
};


// Classe de definition du thread du serveur non chiffre monoclient d'attente des donnees par le reseau
//
class ProcessusLegerServeurDonnees : public QThread
{
private:
	int DrapeauDemandeTerminaison;	// Variable drapeau de demande de terminaison propre du processus

protected:
	QString CheminFichierImgRecep;	// Chemin d'acces et nom du fichier temporaire de stockage de la derniere image transmise
					//  par un client
	char IdentiteFichier[TAILLE_IDENTITE_FICHIER+1];	// Chaine contenant l'identite du dernier fichier recu

public:
	TerminOA *FPTerminOA;								// Pointeur sur la fenetre principale de l'application
	ProcessusLegerServeurCommandes *PLSCTerminOA;		// Pointeur vers le processus leger du canal des commandes

	// Pointeur vers un objet Serveur non chiffre mono client
	//
	PointCommServeurNonChiffreMonoClient *Serveur;
	
	// Constructeur du thread
	//
	ProcessusLegerServeurDonnees(TerminOA *papp,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet,QString p_chemFichierImgReception);
	
	// Destructeur du thread
	//
	virtual ~ProcessusLegerServeurDonnees();
	
	virtual void run();		// Surcharge de la methode run() qui contient le code d'execution du thread
	
	QString IdentiteFichierCourant(void);	// Retourne l'identite du dernier fichier recu
	QString CheminNomFichierCourant(void);	// Retourne le chemin et le nom du dernier fichier recu
	
	void DemandeTerminaison(void);	// Fonction de positionnement de la demande de terminaison propre du processus leger
};


// Classe de la fenetre principale de l'application TerminOA
//
class TerminOA : public KMainWindow
{
    Q_OBJECT

protected slots:
	void paintEvent(QPaintEvent *event);		// Surcharge du slot herite de QWidget
	void SlotPulsar1s(void);					// Slot pour le signal timeout() du QTimer Pulsar1s de pulsation de la seconde
	void SlotBoutonDialogPortsUSB(void);		// Slot pour afficher ou non le dialogue d'affichage de l'etat des ports USB

public slots:
	void SlotBoutonModeIC(void);				// Slot pour le passage en mode affichage de l'image courante
	void SlotBoutonModeDiapo(void);				// Slot pour le passage en mode diaporama
	void SlotBoutonEchLin(void);				// Slot pour le passage en echelle de representation histogramme lineaire
	void SlotBoutonEchLog(void);				// Slot pour le passage en echelle de representation histogramme logarithmique
	void SlotBoutonPalNB(void);					// Slot pour le passage en palette noir et blanc
	void SlotBoutonPalHalpha(void);				// Slot pour le passage en palette h-alpha
	void SlotBoutonPalNoirOrangeBlanc(void);	// Slot pour le passage en palette noir-orange-blanc
	void SlotBoutonPalNoirVertBlanc(void);		// Slot pour le passage en palette noir-vert-blanc
	void SlotBoutonPalNoirBleuBlanc(void);		// Slot pour le passage en palette noir-bleu-blanc
	void SlotBoutonZ0(void);					// Slot pour le passage en visualisation zone Z0
	void SlotBoutonZ1(void);					// Slot pour le passage en visualisation zone Z1
	void SlotBoutonZ2(void);					// Slot pour le passage en visualisation zone Z2
	void SlotBoutonZ3(void);					// Slot pour le passage en visualisation zone Z3
	void SlotBoutonZ4(void);					// Slot pour le passage en visualisation zone Z4
	void SlotBoutonZ5(void);					// Slot pour le passage en visualisation zone Z5
	void SlotBoutonZ6(void);					// Slot pour le passage en visualisation zone Z6
	void SlotBoutonZ7(void);					// Slot pour le passage en visualisation zone Z7
	void SlotBoutonZ8(void);					// Slot pour le passage en visualisation zone Z8
	void SlotBoutonZ9(void);					// Slot pour le passage en visualisation zone Z9
	void SlotSpinBoxCSB(int value);				// Slot pour le changement de la valeur de la spinbox de la consigne seuil bas
	void SlotSpinBoxCSH(int value);				// Slot pour le changement de la valeur de la spinbox de la consigne seuil haut
	void SlotSpinBoxCPuiLUT(int value);			// Slot pour le changement de la valeur de la spinbox de la consigne puissance lut

protected:
	void AfficherHeureUT(void);					// Fonction pour afficher l'heure UT dans le QLabel de la barre de status

	EtatsTerminal EtatTerminOA;					// Etat courant du terminal	
	int CompteurInactiviteSec;					// Compteur d'inactivite en secondes du terminal
	int DeclanchementModeDiaporamaSec;			// Temps d'inactivite en secondes de declanchement du mode diaporama
	int CompteurInterDiapositiveSec;			// Temps en secondes depuis la derniere diapositive dans le mode diaporama
	int CompteurDiapositivesProjetees;			// Nombre de diapositives projetees
	int NumeroDiapositiveCourante;				// Numero de la diapositive courante
	int TempsEntreDiapositive;					// Temps en secondes entre chaque diapositive
	int PeriodeRappelEcranLancementDiaporama;	// Periode de rappel de l'ecran de lancement durant un diaporama en nombre d'images
	volatile int USB[NB_PORTS_USB];				// Etat des ports USB : en volatile pour que le compilateur lise la variable systematiquement
	volatile int TempsEtatUSB[NB_PORTS_USB];	// Temps en seconde de la duree de l'etat courant des ports USB
	QString CheminRepTerminal;					// Chemin vers le repertoire de base du terminal
	QString CheminRepDiaporama;					// Chemin vers le repertoire des images du diaporama
	QString CheminRepDocuments;					// Chemin vers le repertoire des documents a donner 
	QString CheminMntClesUSB;					// Chemin vers le point de montage des cles USB
	QString ChaineIDAleaTerminOA;				// Chaine identifieur aleatoire du TerminOA creee au demarrage

	ProcessusLegerCopieDocuments FnCopieUSB[NB_PORTS_USB];	// Thread : Fonction pour la copie des documents diffuses sur cle USB
	int AffichageBoitePortsUSBInterdit;						// Drapeau pour savoir si on interdit ou non l'affichage du dialogue etat ports USB

	void customEvent(QCustomEvent *ce);						// Surcharge de la fonction de handler des evenements particuliers crees pour TerminOA

public:
							// Constructeur non typable mais avec des argument(s)
	TerminOA(int p_copiedoc,int p_diddiapo, int p_dediapo,int p_peldiapo,QString p_chemRepTerminal,QString p_chemRepDiaporama,QString p_chemRepDocuments,QString p_chemFichImageWeb,QString p_chemMntClesUSB,KApplication *p_appli);
	
	virtual ~TerminOA();				// Destructeur non typable
	
	KApplication *appli;				// Pointeur vers application KApplication parent de l'objet
	
	int ChargerFITSCourante(void);							// Chargement de l'image FITS courante
	void EtatUSB(int i,EtatsPortsUSB etat);					// Fonction de parametrage de l'etat d'un port USB
	void ParamPixmapBoutonDialogPortsUSB(void);				// Fonction de parametrage du pixmap du bouton affichage dialogue etats ports USB
	void AffOrdreDialoguePortsUSB(QString chaine);			// Affichage d'un ordre dans le dialogue de l'etat des ports USB
	void AffLogDialoguePortsUSB(QString chaine);			// Affichage d'un log dans le dialogue de l'etat des ports USB
	void RepProgBarDialoguePortsUSB(int i,int val);			// Representation d'un pourcentage dans le dialogue de l'etat des ports USB
	void RepLedDialoguePortsUSB(int i,int r,int v,int b);	// Representation de la couleur d'une LED dans le dialogue de l'etat des ports USB
	void PasserEnAffichageImageCourante(void);				// On passe en mode affichage de l'image courante
	QString CheminRepertoireTerminal(void);					// Retourne le chemin vers le repertoire de base du terminal
	QString CheminRepertoireDocuments(void);				// Retourne le chemin vers le repertoire de base des documents diffuses
	QString IDTerminOA(void);								// Retourne l'ID aleatoire du TerminOA en cours
	
	QVBox *BoiteRangementVertical;				// Widget de rangement vertical des widgets enfants
	ObjZoneImage *ZoneImage;					// Widget perso de la zone image
	QStatusBar *BarreStatus;					// Widget de barre horizontale pour presenter des informations de status
	//QLabel *LabelInfoBarreStatus;				// Widget label ajoute a la barre de status
	QLabel *LabelHeureUTBarreStatus;			// Widget label ajoute a la barre de status d'affichage de l'heure UT
	QTimer *Pulsar1s;							// Timer de pulsation de la seconde de temps
	QPushButton *BoutonModeIC;					// Bouton pour passer en mode affichage image courante
	QPushButton *BoutonModeDiapo;				// Bouton pour passer en mode diaporama
	QPushButton *BoutonEchLin;					// Bouton pour passer en echelle de representation histogramme lineaire
	QPushButton *BoutonEchLog;					// Bouton pour passer en echelle de representation histogramme logarithmique
	QPushButton *BoutonPalNB;					// Bouton pour passer en palette noir et blanc
	QPushButton *BoutonPalHalpha;				// Bouton pour passer en palette h-alpha
	QPushButton *BoutonPalNoirOrangeBlanc;		// Bouton pour passer en palette noir-orange-blanc
	QPushButton *BoutonPalNoirVertBlanc;		// Bouton pour passer en palette noir-vert-blanc
	QPushButton *BoutonPalNoirBleuBlanc;		// Bouton pour passer en palette noir-bleu-blanc
	QPushButton *BoutonZ0;						// Bouton pour passer en zone de visualisation z0
	QPushButton *BoutonZ1;						// Bouton pour passer en zone de visualisation z1
	QPushButton *BoutonZ2;						// Bouton pour passer en zone de visualisation z2
	QPushButton *BoutonZ3;						// Bouton pour passer en zone de visualisation z3
	QPushButton *BoutonZ4;						// Bouton pour passer en zone de visualisation z4
	QPushButton *BoutonZ5;						// Bouton pour passer en zone de visualisation z5
	QPushButton *BoutonZ6;						// Bouton pour passer en zone de visualisation z6
	QPushButton *BoutonZ7;						// Bouton pour passer en zone de visualisation z7
	QPushButton *BoutonZ8;						// Bouton pour passer en zone de visualisation z8
	QPushButton *BoutonZ9;						// Bouton pour passer en zone de visualisation z9
	QPushButton *BoutonDialogPortsUSB;			// Bouton pour afficher ou non le dialogue d'affichage de l'etat des ports USB
	QSpinBox *SpinBoxCSB;						// SpinBox d'entree de la consigne du seuil bas de visualisation
	QSpinBox *SpinBoxCSH;						// SpinBox d'entree de la consigne du seuil haut de visualisation
	QSpinBox *SpinBoxCPuiLUT;					// SpinBox d'entree de la consigne de la puissance de la LUT de visualisation

	int CopieDocumentPermise;					// Vrai si la copie des documents sur cle USB est permise
	DialogEtatsCles *BoitePortsUSB;				// Boite de dialogue d'affichage de l'etat des ports USB
};

// Evenements associes a la modification du widget de la boite de dialogue d'affichage de l'etat des ports USB
//
class CEventTerminOA_EtatUSB : public QCustomEvent	// Parametrage de l'etat du port USB dans le widget BoitePortsUSB
{
	private:
		int n;					// Numero de port USB
		EtatsPortsUSB etatn;			// Etat du port n

	public:
		CEventTerminOA_EtatUSB(int i,EtatsPortsUSB etat);

		int Lequel(void);			// Retourne le numero du port concerne par l'evenement
		EtatsPortsUSB Etat(void);		// Retourne l'etat du port n
};

class CEventTerminOA_RepProgBarDialoguePortsUSB : public QCustomEvent	// Valeur d'une barre de progression
{
	private:
		int n;					// Numero de port USB
		int valeur;				// Valeur de la barre de progression

	public:
		CEventTerminOA_RepProgBarDialoguePortsUSB(int i,int v);

		int Lequel(void);			// Retourne le numero du port concerne par l'evenement
		int Valeur(void);			// Retourne la valeur de la barre de progression
};

class CEventTerminOA_AffLogDialoguePortsUSB : public QCustomEvent	// Affichage d'un log dans le widget BoitePortsUSB
{
	private:
		QString chaine;				// La chaine

	public:
		CEventTerminOA_AffLogDialoguePortsUSB(QString c);

		QString Valeur(void);			// Retourne la chaine a afficher dans le widget
};

class CEventTerminOA_AffOrdreDialoguePortsUSB : public QCustomEvent	// Affichage d'un ordre dans BoitePortsUSB
{
	private:
		QString chaine;				// La chaine

	public:
		CEventTerminOA_AffOrdreDialoguePortsUSB(QString c);

		QString Valeur(void);			// Retourne la chaine a afficher dans le widget
};


class CEventTerminOA_ChargeAffImgCourante : public QCustomEvent	// Charge et affiche l'image courante
{
	public:
		CEventTerminOA_ChargeAffImgCourante(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_CHRGAFFIMGCOUR) {}
};

class CEventTerminOA_ModeIC : public QCustomEvent	// Passage au mode affichage de l'image courante
{
	public:
		CEventTerminOA_ModeIC(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_MODEIC) {}
};

class CEventTerminOA_ModeDiapo : public QCustomEvent	// Passage au mode diaporama
{
	public:
		CEventTerminOA_ModeDiapo(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_MODEDIAPO) {}
};

class CEventTerminOA_EchLin : public QCustomEvent	// Passage au mode echelle lineaire
{
	public:
		CEventTerminOA_EchLin(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_ECHLIN) {}
};

class CEventTerminOA_EchLog : public QCustomEvent	// Passage au mode echelle logarithmique
{
	public:
		CEventTerminOA_EchLog(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_ECHLOG) {}
};

class CEventTerminOA_PalNB : public QCustomEvent	// Passage en palette noir et blanc
{
	public:
		CEventTerminOA_PalNB(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_PALNB) {}
};

class CEventTerminOA_PalHalpha : public QCustomEvent	// Passage en palette h-alpha
{
	public:
		CEventTerminOA_PalHalpha(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_PALHALPHA) {}
};

class CEventTerminOA_PalNOB : public QCustomEvent	// Passage en palette noir-orange-blanc
{
	public:
		CEventTerminOA_PalNOB(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_PALNOB) {}
};

class CEventTerminOA_PalNVB : public QCustomEvent	// Passage en palette noir-vert-blanc
{
	public:
		CEventTerminOA_PalNVB(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_PALNVB) {}
};

class CEventTerminOA_PalNBB : public QCustomEvent	// Passage en palette noir-bleu-blanc
{
	public:
		CEventTerminOA_PalNBB(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_PALNBB) {}
};

class CEventTerminOA_Z0 : public QCustomEvent		// Passage en zoom 0
{
	public:
		CEventTerminOA_Z0(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z0) {}
};

class CEventTerminOA_Z1 : public QCustomEvent		// Passage en zoom 1
{
	public:
		CEventTerminOA_Z1(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z1) {}
};

class CEventTerminOA_Z2 : public QCustomEvent		// Passage en zoom 2
{
	public:
		CEventTerminOA_Z2(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z2) {}
};

class CEventTerminOA_Z3 : public QCustomEvent		// Passage en zoom 3
{
	public:
		CEventTerminOA_Z3(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z3) {}
};

class CEventTerminOA_Z4 : public QCustomEvent		// Passage en zoom 4
{
	public:
		CEventTerminOA_Z4(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z4) {}
};

class CEventTerminOA_Z5 : public QCustomEvent		// Passage en zoom 5
{
	public:
		CEventTerminOA_Z5(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z5) {}
};

class CEventTerminOA_Z6 : public QCustomEvent		// Passage en zoom 6
{
	public:
		CEventTerminOA_Z6(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z6) {}
};

class CEventTerminOA_Z7 : public QCustomEvent		// Passage en zoom 7
{
	public:
		CEventTerminOA_Z7(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z7) {}
};

class CEventTerminOA_Z8 : public QCustomEvent		// Passage en zoom 8
{
	public:
		CEventTerminOA_Z8(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z8) {}
};

class CEventTerminOA_Z9 : public QCustomEvent		// Passage en zoom 9
{
	public:
		CEventTerminOA_Z9(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_Z9) {}
};

class CEventTerminOA_CSB : public QCustomEvent		// Nouvelle consigne de seuil bas
{
	private:
		int valeur;				// Valeur de la consigne

	public:
		CEventTerminOA_CSB(int c);

		int Valeur(void);			// Retourne la valeur de la consigne
};

class CEventTerminOA_CSH : public QCustomEvent		// Nouvelle consigne de seuil haut
{
	private:
		int valeur;				// Valeur de la consigne

	public:
		CEventTerminOA_CSH(int c);

		int Valeur(void);			// Retourne la valeur de la consigne
};

class CEventTerminOA_CPuiLUT : public QCustomEvent	// Nouvelle consigne de puissance de la LUT
{
	private:
		int valeur;				// Valeur de la consigne

	public:
		CEventTerminOA_CPuiLUT(int c);

		int Valeur(void);			// Retourne la valeur de la consigne
};

class CEventTerminOA_Quit : public QCustomEvent		// Demande d'arret du programme TerminOA
{
	public:
		CEventTerminOA_Quit(void) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_QUIT) {}
};

#endif // _TERMINOA_H_

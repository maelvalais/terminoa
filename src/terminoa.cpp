/* MODULE DE LA CLASSE DE LA FENETRE PRINCIPALE DE L'APPLICATION TerminOA

   LOGICIEL TERMINAL RESEAU DE VISUALISATION D'IMAGES ET RECUPERATION DE DONNEES PAR PERIPHERIQUE USB

  (C)David.Romeuf@univ-lyon1.fr 30/01/2006 par David Romeuf
*/

// Inclusion C
//
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Inclusions C++
//
#include <iostream>
#include <new>
#include <memory>
#include <valarray>
#include <cmath>

using namespace std;

// Inclusions KDE et Qt
//
#include <kapplication.h>
#include <kmainwindow.h>
#include <kled.h>
#include <klocale.h>
#include <qapplication.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qdialog.h>
#include <qfile.h>
#include <qiconset.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qspinbox.h>
#include <qstatusbar.h>
#include <qtextedit.h>
#include <qsound.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qvaluelist.h>

// Inclusions TerminOA
//
#include "terminoa.h"
#include "trameimage.h"
#include "WidgetZoneImage.h"
#include "ListesCommandesReponsesTerminOA.h"

// Inclusion des icons
//
#include <IconCleUSBBarre.xpm>
#include <IconCleUSB.xpm>
#include <IconEchLin.xpm>
#include <IconEchLog.xpm>
#include <IconModeDiaporama.xpm>
#include <IconModeIC.xpm>
#include <IconPaletteHalpha.xpm>
#include <IconPaletteNB.xpm>
#include <IconPaletteNoirBleuBlanc.xpm>
#include <IconPaletteNoirOrangeBlanc.xpm>
#include <IconPaletteNoirVertBlanc.xpm>
#include <IconZoneZ0.xpm>
#include <IconZoneZ1.xpm>
#include <IconZoneZ2.xpm>
#include <IconZoneZ3.xpm>
#include <IconZoneZ4.xpm>
#include <IconZoneZ5.xpm>
#include <IconZoneZ6.xpm>
#include <IconZoneZ7.xpm>
#include <IconZoneZ8.xpm>
#include <IconZoneZ9.xpm>
//#include <Nuvola_devices_usbpendrive_unmountDG.xpm>
//#include <Nuvola_devices_usbpendrive_unmountGD.xpm>

// Taille du buffer pour la copie des fichiers a diffuser
//
#define TAILLE_BUFFER_COPIE_FICHIER		1024

// Temps de latence pour l'auto demontage du port USB
//
#define TEMPS_AUTODEMONTAGE_PORT_USB_SEC	5


// Les messages sonores
//
//#define MESSAGE_SONORE_COPIE_FICHIERS_EN_COURS		"./CopieDesFichiersEnCoursPatientez16bits.au"
//#define MESSAGE_SONORE_FICHIERS_COPIES_RETIREZ_CLE	"./FichiersCopiesSurCleRetirez16bits.au"
//#define MESSAGE_SONORE_ERREUR_DURANT_COPIE_FICHIERS	"./ErreurDurantCopieFichiersRetirezCle16bits.au"
//#define MESSAGE_SONORE_VOUS_POUVEZ_CONNECTER_CLE	"./VousPouvezConnecterVotreCle16bits.au"

// Declaration en globale des objets sonores pour ne pas avoir des problemes d'instabilite avec la synchronisation dans les threads
//  de copie des documents
// 
//QSound MsgSonoreCopieFichiersEnCours(MESSAGE_SONORE_COPIE_FICHIERS_EN_COURS);
//QSound MsgSonoreFichiersCopiesRetirezCle(MESSAGE_SONORE_FICHIERS_COPIES_RETIREZ_CLE);
//QSound MsgSonoreErreurDurantCopieFichiers(MESSAGE_SONORE_ERREUR_DURANT_COPIE_FICHIERS);
//QSound MsgSonoreVousPouvezConnecterVotreCle(MESSAGE_SONORE_VOUS_POUVEZ_CONNECTER_CLE);


// Chaine d'erreur pour la boite de dialogue de l'etat des ports USB dans le cas d'une erreur d'ecriture
//
QString ChaineErreurEcriturePortUSB="<p align=""center""><font size=""+3"" face=""par defaut""  color=""#ff0000"">ERREUR: RETIREZ votre cl&eacute; USB<br>ERROR: UNPLUG your USB keydrive</font></p>";


// Les semaphores et mutex pour synchroniser les processus legers, proteger leurs donnees
//
extern QSemaphore SemaphoreSyncLancementThreadTerminOA;	// Synchronisation du lancement des threads de TerminOA
QMutex MutexAccesFichierFITSCourant(false);		// Mutex du controle d'acces au fichier FITS courant
QMutex MutexAccesFichierCanalDonnees(false);		// Mutex du controle d'acces au dernier fichier recu sur le thread canal des donnees


// Constructeur de la classe de la fenetre principale de l'application TerminOA
//
// KMainWindow est un widget de haut niveau qui permet de gerer la geometrie de la fenetre principale et des widgets enfants 
//
TerminOA::TerminOA(int p_copiedoc,int p_diddiapo, int p_dediapo,int p_peldiapo,QString p_chemRepTerminal,QString p_chemRepDiaporama,QString p_chemRepDocuments,QString p_chemFichImageWeb,QString p_chemMntClesUSB,KApplication *p_appli) : KMainWindow( 0, "TerminOA-KMainWindow" )
{
	// set the shell's ui resource file
	//setXMLFile("terminoaui.rc");
	
	// Initialisation des variables
	//
	EtatTerminOA=EtatLancement;
	
	CompteurInactiviteSec=0;
	DeclanchementModeDiaporamaSec=p_diddiapo;
	PeriodeRappelEcranLancementDiaporama=p_peldiapo;
	CompteurInterDiapositiveSec=0;
	CompteurDiapositivesProjetees=0;
	TempsEntreDiapositive=p_dediapo;
	NumeroDiapositiveCourante=0;
	CopieDocumentPermise=p_copiedoc;

	CheminRepTerminal=p_chemRepTerminal;
	CheminRepDiaporama=p_chemRepDiaporama;
	CheminRepDocuments=p_chemRepDocuments;
	CheminMntClesUSB=p_chemMntClesUSB;

	// Creation de la chaine identifieur aleatoire du TerminOA
	//
	ChaineIDAleaTerminOA="";
	srandom(((unsigned int) time(NULL))+((unsigned int) getpid()));	// getpid() important pour ne pas avoir la meme base reproductible lors de lancements simultanes de TerminOA
	for( int i=0; i < LONGUEUR_ID_ALEA_TERMINOA; i++ )
	{
		char c='A'+(char) (26.0*random()/(RAND_MAX+1.0));
		ChaineIDAleaTerminOA.append(c);
	}

	std::cout << "TerminOA ID: " << ChaineIDAleaTerminOA << std::endl;

	appli=p_appli;
	
	// Creation de la boite de dialogue d'affichage de l'etat des ports USB
	//
	if( (BoitePortsUSB=new (std::nothrow) DialogEtatsCles(this,"TerminOA-KMainWindow-BoitePortsUSB",false,Qt::WStyle_Customize | Qt::WStyle_Dialog | Qt::WStyle_NormalBorder)) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le dialogue DialogEtatsCles:BoitePortsUSB de la fenetre principale KMainWindow." << std::endl;
	}
	BoitePortsUSB->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoitePortsUSB->setMaximumSize(BoitePortsUSB->size());
	BoitePortsUSB->setMinimumSize(BoitePortsUSB->size());
	BoitePortsUSB->textEdit1->setMaxLogLines(1000);
	BoitePortsUSB->progressBarPort1->setTotalSteps(100);
	BoitePortsUSB->progressBarPort2->setTotalSteps(100);
	BoitePortsUSB->progressBarPort3->setTotalSteps(100);
	BoitePortsUSB->progressBarPort4->setTotalSteps(100);
	BoitePortsUSB->progressBarPort1->setCenterIndicator(true);
	BoitePortsUSB->progressBarPort2->setCenterIndicator(true);
	BoitePortsUSB->progressBarPort3->setCenterIndicator(true);
	BoitePortsUSB->progressBarPort4->setCenterIndicator(true);
	
	// Initialisation de l'etat des ports USB
	//
	for( int i=0; i < NB_PORTS_USB; i++ )
	{
		EtatUSB(i,AttenteIntroduction);
		
		// Initialisation des threads de copie sur les cles USB
		//
		FnCopieUSB[i].ParamNumProcessusLeger(i);
		FnCopieUSB[i].FPTerminOA=this;
		FnCopieUSB[i].ParamCheminRepTerminal(p_chemRepTerminal);
		FnCopieUSB[i].ParamCheminDocuments(p_chemRepDocuments);
		FnCopieUSB[i].ParamCheminCleUSB(QString(p_chemMntClesUSB+QString("/usb%1").arg(i+1)));
	}
	
	AffichageBoitePortsUSBInterdit=false;
	
	// Creation du widget de la boite de rangement vertical
	//
	if( (BoiteRangementVertical=new (std::nothrow) QVBox(this,"TerminOA-KMainWindow-BoiteRangementVertical")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QVBox:BoiteRangementVertical de la fenetre principale KMainWindow." << std::endl;
	}
	
	// Creation du widget perso de la zone de representation de l'image au sens large
	//
	// L'option Qt::WNoAutoErase permet de ne pas faire effacer automatiquement la zone du windget avant l'evenement paintEvent
	//
	if( (ZoneImage=new (std::nothrow) ObjZoneImage(BoiteRangementVertical,"TerminOA-KMainWindow-ZoneImage",Qt::WNoAutoErase)) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget ObjZoneImage:ZoneImage de la fenetre principale KMainWindow." << std::endl;
	}
	
	// Configuration du widget ZoneImage
	//
	ChargerFITSCourante(); 
	ZoneImage->EtatAffichage(AffichageImageLancement);
	ZoneImage->ImageLancement(CheminRepTerminal+"/"+FICHIER_ECRAN_LANCEMENT);
	ZoneImage->DocumentsDiffuses(CheminRepDocuments);
	ZoneImage->IdentifieurTerminOA(ChaineIDAleaTerminOA);
	ZoneImage->ImageDiffuseeWeb(p_chemFichImageWeb);

	
	// Creation du widget de barre horizontale pour presenter des informations de status
	//
	if( (BarreStatus=new (std::nothrow) QStatusBar(BoiteRangementVertical,"TerminOA-KMainWindow-BarreStatus")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QStatusBar:BarreStatus de la fenetre principale KMainWindow." << std::endl;
	}
	
	// La barre de status a une dimension verticale fixe (celle de sa creation)
	//
	BarreStatus->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed));
	
	// Creation du widget label d'information ajoute a la barre de status
	//
	//if( (LabelInfoBarreStatus=new (std::nothrow) QLabel("Ok",this,"TerminOA-KMainWindow-LabelInfoBarreStatus")) == NULL )
	//{
	//	std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QLabel:LabelInfoBarreStatus de la barre de status de la fenetre principale KMainWindow." << std::endl;
	//}
	//LabelInfoBarreStatus->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum));
	
	
	// Creation du widget label d'affichage de l'heure UT ajoute a la barre de status
	//
	if( (LabelHeureUTBarreStatus=new (std::nothrow) QLabel("00:00:00 UT",this,"TerminOA-KMainWindow-LabelHeureUTBarreStatus")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QLabel:LabelHeureUTBarreStatus de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	LabelHeureUTBarreStatus->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	
	
	// Creation du widget bouton poussoir de passage en mode affichage image courante ajoute a la barre de status
	//
	if( (BoutonModeIC=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconModeIC_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonModeIC")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonModeIC de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonModeIC->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonModeIC->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en mode diaporama ajoute a la barre de status
	//
	if( (BoutonModeDiapo=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconModeDiaporama_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonModeDiapo")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonModeDiapo de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonModeDiapo->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonModeDiapo->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en echelle de representation de l'histogramme lineaire ajoute a la barre de status
	//
	if( (BoutonEchLin=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconEchLin_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonEchLin")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonEchLin de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonEchLin->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonEchLin->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en echelle de representation de l'histogramme logarithmique ajoute a la barre de status
	//
	if( (BoutonEchLog=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconEchLog_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonEchLog")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonEchLog de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonEchLog->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonEchLog->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en palette de representation noir et blanc ajoute a la barre de status
	//
	if( (BoutonPalNB=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconPaletteNB_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonPalNB")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonPalNB de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonPalNB->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonPalNB->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en palette de representation h-alpha ajoute a la barre de status
	//
	if( (BoutonPalHalpha=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconPaletteHalpha_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonPalHalpha")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonPalHalpha de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonPalHalpha->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonPalHalpha->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en palette de representation noir-orange-blanc ajoute a la barre de status
	//
	if( (BoutonPalNoirOrangeBlanc=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconPaletteNoirOrangeBlanc_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonPalNoirOrangeBlanc")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonPalNoirOrangeBlanc de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonPalNoirOrangeBlanc->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonPalNoirOrangeBlanc->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en palette de representation noir-vert-blanc ajoute a la barre de status
	//
	if( (BoutonPalNoirVertBlanc=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconPaletteNoirVertBlanc_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonPalNoirVertBlanc")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonPalNoirVertBlanc de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonPalNoirVertBlanc->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonPalNoirVertBlanc->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en palette de representation noir-bleu-blanc ajoute a la barre de status
	//
	if( (BoutonPalNoirBleuBlanc=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconPaletteNoirBleuBlanc_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonPalNoirBleuBlanc")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonPalNoirBleuBlanc de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonPalNoirBleuBlanc->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonPalNoirBleuBlanc->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z0 ajoute a la barre de status
	//
	if( (BoutonZ0=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ0_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ0")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ0 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ0->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ0->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z1 ajoute a la barre de status
	//
	if( (BoutonZ1=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ1_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ1")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ1 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ1->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ1->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z2 ajoute a la barre de status
	//
	if( (BoutonZ2=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ2_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ2")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ2 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ2->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ2->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z3 ajoute a la barre de status
	//
	if( (BoutonZ3=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ3_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ3")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ3 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ3->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ3->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z4 ajoute a la barre de status
	//
	if( (BoutonZ4=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ4_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ4")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ4 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ4->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ4->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z5 ajoute a la barre de status
	//
	if( (BoutonZ5=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ5_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ5")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ5 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ5->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ5->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z6 ajoute a la barre de status
	//
	if( (BoutonZ6=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ6_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ6")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ6 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ6->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ6->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z7 ajoute a la barre de status
	//
	if( (BoutonZ7=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ7_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ7")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ7 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ7->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ7->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z8 ajoute a la barre de status
	//
	if( (BoutonZ8=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ8_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ8")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ8 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ8->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ8->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir de passage en zone de representation Z9 ajoute a la barre de status
	//
	if( (BoutonZ9=new (std::nothrow) QPushButton(QIconSet(QPixmap(IconZoneZ9_xpm),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonZ9")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonZ9 de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonZ9->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonZ9->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Creation du widget bouton poussoir pour afficher ou non le dialogue d'affichage de l'etat des ports USB
	//
	if( (BoutonDialogPortsUSB=new (std::nothrow) QPushButton(QIconSet(QPixmap(QString("")),QIconSet::Automatic),"",this,"TerminOA-KMainWindow-BoutonDialogPortsUSB")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QPushButton:BoutonDialogPortsUSB de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	BoutonDialogPortsUSB->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	BoutonDialogPortsUSB->setMaximumSize(TAILLE_BOUTON_MAXI_X,TAILLE_BOUTON_MAXI_Y);
	
	
	// Connexion du signal clicked() envoye par les boutons aux slots respectifs SlotBouton_() de l'objet
	//
	connect(BoutonModeIC,SIGNAL(clicked()),this,SLOT(SlotBoutonModeIC()));
	connect(BoutonModeDiapo,SIGNAL(clicked()),this,SLOT(SlotBoutonModeDiapo()));
	connect(BoutonEchLin,SIGNAL(clicked()),this,SLOT(SlotBoutonEchLin()));
	connect(BoutonEchLog,SIGNAL(clicked()),this,SLOT(SlotBoutonEchLog()));
	connect(BoutonPalNB,SIGNAL(clicked()),this,SLOT(SlotBoutonPalNB()));
	connect(BoutonPalHalpha,SIGNAL(clicked()),this,SLOT(SlotBoutonPalHalpha()));
	connect(BoutonPalNoirOrangeBlanc,SIGNAL(clicked()),this,SLOT(SlotBoutonPalNoirOrangeBlanc()));
	connect(BoutonPalNoirVertBlanc,SIGNAL(clicked()),this,SLOT(SlotBoutonPalNoirVertBlanc()));
	connect(BoutonPalNoirBleuBlanc,SIGNAL(clicked()),this,SLOT(SlotBoutonPalNoirBleuBlanc()));
	connect(BoutonZ0,SIGNAL(clicked()),this,SLOT(SlotBoutonZ0()));
	connect(BoutonZ1,SIGNAL(clicked()),this,SLOT(SlotBoutonZ1()));
	connect(BoutonZ2,SIGNAL(clicked()),this,SLOT(SlotBoutonZ2()));
	connect(BoutonZ3,SIGNAL(clicked()),this,SLOT(SlotBoutonZ3()));
	connect(BoutonZ4,SIGNAL(clicked()),this,SLOT(SlotBoutonZ4()));
	connect(BoutonZ5,SIGNAL(clicked()),this,SLOT(SlotBoutonZ5()));
	connect(BoutonZ6,SIGNAL(clicked()),this,SLOT(SlotBoutonZ6()));
	connect(BoutonZ7,SIGNAL(clicked()),this,SLOT(SlotBoutonZ7()));
	connect(BoutonZ8,SIGNAL(clicked()),this,SLOT(SlotBoutonZ8()));
	connect(BoutonZ9,SIGNAL(clicked()),this,SLOT(SlotBoutonZ9()));
	connect(BoutonDialogPortsUSB,SIGNAL(clicked()),this,SLOT(SlotBoutonDialogPortsUSB()));
	
	
	// Creation du widget d'entree SpinBox de la consigne du seuil bas de visualisation ajoute a la barre de status
	//
	if( (SpinBoxCSB=new (std::nothrow) QSpinBox(MIN_CSB,MAX_CSB,100,this,"TerminOA-KMainWindow-SpinBoxCSB")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QSpinBox:SpinBoxCSB de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	SpinBoxCSB->setPrefix("b");
	
	
	// Creation du widget d'entree SpinBox de la consigne du seuil haut de visualisation ajoute a la barre de status
	//
	if( (SpinBoxCSH=new (std::nothrow) QSpinBox(MIN_CSH,MAX_CSH,1000,this,"TerminOA-KMainWindow-SpinBoxCSH")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QSpinBox:SpinBoxCSH de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	SpinBoxCSH->setPrefix("h");
	
	
	// Creation du widget d'entree SpinBox de la consigne de la puissance de la LUT de visualisation ajoute a la barre de status
	//
	if( (SpinBoxCPuiLUT=new (std::nothrow) QSpinBox(MIN_CPUILUT,MAX_CPUILUT,5,this,"TerminOA-KMainWindow-SpinBoxPuiLUT")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le widget QSpinBox:SpinBoxPuiLUT de la barre de status de la fenetre principale KMainWindow." << std::endl;
	}
	SpinBoxCPuiLUT->setPrefix("^");
	
	
	// On ajoute les widgets a la barre de status
	//
	//BarreStatus->addWidget(LabelInfoBarreStatus,0,FALSE);
	BarreStatus->addWidget(BoutonModeIC,0,TRUE);
	BarreStatus->addWidget(BoutonZ0,0,TRUE);
	BarreStatus->addWidget(BoutonZ1,0,TRUE);
	BarreStatus->addWidget(BoutonZ2,0,TRUE);
	BarreStatus->addWidget(BoutonZ3,0,TRUE);
	BarreStatus->addWidget(BoutonZ4,0,TRUE);
	BarreStatus->addWidget(BoutonZ5,0,TRUE);
	BarreStatus->addWidget(BoutonZ6,0,TRUE);
	BarreStatus->addWidget(BoutonZ7,0,TRUE);
	BarreStatus->addWidget(BoutonZ8,0,TRUE);
	BarreStatus->addWidget(BoutonZ9,0,TRUE);
	BarreStatus->addWidget(SpinBoxCSB,0,TRUE);
	BarreStatus->addWidget(SpinBoxCSH,0,TRUE);
	BarreStatus->addWidget(SpinBoxCPuiLUT,0,TRUE);
	BarreStatus->addWidget(BoutonPalNB,0,TRUE);
	BarreStatus->addWidget(BoutonPalHalpha,0,TRUE);
	BarreStatus->addWidget(BoutonPalNoirOrangeBlanc,0,TRUE);
	BarreStatus->addWidget(BoutonPalNoirVertBlanc,0,TRUE);
	BarreStatus->addWidget(BoutonPalNoirBleuBlanc,0,TRUE);
	BarreStatus->addWidget(BoutonEchLin,0,TRUE);
	BarreStatus->addWidget(BoutonEchLog,0,TRUE);
	BarreStatus->addWidget(BoutonModeDiapo,0,TRUE);
	BarreStatus->addWidget(BoutonDialogPortsUSB,0,TRUE);
	BarreStatus->addWidget(LabelHeureUTBarreStatus,0,TRUE);
	
	
	// Creation du timer de pulsation de la seconde de temps
	//
	if( (Pulsar1s=new (std::nothrow) QTimer(this,"TerminOA-KMainWindow-Pulsar1s")) == NULL )
	{
		std::cerr << "TerminOA: ERREUR: Impossible de creer le timer QTimer:Pulsar1s de la fenetre principale KMainWindow." << std::endl;
	}
	
	// Connexion du signal timeout() envoye par le timer au slot SlotPulsar1s() de l'objet
	//
	connect(Pulsar1s,SIGNAL(timeout()),this,SLOT(SlotPulsar1s()));
	
	// D�arrage du timer
	//
	Pulsar1s->start(1000,FALSE);
	
	
	// Lecture des consignes sauvegardees
	//
	QFile FichierConsignes(CheminRepTerminal+"/"+FICHIER_SAUV_CONSIGNES);
	
	if( FichierConsignes.open(IO_ReadOnly) )
	{
		QTextStream FluxTexte(&FichierConsignes);	// Flux en mode texte
		QString Ligne;					// Une chaine qui sera une ligne du fichier
		
		FichierConsignes.at(0);
		
		// Lecture de la consigne du seuil bas de visualisation
		//
		Ligne=FluxTexte.readLine();
		if( !Ligne.isNull() ) ZoneImage->ConsigneSeuilBas(std::atol(Ligne));
	
		// Lecture de la consigne du seuil haut de visualisation
		//
		Ligne=FluxTexte.readLine();
		if( !Ligne.isNull() ) ZoneImage->ConsigneSeuilHaut(std::atol(Ligne));
	
		// Lecture de la consigne de la puissance de la LUT de visualisation
		//
		Ligne=FluxTexte.readLine();
		if( !Ligne.isNull() ) ZoneImage->PuissanceLUT(std::atof(Ligne)/100.0);
	
		// Lecture du type de la palette utilisee
		//
		Ligne=FluxTexte.readLine();
		if( !Ligne.isNull() ) ZoneImage->TypePalette((TypesPalettes) std::atoi(Ligne));
	
		// Lecture du type d'echelle de l'histogramme
		//
		Ligne=FluxTexte.readLine();
		if( !Ligne.isNull() ) ZoneImage->TypeEchelleHistogramme((TypesEchelles) std::atoi(Ligne));
	
		// Lecture de la zone affichee
		//
		Ligne=FluxTexte.readLine();
		if( !Ligne.isNull() ) ZoneImage->ZoneAfficheeIC((ListeZAPIC) std::atoi(Ligne));
	
		// Lecture du drapeau d'interdiction de l'affichage du dialogue de l'etat des ports USB
		//
		Ligne=FluxTexte.readLine();
		if( !Ligne.isNull() ) AffichageBoitePortsUSBInterdit=std::atoi(Ligne);
	
		FichierConsignes.close();
	}
	
	SpinBoxCSB->setValue((int) ZoneImage->ConsigneSeuilBas());
	SpinBoxCSH->setValue((int) ZoneImage->ConsigneSeuilHaut());
	SpinBoxCPuiLUT->setValue(ZoneImage->PuissanceLUT());
	ParamPixmapBoutonDialogPortsUSB();
	
	// Connexion du signal valueChanged() envoye par les QSpinBox aux slots respectifs SlotSpinBoxVC_() de l'objet
	//
	connect(SpinBoxCSB,SIGNAL(valueChanged(int)),this,SLOT(SlotSpinBoxCSB(int)));
	connect(SpinBoxCSH,SIGNAL(valueChanged(int)),this,SLOT(SlotSpinBoxCSH(int)));
	connect(SpinBoxCPuiLUT,SIGNAL(valueChanged(int)),this,SLOT(SlotSpinBoxCPuiLUT(int)));
}


// Destructeur de l'objet de la fenetre principale
//
TerminOA::~TerminOA()
{
	// Sauvegarde des consignes
	//
	QFile FichierConsignes(CheminRepTerminal+"/"+FICHIER_SAUV_CONSIGNES);
	
	if( FichierConsignes.open(IO_WriteOnly) )
	{
		QTextStream FluxTexte(&FichierConsignes);	// Flux en mode texte
		QString Ligne;					// Une chaine qui sera une ligne du fichier
		
		FichierConsignes.at(0);
		
		// Sauvegarde de la consigne du seuil bas de visualisation
		//
		Ligne=QString("%1\n").arg(ZoneImage->ConsigneSeuilBas());
		FluxTexte << Ligne;
	
		// Sauvegarde de la consigne du seuil haut de visualisation
		//
		Ligne=QString("%1\n").arg(ZoneImage->ConsigneSeuilHaut());
		FluxTexte << Ligne;
	
		// Sauvegarde de la consigne de la puissance de la LUT de visualisation
		//
		Ligne=QString("%1\n").arg(ZoneImage->PuissanceLUT());
		FluxTexte << Ligne;
		
		// Sauvegarde du type de la palette utilisee
		//
		Ligne=QString("%1\n").arg((int) ZoneImage->TypePalette());
		FluxTexte << Ligne;
		
		// Sauvegarde du type d'echelle de l'histogramme
		//
		Ligne=QString("%1\n").arg((int) ZoneImage->TypeEchelleHistogramme());
		FluxTexte << Ligne;
		
		// Sauvegarde de la zone affichee
		//
		Ligne=QString("%1\n").arg((int) ZoneImage->ZoneAfficheeIC());
		FluxTexte << Ligne;
		
		// Sauvegarde du drapeau d'interdiction de l'affichage du dialogue de l'etat des ports USB
		//
		Ligne=QString("%1\n").arg(AffichageBoitePortsUSBInterdit);
		FluxTexte << Ligne;
		
		FichierConsignes.close();
	}
	
	// Arret du timer de pulsation de la seconde de temps
	//
	Pulsar1s->stop();
	
	// Destruction des objets
	//
/*	delete BoitePortsUSB;
	delete Pulsar1s;
	delete SpinBoxCSB;
	delete SpinBoxCSH;
	delete SpinBoxCPuiLUT;
	delete BoutonModeDiapo;
	delete BoutonModeIC;
	delete BoutonEchLog;
	delete BoutonEchLin;
	delete BoutonPalNB;
	delete BoutonPalHalpha;
	delete BoutonPalNoirOrangeBlanc;
	delete BoutonPalNoirVertBlanc;
	delete BoutonPalNoirBleuBlanc;
	delete BoutonZ0;
	delete BoutonZ1;
	delete BoutonZ2;
	delete BoutonZ3;
	delete BoutonZ4;
	delete BoutonZ5;
	delete BoutonZ6;
	delete BoutonZ7;
	delete BoutonZ8;
	delete BoutonZ9;
	delete BoutonDialogPortsUSB;
	delete LabelHeureUTBarreStatus;
	//delete LabelInfoBarreStatus;
	delete BarreStatus;
	delete ZoneImage;
	delete BoiteRangementVertical;
*/
}


// Retourne l'ID aleatoire du TerminOA en cours d'execution
//
QString TerminOA::IDTerminOA(void)
{
	return ChaineIDAleaTerminOA;
}


// Surcharge de la fonction de slot paintEvent heritee du QWidget
//
void TerminOA::paintEvent(QPaintEvent *event)
{
	QPaintEvent *e;
	e=event;

	// On redimensionne la boite de rangement verticale
	//
	BoiteRangementVertical->resize(this->size());
}


// Slot du signal clicked() sur le bouton de passage en mode d'affichage de l'image courante
//
void TerminOA::SlotBoutonModeIC(void)
{
	EtatTerminOA=EtatVisualisationCourante;			// TerminOA passe en mode visualisation image courante
	
	CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
	
	ZoneImage->EtatAffichage(AffichageImageCourante);	// On parametre l'objet de la zone image
	
	ZoneImage->update();					// On demande a l'objet zone image de ce mettre a jour
}


// Slot du signal clicked() sur le bouton de passage en mode diaporama
//
void TerminOA::SlotBoutonModeDiapo(void)
{
	CompteurInactiviteSec=DeclanchementModeDiaporamaSec;	// On simule un temps d'inactivite depasse
	
	SlotPulsar1s();						// Appel du slot Pulsar1s qui traite le basculement en mode diaporama
}


// Slot pour le passage en echelle de representation histogramme lineaire
//
void TerminOA::SlotBoutonEchLin(void)
{
	ZoneImage->TypeEchelleHistogramme(Lineaire);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en echelle de representation histogramme logarithmique
//
void TerminOA::SlotBoutonEchLog(void)
{
	ZoneImage->TypeEchelleHistogramme(Logarithmique);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en palette noir et blanc
//
void TerminOA::SlotBoutonPalNB(void)
{
	ZoneImage->TypePalette(NoirEtBlanc);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en palette h-alpha
//
void TerminOA::SlotBoutonPalHalpha(void)
{
	ZoneImage->TypePalette(Halpha);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en palette NoirOrangeBlanc
//
void TerminOA::SlotBoutonPalNoirOrangeBlanc(void)
{
	ZoneImage->TypePalette(NoirOrangeBlanc);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en palette NoirVertBlanc
//
void TerminOA::SlotBoutonPalNoirVertBlanc(void)
{
	ZoneImage->TypePalette(NoirVertBlanc);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en palette NoirBleuBlanc
//
void TerminOA::SlotBoutonPalNoirBleuBlanc(void)
{
	ZoneImage->TypePalette(NoirBleuBlanc);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z0
//
void TerminOA::SlotBoutonZ0(void)
{
	ZoneImage->ZoneAfficheeIC(Z0);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z1
//
void TerminOA::SlotBoutonZ1(void)
{
	ZoneImage->ZoneAfficheeIC(Z1);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z2
//
void TerminOA::SlotBoutonZ2(void)
{
	ZoneImage->ZoneAfficheeIC(Z2);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z3
//
void TerminOA::SlotBoutonZ3(void)
{
	ZoneImage->ZoneAfficheeIC(Z3);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z4
//
void TerminOA::SlotBoutonZ4(void)
{
	ZoneImage->ZoneAfficheeIC(Z4);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z5
//
void TerminOA::SlotBoutonZ5(void)
{
	ZoneImage->ZoneAfficheeIC(Z5);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z6
//
void TerminOA::SlotBoutonZ6(void)
{
	ZoneImage->ZoneAfficheeIC(Z6);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z7
//
void TerminOA::SlotBoutonZ7(void)
{
	ZoneImage->ZoneAfficheeIC(Z7);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z8
//
void TerminOA::SlotBoutonZ8(void)
{
	ZoneImage->ZoneAfficheeIC(Z8);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour le passage en visualisation zone Z9
//
void TerminOA::SlotBoutonZ9(void)
{
	ZoneImage->ZoneAfficheeIC(Z9);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
		
		ZoneImage->update();
	}
}


// Slot pour afficher ou non le dialogue d'affichage de l'etat des ports USB
//
void TerminOA::SlotBoutonDialogPortsUSB(void)
{
	if( AffichageBoitePortsUSBInterdit == true ) AffichageBoitePortsUSBInterdit=false; else AffichageBoitePortsUSBInterdit=true;
	
	ParamPixmapBoutonDialogPortsUSB();
}


// Slot pour le changement de la valeur de la spinbox de la consigne du seuil bas de visualisation
//
void TerminOA::SlotSpinBoxCSB(int value)
{
	ZoneImage->ConsigneSeuilBas(value);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
	
		ZoneImage->update();
	}
}


// Slot pour le changement de la valeur de la spinbox de la consigne du seuil haut de visualisation
//
void TerminOA::SlotSpinBoxCSH(int value)
{
	ZoneImage->ConsigneSeuilHaut(value);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
	
		ZoneImage->update();
	}
}


// Slot pour le changement de la valeur de la spinbox de la consigne de la puissance de la lut de visualisation
//
void TerminOA::SlotSpinBoxCPuiLUT(int value)
{
	ZoneImage->PuissanceLUT(((double) value)/100.0);
	
	if( EtatTerminOA != EtatVisualisationCourante )
	{
		SlotBoutonModeIC();
	}
	else
	{
		CompteurInactiviteSec=0;				// Le terminal vient d'etre actif
	
		ZoneImage->update();
	}
}


// Slot du signal timeout() du QTimer de pulsation de la seconde de temps
//
void TerminOA::SlotPulsar1s(void)
{
	// On ajoute la seconde d'inactivite
	//
	CompteurInactiviteSec++;
	
	// Affichage de l'heure UT dans la barre de status
	//
	AfficherHeureUT();
	
	// Si le temps d'inactivite depasse ou egal le temps de declanchement du mode diaporama
	//
	if( CompteurInactiviteSec >= DeclanchementModeDiaporamaSec )
	{
		// Si le mode courant est different du mode diaporama
		//
		if( EtatTerminOA != EtatDiaporama )
		{
			EtatTerminOA=EtatDiaporama;				// Cet objet passe en mode diaporama
			ZoneImage->EtatAffichage(AffichageImageDiaporama);	// Le widget de la zone image passe en mode diaporama
			CompteurInterDiapositiveSec=0;
		}
		else
		{
			if( CompteurInterDiapositiveSec >= TempsEntreDiapositive ) CompteurInterDiapositiveSec=0; else CompteurInterDiapositiveSec++;
		}
		
		// Si on doit afficher une nouvelle diapositive
		//
		if( CompteurInterDiapositiveSec == 0 )
		{
			QString Numero;			// Composition du numero de l'image courante
			QString CheminDiapo;	// Chemin complet vers le fichier
			
			if( CompteurDiapositivesProjetees >= PeriodeRappelEcranLancementDiaporama )
			{
				// Toutes les n diapositives projetees on affiche l'ecran de lancement de TerminOA
				//
				CompteurDiapositivesProjetees=0;
				
				CheminDiapo=CheminRepTerminal+"/"+FICHIER_ECRAN_LANCEMENT;
				
				//if( MsgSonoreVousPouvezConnecterVotreCle.available() ) if ( MsgSonoreVousPouvezConnecterVotreCle.isFinished() ) MsgSonoreVousPouvezConnecterVotreCle.play();
			}
			else
			{
				// Composition du chemin vers le fichier de l'image de diapositive courante
				//
				Numero=QString("%1").arg(NumeroDiapositiveCourante,4);
				Numero.replace(" ","0");
				
				CheminDiapo=CheminRepDiaporama+"/DIAPO_"+Numero+".png";
			
				// La prochaine fois on affichera une autre image
				//
				NumeroDiapositiveCourante++;
				
				// On a projete une image de plus
				//
				CompteurDiapositivesProjetees++;
			}
		
			// Si le fichier existe bien
			//
			if( QFile(CheminDiapo).exists() )
			{
				// Si le fichier existe alors on parametre le widget de la zone image
				//
				ZoneImage->ImageDiaporama(CheminDiapo);
			}
			else
			{
				// Le fichier n'existe pas alors on revient au premier
				//
				NumeroDiapositiveCourante=0;
				
				Numero=QString("%1").arg(NumeroDiapositiveCourante,4);
				Numero.replace(" ","0");
				
				CheminDiapo=CheminRepDiaporama+"/DIAPO_"+Numero+".png";
				
				// Si le fichier existe bien
				//
				if( QFile(CheminDiapo).exists() )
				{
					// Si le fichier existe alors on parametre le widget de la zone image
					//
					ZoneImage->ImageDiaporama(CheminDiapo);
			
					// La prochaine fois on affichera une autre image
					//
					NumeroDiapositiveCourante++;
				}
				else
				{
					// Si le premier fichier n'existe pas alors on affiche l'image de lancement
					//
					ZoneImage->ImageDiaporama(CheminRepTerminal+"/"+FICHIER_ECRAN_LANCEMENT);
				}	
			}
			
			// On demande au widget ZoneImage de se mettre a jour
			//
			ZoneImage->update();
		}
	}
	
	// Si on est en visualisation de l'image courante
	//
/*	if( EtatTerminOA == EtatVisualisationCourante )
	{
		QString Texte;
		
		//Texte=QString("S(%1,%2)^(%3)").arg(ZoneImage->ValSeuilBas()).arg(ZoneImage->ValSeuilHaut()).arg(((double)ZoneImage->PuissanceLUT())/100.0,4,'f',2);
		
		//LabelInfoBarreStatus->setText(Texte);
	}
*/	

	// Si la copie de documents est permise
	//
	if( CopieDocumentPermise )
	{
		// On test la presence des cles USB pour diffusion des documents
		//
		for( int i=0; i < NB_PORTS_USB; i++ )
		{
			// Si on est dans un etat ou on peut tester la presence d'une cle USB
			//
			if( USB[i] == AttenteIntroduction && !FnCopieUSB[i].running() )
			{
				// Chemin vers le point de montage de la cle USB courante
				//
				QString CheminCle=CheminMntClesUSB+QString("/usb%1").arg(i+1);
			
				// Si la cle est montee car le repertoire de montage existe et est valide
				//
				if( QDir(CheminCle+"/.").exists() )
				{
					// Composition du nom du fichier de la trace de diffusion TerminOA
					//
					QString FichierTraceDiffusionTerminOA=CheminCle+REPERTOIRE_DIFFUSION+"/"+IDTerminOA();

					// Si le fichier de trace de diffusion TerminOA n'est pas present sur la cle
					//
					if( !QFile(FichierTraceDiffusionTerminOA).exists() )
					{
						// Aucune diffusion sur cette cle donc on peut passer a l'etat de diffusion
						//
						EtatUSB(i,CleMontee);			// On passe en etat cle montee

						//std::cout << "TerminOA: Cle USB" << i+1 << " detectee." << std::endl;

						// On demarre le processus leger (thread) de copie des documents
						//
						FnCopieUSB[i].start();
					}
				}
			}
			else
			{
				TempsEtatUSB[i]++;				// On incremente le temps de l'etat courant

				// Si les documents sont copies et le processus leger termine
				//
				if( USB[i] == CleDocumentsCopies && !FnCopieUSB[i].running() )
				{
					// Alors on passe dans l'etat d'attente de l'introduction d'une cle USB
					//
					EtatUSB(i,AttenteIntroduction);	// On passe en etat attente d'introduction

					//std::cout << "TerminOA: Cle USB" << i+1 << " retiree." << std::endl;
				}
			}
			//cout << "Etat" << i << " " << USB[i] << std::endl;
		}
	
		// Si il n'est pas interdit d'afficher le dialogue de l'etat des ports USB
		//
		if( !AffichageBoitePortsUSBInterdit )
		{
			int activite=false;	// Pour savoir si les ports ont une activite
		
			// Y a t il une activite sur un port ?
			//
			for( int i=0; i < NB_PORTS_USB; i++ )
			{
				if( USB[i] >= CleMontee )
				{
					activite=TRUE;
					break;
				}
			}
		
			if( activite ) if( !BoitePortsUSB->isVisible() ) BoitePortsUSB->show();
			if( !activite ) if( BoitePortsUSB->isVisible() ) BoitePortsUSB->hide();
		}
		else
		{
			if( BoitePortsUSB->isVisible() ) BoitePortsUSB->hide();
		}
	}
}


// Fonction d'affichage de l'heure UT dans la barre de status
//
void TerminOA::AfficherHeureUT(void)
{
	// On recupere l'heure courante UT du systeme
	//
	QTime HeureUT(QTime::currentTime(Qt::UTC));
	
	// Composition de la chaine de l'heure
	//
	QString ChaineHeureUT=QString("%1:%2:%3 UT").arg(HeureUT.hour(),2).arg(HeureUT.minute(),2).arg(HeureUT.second(),2);
	if( ChaineHeureUT.mid(0,1) == QString(" ") ) ChaineHeureUT.replace(0,1,"0");
	if( ChaineHeureUT.mid(3,1) == QString(" ") ) ChaineHeureUT.replace(3,1,"0");
	if( ChaineHeureUT.mid(6,1) == QString(" ") ) ChaineHeureUT.replace(6,1,"0");
	
	// Changement du texte du label
	//
	LabelHeureUTBarreStatus->setText(ChaineHeureUT);
}


// Fonction de chargement de l'image FITS courante
//
int TerminOA::ChargerFITSCourante(void)
{
	MutexAccesFichierFITSCourant.lock();
	
	int retour=ZoneImage->ChargerFITSCourante(CheminRepTerminal);
	
	MutexAccesFichierFITSCourant.unlock();
	
	return retour;
}


// Fonction de parametrage de l'etat d'un port USB
//
void TerminOA::EtatUSB(int i,EtatsPortsUSB etat)
{
	USB[i]=etat;		// On parametre l'etat
	TempsEtatUSB[i]=0;	// On vient de passer dans cet etat
	
	switch( USB[i] )
	{
		case AttenteIntroduction:
			RepLedDialoguePortsUSB(i,0,50,0);
			RepProgBarDialoguePortsUSB(i,0);
			RepProgBarDialoguePortsUSB(i,-1);
			AffOrdreDialoguePortsUSB(QString("<p align=""center""><font size=""+3"" face=""par defaut"" color=""#ffffff"">Ins&eacute;rez votre cl&eacute; USB<br>Plug your USB keydrive</font></p>"));
			break;
			
		case CleMontee:
			RepLedDialoguePortsUSB(i,0,150,0);
			RepProgBarDialoguePortsUSB(i,-1);
			AffOrdreDialoguePortsUSB(QString(""));
			AffLogDialoguePortsUSB(QString("Port%1 : cle montee / keydrive mounted.").arg(i+1));
			break;
			
		case CleCopieDocuments:
			RepProgBarDialoguePortsUSB(i,-1);
			AffLogDialoguePortsUSB(QString("Port%1 : copie des documents demarree / file copy starting.").arg(i+1));
			break;
			
		case CleDocumentsCopies:
			RepLedDialoguePortsUSB(i,0,255,0);
			RepProgBarDialoguePortsUSB(i,100);
			AffLogDialoguePortsUSB(QString("Port%1 : copie des documents TERMINEE / file copy FINISHED.").arg(i+1));
			AffOrdreDialoguePortsUSB(QString("<p align=""center""><font size=""+3"" face=""par defaut"" color=""#ffff00"">RETIREZ votre cl&eacute; USB<br>UNPLUG your USB keydrive</font></p>"));
			break;
	}
}


// Fonction de parametrage du pixmap du bouton affichage dialogue etats ports USB
//
void TerminOA::ParamPixmapBoutonDialogPortsUSB(void)
{
	if( AffichageBoitePortsUSBInterdit )
	{
		BoutonDialogPortsUSB->setPixmap(QPixmap(IconCleUSB_xpm));
	}
	else
	{
		BoutonDialogPortsUSB->setPixmap(QPixmap(IconCleUSBBarre_xpm));
	}
}


// Affichage d'un ordre dans le dialogue de l'etat des ports USB
//
void TerminOA::AffOrdreDialoguePortsUSB(QString chaine)
{
	BoitePortsUSB->textLabelOrdre->setText(chaine);
}


// Affichage d'un log dans le dialogue de l'etat des ports USB
//
void TerminOA::AffLogDialoguePortsUSB(QString chaine)
{
	// On recupere l'heure courante UT du systeme
	//
	QTime Heure(QTime::currentTime(Qt::LocalTime));
	
	QString ChaineHeure=QString("%1:%2:%3-").arg(Heure.hour(),2).arg(Heure.minute(),2).arg(Heure.second(),2);
	if( ChaineHeure.mid(0,1) == QString(" ") ) ChaineHeure.replace(0,1,"0");
	if( ChaineHeure.mid(3,1) == QString(" ") ) ChaineHeure.replace(3,1,"0");
	if( ChaineHeure.mid(6,1) == QString(" ") ) ChaineHeure.replace(6,1,"0");
	
	BoitePortsUSB->textEdit1->append(ChaineHeure+chaine);
}


// Representation d'un pourcentage dans le dialogue de l'etat des ports USB
//
// CE:	On passe le numero du port USB ;
//
//	On passe la valeur de 0 �100 (-1 pour non representation de la progression)
//
void TerminOA::RepProgBarDialoguePortsUSB(int i,int val)
{
	switch( i )
	{
		case 0:	BoitePortsUSB->progressBarPort1->setProgress(val);	break;
		case 1:	BoitePortsUSB->progressBarPort2->setProgress(val);	break;
		case 2:	BoitePortsUSB->progressBarPort3->setProgress(val);	break;
		case 3:	BoitePortsUSB->progressBarPort4->setProgress(val);	break;
	}
}


// Representation de la couleur d'une LED dans le dialogue de l'etat des ports USB
//
// CE:	On passe le numero du port USB ;
//
//	On passe les doses de Rouge, de Vert et de Bleu ;
//
void TerminOA::RepLedDialoguePortsUSB(int i,int r,int v,int b)
{
	switch( i )
	{
		case 0:	BoitePortsUSB->kLedPort1->setColor(QColor(r,v,b));	break;
		case 1:	BoitePortsUSB->kLedPort2->setColor(QColor(r,v,b));	break;
		case 2:	BoitePortsUSB->kLedPort3->setColor(QColor(r,v,b));	break;
		case 3:	BoitePortsUSB->kLedPort4->setColor(QColor(r,v,b));	break;
	}
}


// Retourne le chemin vers le repertoire de base du terminal
//
QString TerminOA::CheminRepertoireTerminal(void)
{
	return CheminRepTerminal;
}


// Retourne le chemin vers le repertoire de base des documents diffuses
//
QString TerminOA::CheminRepertoireDocuments(void)
{
	return CheminRepDocuments;
}


// On passe en mode affichage de l'image courante
//
void TerminOA::PasserEnAffichageImageCourante(void)
{
	SlotBoutonModeIC();
}


// Surcharge de la fonction de handler des evenements particuliers crees et adresses pour TerminOA
//
void TerminOA::customEvent(QCustomEvent *ce)
{
	// On aiguille selon le type d'evenement particulier recu par TerminOA
	//

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_ETAT_USB )
	{
		// Evenement de parametrage de l'etat du port USB
		//
		CEventTerminOA_EtatUSB *event=(CEventTerminOA_EtatUSB *) ce;	// Typage de l'evenement

		EtatUSB(event->Lequel(),event->Etat());

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_REPBARPROGUSB )
	{
		// Evenement de modification de la valeur d'une barre de progression
		//
		CEventTerminOA_RepProgBarDialoguePortsUSB *event=(CEventTerminOA_RepProgBarDialoguePortsUSB *) ce;

		RepProgBarDialoguePortsUSB(event->Lequel(),event->Valeur());

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_AFFLOGUSB )
	{
		// Evenement d'affichage d'un log dans le widget BoitePortsUSB
		//
		CEventTerminOA_AffLogDialoguePortsUSB *event=(CEventTerminOA_AffLogDialoguePortsUSB *) ce;

		AffLogDialoguePortsUSB(event->Valeur());

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_AFFORDUSB )
	{
		// Evenement d'affichage d'un log dans le widget BoitePortsUSB
		//
		CEventTerminOA_AffOrdreDialoguePortsUSB *event=(CEventTerminOA_AffOrdreDialoguePortsUSB *) ce;

		AffOrdreDialoguePortsUSB(event->Valeur());

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_CHRGAFFIMGCOUR )
	{
		// Evenement de chargement et d'affichage de l'image FITS courante
		//
		ChargerFITSCourante();
		SlotBoutonZ0();				// On l'affiche systematiquement en plein ecran

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_MODEIC )
	{
		// Evenement de passage en mode affichage de l'image courante
		//
		SlotBoutonModeIC();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_MODEDIAPO )
	{
		// Evenement de passage en mode diaporama
		//
		SlotBoutonModeDiapo();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_ECHLIN )
	{
		// Evenement de passage en mode echelle lineaire
		//
		SlotBoutonEchLin();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_ECHLOG )
	{
		// Evenement de passage en mode echelle logarithmique
		//
		SlotBoutonEchLog();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_PALNB )
	{
		// Evenement de passage en palette noir et blanc
		//
		SlotBoutonPalNB();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_PALHALPHA )
	{
		// Evenement de passage en palette h-alpha
		//
		SlotBoutonPalHalpha();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_PALNOB )
	{
		// Evenement de passage en palette noir-orange-blanc
		//
		SlotBoutonPalNoirOrangeBlanc();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_PALNVB )
	{
		// Evenement de passage en palette noir-vert-blanc
		//
		SlotBoutonPalNoirVertBlanc();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_PALNBB )
	{
		// Evenement de passage en palette noir-bleu-blanc
		//
		SlotBoutonPalNoirBleuBlanc();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z0 )
	{
		// Evenement de passage en zoom de la partie 0
		//
		SlotBoutonZ0();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z1 )
	{
		// Evenement de passage en zoom de la partie 1
		//
		SlotBoutonZ1();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z2 )
	{
		// Evenement de passage en zoom de la partie 2
		//
		SlotBoutonZ2();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z3 )
	{
		// Evenement de passage en zoom de la partie 3
		//
		SlotBoutonZ3();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z4 )
	{
		// Evenement de passage en zoom de la partie 4
		//
		SlotBoutonZ4();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z5 )
	{
		// Evenement de passage en zoom de la partie 5
		//
		SlotBoutonZ5();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z6 )
	{
		// Evenement de passage en zoom de la partie 6
		//
		SlotBoutonZ6();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z7 )
	{
		// Evenement de passage en zoom de la partie 7
		//
		SlotBoutonZ7();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z8 )
	{
		// Evenement de passage en zoom de la partie 8
		//
		SlotBoutonZ8();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_Z9 )
	{
		// Evenement de passage en zoom de la partie 9
		//
		SlotBoutonZ9();

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_CSB )
	{
		// Evenement de changement de la consigne de seuil bas
		//
		CEventTerminOA_CSB *event=(CEventTerminOA_CSB *) ce;
		SpinBoxCSB->setValue(event->Valeur());
		SlotSpinBoxCSB(event->Valeur());

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_CSH )
	{
		// Evenement de changement de la consigned de seuil haut
		//
		CEventTerminOA_CSH *event=(CEventTerminOA_CSH *) ce;
		SpinBoxCSH->setValue(event->Valeur());
		SlotSpinBoxCSH(event->Valeur());

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_CPUILUT )
	{
		// Evenement de changement de la consigne de puissance de la LUT de visualisation
		//
		CEventTerminOA_CPuiLUT *event=(CEventTerminOA_CPuiLUT *) ce;
		SpinBoxCPuiLUT->setValue(event->Valeur());
		SlotSpinBoxCPuiLUT(event->Valeur());

		return;
	}

	if( ce->type() == ID_CUSTOM_EVENT_TERMINOA_QUIT )
	{
		// Evenement de demande d'arret du programme TerminOA
		//
		appli->closeAllWindows();
		appli->quit();

		return;
	}
}


// Les evenements particuliers definis pour TerminOA ----------------------------------------------------------------------------

// L'evenement de parametrage de l'etat d'un port USB
//
CEventTerminOA_EtatUSB::CEventTerminOA_EtatUSB(int i,EtatsPortsUSB etat) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_ETAT_USB)
{
	// Stockage des parametres dans l'objet d'evenement
	//
	n=i;
	etatn=etat;
}

int CEventTerminOA_EtatUSB::Lequel(void)
{
	return n;
}

EtatsPortsUSB CEventTerminOA_EtatUSB::Etat(void)
{
	return etatn;
}


// L'evenement de modification de la valeur de la barre de progression d'un port USB
//
CEventTerminOA_RepProgBarDialoguePortsUSB::CEventTerminOA_RepProgBarDialoguePortsUSB(int i,int v) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_REPBARPROGUSB)
{
	// Stockage des parametres dans l'objet d'evenement
	//
	n=i;
	valeur=v;
}

int CEventTerminOA_RepProgBarDialoguePortsUSB::Lequel(void)
{
	return n;
}

int CEventTerminOA_RepProgBarDialoguePortsUSB::Valeur(void)
{
	return valeur;
}

// L'evenement d'affichage d'un log dans le widget BoitePortsUSB
//
CEventTerminOA_AffLogDialoguePortsUSB::CEventTerminOA_AffLogDialoguePortsUSB(QString c) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_AFFLOGUSB)
{
	// Stockage des parametres dans l'objet d'evenement
	//
	chaine=c;
}

QString CEventTerminOA_AffLogDialoguePortsUSB::Valeur(void)
{
	return chaine;
}

// L'evenement d'affichage d'un ordre dans le widget BoitePortsUSB
//
CEventTerminOA_AffOrdreDialoguePortsUSB::CEventTerminOA_AffOrdreDialoguePortsUSB(QString c) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_AFFORDUSB)
{
	// Stockage des parametres dans l'objet d'evenement
	//
	chaine=c;
}

QString CEventTerminOA_AffOrdreDialoguePortsUSB::Valeur(void)
{
	return chaine;
}

// Nouvelle consigne de seuil bas
//
CEventTerminOA_CSB::CEventTerminOA_CSB(int v) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_CSB)
{
	// Stockage des parametres dans l'objet d'evenement
	//
	valeur=v;
}

int CEventTerminOA_CSB::Valeur(void)
{
	return valeur;
}

// Nouvelle consigne de seuil haut
//
CEventTerminOA_CSH::CEventTerminOA_CSH(int v) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_CSH)
{
	// Stockage des parametres dans l'objet d'evenement
	//
	valeur=v;
}

int CEventTerminOA_CSH::Valeur(void)
{
	return valeur;
}

// Nouvelle consigne de puissance de la LUT
//
CEventTerminOA_CPuiLUT::CEventTerminOA_CPuiLUT(int v) : QCustomEvent(ID_CUSTOM_EVENT_TERMINOA_CPUILUT)
{
	// Stockage des parametres dans l'objet d'evenement
	//
	valeur=v;
}

int CEventTerminOA_CPuiLUT::Valeur(void)
{
	return valeur;
}


// - Thread ---------------------------------------------------------------------------------------------------------------------

// Constructeur du thread de copie des documents sur cle USB
//
ProcessusLegerCopieDocuments::ProcessusLegerCopieDocuments()
{
}


// Fonction de parametrage du numero du processus leger
//
void ProcessusLegerCopieDocuments::ParamNumProcessusLeger(int i)
{
	n=i;
}


// Fonction de parametrage du chemin vers le repertoire de base du terminal
//
void ProcessusLegerCopieDocuments::ParamCheminRepTerminal(QString chemin)
{
	CheminRepTerminal=chemin;
}


// Fonction de parametrage du chemin vers le repertoire des documents a transferer
//
void ProcessusLegerCopieDocuments::ParamCheminDocuments(QString chemin)
{
	CheminDocuments=chemin;
}


// Fonction de parametrage du chemin vers le point de montage de la cle USB
//
void ProcessusLegerCopieDocuments::ParamCheminCleUSB(QString chemin)
{
	CheminCleUSB=chemin;
}


// Surcharge de la methode run() qui contient le code d'execution du thread de copie des documents diffuses par TerminOA
//
// ATTENTION : QT3 n'est pas thread-safe (c'est a dire que les fonctions ne peuvent pas etre appellees par plusieurs threads a la fois).
//		Les widgets doivent etre modifies uniquement par l'application maitre qui gere la boucle evenementielle. Donc un thread
//		qui doit modifier un widget de l'application doit communiquer par un evenement particulier QCustomEvent via la fontion
//		thread-safe QApplication::postEvent(). Il ne doit pas modifier directement le widget sinon l'application plantera.
//
void ProcessusLegerCopieDocuments::run()
{
	QString CheminRepOA;	// Chemin vers le repertoire de transfert
	QDir Repertoire;	// Un objet repertoire

	//std::cout << "Demarrage de la copie :" << CheminDocuments.ascii() << " vers " << CheminCleUSB.ascii() << std::endl;


	// On passe l'etat du port USB en mode copie des documents en envoyant un evenement au thread principal de la boucle evenementielle
	//
	CEventTerminOA_EtatUSB *ce_copiedoc=new CEventTerminOA_EtatUSB(n,CleCopieDocuments);	// ce_copiedoc sera detruit par Qt
	QApplication::postEvent(FPTerminOA,ce_copiedoc);


	// On cree la liste des repertoires et sous repertoires des documents a diffuser
	//
	QDir RepertoireDocuments(CheminDocuments);			// Objet repertoire racine des documents a diffuser
	
	QStringList ListeRepertoiresADiffuser;				// Objet liste des repertoires a diffuser
	
	ListeRepertoiresADiffuser+=RepertoireDocuments.entryList("*",QDir::Dirs);	// Ajout de la liste des repertoires dans la racine
	
	// On ajoute le chemin complet au nom des repertoires
	//
	for( QStringList::Iterator rc=ListeRepertoiresADiffuser.begin(); rc != ListeRepertoiresADiffuser.end(); rc++ ) if( *rc != "." && *rc != ".." ) *rc=RepertoireDocuments.path()+"/"+*rc;
	
	// On ajoute tous les sous repertoires recursivement a la liste
	//
	QStringList::Iterator rc=ListeRepertoiresADiffuser.begin();
	
	while( rc != ListeRepertoiresADiffuser.end() )
	{
		// Si il ne s'agit pas d'un repertoire courant ou d'un repertoire au dessus
		//
		if( *rc != "." && *rc != ".." )
		{
			// Nombre d'elements de la liste avant l'ajout potentiel de nouveaux repertoires
			//
			QStringList::size_type taa=ListeRepertoiresADiffuser.size();
			
			// Dernier element de la liste avant ajout potentiel
			//
			QStringList::Iterator dernieraa=ListeRepertoiresADiffuser.fromLast();	
			
			// Repertoire courant a analyser pour completer la liste
			//
			QDir RepC(*rc);
			
			// On ajoute les sous repertoires du repertoire courant a la liste des repertoires a diffuser
			//
			ListeRepertoiresADiffuser+=RepC.entryList("*",QDir::Dirs);
			
			// Nombre d'elements de la liste apres l'ajout de nouveaux repertoires
			//
			QStringList::size_type tapa=ListeRepertoiresADiffuser.size();
			
			// On compose le chemin complet pour les nouveaux repertoires ajoutes par la derniere requete
			//
			if( taa == 0 )
			{
				// Il n'y avait aucune entree donc on partira du depart
				//
				dernieraa=ListeRepertoiresADiffuser.begin();
			}
			
			// Si la requete a ajoutee des repertoires a la liste
			//
			if( taa != tapa )
			{
				// On partira du premier ajout si la liste contenait un membre
				//
				if( taa != 0 ) dernieraa++;
				
				// Tant qu'on trouve des repertoires ajoutes, on compose le chemin complet
				//
				while( dernieraa != ListeRepertoiresADiffuser.end() )
				{
					if( *dernieraa != "." && *dernieraa != ".." )
					{
						*dernieraa=*rc+"/"+*dernieraa;
					}
					dernieraa++;
				}
			}
		}
		
		// Repertoire suivant de la liste
		//
		rc++;
	}
	
	// On ajoute le repertoire racine
	//
	ListeRepertoiresADiffuser+=CheminDocuments;
	
	
	// Composition de la liste des fichiers a diffuser
	//
	QStringList ListeFichiersADiffuser;
	
	for( QStringList::Iterator rc=ListeRepertoiresADiffuser.begin(); rc != ListeRepertoiresADiffuser.end(); rc++ )
	{
		// Si il ne s'agit pas d'un repertoire courant ou d'un repertoire au dessus
		//
		if( *rc != "." && *rc != ".." )
		{
			// Nombre d'elements de la liste avant l'ajout potentiel de nouveaux fichiers
			//
			QStringList::size_type taa=ListeFichiersADiffuser.size();
			
			// Dernier element de la liste avant ajout potentiel
			//
			QStringList::Iterator dernieraa=ListeFichiersADiffuser.fromLast();
			
			// Repertoire courant a analyser pour completer la liste
			//
			QDir RepC(*rc);

			// Ajout des fichiers du repertoire courant a la liste des fichiers a diffuser
			//
			ListeFichiersADiffuser+=RepC.entryList("*",QDir::Files);
			
			// Nombre d'elements de la liste apres l'ajout de nouveaux fichiers
			//
			QStringList::size_type tapa=ListeFichiersADiffuser.size();
			
			// On compose le chemin complet pour les nouveaux fichiers ajoutes
			//
			if( taa == 0 )
			{
				// Il n'y avait aucune entree donc on partira du depart
				//
				dernieraa=ListeFichiersADiffuser.begin();
			}
			
			// Si la requete a ajoutee des fichiers a la liste
			//
			if( taa != tapa )
			{
				// On partira du premier ajout si la liste contenait un membre
				//
				if( taa != 0 ) dernieraa++;
				
				// Tant qu'on trouve des fichiers ajoutes, on compose le chemin complet
				//
				while( dernieraa != ListeFichiersADiffuser.end() )
				{
					*dernieraa=*rc+"/"+*dernieraa;
					dernieraa++;
				}
			}
		}
	}
	
	
	// Repertoire ou seront stockees les donnees diffusees sur le point de montage
	//
	CheminRepOA=CheminCleUSB+REPERTOIRE_DIFFUSION;
	
	// Si le repertoire n'existe pas on le cree
	//
	if( !QDir(CheminRepOA).exists() )
	{
		if( Repertoire.mkdir(CheminRepOA,TRUE) == FALSE )
		{
			// Impossible de creer le repertoire
			//
			CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 TerminOA: ERREUR: Impossible de creer le repertoire : ").arg(n+1)+CheminRepOA));
			QApplication::postEvent(FPTerminOA,celog);

			CEventTerminOA_AffOrdreDialoguePortsUSB *ceordre=new CEventTerminOA_AffOrdreDialoguePortsUSB(QString(ChaineErreurEcriturePortUSB));
			QApplication::postEvent(FPTerminOA,ceordre);

			//
			//if( MsgSonoreErreurDurantCopieFichiers.available() )
			//{
			//	if( MsgSonoreErreurDurantCopieFichiers.isFinished() ) MsgSonoreErreurDurantCopieFichiers.play();
			//	while( !MsgSonoreErreurDurantCopieFichiers.isFinished() );
			//}
			//else
			//{
			//	QApplication::beep();
			//}

			sleep(2*TEMPS_AUTODEMONTAGE_PORT_USB_SEC);

			CEventTerminOA_EtatUSB *ce_attente=new CEventTerminOA_EtatUSB(n,AttenteIntroduction);
			QApplication::postEvent(FPTerminOA,ce_attente);

			return;
		}
	}


	// On cree le fichier de trace de la date de diffusion du TerminOA
	//
	QString NomFichierTraceDiffusionTerminOA=CheminRepOA+"/"+FPTerminOA->IDTerminOA();

	QFile FichierTrace(NomFichierTraceDiffusionTerminOA);

	if( FichierTrace.open(IO_WriteOnly) )
	{
		// C'est un fichier vide, juste une trace
		//
		FichierTrace.flush();
		FichierTrace.close();
	}
	else
	{
		// Creation du fichier de trace impossible
		//
		CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 TerminOA: ERREUR: Impossible de creer le fichier: ").arg(n+1)+NomFichierTraceDiffusionTerminOA));
		QApplication::postEvent(FPTerminOA,celog);

		CEventTerminOA_AffOrdreDialoguePortsUSB *ceordre=new CEventTerminOA_AffOrdreDialoguePortsUSB(QString(ChaineErreurEcriturePortUSB));
		QApplication::postEvent(FPTerminOA,ceordre);

		sleep(2*TEMPS_AUTODEMONTAGE_PORT_USB_SEC);

		CEventTerminOA_EtatUSB *ce_attente=new CEventTerminOA_EtatUSB(n,AttenteIntroduction);
		QApplication::postEvent(FPTerminOA,ce_attente);

		return;
	}


	// Affichage dans le widget de l'etat de copie en cours
	//
	CEventTerminOA_AffOrdreDialoguePortsUSB *ceordrecpyencours=new CEventTerminOA_AffOrdreDialoguePortsUSB(QString("<p align=""center""><font size=""+3"" face=""par defaut"" color=""#ff00ff"">Copie des documents en cours<br>Copy in progress please wait</font></p>"));
	QApplication::postEvent(FPTerminOA,ceordrecpyencours);

	// Si l'interface sonore est disponible on joue le message sonore
	//
	//
	//if( MsgSonoreCopieFichiersEnCours.available() )
	//{
	//	QSound::play(CheminRepTerminal+"/"+MESSAGE_SONORE_COPIE_FICHIERS_EN_COURS);
	//	if( MsgSonoreCopieFichiersEnCours.isFinished() ) MsgSonoreCopieFichiersEnCours.play();
	//	while( !MsgSonoreCopieFichiersEnCours.isFinished() );
	//}


	// Composition de la liste des repertoires a creer
	//
	QStringList ListeRepertoiresACreer;
	
	uint ipcd=CheminDocuments.length();	// Indice du premier caractere du repertoire racine a diffuser dans les chemins complets
	
	for( QStringList::Iterator rc=ListeRepertoiresADiffuser.begin(); rc != ListeRepertoiresADiffuser.end(); rc++ )
	{
		// Si il ne s'agit pas d'un repertoire courant ou d'un repertoire au dessus
		//
		if( *rc != "." && *rc != ".." )
		{
			QString RepACreer;	// Chaine pour nom du repertoire a creer
			
			// Composition du nom complet du repertoire
			//
			RepACreer=CheminRepOA+QString(*rc).right(QString(*rc).length()-ipcd);
			
			// Ajout a la liste des repertoires a creer
			//
			ListeRepertoiresACreer+=RepACreer;
		}
	}
	
	
	// Nombre d'action a realiser pour diffuser : nombre de repertoires a creer + nombre de fichiers a copier
	//
	unsigned long nbactions=(unsigned long) ListeRepertoiresACreer.size();
	nbactions+=(unsigned long) ListeFichiersADiffuser.size();
	
	// Numero d'action actuel
	//
	unsigned long action=0;
	
	
	// Creation des repertoires sur le point de montage
	//
	for( QStringList::Iterator rc=ListeRepertoiresACreer.begin(); rc != ListeRepertoiresACreer.end(); rc++ )
	{
		// On realise une action
		//
		action++;
		
		// Le pourcentage dans la barre de progression
		//
		unsigned long pourcentage=action*100/nbactions;
		
		CEventTerminOA_RepProgBarDialoguePortsUSB *cepourcrearep=new CEventTerminOA_RepProgBarDialoguePortsUSB(n,pourcentage);
		QApplication::postEvent(FPTerminOA,cepourcrearep);

		// Si le repertoire n'existe pas
		//
		if( !QDir(*rc).exists() )
		{
			// On le cree
			//
			if( Repertoire.mkdir(*rc,TRUE) == FALSE )
			{
				// Impossible de creer le repertoire
				//
				CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 TerminOA: ERREUR: Impossible de creer le repertoire : ").arg(n+1)+*rc));
				QApplication::postEvent(FPTerminOA,celog);

				CEventTerminOA_AffOrdreDialoguePortsUSB *ceordre=new CEventTerminOA_AffOrdreDialoguePortsUSB(QString(ChaineErreurEcriturePortUSB));
				QApplication::postEvent(FPTerminOA,ceordre);

				//if( MsgSonoreErreurDurantCopieFichiers.available() )
				//{
				//	if( MsgSonoreCopieFichiersEnCours.isFinished() ) MsgSonoreErreurDurantCopieFichiers.play();
				//	while( !MsgSonoreErreurDurantCopieFichiers.isFinished() );
				//}
				//else
				//{
				//	QApplication::beep();
				//}

				sleep(2*TEMPS_AUTODEMONTAGE_PORT_USB_SEC);

				CEventTerminOA_EtatUSB *ce_attente=new CEventTerminOA_EtatUSB(n,AttenteIntroduction);
				QApplication::postEvent(FPTerminOA,ce_attente);

				return;
			}

			// Vide le buffer cache sur le disque
			//
			sync();
		}
	}

	
	
	// Copie des fichiers a diffuser
	//
	char Buffer[TAILLE_BUFFER_COPIE_FICHIER];
	
	for( QStringList::Iterator fc=ListeFichiersADiffuser.begin(); fc != ListeFichiersADiffuser.end(); fc++ )
	{
		// On realise une action
		//
		action++;
		
		// Le pourcentage dans la barre de progression
		//
		unsigned long pourcentage=action*100/nbactions;
		
		CEventTerminOA_RepProgBarDialoguePortsUSB *cepourcpyfi=new CEventTerminOA_RepProgBarDialoguePortsUSB(n,pourcentage);
		QApplication::postEvent(FPTerminOA,cepourcpyfi);
		
		// Objet du fichier source courant
		//
		QFile FichierSource(*fc);
		
		// Si on arrive a l'ouvrir en lecture seule
		//
		if( FichierSource.open(IO_ReadOnly) )
		{
			// Nom du fichier de destination sans le point de montage du port USB
			//
			QString NomFichierDestSansPointMontage=QString(*fc).right(QString(*fc).length()-ipcd);
			
			// Affichage du message dans la fenetre de log
			//
			if( FichierSource.size() < 1048576 )
			{
				// On affiche en kilo octets
				//
				CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 (%2 Ko): "+NomFichierDestSansPointMontage).arg(n+1).arg( ((double) FichierSource.size())/1024.0,0,'f',2)));
				QApplication::postEvent(FPTerminOA,celog);
			}
			else
			{
				// On affiche en mega octets
				//
				CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 (%2 Mo): "+NomFichierDestSansPointMontage).arg(n+1).arg( ((double) FichierSource.size())/1024.0/1024.0,0,'f',2)));
				QApplication::postEvent(FPTerminOA,celog);
			}
			
			// Composition du nom complet du repertoire
			//
			QString NomFichierDestination=CheminRepOA+NomFichierDestSansPointMontage;
			
			// Objet du fichier destination
			//
			QFile FichierDestination(NomFichierDestination);
			
			// Creation du fichier de destination
			//
			if( FichierDestination.open(IO_WriteOnly) )
			{
				Q_LONG retourrb;		// Valeur retounee par readBlock()
				Q_LONG retourwb;		// Valeur retournee par writeBlock()
				
				do
				{
					// Lecture d'un block
					//
					retourrb=FichierSource.readBlock(Buffer,TAILLE_BUFFER_COPIE_FICHIER);
					
					if( retourrb == -1 )
					{
						// Lecture d'un block impossible
						//
						CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 TerminOA: ERREUR: Impossible de lire le fichier: ").arg(n+1)+*fc));
						QApplication::postEvent(FPTerminOA,celog);

						CEventTerminOA_AffOrdreDialoguePortsUSB *ceordre=new CEventTerminOA_AffOrdreDialoguePortsUSB(QString(ChaineErreurEcriturePortUSB));
						QApplication::postEvent(FPTerminOA,ceordre);

						FichierSource.close();
						
						//if( MsgSonoreErreurDurantCopieFichiers.available() )
						//{
						//	if( MsgSonoreCopieFichiersEnCours.isFinished() ) MsgSonoreErreurDurantCopieFichiers.play();
						//	while( !MsgSonoreErreurDurantCopieFichiers.isFinished() );
						//}
						//else
						//{
						//	QApplication::beep();
						//}

						sleep(2*TEMPS_AUTODEMONTAGE_PORT_USB_SEC);

						CEventTerminOA_EtatUSB *ce_attente=new CEventTerminOA_EtatUSB(n,AttenteIntroduction);
						QApplication::postEvent(FPTerminOA,ce_attente);

						return;
					}
					else
					{
						// Si on a lu un block ou une portion de block
						//
						if( retourrb != 0 )
						{
							// On ecrit sur la destination
							//
							retourwb=FichierDestination.writeBlock(Buffer,retourrb);
							
							// Si on n'a pas bien ecrit autant qu'on a lu
							//
							if( retourwb != retourrb )
							{
								// Erreur d'ecriture
								//
								CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 TerminOA: ERREUR: Copie incomplete de ").arg(n+1)+*fc));
								QApplication::postEvent(FPTerminOA,celog);

								CEventTerminOA_AffOrdreDialoguePortsUSB *ceordre=new CEventTerminOA_AffOrdreDialoguePortsUSB(QString(ChaineErreurEcriturePortUSB));
								QApplication::postEvent(FPTerminOA,ceordre);

								FichierSource.close();
								FichierDestination.close();
								
								//if( MsgSonoreErreurDurantCopieFichiers.available() )
								//{
								//	if( MsgSonoreCopieFichiersEnCours.isFinished() ) MsgSonoreErreurDurantCopieFichiers.play();
								//	while( !MsgSonoreErreurDurantCopieFichiers.isFinished() );
								//}
								//else
								//{
								//	QApplication::beep();
								//}

								sleep(2*TEMPS_AUTODEMONTAGE_PORT_USB_SEC);

								CEventTerminOA_EtatUSB *ce_attente=new CEventTerminOA_EtatUSB(n,AttenteIntroduction);
								QApplication::postEvent(FPTerminOA,ce_attente);

								return;
							}
						}
					}
					
				} while( retourrb != 0 );	// Tant qu'il y a des blocks a lire et donc a ecrire
				
				// On pousse le flux sur la cle
				//
				FichierDestination.flush();
				
				// Fermeture du fichier de destination
				//
				FichierDestination.close();

				// Vide le buffer cache sur le disque
				//
				sync();
			}
			else
			{
				// Creation du fichier impossible
				//
				CEventTerminOA_AffLogDialoguePortsUSB *celog=new CEventTerminOA_AffLogDialoguePortsUSB(QString(QString("Port%1 TerminOA: ERREUR: Impossible de creer le fichier: ").arg(n+1)+NomFichierDestination));
				QApplication::postEvent(FPTerminOA,celog);

				CEventTerminOA_AffOrdreDialoguePortsUSB *ceordre=new CEventTerminOA_AffOrdreDialoguePortsUSB(QString(ChaineErreurEcriturePortUSB));
				QApplication::postEvent(FPTerminOA,ceordre);

				FichierSource.close();

				//if( MsgSonoreErreurDurantCopieFichiers.available() )
				//{
				//	if( MsgSonoreCopieFichiersEnCours.isFinished() ) MsgSonoreErreurDurantCopieFichiers.play();
				//	while( !MsgSonoreErreurDurantCopieFichiers.isFinished() );
				//}
				//else
				//{
				//	QApplication::beep();
				//}

				sleep(2*TEMPS_AUTODEMONTAGE_PORT_USB_SEC);

				CEventTerminOA_EtatUSB *ce_attente=new CEventTerminOA_EtatUSB(n,AttenteIntroduction);
				QApplication::postEvent(FPTerminOA,ce_attente);

				return;
			}
			
			// Fermeture du fichier source
			//
			FichierSource.close();
		}
	}
	
	// Vide le buffer cache sur le disque
	//
	sync();

	
	// Si l'interface sonore est disponible on joue le message sonore
	//
	//if( MsgSonoreFichiersCopiesRetirezCle.available() )
	//{
	//	if( MsgSonoreFichiersCopiesRetirezCle.isFinished() ) MsgSonoreFichiersCopiesRetirezCle.play();
	//	while( !MsgSonoreFichiersCopiesRetirezCle.isFinished() );
	//}
	//else
	//{
	//	QApplication::beep();
	//}


	// On passe l'etat du port USB en mode documents copies
	//
	CEventTerminOA_EtatUSB *ce_doccopies=new CEventTerminOA_EtatUSB(n,CleDocumentsCopies);
	QApplication::postEvent(FPTerminOA,ce_doccopies);

	// On attend deux fois plus que le temps de demontage automatique pour que la personne ait le temps d'enlever sa cle
	//
	sleep(2*TEMPS_AUTODEMONTAGE_PORT_USB_SEC);
}


// - Thread ---------------------------------------------------------------------------------------------------------------------

// Constructeur du processus leger serveur chiffre monoclient d'attente des commandes par le reseau
//
// CE:	On passe un pointeur vers la fenetre principale de l'application TerminOA ;
//
//		On passe l'adresse IP d'attachement en valeur host (0x________) ;
//
//		On passe le port d'attachement en valeur host (0x____) ;
//
//		On passe l'adresse IP du client autorise en valeur host (0x________) ;
//
//		On passe le timeout en secondes pour la tentative d'ecriture de donnees dans la socket ;
//
//		On passe le timeout en secondes pour la tentative de lecture de donnees dans la socket ;
//
//		On passe le timeout en secondes de l'initiative de la negociation TLS/SSL ;
//
//		On passe un pointeur sur la fonction C de handler du signal SIGPIPE ;
//
//		On passe un pointeur sur une chaine de char qui contient le mot de passe pour acceder a la cle privee du serveur SSL
//		 Ce mot de passe ne doit pas contenir plus de TAILLE_MAX_MDP_CLE_PRIVEE-1 caracteres ;
//
//		On passe un pointeur sur char vers un buffer de stockage du mot de passe pour acceder a la cle privee du serveur SSL
//		 Ce buffer doit etre reserve avec TAILLE_MAX_MDP_CLE_PRIVEE elements ;
//
//		On passe un pointeur sur la fonction C appelee par la librairie SSL lors de la demande du mot de passe pour acceder a la cle
//		 privee du serveur SSL stockee dans un fichier PEM ;
//
//		On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM du certificat
//		 de l'autorite de certification CA qui a signe les certificats du serveur ;
//
//		On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM du certificat du serveur ;
//
//		On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM de la cle privee du serveur ;
//
//		On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM des parametres Diffie-Hellman aleatoires ;
//
//		On passe la chaine de la liste des chiffreurs que le serveur doit utiliser ;
//
// CS:	-
//
ProcessusLegerServeurCommandes::ProcessusLegerServeurCommandes(TerminOA *papp,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet,int pTimeoutNegoTLSSSL,void (*pFnHandlerSIGPIPE)(int),const char *MdpClePriveeServeur,char *BuffStockMdpClePriveeServeur,int (*pFnMotDePasseClePriveeChiffree)(char*, int, int, void*),const char *pCheminCertificatCA,const char *pCheminCertificatServeur,const char *pCheminClePriveeServeur,const char *pParametresDH,const char *pListeChiffreurs)
{
	// Initialisation des variables
	//
	DrapeauDemandeTerminaison=false;
	
	// Instanciation de l'objet serveur chiffre monoclient
	//
	if( (Serveur=new (std::nothrow) PointCommServeurChiffreMonoClient(false,pAdresse,pPort,pAdresseClient,pNbLClientsMax,pTimeoutSocketPut,pTimeoutSocketGet,pTimeoutNegoTLSSSL,true,pFnHandlerSIGPIPE,MdpClePriveeServeur,BuffStockMdpClePriveeServeur,pFnMotDePasseClePriveeChiffree,pCheminCertificatCA,pCheminCertificatServeur,pCheminClePriveeServeur,pParametresDH,pListeChiffreurs)) == NULL )
	{
		std::cerr << "ProcessusLegerServeurCommandes: ERREUR: Impossible de creer l'objet serveur chiffre monoclient." << std::endl;
	}
	
	// Pointeur vers la fenetre principale de l'application TerminOA
	//
	FPTerminOA=papp;
}


// Destructeur du processus leger serveur chiffre monoclient d'attente des commandes par le reseau
//
ProcessusLegerServeurCommandes::~ProcessusLegerServeurCommandes()
{
	// Si le thread est encore actif et donc que le serveur ne s'est pas termine normalement
	//
	if( running() )
	{
		terminate();	// On termine le thread de maniere brutale
	}
	
	// On supprime l'objet Serveur
	//
	delete Serveur;
}


// Fonction de la demande de terminaison propre du processus leger
//
void ProcessusLegerServeurCommandes::DemandeTerminaison(void)
{
	DrapeauDemandeTerminaison=true;
	
	// Les sockets passent en mode non bloquant
	//
	Serveur->SocketNonBloquante();
	Serveur->SocketSessionNonBloquante();
}


// Surcharge de la methode run() qui contient le code d'execution du thread
//  du serveur chiffre monoclient d'attente des commandes par le reseau
//
// ATTENTION : QT3 n'est pas thread-safe (c'est a dire que les fonctions ne peuvent pas etre appellees par plusieurs threads a la fois).
//		Les widgets doivent etre modifies uniquement par l'application maitre qui gere la boucle evenementielle. Donc un thread
//		qui doit modifier un widget de l'application doit communiquer par un evenement particulier QCustomEvent via la fontion
//		thread-safe QApplication::postEvent(). Il ne doit pas modifier directement le widget sinon l'application plantera.
//
void ProcessusLegerServeurCommandes::run()
{
	int Sortir=false;	// Drapeau d'indication de sortie de la boucle de traitement des demandes de connexion
	
	// On capture un element du semaphore de synchronisation
	//
	SemaphoreSyncLancementThreadTerminOA++;
	
	// On lance l'ecoute reseau systeme (socket normale) en attente d'une connexion sur le port du service IP:Port
	//
	// Chaque connexion individuelle sera traitee dans la boucle d'acceptation des sessions qui va suivre
	//
	if( !Serveur->EcouteReseau() ) return;
	
	// Traitement de chaque demande de connexion
	//
	do
	{
		// Attente et acceptation d'une requete de connexion par un client autorise
		//
		if( Serveur->PossibleLireDonneesSocket(100) )		// S'il y a une tentative de connexion a lire sous 100ms (TRES IMPORTANT POUR NE PAS RESTER BLOQUE SUR accept() )
		if( Serveur->AttenteAccepterSessionAutorisee() )	// Il y a une demande de connexion a lire alors on accept()
		{
			// Si il y a eu une demande de connexion et que le client est autorise
			//
			if( Serveur->SessionAccepteeAutorisee() )
			{
				// Si on a demande au processus de terminer proprement son execution
				//
				if( !DrapeauDemandeTerminaison )
				{
					// On demande l'initiation de la negociation SSL
					//
					if( Serveur->NegociationConnexionSSL() )
					{
						// Si on a demande au processus de terminer proprement son execution
						//
						if( !DrapeauDemandeTerminaison )
						{
							int BoucleCommandes=true;	// Indicateur de l'etat de la boucle d'attente des  commandes du client
							int SesameOuvreToi=false;	// Chaine mot de passe pour Login
											//  et accepter les commandes clients
							unsigned long CompteurEnAttente=0;	// Compteur de commandes en attentes recues
							QString ChaineGets;		// Chaine recue
							ChaineGets.reserve(TAILLE_MINI_CHAINE);
							QString ChainePuts;		// Chaine pour BIO_puts()
							ChainePuts.reserve(TAILLE_MINI_CHAINE);
							char ChaineRecue[TAILLE_MAXI_CHAINE_BIO];	// Chaine recues par BIO_gets()
							
							// Chaine de bienvenue au client
							//
							ChainePuts="Bienvenue sur TerminOA Canal des Commandes (Chiffrement:"+QString(SSL_CIPHER_get_name(Serveur->DescripteurChiffreurConnexionSSL()))+", Protocoles:"+QString(SSL_CIPHER_get_version(Serveur->DescripteurChiffreurConnexionSSL()))+")\n";
							
							if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
							
							// Boucle d'attente des commandes du client
							//
							while( BoucleCommandes && !DrapeauDemandeTerminaison )
							{
								int RetourGets;		// Valeur retournee par la fonction de reception
								long Valeur1;		// Valeur pour extraction de parametre
								QString ParamChaine1;	// Chaine pour extraction de parametre
								ParamChaine1.reserve(TAILLE_MINI_CHAINE);
								
								if( (RetourGets=Serveur->RecevoirChaineBIO(ChaineRecue)) > 0 )
								{
									int i=0;		// Variable indice
									int IdCmdClient;	// Id de commande client
									
									// On coupe la chaine au premier \r ou \n
									//
									while( ChaineRecue[i] != 0 )
									{
										if( ChaineRecue[i] == '\r' )
										{
											ChaineRecue[i]=0;
											break;
										}
										if( ChaineRecue[i] == '\n' )
										{
											ChaineRecue[i]=0;
											break;
										}
										i++;
									}
									
									// On recopie la chaine recue dans la QString
									//
									ChaineGets=QString(ChaineRecue);
									
									if( SesameOuvreToi && Serveur->ObjetModeVerbeux() ) std::cout << "TerminOA: S<-C " << RetourGets << ": " << ChaineGets << std::endl;
									
									// Recherche de la commande dans la liste
									//
									for( IdCmdClient=0; ListeCmdClientTerminOA[IdCmdClient] != QString(""); IdCmdClient++ ) if( RetourGets > 2 ) if( ListeCmdClientTerminOA[IdCmdClient] == ChaineGets.left(ListeCmdClientTerminOA[IdCmdClient].length()) ) break;
									
									// Si la chaine login/mdp sesame n'a pas ete recue
									//
									if( !SesameOuvreToi )
									{
										if( IdCmdClient == TERMINOA_CMD_SESAMEOUVRETOI )
										{
											SesameOuvreToi=true;
										}
										
										IdCmdClient=-1;
									}
									
									// Selon l'identifieur de la commande
									//
									switch( IdCmdClient )
									{
										case TERMINOA_CMD_ARRET:
											//
											// Arret et sortie du logiciel
											//
											{
												CEventTerminOA_Quit *event=new CEventTerminOA_Quit();
												QApplication::postEvent(FPTerminOA,event);
											}
										
											break;
										
										case TERMINOA_CMD_DECONNEXION:
											//
											// Deconnexion du client
											//
											BoucleCommandes=false;
											break;
										
										case TERMINOA_CMD_EN_ATTENTE:
											//
											// Serveur en attente, en vie ?
											//
											CompteurEnAttente++;
										
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_EN_ATTENTE]+QString("%1\n").arg(CompteurEnAttente);
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_AFFICHER:
											//
											// Afficher une image arrivee
											//
											ParamChaine1=ChaineGets.right(ChaineGets.length()-ListeCmdClientTerminOA[IdCmdClient].length());
										
											// Si l'identite de l'image stockee sur le canal des
											//  donnees correspond a la demande d'affichage
											//
											if( ParamChaine1 == threadCanalDonnees->IdentiteFichierCourant() )
											{
												int ni;		// nouvelle image ?
										
												// On copie l'image recue dans l'image courante
												//
												MutexAccesFichierFITSCourant.lock();
												MutexAccesFichierCanalDonnees.lock();
										
												if( !(ni=CopierDernierRecuDansFITSCourant()) )
												{
													std::cerr << "ProcessusLegerServeurCommandes: ERREUR: CopierDernierRecuDansFITSCourant(): Impossible de copier le dernier fichier recu dans l'image courante." << std::endl;
												}
										
												MutexAccesFichierFITSCourant.unlock();
												MutexAccesFichierCanalDonnees.unlock();
										
												// On passe le terminal en mode affichage
												//  de l'image courante
												if( ni )
												{
													CEventTerminOA_ChargeAffImgCourante *event=new CEventTerminOA_ChargeAffImgCourante();
													QApplication::postEvent(FPTerminOA,event);

													ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_AFFICHER];
													ChainePuts.append("\n");
												}
												else
												{
													ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_NONOK_AFFICHER];
													ChainePuts.append("\n");
												}
											}
											else
											{
												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_NONOK_AFFICHER];
												ChainePuts.append("\n");
											}
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_MODE_IC:
											//
											// Passer en mode affichage image courante
											//
											{
											CEventTerminOA_ModeIC *event=new CEventTerminOA_ModeIC();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_MODE_IC];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_MODE_DIAPO:
											//
											// Passer en mode affichage diaporama
											//
											{
											CEventTerminOA_ModeDiapo *event=new CEventTerminOA_ModeDiapo();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_MODE_DIAPO];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_ECH_LIN:
											//
											// Passer en echelle lineaire
											//
											{
											CEventTerminOA_EchLin *event=new CEventTerminOA_EchLin();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_ECH_LIN];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_ECH_LOG:
											//
											// Passer en echelle logarihtmique
											//
											{
											CEventTerminOA_EchLog *event=new CEventTerminOA_EchLog();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_ECH_LOG];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_PAL_NB:
											//
											// Passer en palette noir et blanc
											//
											{
											CEventTerminOA_PalNB *event=new CEventTerminOA_PalNB();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_PAL_NB];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_PAL_HALPHA:
											//
											// Passer en palette h-alpha
											//
											{
											CEventTerminOA_PalHalpha *event=new CEventTerminOA_PalHalpha();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_PAL_HALPHA];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_PAL_NOB:
											//
											// Passer en palette noir orange blanc
											//
											{
											CEventTerminOA_PalNOB *event=new CEventTerminOA_PalNOB();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_PAL_NOB];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_PAL_NVB:
											//
											// Passer en palette noir vert blanc
											//
											{
											CEventTerminOA_PalNVB *event=new CEventTerminOA_PalNVB();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_PAL_NVB];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_PAL_NBB:
											//
											// Passer en palette noir bleu blanc
											//
											{
											CEventTerminOA_PalNBB *event=new CEventTerminOA_PalNBB();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_PAL_NBB];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z0:
											//
											// Passer en visu zone 0
											//
											{
											CEventTerminOA_Z0 *event=new CEventTerminOA_Z0();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z0];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z1:
											//
											// Passer en visu zone 1
											//
											{
											CEventTerminOA_Z1 *event=new CEventTerminOA_Z1();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z1];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z2:
											//
											// Passer en visu zone 2
											//
											{
											CEventTerminOA_Z2 *event=new CEventTerminOA_Z2();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z2];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z3:
											//
											// Passer en visu zone 3
											//
											{
											CEventTerminOA_Z3 *event=new CEventTerminOA_Z3();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z3];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z4:
											//
											// Passer en visu zone 4
											//
											{
											CEventTerminOA_Z4 *event=new CEventTerminOA_Z4();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z4];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z5:
											//
											// Passer en visu zone 5
											//
											{
											CEventTerminOA_Z5 *event=new CEventTerminOA_Z5();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z5];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z6:
											//
											// Passer en visu zone 6
											//
											{
											CEventTerminOA_Z6 *event=new CEventTerminOA_Z6();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z6];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z7:
											//
											// Passer en visu zone 7
											//
											{
											CEventTerminOA_Z7 *event=new CEventTerminOA_Z7();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z7];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z8:
											//
											// Passer en visu zone 8
											//
											{
											CEventTerminOA_Z8 *event=new CEventTerminOA_Z8();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z8];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_Z9:
											//
											// Passer en visu zone 9
											//
											{
											CEventTerminOA_Z9 *event=new CEventTerminOA_Z9();
											QApplication::postEvent(FPTerminOA,event);
											}
											
											ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_Z9];
											ChainePuts.append("\n");
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_CSB:
											//
											// Fixer la consigne de seuil bas
											//
											ParamChaine1=ChaineGets.right(ChaineGets.length()-ListeCmdClientTerminOA[IdCmdClient].length());
										
											Valeur1=ParamChaine1.toLong();
										
											if( Valeur1 >= MIN_CSB && Valeur1 <= MAX_CSB ) 
											{
												CEventTerminOA_CSB *event=new CEventTerminOA_CSB((int) Valeur1);
												QApplication::postEvent(FPTerminOA,event);

												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_CSB];
												ChainePuts.append("\n");
											}
											else
											{
												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_NONOK_CSB];
												ChainePuts.append("\n");
											}
											
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_CSH:
											//
											// Fixer la consigne de seuil haut
											//
											ParamChaine1=ChaineGets.right(ChaineGets.length()-ListeCmdClientTerminOA[IdCmdClient].length());
										
											Valeur1=ParamChaine1.toLong();
										
											if( Valeur1 >= MIN_CSH && Valeur1 <= MAX_CSH ) 
											{
												CEventTerminOA_CSH *event=new CEventTerminOA_CSH((int) Valeur1);
												QApplication::postEvent(FPTerminOA,event);
										
												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_CSH];
												ChainePuts.append("\n");
											}
											else
											{
												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_NONOK_CSH];
												ChainePuts.append("\n");
											}
											
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										case TERMINOA_CMD_CPUILUT:
											//
											// Fixer la consigne de puissance de la LUT
											//
											ParamChaine1=ChaineGets.right(ChaineGets.length()-ListeCmdClientTerminOA[IdCmdClient].length());
										
											Valeur1=ParamChaine1.toLong();
										
											if( Valeur1 >= MIN_CPUILUT && Valeur1 <= MAX_CPUILUT ) 
											{
												CEventTerminOA_CPuiLUT *event=new CEventTerminOA_CPuiLUT((int) Valeur1);
												QApplication::postEvent(FPTerminOA,event);
										
												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_OK_CPUILUT];
												ChainePuts.append("\n");
											}
											else
											{
												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_NONOK_CPUILUT];
												ChainePuts.append("\n");
											}
											
										
											if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
										
											break;
										
										default:
											// Si la chaine login/mdp sesame est recue
											//
											if( SesameOuvreToi && IdCmdClient != -1 )
											{
												ChainePuts=ListeRepServeurTerminOA[TERMINOA_REP_CMD_INCONNUE];
												ChainePuts.append("\n");
										
												if( Serveur->EnvoyerChaineBIO(ChainePuts.ascii()) <= 0 ) BoucleCommandes=false;
											}
											break;
									}
								}
								else
								{
									BoucleCommandes=false;
								}
							}
							
							// Fermeture de la connexion SSL courante
							//
							Serveur->FermetureConnexionSSL();
						}
					}
				}
			}
		}
		else
		{
			Sortir=true;
		}
		
		if( DrapeauDemandeTerminaison ) Sortir=true;
		
	} while( !Sortir );
}


// Fonction de copie du dernier fichier recu dans le fichier FITS courant
//
// CE:	-
//
// CS:	La fonction est vraie si la copie s'est bien deroulee, fausse dans le cas contraire
//
int ProcessusLegerServeurCommandes::CopierDernierRecuDansFITSCourant(void)
{
	int IdSource;					// Descripteur du fichier source
	int IdDestination;				// Descripteur du fichier destination
	ssize_t RetourRead;				// Code retour de read()
	ssize_t RetourWrite;				// Code retour de write()
	int errno_avant;				// Sauvegarde de errno
	char Buffer[TAILLE_BUFFER_COPIER_FICHIER];	// Buffer lecture / emission
	
	
	// Chemin et nom de fichier de la source
	//
	QString Source=threadCanalDonnees->CheminNomFichierCourant();
	
	// Chemin et nom de fichier de la destination
	//
	QString Destination=FPTerminOA->CheminRepertoireTerminal()+"/"+FICHIER_IMAGE_COURANTE;
	
	// Ouverture de la source en lecture seule
	//
	if( (IdSource=open(Source.ascii(),O_RDONLY)) == -1 )
	{
		std::cerr << "ProcessusLegerServeurCommandes: ERREUR: CopierDernierRecuDansFITSCourant(): open(" << Source.ascii() << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		
		return false;
	}
	
	// Ouverture de la destination en ecriture
	//
	if( (IdDestination=open(Destination.ascii(),O_CREAT | O_WRONLY | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == -1 )
	{
		std::cerr << "ProcessusLegerServeurCommandes: ERREUR: CopierDernierRecuDansFITSCourant(): open(" << Destination.ascii() << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		
		return false;
	}
	
	// On envoie les donnees du fichier
	//
	do
	{
		RetourRead=read(IdSource,Buffer,TAILLE_BUFFER_COPIER_FICHIER);
		
		switch( RetourRead )
		{
			case -1:
				// Erreur de lecture
				//
				std::cerr << "ProcessusLegerServeurCommandes: ERREUR: CopierDernierRecuDansFITSCourant(): read(): Impossible de lire un fragment du fichier: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
				
				close(IdSource);
				close(IdDestination);
				
				return false;
				
			case 0:
				// Fin du fichier
				break;
				
			default:
				// Lecture de RetourRead octets dans le fichier a copier
				//
				
				// Ecriture du dernier buffer lu dans le fichier
				//
				errno_avant=errno;
				
				RetourWrite=write(IdDestination,Buffer,RetourRead);
				
				if( RetourWrite < 0 || errno != errno_avant )
				{
					if( errno != errno_avant )
					{
						std::cerr << "ProcessusLegerServeurCommandes: ERREUR: CopierDernierRecuDansFITSCourant(): write(): Impossible d'ecrire un paquet de donnees: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
						
						close(IdSource);
						close(IdDestination);
						
						return false;
					}
				}
				
				break;
		}
		
	} while( RetourRead != 0 );
	
	// Fermeture du fichier source
	//
	if( close(IdSource) == -1 )
	{
		std::cerr << "ProcessusLegerServeurCommandes: ERREUR: CopierDernierRecuDansFITSCourant(): close(" << Source.ascii() << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
	
		return false;
	}
	
	// Fermeture du fichier destination
	//
	if( close(IdDestination) == -1 )
	{
		std::cerr << "ProcessusLegerServeurCommandes: ERREUR: CopierDernierRecuDansFITSCourant(): close(" << Destination.ascii() << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
	
		return false;
	}
	
	return true;
}


// - Thread ---------------------------------------------------------------------------------------------------------------------

// Constructeur du processus leger serveur non chiffre monoclient d'attente des donnees par le reseau
//
// CE:	On passe un pointeur vers la fenetre principale de l'application TerminOA ;
//
//		On passe l'adresse IP d'attachement en valeur host (0x________) ;
//
//		On passe le port d'attachement en valeur host (0x____) ;
//
//		On passe l'adresse IP du client autorise en valeur host (0x________) ;
//
//		On passe le timeout en secondes pour la tentative d'ecriture de donnees dans la socket ;
//
//		On passe le timeout en secondes pour la tentative de lecture de donnees dans la socket ;
//
//		On passe le chemin d'acces et le nom du fichier temporaire de stockage de la derniere image transmise par le client ;
//
// CS:	-
//
ProcessusLegerServeurDonnees::ProcessusLegerServeurDonnees(TerminOA *papp,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet,QString p_chemFichierImgReception)
{
	// Initialisation des variables
	//
	DrapeauDemandeTerminaison=false;
	
	for( int i=0; i <= TAILLE_IDENTITE_FICHIER; i++ ) *(IdentiteFichier+i)=0;
	
	// Instanciation de l'objet serveur non chiffre monoclient
	//
	if( (Serveur=new (std::nothrow) PointCommServeurNonChiffreMonoClient(false,pAdresse,pPort,pAdresseClient,pNbLClientsMax,pTimeoutSocketPut,pTimeoutSocketGet)) == NULL )
	{
		std::cerr << "ProcessusLegerServeurDonnees: ERREUR: Impossible de creer l'objet serveur non chiffre monoclient." << std::endl;
	}
	
	// Pointeur vers la fenetre principale de l'application TerminOA
	//
	FPTerminOA=papp;
	CheminFichierImgRecep=p_chemFichierImgReception;
}


// Destructeur du processus leger serveur non chiffre monoclient d'attente des donnees par le reseau
//
ProcessusLegerServeurDonnees::~ProcessusLegerServeurDonnees()
{
	// Si le thread est encore actif et donc que le serveur ne s'est pas termine normalement
	//
	if( running() )
	{
		terminate();	// On termine le thread de maniere brutale
	}
	
	// On supprime l'objet Serveur
	//
	delete Serveur;
}


// Fonction de la demande de terminaison propre du processus leger
//
void ProcessusLegerServeurDonnees::DemandeTerminaison(void)
{
	DrapeauDemandeTerminaison=true;
	
	// Les sockets passent en mode non bloquant
	//
	Serveur->SocketNonBloquante();
	Serveur->SocketSessionNonBloquante();
}


// Surcharge de la methode run() qui contient le code d'execution du thread
//  du serveur non chiffre monoclient d'attente des donnees
//
void ProcessusLegerServeurDonnees::run()
{
	int Sortir=false;			// Drapeau d'indication de sortie de la boucle de traitement des demandes de connexion
	unsigned long CompteurImagesRecues=0;
	
	// On capture un element du semaphore de synchronisation
	//
	SemaphoreSyncLancementThreadTerminOA++;
	
	// On lance l'ecoute reseau systeme (socket normale) en attente d'une connexion sur le port du service IP:Port
	//
	// Chaque connexion individuelle sera traitee dans la boucle d'acceptation des sessions qui va suivre
	//
	if( !Serveur->EcouteReseau() ) return;
	
	// Traitement de chaque demande de connexion
	//
	do
	{
		// Attente et acceptation d'une requete de connexion par un client autorise
		//
		if( Serveur->PossibleLireDonneesSocket(100) )		// S'il y a une tentative de connexion a lire sous 100ms (TRES IMPORTANT POUR NE PAS RESTER BLOQUE SUR accept() )
		if( Serveur->AttenteAccepterSessionAutorisee() )	// Il y a une demande de connexion a lire alors on accept()
		{
			// Si il y a eu une demande de connexion et que le client est autorise
			//
			if( Serveur->SessionAccepteeAutorisee() )
			{
				// Si on a demande au processus de terminer proprement son execution
				//
				if( !DrapeauDemandeTerminaison )
				{
					int BoucleDonnees=true;					// Indicateur de l'etat de la boucle d'attente des donnees du client
					QString ChainePuts;						// Chaine pour BIO_puts()
					ChainePuts.reserve(TAILLE_MINI_CHAINE);
					char cmagti[]=CH_MAG_EDTITERMINOA;		// Chaine magique depart trame image d'un TerminOA
					
					// Chaine de bienvenue au client
					//
					ChainePuts="Bienvenue sur TerminOA Canal des Donnees\n";
					
					if( Serveur->EnvoyerChaineSocketSession(ChainePuts.ascii()) <= 0 ) BoucleDonnees=false;


					// Boucle d'attente des donnees du client
					//
					do
					{
						char octet;		// Un octet

						// Si il est possible de lire des donnees sur la socket dans la seconde qui suit
						//
						if( Serveur->PossibleLireDonneesSocketSession(1000) )
						{
							if( Serveur->LireDonneesSocketSession(&octet,1) > 0 )
							{
								// Si le caractere lu correspond au premier caractere de la chaine magique d'une trame image TerminOA
								//
								if( octet == cmagti[0] )
								{
									int nb_cokcm=1;	// Nombre de caracteres a la suite identifies comme la chaine magique de depart de trame image d'un TerminOA

									// On essaye d'identifier la chaine magique de depart de trame image
									//
									while( nb_cokcm < TAILLE_CH_MAG_EDTITERMINOA )	// ATTENTION: On compte aussi le 0 en fin de chaine
									{
										// Si on peut lire immediatement un caractere
										//
										if( Serveur->PossibleLireDonneesSocketSession(0) )
										{
											if( Serveur->LireDonneesSocketSession(&octet,1) > 0 )
											{
												// Si le caractere courant correspond bien a la suite dans la chaine magique
												//
												if( octet == cmagti[nb_cokcm] )
												{
													// L'octet courant correspond a la chaine magique
													//
													nb_cokcm++;
												}
												else break;	// Sinon on arrete la car ce n'est pas la chaine magique de depart de trame
											}
											else
											{
												// Il y a une erreur ou la connexion est perdue...
												//
												BoucleDonnees=false;
											}
										}
										else break;
									}

									// Si la chaine magique de depart de trame image est reperee dans le flux avec tous ses caracteres
									//
									if( nb_cokcm == TAILLE_CH_MAG_EDTITERMINOA )
									{
										long NbOcR;
										QString ChaineRetourClient;	// Pour composer la chaine de retour
										ChaineRetourClient.reserve(TAILLE_MINI_CHAINE);
							
										// On verrouille l'acces au dernier fichier recu (bloquant si il est utilise)
										//
										MutexAccesFichierCanalDonnees.lock();
							
										if( (NbOcR=Serveur->RecevoirFichierSocketSession(CheminFichierImgRecep.ascii(),IdentiteFichier)) )
										{
											// On a recu une image de plus
											//
											CompteurImagesRecues++;
								
											// Chaine en retour au client donnees
											//
											ChaineRetourClient=QString("<OK>\n");

											// Chaine en retour sur le canal chiffre des commandes
											//
											QString ChaineRetourCC;
											ChaineRetourCC.reserve(TAILLE_MINI_CHAINE);
											ChaineRetourCC=ListeRepServeurTerminOA[TERMINOA_REP_OK_IMG_RECUE]+QString("%1 %2 %3\n").arg(CompteurImagesRecues).arg(NbOcR).arg(IdentiteFichier);

											if( PLSCTerminOA->Serveur->EnvoyerChaineBIO(ChaineRetourCC.ascii()) <= 0 )
											{
												std::cerr << "ProcessusLegerServeurDonnees: ERREUR: Impossible d'envoyer la chaine reponse a la reception d'une image sur le canal des commandes." << std::endl;
											}
										}
										else
										{
											// Chaine en retour au client
											//
											ChaineRetourClient=QString("<NON-OK>\n");
								
											// Echec de la reception du fichier image
											//
											std::cerr << "ProcessusLegerServeurDonnees: ERREUR: Une erreur est survenue lors de la reception du fichier image." << std::endl;
								
											IdentiteFichier[0]=0;
								
											BoucleDonnees=false;
										}
							
										// On deverrouille l'acces au dernier fichier recu
										//
										MutexAccesFichierCanalDonnees.unlock();
							
										// Emission de la reponse au client
										//
										if( Serveur->EnvoyerChaineSocketSession(ChaineRetourClient.ascii()) <= 0 )
										{
											std::cerr << "ProcessusLegerServeurDonnees: ERREUR: Impossible d'envoyer la reponse au client apres la reception d'une image." << std::endl;
								
											IdentiteFichier[0]=0;
							
											BoucleDonnees=false;
										}
									}
								}
							}
							else
							{
								// Il y a une erreur ou la connexion est perdue...
								//
								BoucleDonnees=false;
							}
						}
						else
						{
							// On peut envoyer un "es-tu vivant ?" au client pour eviter un timeout.
							//  Le serveur ou le client TerminOA ferme automatiquement la connexion si elle est inactive.
							//
							if( Serveur->EnvoyerChaineSocketSession("#") <= 0 )
							{
								// Erreur lors de l'emission, la liaison n'est pas correcte, on peut refermer cette connexion
								//
								std::cerr << "ProcessusLegerServeurDonnees: run(): ERREUR: Impossible d'envoyer l'octet es-tu en vie ?." << std::endl;

								BoucleDonnees=false;
							}

							sleep(1);	// On patiente pour ne pas emballer la liaison
						}

					} while( BoucleDonnees && !DrapeauDemandeTerminaison );

					// Fermeture de la connexion courante
					//
					Serveur->FermetureSession();
				}
			}
		}
		else
		{
			Sortir=true;
		}
		
		if( DrapeauDemandeTerminaison ) Sortir=true;
		
	} while( !Sortir );
}


// Fonction retournant l'identite du dernier fichier recu
//
QString ProcessusLegerServeurDonnees::IdentiteFichierCourant(void)
{
	return QString(IdentiteFichier);
}


// Fonction retournant le chemin et le nom du dernier fichier recu
//
QString ProcessusLegerServeurDonnees::CheminNomFichierCourant(void)
{
	return CheminFichierImgRecep;
}


#include "terminoa.moc"

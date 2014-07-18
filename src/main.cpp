/* MODULE PRINCIPAL DE L'APPLICATION KDE : TerminOA

   LOGICIEL TERMINAL RESEAU DE VISUALISATION D'IMAGES ET RECUPERATION DE DONNEES PAR PERIPHERIQUE USB

  (C)David.Romeuf@univ-lyon1.fr 30/01/2006 par David Romeuf
*/

// Inclusions C++
//
#include <iostream>
#include <new>

// Inclusions Qt et KDE
//
#include <qdir.h>
#include <qstring.h>
#include <qcstring.h>
#include <qsemaphore.h>
#include <qwidget.h>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

// Inclusions de l'applications
//
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "terminoa.h"

// Declarations
//

static const char description[] = I18N_NOOP("A KDE KPart Application");
static const char version[] = "1.0";

// Liste des options supportees par la ligne de commande
//
enum ArgumentsApplication {
	CheminRepTerminal,
	CheminRepDiaporama,
	CheminRepDocuments,
	CheminFichImgWeb,
	CheminMntClesUSB,
	DureeInacDeclModeDiapo,
	DureeEntreDiapo,
	PeriodeRapEcrLanc,
	CheminFichCertCA_OA,
	CheminFichCertServTerminOA,
	CheminFichClePriveeServTerminOA,
	CheminFichParamDH,
	MdpClePriveeServeur,
	AdresseClientAutorise,
	PortCanalCommandes,
	PortCanalDonnees,
	ArretSysteme,
	CopieDocumentsPermise
};

const char *OptionsLC[] = {
	"chemterm",
	"chemdia",
	"chemdoc",
	"chemimgweb",
	"chemclesusb",
	"diddiapo",
	"dediapo",
	"peldiapo",
	"chemficaoa",
	"chemficertserveur",
	"chemficleprivserveur",
	"chemfiparamdh",
	"mdpcleprivserveur",
	"adresseclientautorise",
	"portcanalcommandes",
	"portcanaldonnees",
	"arretsysteme",
	"copiedoc"
};

static KCmdLineOptions options[] =
{
	{"chemterm ",I18N_NOOP("Chemin vers le repertoire du terminal"),"/TerminOA"},
	{"chemdia ",I18N_NOOP("Chemin vers le repertoire du diaporama"),"/TerminOA/Diaporama"},
	{"chemdoc ",I18N_NOOP("Chemin vers le repertoire des documents"),"/TerminOA/Documents"},
	{"chemimgweb ",I18N_NOOP("Chemin et nom du fichier diffuse en temp reel sur le web"),"/TerminOA/Documents/last.jpg"},
	{"chemclesusb ",I18N_NOOP("Chemin vers le point de montage des cles USB"),"/mediaTerminOA"},
	{"diddiapo ",I18N_NOOP("Duree d'inactivite de declanchement du mode diaporama en secondes"),"60"},
	{"dediapo ",I18N_NOOP("Duree entre chaque diapositive en secondes"),"10"},
	{"peldiapo ",I18N_NOOP("Periode d'affichage de l'ecran de lancement en nombre de diapositives projetees"),"5"},
	{"chemficaoa ",I18N_NOOP("Chemin et nom du certificat (PEM) du CA des OA"),"/TerminOA/ssl/CertificatCA_OA.pem"},
	{"chemficertserveur ",I18N_NOOP("Chemin et nom du certificat de ce serveur TerminOA"),"/TerminOA/ssl/CertificatServeurTerminOA.pem"},
	{"chemficleprivserveur ",I18N_NOOP("Chemin et nom du fichier PEM contenant la cle privee de ce serveur TerminOA"),"/TerminOA/ssl/ClePriveeServeurTerminOA.pem"},
	{"chemfiparamdh ",I18N_NOOP("Chemin et nom du fichier PEM contenant l'alea des parametres Diffie-Hellman de ce serveur"),"/TerminOA/ssl/Parametres-Diffie-Hellman-TerminOA.pem"},
	{"mdpcleprivserveur ",I18N_NOOP("Mot de passe d'acces a la cle privee de ce serveur"),"???"},
	{"adresseclientautorise ",I18N_NOOP("Adresse IP du client autorise sous la forme x.x.x.x"),"192.168.6.1"},
	{"portcanalcommandes ",I18N_NOOP("Numero du port tcp pour le canal des commandes"),"22443"},
	{"portcanaldonnees ",I18N_NOOP("Numero du port tcp pour le canal des donnees"),"22444"},
	{"arretsysteme ",I18N_NOOP("Lancement de l'arret du systeme en quittant TerminOA"),"n"},
	{"copiedoc ",I18N_NOOP("Copie de documents permise sur une cle USB detectee"),"n"},
//	{" ", I18N_NOOP(""),""},
	KCmdLineLastOption
};


// - Pour les processus legers ---------------------------------------------------------------------------------------------------------

// Temps d'attente de la terminaison d'un processus leger serveur reseau
//
#define TEMPS_ATTENTE_TERMINAISON_PROCESSUS_SERVEUR	10000

#define TIMEOUT_EMISSION	5	// Timeout des liaisons pour les canaux de commandes et de donnees
#define TIMEOUT_RECEPTION	120

char MotDePasseClePriveeServeurTERMINOA[TAILLE_MAX_MDP_CLE_PRIVEE];	// Mot de passe pour acces a la cle privee du serveur

// FONCTION APPELEE PAR LA LIBRAIRIE SSL POUR LIRE OU STOCKER LES FICHIERS PEM CONTENANT UNE CLE CHIFFREE
//
// CE:	La librairie SSL passe un pointeur vers le tableau ou doit etre copie le mot de passe ;
//
// 	La librairie SSL passe la dimension maximale du mot de passe ;
//
// 	La librairie SSL passe 0 si la fonction est utilisee pour lire/decryptee, ou, 1 si la fonction est appelee pour ecrire/encryptee
//
// 	La librairie SSL passe un pointeur vers une donnee passee par la routine PEM. Il permet qu'une donnee arbitraire soit passee
// 	 a cette fonction par une application (comme par exemple un identifieur de fenetre dans une application graphique).
//
// CS:	La fonction doit retourner la longueur du mot de passe.
//
int FnMotDePasseClePriveeChiffreeTERMINOA(char *buf,int size,int rwflag,void *data)
{
	rwflag=rwflag;	// pour eviter un warning lors de la compilation
	data=data;
	
	if( size < (int) (strlen(MotDePasseClePriveeServeurTERMINOA)+1) ) return 0;

	strcpy(buf,MotDePasseClePriveeServeurTERMINOA);

	return( strlen(buf) );
}

// FONCTION DE HANDLER DU SIGNAL SIGPIPE
//
// CE:	Le systeme passe le signal ;
//
// CS:	-
//
void FnHandlerSIGPIPETerminOA(int signal)
{
	std::cerr << "PointCommServeurChiffreMonoClient: Signal " << signal << "->SIGPIPE<- recu par le processus." << std::endl;
}

/*
// Client en test
char MotDePasseClePriveeClient1TERMINOA[TAILLE_MAX_MDP_CLE_PRIVEE];	// Mot de passe pour acces a la cle privee du client

int FnMotDePasseClePriveeChiffreeClient1TERMINOA(char *buf,int size,int rwflag,void *data)
{
	rwflag=rwflag;	// pour eviter un warning lors de la compilation
	data=data;
	
	if( size < (int) (strlen(MotDePasseClePriveeClient1TERMINOA)+1) ) return 0;

	strcpy(buf,MotDePasseClePriveeClient1TERMINOA);

	return( strlen(buf) );
}
*/

QSemaphore SemaphoreSyncLancementThreadTerminOA(2);

// - Fin Pour les processus legers -----------------------------------------------------------------------------------------------------


// Fonction principale de l'application
//
int main(int argc, char **argv)
{
	int lancement=true;			// Drapeau pour savoir si on doit lancer l'application
	int ArretSystemeEnQuittant=false;	// Drapeau pour le lancement de l'arret du systeme en quittant l'application
	
	// Renseignements KDE
	//
	KAboutData about("terminoa", I18N_NOOP("TerminOA"), version, description,KAboutData::License_GPL, "(C) 2006 David Romeuf", 0, 0, "David.Romeuf@univ-lyon1.fr");
	about.addAuthor( "David Romeuf", 0, "David.Romeuf@univ-lyon1.fr" );
	
	
	// Initialisation des options de la ligne de commande (avec les Qt et KDE specifiques)
	//
	KCmdLineArgs::init(argc, argv, &about);
	
	// Ajout des options possibles sur la ligne de commande supportees par l'application
	//
	KCmdLineArgs::addCmdLineOptions(options);
	
	// Acces aux arguments reconnus par l'application
	//
	KCmdLineArgs *arguments=KCmdLineArgs::parsedArgs();
	
	// On test la validite des arguments
	//
	if( !QDir(arguments->getOption(OptionsLC[CheminRepTerminal])).exists() )
	{
		std::cerr << "TerminOA: ERREUR: Le repertoire " << arguments->getOption(OptionsLC[CheminRepTerminal]) << " n'existe pas." << std::endl;
		lancement=false;
	}
	
	if( !QDir(arguments->getOption(OptionsLC[CheminRepDiaporama])).exists() )
	{
		std::cerr << "TerminOA: ERREUR: Le repertoire " << arguments->getOption(OptionsLC[CheminRepDiaporama]) << " n'existe pas." << std::endl;
		lancement=false;
	}
	
	if( !QDir(arguments->getOption(OptionsLC[CheminRepDocuments])).exists() )
	{
		std::cerr << "TerminOA: ERREUR: Le repertoire " << arguments->getOption(OptionsLC[CheminRepDocuments]) << " n'existe pas." << std::endl;
		lancement=false;
	}
		
	if( !QDir(arguments->getOption(OptionsLC[CheminMntClesUSB])).exists() )
	{
		std::cerr << "TerminOA: ERREUR: Le repertoire " << arguments->getOption(OptionsLC[CheminMntClesUSB]) << " n'existe pas." << std::endl;
		lancement=false;
	}
	
	struct in_addr AdresseClient;
	
	if( !inet_aton(arguments->getOption(OptionsLC[AdresseClientAutorise]),&AdresseClient) )
	{
		std::cerr << "TerminOA: ERREUR: L'adresse du client autorise " << arguments->getOption(OptionsLC[AdresseClientAutorise]) << "est invalide." << std::endl;
		lancement=false;
	}
	AdresseClient.s_addr=ntohl(AdresseClient.s_addr);
	
	if( QString(arguments->getOption(OptionsLC[ArretSysteme])) == QString("o") ) ArretSystemeEnQuittant=true;
	
	
	if( lancement )
	{
		// Creation d'un objet application KDE
		//
		KApplication appli;
		
		// Pointeur sur un objet de fenetre principale KDE
		//
		TerminOA *FenetrePrincipale=0;		// Pointeur sur objet fenetre principale de notre application

		// Si l'application est restauree par le gestionnaire de session
		//
		if( appli.isRestored() )
		{
			// On restaure l'application a l'aide de l'objet de configuration de la session sauve lors de la fermeture de session
			//
			RESTORE(TerminOA((QString(arguments->getOption(OptionsLC[CopieDocumentsPermise])) == QString("o")) ? true : false,QString(arguments->getOption(OptionsLC[DureeInacDeclModeDiapo])).toInt(),QString(arguments->getOption(OptionsLC[DureeEntreDiapo])).toInt(),QString(arguments->getOption(OptionsLC[PeriodeRapEcrLanc])).toInt(),arguments->getOption(OptionsLC[CheminRepTerminal]),arguments->getOption(OptionsLC[CheminRepDiaporama]),arguments->getOption(OptionsLC[CheminRepDocuments]),arguments->getOption(OptionsLC[CheminFichImgWeb]),arguments->getOption(OptionsLC[CheminMntClesUSB]),&appli));
		}
		else
		{
			// Pas de restauration de session donc on demarre l'application normalement
			//
			
			
			// Creation de l'objet fenetre principale de l'application
			//
        		if( (FenetrePrincipale=new (std::nothrow) TerminOA((QString(arguments->getOption(OptionsLC[CopieDocumentsPermise])) == QString("o")) ? true : false,QString(arguments->getOption(OptionsLC[DureeInacDeclModeDiapo])).toInt(),QString(arguments->getOption(OptionsLC[DureeEntreDiapo])).toInt(),QString(arguments->getOption(OptionsLC[PeriodeRapEcrLanc])).toInt(),arguments->getOption(OptionsLC[CheminRepTerminal]),arguments->getOption(OptionsLC[CheminRepDiaporama]),arguments->getOption(OptionsLC[CheminRepDocuments]),arguments->getOption(OptionsLC[CheminFichImgWeb]),arguments->getOption(OptionsLC[CheminMntClesUSB]),&appli)) == NULL )
			{
    				std::cerr << "TerminOA: ERREUR: Impossible de creer la fenetre principale KMainWindow de l'application." << std::endl;
				appli.exit(-1);
			}
				
			// On fixe la fenetre principale pour l'objet application KDE
			//
			appli.setMainWidget(FenetrePrincipale);
		
			// On fixe quelques proprietes de la fenetre principale heritees de QWidget
			//
			FenetrePrincipale->setMinimumSize(QSize(TAILLE_X_BASE_TERMINOA,TAILLE_Y_BASE_TERMINOA));
			
			// On affiche la fenetre principale
			//
        		FenetrePrincipale->show();
			
			// La fenetre principale occupera la dimension maximale
			//
			FenetrePrincipale->showMaximized();
			
			
// std::cout << "Dimension KMainWindow:" << QSize(FenetrePrincipale->size()).width() << "x" << QSize(FenetrePrincipale->size()).height() << std::endl;

		}
		
		// Lancement du processus leger serveur reseau des commandes sur le TerminOA
		//
		ProcessusLegerServeurCommandes PLServeurCommandes(FenetrePrincipale,INADDR_ANY,QString(arguments->getOption(OptionsLC[PortCanalCommandes])).toInt(),AdresseClient.s_addr,2,TIMEOUT_EMISSION,TIMEOUT_RECEPTION,5,FnHandlerSIGPIPETerminOA,arguments->getOption(OptionsLC[MdpClePriveeServeur]),MotDePasseClePriveeServeurTERMINOA,FnMotDePasseClePriveeChiffreeTERMINOA,arguments->getOption(OptionsLC[CheminFichCertCA_OA]),arguments->getOption(OptionsLC[CheminFichCertServTerminOA]),arguments->getOption(OptionsLC[CheminFichClePriveeServTerminOA]),arguments->getOption(OptionsLC[CheminFichParamDH]),"HIGH");
		
		PLServeurCommandes.start();
		
		// Lancement du processus leger serveur reseau des donnees sur le TerminOA
		//
		ProcessusLegerServeurDonnees PLServeurDonnees(FenetrePrincipale,INADDR_ANY,QString(arguments->getOption(OptionsLC[PortCanalDonnees])).toInt(),AdresseClient.s_addr,2,TIMEOUT_EMISSION,TIMEOUT_RECEPTION,QString(arguments->getOption(OptionsLC[CheminRepTerminal])+"/DerniereReception.tmp"));

		PLServeurDonnees.PLSCTerminOA=&PLServeurCommandes;
		
		PLServeurDonnees.start();
		
		// Les pointeurs entre les processus
		//
		PLServeurCommandes.threadCanalDonnees=&PLServeurDonnees;
		
		// Tant que les threads ne sont pas tous lances et operationnels
		//
		while( SemaphoreSyncLancementThreadTerminOA.available() > 0 );
		
		// FenetrePrincipale a un drapeau WDestructiveClose par defaut, elle se detruira elle meme.
		//
		int retour=appli.exec();
		
		// Si le processus leger serveur reseau des commandes sur le TerminOA tourne encore
		//  on demande la terminaison propre par lui meme
		//
		if( PLServeurCommandes.running() )
		{
			PLServeurCommandes.DemandeTerminaison();
//			PLServeurCommandes.terminate();
		}
		
		// Si le processus leger serveur reseau des donnees sur le TerminOA tourne encore
		//  on demande la terminaison propre par lui meme
		//
		if( PLServeurDonnees.running() )
		{
			PLServeurDonnees.DemandeTerminaison();
//			PLServeurDonnees.terminate();
		}
		
		
		// On attend la terminaison du processus leger avant de retourner la valeur
		//
		PLServeurCommandes.wait(TEMPS_ATTENTE_TERMINAISON_PROCESSUS_SERVEUR);
		
		// On attend la terminaison du processus leger avant de retourner la valeur
		//
		PLServeurDonnees.wait(TEMPS_ATTENTE_TERMINAISON_PROCESSUS_SERVEUR);
		
		// On lave la liste des options et arguments de la ligne de commande de l'application
		//
		arguments->clear();
		
		// Si on a demande l'arret du systeme en quittant l'application TerminOA
		//
		if( ArretSystemeEnQuittant )
		{
			std::cout << "Lancement de la demande de l'arret du systeme dans 60 secondes." << std::endl;
			
			// On utilise la commande propre du systeme via la commande sudo
			//
			// Il faut installer l'utilitaire sudo et configurer /etc/sudoers avec la ligne :
			//
			//  dromeuf jedi=NOPASSWD: /sbin/halt
			//
			//  qui permet par exemple a l'utilisateur dromeuf depuis la machine jedi, sans mot de passe,
			//   de lancer la commande /sbin/halt
			//
			system("/bin/sync ; /bin/sleep 60s ; /usr/bin/sudo /sbin/halt");
		}
		
		// Resultat de l'execution de la QApplication heritee par KApplication
		//
		return retour;
	}
}

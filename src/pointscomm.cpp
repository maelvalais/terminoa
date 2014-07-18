/*
 * MODULE DES CLASSES DE POINTS DE COMMUNICATION
 *
 * (C)David.Romeuf@univ-lyon1.fr 16/03/2006 par David Romeuf
 *
 *
 * 24-06-2014 (Maël Valais)
 * 		Il y a eu des changements dans ssl.h entre la version de 2006 et celle d'aujourd'hui :
 * 		pas mal de membres sont passés en "const". C'est le cas sur :
 *			const SSL_METHOD*
 *			const SSL_CIPHER*
 * 		Il a juste fallu corriger tout cela dans le code existant ;
 * 		pas besoin d'includes différents (ou de librairies différentes) de celles de 2014.
 *
 */
// Inclusions C++
//
#include <iostream>

using namespace std;

// Inclusions C
//
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Inclusions propres
//
#include <pointscomm.h>


//--------------------------------------------------------------------------------------------------------------------------------------


// Constructeur d'un point de communication
//
PointComm::PointComm(int pverbeux,int pTimeoutSocketPut,int pTimeoutSocketGet)
{
	IdSocket=-1;
	AdresseIP=0;
	Port=0;
	memset(&AdresseSocket,0,sizeof(struct sockaddr_in));
	
	ModeVerbeux=pverbeux;
	TimeoutSocketPut=pTimeoutSocketPut;
	TimeoutSocketGet=pTimeoutSocketGet;
	
	SocketCree=false;
}


// Destructeur d'un point de communication
//
PointComm::~PointComm()
{
}


// Passage de la socket en mode bloquant
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
//
int PointComm::SocketBloquante(void)
{
	if( IdSocket != -1 )
	{
		// Parametrage de la socket en mode bloquant pour attente sans consommation de CPU
		//
		int AttributsSocket=fcntl(IdSocket,F_GETFL);
		
		if( fcntl(IdSocket,F_SETFL,AttributsSocket & (~O_NONBLOCK)) == -1 ) return false; else return true;
	}
	
	return false;
}


// Passage de la socket en mode NON bloquant
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
//
int PointComm::SocketNonBloquante(void)
{
	if( IdSocket != -1 )
	{
		// Parametrage de la socket en mode non bloquant pour gerer un timeout
		//
		int AttributsSocket=fcntl(IdSocket,F_GETFL);
		
		if( fcntl(IdSocket,F_SETFL,AttributsSocket | O_NONBLOCK) == -1 ) return false; else return true;
	}
	
	return false;
}


// Pour savoir si des donnees sont disponibles en lecture sur la socket ?
//
// CE:	On passe le timeout en ms de cette interrogration. La fonction est bloquante en attente si timeout > 0.
//	 non bloquante et immediate si timeout=0 ou infinie si timeout < 0 (INFTIM) ;
//
// CS:	La fonction est vraie si une donnee est disponible en lecture sur la socket
//	 == 0 (fausse) si une donnee n'est pas disponible apres timeout millisecondes
//	 == -1 si une erreur s'est produite lors de cette interrogation ;
// 
int PointComm::PossibleLireDonneesSocket(int timeout)
{
	struct pollfd Evenement;	// Structure de donnees pour interroger un descripteur de socket
	
	Evenement.fd=IdSocket;
	Evenement.events=POLLIN;	// Lecture possible sur le descripteur ?
	Evenement.revents=0;		// Reponse
	
	int retour=poll(&Evenement,1,timeout);

	switch( retour )
	{
		case 1:
			// Il y a une reponse a l'interrogation
			//
			if( Evenement.revents & POLLIN ) return true; else return false;
			
		case 0:		// timeout
		case -1:	// erreur
			return retour;
			
		default:
			// Impossible logiquement donc erreur
			//
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: PossibleLireDonneesSocket(): poll(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			return -1;
	}
}


// Pour savoir si on peut ecrire des donnees sur la socket ?
//
// CE:	On passe le timeout en ms de cette interrogration. La fonction est bloquante en attente si timeout > 0.
//	 non bloquante et immediate si timeout=0 ou infinie si timeout < 0 (INFTIM) ;
//
// CS:	La fonction est vraie si on peut ecrire sur la socket
//	 == 0 (fausse) si on ne peut pas ecrire apres timeout millisecondes
//	 == -1 si une erreur s'est produite lors de cette interrogation ;
// 
int PointComm::PossibleEcrireDonneesSocket(int timeout)
{
	struct pollfd Evenement;	// Structure de donnees pour interroger un descripteur de socket
	
	Evenement.fd=IdSocket;
	Evenement.events=POLLOUT;	// Ecriture possible sur le descripteur ?
	Evenement.revents=0;		// Reponse
	
	int retour=poll(&Evenement,1,timeout);

	switch( retour )
	{
		case 1:
			// Il y a une reponse a l'interrogation
			//
			if( Evenement.revents & POLLOUT ) return true; else return false;
			
		case 0:		// timeout
		case -1:	// erreur
			return retour;
			
		default:
			// Impossible logiquement donc erreur
			//
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: PossibleEcrireDonneesSocket(): poll(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			return -1;
	}
}


// Fonction d'emission de donnees sur la socket et de traitement de son code retour
//
// CE:	On passe un pointeur vers les donnees a envoyer ;
//
//		On passe le nombre d'octets a transferer ;
//
// CS:	La fonction retourne la valeur retournee par send() ;
//
int PointComm::EnvoyerDonneesSocket(void *buf,int nb)
{
	int errno_avant=errno;		// Sauvegarde de errno
	int RetourSend;				// Valeur retournee par send()

	// Emission des donnees
	//
	RetourSend=send(IdSocket,buf,nb,0);
	
	if( RetourSend < 0 || errno != errno_avant )
	{
		if( errno != errno_avant )
		{
			RetourSend=-1;		// Pour detecter l'erreur simplement avec le code retour

			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerDonneesSocket(): send(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointComm: S->C " << RetourSend << std::endl;
	}

	return RetourSend;
}


// Fonction d'emission d'une chaine sur la socket et traitement du code de retour
//
// CE:	On passe un pointeur vers la chaine a envoyer ;
//
// CS:	La fonction retourne la valeur retournee par send() ;
//
int PointComm::EnvoyerChaineSocket(const char *chaine)
{
	if( IdSocket != -1 )
	{
		int errno_avant=errno;		// Sauvegarde de errno
		int RetourSend;			// Valeur retournee par send()
		
		// Emission de la chaine
		//
		RetourSend=send(IdSocket,chaine,strlen(chaine),0);
		
		if( RetourSend < 0 || errno != errno_avant )
		{
			if( errno != errno_avant )
			{
				RetourSend=-1;		// Pour detecter l'erreur simplement avec le code retour
				
				if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerChaineSocket(): send(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			}
		}
		else
		{
			if( ModeVerbeux ) std::cerr << "PointComm: " << RetourSend <<": " << chaine << std::endl;
		}
		
		return RetourSend;
	}
	
	return -1;
}


// Envoyer un fichier sur la socket et traitement du code de retour
//
// La fonction precede le fichier par un entier (unsigned long) qui contient la taille du fichier en octets
//
// CE:	On passe un pointeur vers le chemin d'acces et le nom du fichier ;
//
//	On passe un pointeur sur une chaine de TAILLE_IDENTITE_FICHIER caracteres qui contient en fait l'identite du fichier a transferer.
//	 L'identite du fichier contient toujours des caracteres ASCII ; 
//
// CS:	La fonction retourne le nombre d'octets transferes ou une valeur negative en cas d'erreur ;
//
long PointComm::EnvoyerFichierSocket(const char *nom,const char *id_fichier)
{
	// L'identite du fichier doit contenir TAILLE_IDENTITE_FICHIER caracteres
	//
	if( strlen(id_fichier) != TAILLE_IDENTITE_FICHIER )
	{
		if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): l'identite du fichier a envoyer ne contient pas les " << TAILLE_IDENTITE_FICHIER << " caracteres." << std::endl;
	
		return -1;
	}
	
	if( IdSocket != -1 )
	{
		int IdFichier;			// Descripteur du fichier a envoyer
		int errno_avant;		// Sauvegarde de errno
		int RetourSend;			// Valeur retournee par send()
		long NbOctetsTransferes=0;	// Le nombre d'octets transferes
		struct stat Renseignements;	// Renseignements sur le fichier a envoyer
		
		// On ouvre en lecture seule le fichier a transferer
		//
		if( (IdFichier=open(nom,O_RDONLY)) == -1 )
		{
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): open(" << nom << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			
			return IdFichier;
		}
		
		// On recupere les informations sur le fichier
		//
		if( fstat(IdFichier,&Renseignements) == -1 )
		{
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): stat(" << nom << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			
			return -1;
		}
		
		// On commence par envoyer au destinataire la taille du fichier qui va suivre dans un unsigned long
		//
		unsigned long TailleFichier=(unsigned long) Renseignements.st_size;
		
		errno_avant=errno;
		
		RetourSend=send(IdSocket,&TailleFichier,sizeof(unsigned long),0);
		
		if( RetourSend < 0 || errno != errno_avant )
		{
			if( errno != errno_avant )
			{
				if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): send(): Impossible d'envoyer la taille du fichier: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
				
				close(IdFichier);
				
				return -1;
			}
		}
		
		// On communique ensuite son identite
		//
		if( EnvoyerChaineSocket(id_fichier) != TAILLE_IDENTITE_FICHIER )
		{
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): EnvoyerChaineSocket(): Impossible d'envoyer l'identite du fichier." << std::endl;
			
			close(IdFichier);
			
			return -1;
		}
		
		// On envoie les donnees du fichier
		//
		char Buffer[TAILLE_BUFFER_ENVOYER_FICHIER];	// Buffer lecture / emission
		ssize_t RetourRead;				// Code retour de read()
		
		do
		{
			RetourRead=read(IdFichier,Buffer,TAILLE_BUFFER_ENVOYER_FICHIER);
			
			switch( RetourRead )
			{
				case -1:
					// Erreur de lecture
					//
					if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): read(): Impossible de lire un fragment du fichier: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
					
					close(IdFichier);
					
					return RetourRead;
					
				case 0:
					// Fin du fichier
					break;
					
				default:
					// Lecture de RetourRead octets dans le fichier a transferer
					//
					
					// Emission du dernier buffer lu dans le fichier au destinataire
					//
					errno_avant=errno;
					
					RetourSend=send(IdSocket,Buffer,RetourRead,0);
					
					if( RetourSend < 0 || errno != errno_avant )
					{
						if( errno != errno_avant )
						{
							if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): send(): Impossible d'envoyer un paquet de donnees au destinataire: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
							
							close(IdFichier);
							
							return -1;
						}
					}
					else
					{
						// On incremente le nombre d'octets transferes
						//
						NbOctetsTransferes+=(long) RetourSend;
					}
					
					break;
			}
			
		} while( RetourRead != 0 );
		
		// On ferme le fichier a transferer
		//
		if( close(IdFichier) == -1 )
		{
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: EnvoyerFichierSocket(): close(" << nom << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			
			return -1;
		}
		
		return NbOctetsTransferes;
	}
	
	return -1;
}


// Recevoir des donnees de la socket et les stocker
//
// CE:	On passe un pointeur vers une zone memoire de stockage ;
//
//	On passe le nombre de caracteres au maximum ;
//
// CS:	La fonction retourne le code d'erreur de recv() et/ou le nombre d'octets lus
//
int PointComm::LireDonneesSocket(void *buf,int max)
{
	int retour=-1;			// Valeur de retour de la fonction
	
	if( IdSocket != -1 )
	{
		if( (retour=recv(IdSocket,buf,max,0)) <= 0 )
		{
			if( ModeVerbeux )
			{
				std::cerr << "PointComm: ERREUR: LireDonneesSocket(): recv() : errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			}
		}
	}
	
	return retour;
}


// Recevoir nb octets sur la socket et les stocker
//
// CE:	On passe un pointeur vers une zone memoire de stockage ;
//
//		On passe le nombre d'octets a lire ;
//
// CS:	La fonction retourne le code d'erreur de recv() et/ou le nombre d'octets lus
//
int PointComm::LireNbDonneesSocket(void *buf,int nb)
{
	int n;			// Retour de la fonction de lecture
	int nbrecu=0;	// Nombre d'octets deja recus

	do
	{
		// On demande la lecture de ce qu'il reste a lire a l'indice nbrecu courant pour completer
		//
		n=LireDonneesSocket((((char *) buf))+nbrecu,nb-nbrecu);

		if( n > 0 ) nbrecu+=n; else return n;

	} while( nbrecu != nb );

	return nbrecu;
}


// Recevoir une ligne sur la socket terminee par '\n'
//
// CE:	On passe un pointeur vers une zone memoire de stockage ;
//
//	On passe le nombre de caracteres au maximum ou la ligne sera coupee si elle est plus longue ;
//
// CS:	La fonction retourne le code d'erreur de recv() et/ou le nombre d'octets lus de la ligne
//
int PointComm::LireLigneSocket(char *buf,int max)
{
	int retour=-1;			// Valeur de retour de la fonction
	
	if( IdSocket != -1 && max > 0 )
	{
		char c;			// Lecture caractere par caractere jusqu'a la fin de ligne
		
		retour=0;		// Pour l'instant aucun caractere
		
		do
		{
			int retour_recv;	// Valeur de retour de recv()
			
			if( (retour_recv=recv(IdSocket,&c,1,0)) <= 0 )
			{
				if( ModeVerbeux )
				{
					std::cerr << "PointComm: ERREUR: LireLigneSocket(): recv() : errno=" << errno << " : " << strerror(errno) << "." << std::endl;
				}
				
				return retour_recv;
			}
			
			max--;			// Un caractere de moins possible
			
			*(buf+retour)=c;	// Ajout du caractere a la ligne courante
			
			retour++;		// On a recu et ajoute un caractere de plus
			
		} while( c != '\n' && max >= 0 ); 
	}
	
	return retour;
}


// Pour savoir si l'objet est en mode verbeux
//
// CE:	-
//
// CS:	Retourne le mode d'affichage de l'objet
//
int PointComm::ObjetModeVerbeux(void)
{
	return ModeVerbeux;
}


// Destructeur d'un point de communication
//
int PointComm::CloseIdSocket(void)
{
	return close(IdSocket);
}



//--------------------------------------------------------------------------------------------------------------------------------------


// Constructeur d'un point de communication serveur non chiffre
//
// CE:	On passe true pour etre en mode verbeux, false pour aucun message ;
//
//	On passe l'adresse IP d'attachement en valeur host (0x________) ;
//
//	On passe le port d'attachement en valeur host (0x____) ;
//
//	On passe l'adresse IP du client autorise en valeur host (0x________) ;
//
//	On passe le timeout en secondes pour la tentative d'ecriture de donnees dans la socket ;
//
//	On passe le timeout en secondes pour la tentative de lecture de donnees dans la socket ;
//
PointCommServeurNonChiffreMonoClient::PointCommServeurNonChiffreMonoClient(int pverbeux,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet) : PointComm(pverbeux,pTimeoutSocketPut,pTimeoutSocketGet)
{
	// Initialisation des variables
	//
	ObjetInitialise=false;
	SocketAttachee=false;
	SocketEcoute=false;
	ClientConnecte=false;
	
	// L'adresse IP d'attachement
	//
	AdresseIP=pAdresse;
	
	// Le port IP d'attachement
	//
	Port=pPort;
	
	// L'adresse IP du client autorise
	//
	AdresseIPClientAutorise=pAdresseClient;
	
	// Le nombre de connexions pedentes maximum dans la file des connexions
	//
	NbListeClientsMax=pNbLClientsMax;
}


// Destructeur d'un point de communication serveur non chiffre
//
PointCommServeurNonChiffreMonoClient::~PointCommServeurNonChiffreMonoClient()
{
	if( IdSocketSession != -1 ) close(IdSocketSession);
	if( IdSocket != -1 ) close(IdSocket);
}


// Fonction d'initialisation du point de communication serveur non chiffre
//
int PointCommServeurNonChiffreMonoClient::Initialisation(void)
{
	ObjetInitialise=true;
	
	return true;
}


// Fonction de creation du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommServeurNonChiffreMonoClient::CreationReseau(void)
{
	// Si l'objet n'a pas ete initialise alors on le fait
	//
	if( !ObjetInitialise ) if( !Initialisation() ) return false;
	
	
	// Creation d'un point de communication pour le serveur, une socket du domaine IPv4 (PF_INET Linux),
	//  de type flux de donnees binaire echange continu avec integrite et fiabilite maximale (SOCK_STREAM)
	//  et de protocole specifique IP (/etc/protocols)
	//
	if( (IdSocket=socket(PF_INET,SOCK_STREAM,0)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Impossible d'obtenir une socket pour le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EPROTONOSUPPORT ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Type de protocole ou protocole non disponible dans ce domaine de communication." << std::endl;
			if( errno == EAFNOSUPPORT ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Famille d'adresses non supportee." << std::endl;
			if( errno == ENFILE ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Table de descripteur par processus pleine." << std::endl;
			if( errno == EMFILE ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Table des fichiers pleine." << std::endl;
			if( errno == EACCES ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Creation d'une telle socket non autorise." << std::endl;
			if( errno == ENOBUFS || errno == ENOMEM ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Pas assez de memoire pour allouer les buffers necessaires." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): socket(): Protocole demande inconnu." << std::endl;
		}

		return false;
	}

	// On parametre le descripteur de socket pour que l'application rende l'adresse et le port immediatement apres sa fermeture
	//  sans duree de retention par le noyau, et, que plusieurs sockets puissent s'attacher au meme port (SO_REUSEADDR=1)
	//
	int ParametreSocket=1;
	
	if( setsockopt(IdSocket,SOL_SOCKET,SO_REUSEADDR,&ParametreSocket,sizeof(int)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Impossible de parametrer le descripteur de la socket SO_REUSEADDR pour le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Le descripteur est un fichier et pas une socket." << std::endl;
			if( errno == ENOPROTOOPT ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Parametre option inconnue pour ce protocole." << std::endl;
			if( errno == EFAULT ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Mauvais pointeur passe en parametre." << std::endl;
		}

		return false;
	}
	
	SocketCree=true;
	
	return true;
}


// Fonction d'attachement du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommServeurNonChiffreMonoClient::AttachementReseau(void)
{
	// Si la socket n'est pas cree alors on le fait
	//
	if( !SocketCree ) if( !CreationReseau() ) return false;
	
	
	// On attache/nomme le point de communication a une adresse:port du domaine parametre (IP:PORT)
	//
	memset(&AdresseSocket,0,sizeof(struct sockaddr_in));

	AdresseSocket.sin_family=AF_INET;			// Famille de socket IP  (man ip)
	AdresseSocket.sin_addr.s_addr=htonl(AdresseIP);		// Adresse IP du point de communication
	AdresseSocket.sin_port=htons(Port);			// Le numero de port d'ecoute
	
	if( bind(IdSocket,(struct sockaddr *) &AdresseSocket,sizeof(struct sockaddr_in)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurNonChiffreMonoClient: ERREUR: AttachementReseau(): bind(): Impossible d'attacher la socket a l'adresse:port pour le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttachementReseau(): bind(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttachementReseau(): bind(): La socket possede deja une adresse." << std::endl;
			if( errno == EACCES ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttachementReseau(): bind(): L'adresse n'est utilisable qu'en super utilisateur." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttachementReseau(): bind(): Le descripteur est un fichier et pas une socket." << std::endl;
		}

		return false;
	}
	
	SocketAttachee=true;
	
	return true;
}


// Fonction d'ecoute du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommServeurNonChiffreMonoClient::EcouteReseau(void)
{
	// Si la socket n'est pas attachee alors on le fait
	//
	if( !SocketAttachee ) if( !AttachementReseau() ) return false;
	
	
	// On place l'application serveur en etat d'ecoute des connexions entrantes sur l'adresse:port definie. On fixe la limite de la liste des requetes de connexions entrantes en attente de traitement par l'application a ListeClientsMax.
	//
	if( listen(IdSocket,NbListeClientsMax) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurNonChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Impossible d'initier l'ecoute des requetes de connexions entrantes sur le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EADDRINUSE ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Une autre socket ecoute sur la meme adresse:port." << std::endl;
			if( errno == EBADF ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Le descripteur n'est pas une socket." << std::endl;
			if( errno == EOPNOTSUPP ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Ce type de socket ne supporte pas listen()." << std::endl;
		}

		return false;
	}
	
	SocketEcoute=true;
	
	return true;
}


// Attente et acceptation d'une session de connexion
//
// CE:	-
//
// CS:	La fonction est vraie en cas de connexion, IdSocketSession != -1 si le client est autorise / Fausse en cas de probleme ;
//
int PointCommServeurNonChiffreMonoClient::AttenteAccepterSessionAutorisee(void)
{
	// Si le serveur n'est pas en ecoute sur IP:Port, on le lance
	//
	if( !SocketEcoute ) if( !EcouteReseau() ) return false;
	
	
	// Aucun client n'est connecte
	//
	ClientConnecte=false;
	
	// Initialisation des variables pour traiter le client potentiel
	//
	memset(&AdresseSocketClient,0,sizeof(struct sockaddr_in));	// Tous les octets de la structure a zero pour initialisation
	socklen_t TailleStructAdresseSocketClient;			// Nombre d'octets de la structure de donnees de l'adresse du client
	TailleStructAdresseSocketClient=sizeof(struct sockaddr_in);	// obligatoire : Le nombre d'octets reserves

	// On extrait et traite la premiere entree de la liste des requetes de connexions entrantes en attente
	//
	if( (IdSocketSession=accept(IdSocket,(struct sockaddr *) &AdresseSocketClient,&TailleStructAdresseSocketClient)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Impossible de recuperer la premiere entree de la liste des requetes de connexions entrantes sur le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EAGAIN || errno == EWOULDBLOCK ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La socket est en mode non bloquante mais il n'y a aucune requete de connexion dans la liste." << std::endl;
			if( errno == EBADF ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Le descripteur n'est pas une socket." << std::endl;
			if( errno == EOPNOTSUPP ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La socket de reference n'est pas une SOCK_STREAM." << std::endl;
			if( errno == EINTR ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Appel systeme interrompu par l'arrivee d'un signal avant qu'une connexion valide ne survienne." << std::endl;
			if( errno == ECONNABORTED ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Une connexion a ete abandonnee." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La socket n'est pas en attente de connexion." << std::endl;
			if( errno == EMFILE ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La limite de la table des descripteur est atteinte." << std::endl;
			if( errno == EFAULT ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): L'adresse n'est pas dans l'espace d'adressage accessible en ecriture." << std::endl;
			if( errno == ENOBUFS || errno == ENOMEM ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Pas assez de memoire disponible." << std::endl;
			if( errno == EPROTO ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Erreur de protocole." << std::endl;
			if( errno == EPERM ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Connexion interdite par le firewall." << std::endl;
		}

		return false;
	}
	else
	{
		// Une requete de connexion valide d'un client est a traiter
		//
		if( TailleStructAdresseSocketClient == sizeof(struct sockaddr_in) )
		{
			if( ModeVerbeux )
			{
				char Chaine[TAILLE_MAXI_CHAINE_COUT_NET];
				
				sprintf(Chaine,"PointCommServeurNonChiffreMonoClient: Requete de connexion entrante du client %s(0x%X):%d",inet_ntoa(AdresseSocketClient.sin_addr),ntohl(AdresseSocketClient.sin_addr.s_addr),AdresseSocketClient.sin_port);
				
				std::cerr << Chaine << std::endl;
			}
		}
		else
		{
			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): Code a reecrire pour traiter et afficher l'adresse du client apres accept()." << std::endl;
			
			return false;
		}

		// Comme le serveur est mono-client on filtre l'adresse du client deja a ce niveau
		//
		if( ntohl(AdresseSocketClient.sin_addr.s_addr) == AdresseIPClientAutorise )
		{
			// Le client est autorise
			//
			struct timeval timeout_sock_put;	// Timeout socket pour l'emission
			struct timeval timeout_sock_get;	// Timeout socket pour la reception
			
			// La socket de session repasse dans le mode bloquant pour la suite des echanges
			//
			SocketSessionBloquante();
			
			// On fixe un timeout pour l'emission
			//
			timeout_sock_put.tv_sec=TimeoutSocketPut;
			timeout_sock_put.tv_usec=0;
			
			if( setsockopt(IdSocketSession,SOL_SOCKET,SO_SNDTIMEO,(void *) &timeout_sock_put,sizeof(struct timeval)) == -1 )
			{
				if( ModeVerbeux )
				{
					char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_SSL];
				
					sprintf(ChaineErreur,"PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): setsockopt(): Impossible de parametrer SO_SNDTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
					std::cerr << ChaineErreur << std::endl;
					
					if( errno == EBADF ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
					if( errno == ENOPROTOOPT ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
				}
			}
			
			// On fixe un timeout pour la reception
			//
			timeout_sock_get.tv_sec=TimeoutSocketGet;
			timeout_sock_get.tv_usec=0;
			
			if( setsockopt(IdSocketSession,SOL_SOCKET,SO_RCVTIMEO,(void *) &timeout_sock_get,sizeof(struct timeval)) == -1 )
			{
				if( ModeVerbeux )
				{
					char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_SSL];
				
					sprintf(ChaineErreur,"PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): setsockopt(): Impossible de parametrer SO_RCVTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
					std::cerr << ChaineErreur << std::endl;
				
					if( errno == EBADF ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
					if( errno == ENOPROTOOPT ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
				}
			}
			
			// Information de connexion
			//
			if( ModeVerbeux )
			{
				char Chaine[TAILLE_MAXI_CHAINE_COUT_NET];
				
				sprintf(Chaine,"PointCommServeurNonChiffreMonoClient: Connexion acceptee avec le client %s(0x%X):%d",inet_ntoa(AdresseSocketClient.sin_addr),ntohl(AdresseSocketClient.sin_addr.s_addr),AdresseSocketClient.sin_port);
				
				std::cerr << Chaine << std::endl;
			}
			
			// Un client est connecte
			//
			ClientConnecte=true;
			
			return true;
		}
		else
		{
			// Ce client n'est pas autorise, on ferme la requete de connexion et on en attend une autre
			//
			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: Ce client n'est pas autorise, connexion refusee." << std::endl;
			
			close(IdSocketSession);
			IdSocketSession=-1;
			
			ClientConnecte=false;
			
			return true;
		}
	}
	
	ClientConnecte=false;
	
	return false;
}


// Fonction pour savoir si une session a ete acceptee et autorisee
//
// CE:	-
//
// CS:	La fonction est vraie si la session est autorisee
//
int PointCommServeurNonChiffreMonoClient::SessionAccepteeAutorisee(void)
{
	if( (IdSocketSession != -1) && (ntohl(AdresseSocketClient.sin_addr.s_addr) == AdresseIPClientAutorise) )
	{
		ClientConnecte=true;
		
		return true;
	}
	
	ClientConnecte=false;
	
	return false; 
}


// Pour savoir si un client est actuellement connecte ?
//
// CE:	-
//
// CS:	Vrai si un client est connecte ;
//
int PointCommServeurNonChiffreMonoClient::UnClientEstConnecte(void)
{
	return ClientConnecte;
}


// Fermeture de la connexion courante
//
// CE:	-
//
// CS:	-
//
void PointCommServeurNonChiffreMonoClient::FermetureSession(void)
{
	// On signale au client que la socket ne sera plus utilisee
	//
	if( shutdown(IdSocketSession,SHUT_RDWR) < 0 )
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: FermetureSession(): shutdown(): Impossible de signaler au client que la socket ne sera plus utilisee." << std::endl;
	}
	
	// Fermeture de la socket de session
	//
	int retour=close(IdSocketSession);
	
	// Si la fermeture s'est bien passee
	//
	if( !retour )
	{
		if( ModeVerbeux )
		{
			char Chaine[TAILLE_MAXI_CHAINE_COUT_NET];
			
			sprintf(Chaine,"PointCommServeurNonChiffreMonoClient: Connexion fermee avec le client %s(0x%X):%d",inet_ntoa(AdresseSocketClient.sin_addr),ntohl(AdresseSocketClient.sin_addr.s_addr),AdresseSocketClient.sin_port);
			
			std::cerr << Chaine << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: FermetureSession(): close(): Impossible de fermer la connexion." << std::endl;
	}
	
	// Le client n'est pas connecte
	//
	ClientConnecte=false;
	
	// Descripteur de la socket de session initialise
	//
	IdSocketSession=-1;
}


// Fonction d'emission d'une chaine sur la socket de session et de traitement de son code retour
//
// CE:	On passe un pointeur vers la chaine a envoyer ;
//
// CS:	La fonction retourne la valeur retournee par send() ;
//
int PointCommServeurNonChiffreMonoClient::EnvoyerChaineSocketSession(const char *chaine)
{
	int errno_avant=errno;		// Sauvegarde de errno
	int RetourSend;			// Valeur retournee par send()
	
	// Emission de la chaine
	//
	RetourSend=send(IdSocketSession,chaine,strlen(chaine),0);
	
	if( RetourSend < 0 || errno != errno_avant )
	{
		if( errno != errno_avant )
		{
			RetourSend=-1;		// Pour detecter l'erreur simplement avec le code retour
			
			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: EnvoyerChaineSocketSession(): send(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: S->C " << RetourSend <<": " << chaine << std::endl;
	}

	return RetourSend;
}


// Fonction d'emission de donnees sur la socket de session et de traitement de son code retour
//
// CE:	On passe un pointeur vers les donnees a envoyer ;
//
//		On passe le nombre d'octets a transferer ;
//
// CS:	La fonction retourne la valeur retournee par send() ;
//
int PointCommServeurNonChiffreMonoClient::EnvoyerDonneesSocketSession(void *buf,int nb)
{
	int errno_avant=errno;		// Sauvegarde de errno
	int RetourSend;				// Valeur retournee par send()

	// Emission des donnees
	//
	RetourSend=send(IdSocketSession,buf,nb,0);
	
	if( RetourSend < 0 || errno != errno_avant )
	{
		if( errno != errno_avant )
		{
			RetourSend=-1;		// Pour detecter l'erreur simplement avec le code retour

			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: EnvoyerDonneesSocketSession(): send(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: S->C " << RetourSend << std::endl;
	}

	return RetourSend;
}


// Recevoir des donnees de la socket de session et les stocker
//
// CE:	On passe un pointeur vers une zone memoire de stockage ;
//
// CS:	La fonction retourne le code d'erreur de recv()
//
int PointCommServeurNonChiffreMonoClient::LireDonneesSocketSession(void *buf,int max)
{
	int retour=-1;			// Valeur de retour de la fonction
	
	if( IdSocketSession != -1 )
	{
		if( (retour=recv(IdSocketSession,buf,max,0)) < 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: LireDonneesSocketSession(): LireDonnees(): recv() : errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
	}
	
	return retour;
}


// Recevoir un fichier par la socket de session
//
// Le fichier est precede par un entier (unsigned long) qui contient la taille du fichier en octets.
//
// CE:	On passe le chemin d'acces et le nom du fichier dans lequel le stocker ;
//
//	On passe un pointeur sur char sur un espace de TAILLE_IDENTITE_FICHIER+1 octets qui contiendra l'identite du fichier.
//	 l'identite du fichier est toujours une chaine de caracteres ASCII ;
//
// CS:	La fonction retourne le nombre d'octets recus, une valeur negative dans le cas d'une erreur ;
//
long PointCommServeurNonChiffreMonoClient::RecevoirFichierSocketSession(const char *nom,char *identite)
{
	if( IdSocketSession != -1 )
	{
		int errno_avant=errno;					// Sauvegarde de errno
		int IdFichier;							// Descripteur du fichier de stockage
		int RetourRecv;							// Code retour de recv()
		int RetourWrite;						// Code retour de write()
		time_t TempsDebutReception=time(NULL);	// Le temps au debut de la reception
		unsigned long NbOctetsRecus=0;			// Nombre d'octets recus
		unsigned long TailleFichier=0;			// Pour stocker la taille du fichier
		

		// Reception de la taille du fichier
		//
		NbOctetsRecus=0;

		do
		{
			if( (RetourRecv=recv(IdSocketSession,((char *) &TailleFichier)+NbOctetsRecus,sizeof(unsigned long)-NbOctetsRecus,0)) <= 0 )
			{
				if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: RecevoirFichierSocketSession(): recv(): Impossible de recuperer la taille du fichier: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			
				return RetourRecv;
			}

			NbOctetsRecus+=RetourRecv;

		} while( NbOctetsRecus < sizeof(unsigned long) );

		// Reception de l'identite du fichier : ATTENTION toujours des caracteres ASCII
		//
		NbOctetsRecus=0;

		do
		{
			if( (RetourRecv=recv(IdSocketSession,identite+NbOctetsRecus,TAILLE_IDENTITE_FICHIER-NbOctetsRecus,0)) <= 0 )
			{
				if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: RecevoirFichierSocketSession(): recv(): Impossible de recuperer l'identite du fichier: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			
				for( int i=0; i <= TAILLE_IDENTITE_FICHIER; i++) *(identite+i)=0;
			
				return RetourRecv;
			}

			NbOctetsRecus+=RetourRecv;

		} while( NbOctetsRecus < TAILLE_IDENTITE_FICHIER );

		*(identite+TAILLE_IDENTITE_FICHIER)=0;


		// Creation du fichier
		//
		if( (IdFichier=open(nom,O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP)) == -1 )
		{
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: RecevoirFichierSocketSession(): open(" << nom << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			
			return IdFichier;
		}
		
		// On attend TailleFichier octets sur sur la socket sans depassement de temps
		//
		NbOctetsRecus=0;

		char Buffer[TAILLE_BUFFER_RECEVOIR_FICHIER];	// Buffer lecture / enregistrement
		
		do
		{
			// Si on est bien dans les temps
			//
			if( (time(NULL)-TempsDebutReception) > TimeoutSocketGet )
			{
				close(IdFichier);
				
				return -1;
			}
			
			// Attente et lecture d'un paquet du message
			//
			RetourRecv=recv(IdSocketSession,Buffer,((TailleFichier-NbOctetsRecus) >= TAILLE_BUFFER_RECEVOIR_FICHIER ) ? TAILLE_BUFFER_RECEVOIR_FICHIER : (TailleFichier-NbOctetsRecus),0);

			switch( RetourRecv )
			{
				case -1:
					// Erreur de lecture sur la socket
					//
					if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: RecevoirFichierSocketSession(): recv(): Impossible de lire un fragment du fichier sur la socket de session: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
					
					close(IdFichier);
					
					return RetourRecv;
				
				default:
					// Lecture de RetourRecv octets dans la socket ok
					//
					
					// Ecriture du dernier buffer lu dans la socket sur le fichier
					//
					errno_avant=errno;
					
					RetourWrite=write(IdFichier,Buffer,RetourRecv);
					
					if( RetourWrite < 0 || errno != errno_avant || RetourWrite != RetourRecv )
					{
						if( errno != errno_avant )
						{
							if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: RecevoirFichierSocketSession(): write(): Impossible d'ecrire un paquet de donnees dans le fichier: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
							
							close(IdFichier);
							
							return -1;
						}
					}
					else
					{
						// On incremente le nombre d'octets transferes
						//
						NbOctetsRecus+=(unsigned long) RetourWrite;
					}
					
					break;
			}
			
		} while( NbOctetsRecus < TailleFichier );
		
		// On ferme le fichier
		//
		if( close(IdFichier) == -1 )
		{
			if( ModeVerbeux ) std::cerr << "PointComm: ERREUR: RecevoirFichierSocketSession(): close(" << nom << "): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			
			return -1;
		}
		
		return (long) NbOctetsRecus;
	}
	
	return -1;
}


// Fonction d'emission d'un caractere hors bande sur la socket de session
//
// ATTENTION: Pour que le caractere hors bande soit correctement lu vers le destinataire, il ne doit pas etre le seul dans le tampon
//	       de reception du destinataire. C'est la raison pour laquelle, le caractere hors bande est en fait le dernier d'une
//	       petite chaine de caractere envoyee comme "caractere hors bande", sinon, s'il est seul dans le tampon de reception
//	       du destinataire, alors celui-ci ne le detecte pas comme lisible, ce qui peut conduire au bloquage de l'algorithme.
//
// CE:	-
//
// CS:	La fonction est vraie si l'emission a reussie ;
//
int PointCommServeurNonChiffreMonoClient::EcrireCaractereHorsBandeSocketSession(void)
{
	// Si le descripteur de la socket de session est coherent
	//
	if( IdSocketSession != -1 )
	{
		if( send(IdSocketSession,CARACTERE_HORS_BANDE,strlen(CARACTERE_HORS_BANDE),MSG_OOB) <= 0 ) return false; else return true;
	}
	
	return false;
}


// Fonction de lecture jusqu'au caractere hors bande sur la socket de session
//
// CE:	ATTENTION : La socket de session doit etre en mode d'inclusion du caractere hors bande = SO_OOBINLINE ;
//
// CS:	La fonction est vraie si l'operation de lecture a reussie ;
//
int PointCommServeurNonChiffreMonoClient::LireJusquAuCaractereHorsBandeSocketSession(void)
{
	// Si le descripteur de la socket de session est coherent
	//
	if( IdSocketSession != -1 )
	{
		int CaractereHorsBande=0;		// Indicateur de caractere hors bande vrai ou faux
		char buffer[TAILLE_BUFFER_SOCKET];	// Buffer de stockage en reception perdue
		
		// CaractereHorsBande est vraie si le prochain caractere a lire est un caractere hors bande
		//
		if( ioctl(IdSocketSession,SIOCATMARK,&CaractereHorsBande) < 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: LireJusquAuCaractereHorsBandeSocketSession(): ioctl(): Test prochain caractere hors bande ? : errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
		
		// Tant que le prochain caractere a lire n'est pas le caractere hors bande
		//
		while( !CaractereHorsBande )
		{
			// On lit les donnees en pure perte jusqu'au caractere hors bande
			//
			if( recv(IdSocketSession,buffer,TAILLE_BUFFER_SOCKET,0) <= 0 )
			{
				// Erreur de lecture ou timeout
				//
				if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: LireJusquAuCaractereHorsBandeSocketSession(): recv() lecture jusqu'au caractere hors bande: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
				
				return false;
			}
			
			if( ioctl(IdSocketSession,SIOCATMARK,&CaractereHorsBande) < 0 )
			{
				if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: LireJusquAuCaractereHorsBandeSocketSession(): ioctl(): Test prochain caractere hors bande ? : errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			}
		}
		
		// Le prochain caractere a lire est le caractere hors bande, on le lit (buffer[0] contient le caractere)
		//
		if( recv(IdSocketSession,buffer,1,0) <= 0 )
		{
			if( ModeVerbeux )
			{
				if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: LireJusquAuCaractereHorsBandeSocketSession(): recv() MSG_OOB: erreur de lecture du caractere hors bande: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			}
			
			return false;
		}
		
		return true;
	}
	
	return false;
}


// Passage de la socket de session en mode lecture en ligne du caractere hors bande
//
// CE:	-
//
// CS:	La fonction retourne le code d'erreur de setsockopt()
//
int PointCommServeurNonChiffreMonoClient::SocketSessionHorsBandeInLine(void)
{
	if( IdSocketSession != -1 )
	{
		int On=1;
		
		return setsockopt(IdSocketSession,SOL_SOCKET,SO_OOBINLINE,&On,sizeof(int));
	}
	
	return -1;
}


// Passage de la socket de session courante en mode bloquant
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
//
int PointCommServeurNonChiffreMonoClient::SocketSessionBloquante(void)
{
	if( IdSocketSession != -1 )
	{
		// Parametrage de la socket en mode bloquant pour attente sans consommation de CPU
		//
		int AttributsSocketSession=fcntl(IdSocketSession,F_GETFL);
		
		if( fcntl(IdSocketSession,F_SETFL,AttributsSocketSession & (~O_NONBLOCK)) == -1 ) return false; else return true;
	}
	
	return false;
}


// Passage de la socket de session courante en mode NON bloquant
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
//
int PointCommServeurNonChiffreMonoClient::SocketSessionNonBloquante(void)
{
	if( IdSocketSession != -1 )
	{
		// Parametrage de la socket en mode non bloquant pour gerer un timeout
		//
		int AttributsSocketSession=fcntl(IdSocketSession,F_GETFL);
		
		if( fcntl(IdSocketSession,F_SETFL,AttributsSocketSession | O_NONBLOCK) == -1 ) return false; else return true;
	}
	
	return false;
}


// Pour savoir si des donnees sont disponibles en lecture sur la socket ?
//
// CE:	On passe le timeout en ms de cette interrogration. La fonction est bloquante en attente si timeout > 0.
//	 non bloquante et immediate si timeout=0 ou infinie si timeout < 0 (INFTIM) ;
//
// CS:	La fonction est vraie si une donnee est disponible en lecture sur la socket
//	 == 0 (fausse) si une donnee n'est pas disponible apres timeout millisecondes
//	 == -1 si une erreur s'est produite lors de cette interrogation ;
// 
int PointCommServeurNonChiffreMonoClient::PossibleLireDonneesSocketSession(int timeout)
{
	struct pollfd Evenement;	// Structure de donnees pour interroger un descripteur de socket
	
	Evenement.fd=IdSocketSession;
	Evenement.events=POLLIN;	// Lecture possible sur le descripteur ?
	Evenement.revents=0;		// Reponse
	
	int retour=poll(&Evenement,1,timeout);

	switch( retour )
	{
		case 1:
			// Il y a une reponse a l'interrogation
			//
			if( Evenement.revents & POLLIN ) return true; else return false;
			
		case 0:		// timeout
		case -1:	// erreur
			return retour;
			
		default:
			// Impossible logiquement donc erreur
			//
			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: PossibleLireDonneesSocketSession(): poll(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			return -1;
	}
}


// Pour savoir si on peut ecrire des donnees sur la socket ?
//
// CE:	On passe le timeout en ms de cette interrogration. La fonction est bloquante en attente si timeout > 0.
//	 non bloquante et immediate si timeout=0 ou infinie si timeout < 0 (INFTIM) ;
//
// CS:	La fonction est vraie si on peut ecrire sur la socket
//	 == 0 (fausse) si on ne peut pas ecrire apres timeout millisecondes
//	 == -1 si une erreur s'est produite lors de cette interrogation ;
// 
int PointCommServeurNonChiffreMonoClient::PossibleEcrireDonneesSocketSession(int timeout)
{
	struct pollfd Evenement;	// Structure de donnees pour interroger un descripteur de socket
	
	Evenement.fd=IdSocketSession;
	Evenement.events=POLLOUT;	// Ecriture possible sur le descripteur ?
	Evenement.revents=0;		// Reponse
	
	int retour=poll(&Evenement,1,timeout);

	switch( retour )
	{
		case 1:
			// Il y a une reponse a l'interrogation
			//
			if( Evenement.revents & POLLOUT ) return true; else return false;
			
		case 0:		// timeout
		case -1:	// erreur
			return retour;
			
		default:
			// Impossible logiquement donc erreur
			//
			if( ModeVerbeux ) std::cerr << "PointCommServeurNonChiffreMonoClient: ERREUR: PossibleEcrireDonneesSocketSession(): poll(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			return -1;
	}
}


//--------------------------------------------------------------------------------------------------------------------------------------


// Constructeur d'un point de communication serveur chiffre mono client
//
// CE:	On passe true pour etre en mode verbeux, false pour aucun message ;
//
//	On passe l'adresse IP d'attachement en valeur host (0x________) ;
//
//	On passe le port d'attachement en valeur host (0x____) ;
//
//	On passe l'adresse IP du client autorise en valeur host (0x________) ;
//
//	On passe le timeout en secondes pour la tentative d'ecriture de donnees dans la socket ;
//
//	On passe le timeout en secondes pour la tentative de lecture de donnees dans la socket ;
//
//	On passe le timeout en secondes de l'initiative de la negociation TLS/SSL ;
//
//	On passe true si on veut parametrer la redirection de SIGPIPE (ecriture dans tube ferme) ou false dans le cas contraire ;
//
//	On passe un pointeur sur la fonction C de handler du signal SIGPIPE ;
//
//	On passe un pointeur sur une chaine de char qui contient le mot de passe pour acceder a la cle privee du serveur SSL
//	 Ce mot de passe ne doit pas contenir plus de TAILLE_MAX_MDP_CLE_PRIVEE-1 caracteres ;
//
//	On passe un pointeur sur char vers un buffer de stockage du mot de passe pour acceder a la cle privee du serveur SSL
//	 Ce buffer doit etre reserve avec TAILLE_MAX_MDP_CLE_PRIVEE elements ;
//
//	On passe un pointeur sur la fonction C appelee par la librairie SSL lors de la demande du mot de passe pour acceder a la cle
//	 privee du serveur SSL stockee dans un fichier PEM ;
//
//	On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM du certificat
//	 de l'autorite de certification CA qui a signe les certificats du serveur ;
//
//	On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM du certificat du serveur ;
//
//	On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM de la cle privee du serveur ;
//
//	On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM des parametres Diffie-Hellman aleatoires ;
//
//	On passe la chaine de la liste des chiffreurs que le serveur doit utiliser ;
//
// CS:	-
//
PointCommServeurChiffreMonoClient::PointCommServeurChiffreMonoClient(int pverbeux,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet,int pTimeoutNegoTLSSSL,int pParamSIGPIPE,void (*pFnHandlerSIGPIPE)(int),const char *MdpClePriveeServeur,char *BuffStockMdpClePriveeServeur,int (*pFnMotDePasseClePriveeChiffree)(char*, int, int, void*),const char *pCheminCertificatCA,const char *pCheminCertificatServeur,const char *pCheminClePriveeServeur,const char *pParametresDH,const char *pListeChiffreurs) : PointComm(pverbeux,pTimeoutSocketPut,pTimeoutSocketGet)
{
	// Initialisation des variables
	//
	IdSocket=-1;			// Socket de la communication serveur
	IdSocketSession=-1;		// Socket de la communication pour la session courante acceptee
	ContexteSSL=NULL;		// Le contexte SSL de l'objet sera a creer
	ObjetBIO=NULL;			// L'objet de communication de la couche SSL
	ParametresDH=NULL;		// Le pointeur vers les parametres Diffie-Hellman
	SocketSessionSSL_BIO=NULL;	// Pointeur vers socket de session SSL dans un objet BIO
	StructSSLServeur=NULL;		// Pointeur vers structure SSL du serveur
	ConnexionSSL=NULL;		// Pointeur vers un objet BIO de la connexion SSL
	ConnexionBuffSSL=NULL;		// Pointeur vers un objet BIO de la connexion SSL en mode buffer ligne
	SSLInitialisee=false;		// La couche SSL n'est pas initialisee pour l'instant
	SocketAttachee=false;		// La socket n'est pas attachee pour l'instant
	SocketEcoute=false;		// La socket n'est pas a l'ecoute pour l'instant
	ClientConnecte=false;		// Aucun client n'est connecte actuellement
	
	// Initialisation du contexte SSL qui contient les cles, les certificats a utiliser pour des connexions
	//
	SSL_load_error_strings();	// Chargement des chaines d'erreurs de la librairie
	
	SSL_library_init();		// Initialisation de la librairie OpenSSL : Chargement des differents algorithmes de chiffrement...
	
	MethodeVersionSSL=SSLv23_method();	// Methode du protocole SSL a utiliser : il existe aussi SSLv23_server_method()
	
	// L'adresse IP d'attachement
	//
	AdresseIP=pAdresse;
	
	// Le port IP d'attachement
	//
	Port=pPort;
	
	// L'adresse IP du client autorise
	//
	AdresseIPClientAutorise=pAdresseClient;
	
	// Le nombre de connexions pedentes maximum dans la file des connexions
	//
	NbListeClientsMax=pNbLClientsMax;
	
	// Timeout en secondes de l'initiative de la negociation TLS/SSL
	//
	TimeoutSSLAccept=pTimeoutNegoTLSSSL;
	
	// Recopie du mot de passe dans le buffer de stockage passe au constructeur
	//
	if( strlen(MdpClePriveeServeur) < (TAILLE_MAX_MDP_CLE_PRIVEE-1) ) strcpy(BuffStockMdpClePriveeServeur,MdpClePriveeServeur); else *BuffStockMdpClePriveeServeur=0;
	
	// Pointeur sur la fonction C appelee lors de l'acces a un fichier PEM chiffre par un mot de passe (typiquement pour l'acces
	//  a la cle privee, qui est conservee chiffree par la passphrase PEM, ou mot de passe)
	//
	FnMotDePasseClePriveeChiffree=pFnMotDePasseClePriveeChiffree;
	
	// Si on doit ou non parametrer le signal SIGPIPE
	//
	ParamSIGPIPE=pParamSIGPIPE;
	
	// Pointeur sur la fonction C de handler du signal SIGPIPE
	//
	FnHandlerSIGPIPE=pFnHandlerSIGPIPE;
	
	// On stocke le nom du fichier du certificat de l'autorite de certification CA
	//
	if( strlen(pCheminCertificatCA) < (TAILLE_MAXI_CHAINE_FICHIER_SSL-1) ) strcpy(CertificatCA,pCheminCertificatCA); else *CertificatCA=0;
	
	// On stocke le nom du fichier du certificat du serveur
	//
	if( strlen(pCheminCertificatServeur) < (TAILLE_MAXI_CHAINE_FICHIER_SSL-1) ) strcpy(CertificatServeur,pCheminCertificatServeur); else *CertificatServeur=0;
	
	// On stocke le nom du fichier de la cle privee du serveur
	//
	if( strlen(pCheminClePriveeServeur) < (TAILLE_MAXI_CHAINE_FICHIER_SSL-1) ) strcpy(ClePriveeServeur,pCheminClePriveeServeur); else *ClePriveeServeur=0;
	
	// On stocke le nom du fichier des parametres Diffie-Hellman
	//
	if( strlen(pParametresDH) < (TAILLE_MAXI_CHAINE_FICHIER_SSL-1) ) strcpy(ChemParametresDH,pParametresDH); else *ChemParametresDH=0;
	
	// On stocke la chaine de la liste des chiffreurs que le serveur doit utiliser
	//
	if( strlen(pListeChiffreurs) < (TAILLE_MAXI_CHAINE_VARIABLE_SSL-1) ) strcpy(ListeChiffreurs,pListeChiffreurs); else *ListeChiffreurs=0;
}


// Destructeur d'un point de communication serveur chiffre
//
PointCommServeurChiffreMonoClient::~PointCommServeurChiffreMonoClient()
{
	if( ConnexionBuffSSL != NULL ) BIO_free(ConnexionBuffSSL);
	if( ConnexionSSL != NULL ) BIO_free(ConnexionSSL);
	//if( StructSSLServeur != NULL && SocketSessionSSL_BIO == NULL ) SSL_free(StructSSLServeur);	// Il est libere par BIO_free(SocketSessionSSL_BIO) dans le cas d'une connexion realisee  SINON CRASH DE L'APPLICATION
	if( SocketSessionSSL_BIO != NULL ) BIO_free(SocketSessionSSL_BIO);
	if( IdSocketSession != -1 ) close(IdSocketSession);
	if( IdSocket != -1 ) close(IdSocket);
	if( ParametresDH != NULL ) DH_free(ParametresDH);
	if( ObjetBIO != NULL ) BIO_free(ObjetBIO);
	if( ContexteSSL != NULL ) SSL_CTX_free(ContexteSSL);
	ERR_free_strings();
}


// Fonction d'initialisation du point de communication serveur chiffre
//
int PointCommServeurChiffreMonoClient::Initialisation(void)
{
	// Creation d'un objet de contexte SSL selon une methode de version du protocole
	//
	if( (ContexteSSL=SSL_CTX_new(MethodeVersionSSL)) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_new() : impossible de creer le contexte SSL." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// Enregistrement dans le contexte SSL du serveur de la fonction appelee lors d'un acces a un fichier PEM chiffre
	//  par un mot de passe (typiquement pour l'acces a la cle privee, qui est conservee chiffre par la passphrase PEM,
	//  ou mot de passe)
	//
	SSL_CTX_set_default_passwd_cb(ContexteSSL,FnMotDePasseClePriveeChiffree);
	
	// Chargement du certificat de l'autorite de certification de confiance (le CA) dans le contexte SSL pour le serveur
	//
	if( !SSL_CTX_load_verify_locations(ContexteSSL,CertificatCA,"/etc/ssl/certs") )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_load_verify_locations(): impossible de charger le certificat de l'autorite de confiance pour les certificats de ce serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// Chargement du certificat et donc de la cle publique du serveur dans le contexte SSL pour le serveur
	//
	if( SSL_CTX_use_certificate_file(ContexteSSL,CertificatServeur,SSL_FILETYPE_PEM) != 1 )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_use_certificate_file(): impossible de charger le certificat (cle publique) du serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}

	// Chargement de la cle privee du serveur dans le contexte SSL pour le serveur
	//
	if( SSL_CTX_use_PrivateKey_file(ContexteSSL,ClePriveeServeur,SSL_FILETYPE_PEM) != 1 )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_use_PrivateKey_file(): impossible de charger la cle privee du serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// On verifie la conformite de la cle privee chargee
	//
	if( !SSL_CTX_check_private_key(ContexteSSL) )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_check_private_key(): impossible de verifier la conformite de la cle privee du serveur chargee dans le contexte." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// Chargement des parametres aleatoires Diffie-Hellman (dhparams) obtenus via la commande openssl.
	// On utilise cette methode pour ameliorer la vitesse de reponse du serveur avec un tres bon alea car la generation consomme
	//  beaucoup de temps.
	//
	if( (ObjetBIO=BIO_new_file(ChemParametresDH,"r")) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): BIO_new_file(): Impossible d'ouvrir le fichier des parametres Diffie-Hellman du serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	if( (ParametresDH=PEM_read_bio_DHparams(ObjetBIO,NULL,NULL,NULL)) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): PEM_read_bio_DHparams(): Impossible de lire le fichier des parametres Diffie-Hellman du serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	if( !BIO_free(ObjetBIO) ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): BIO_free(): Impossible de liberer ObjetBIO." << std::endl;
	ObjetBIO=NULL;
	
	// Verification des parametres Diffie-Hellman
	//
	int CodeVerifParametresDH;					// Code retour de la verification des parametres DH
	
	if( !DH_check(ParametresDH,&CodeVerifParametresDH) )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): DH_check(): Erreur lors de la verification des parametres Diffie-Hellman du serveur." << std::endl;
		
			if( CodeVerifParametresDH & DH_CHECK_P_NOT_SAFE_PRIME ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): DH_check(): DH_CHECK_P_NOT_SAFE_PRIME." << std::endl;
		
			if( CodeVerifParametresDH & DH_NOT_SUITABLE_GENERATOR ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): DH_check(): DH_NOT_SUITABLE_GENERATOR." << std::endl;
		
			if( CodeVerifParametresDH & DH_UNABLE_TO_CHECK_GENERATOR ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): DH_check(): DH_UNABLE_TO_CHECK_GENERATOR." << std::endl;
		
			AfficheErreurPileLibrairieSSL();
		}
		
		return false;
	}
	
	// On charge les parametres Diffie-Hellman dans le contexte du serveur
	//
	if( !SSL_CTX_set_tmp_dh(ContexteSSL,ParametresDH) )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_set_tmp_dh(): Impossible de charger les parametres Diffie-Hellman dans le contexte du serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// On parametre la liste des chiffreurs que le contexte peut utiliser (openssl ciphers -v 'HIGH')
	//
	if( !SSL_CTX_set_cipher_list(ContexteSSL,ListeChiffreurs) )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_set_cipher_list(): Impossible de parametrer la liste des chiffreurs utilisables dans le contexte du serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}

	// On parametre le type de verification imposee lors de la negociation, l'autentification des paires
	//  SSL_VERIFY_PEER = Serveur : demande le certificat du client, Client : verification du certificat du serveur
	//  SSL_VERIFY_FAIL_IF_NO_PEER_CERT = Serveur : si le client ne retourne pas son certificat la negociation immediatement arretee.
	//
	SSL_CTX_set_verify(ContexteSSL,SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,NULL);
	
	// Le contexte est cree et parametre, il faut maintenant lui donner un identifieur caracteristique pour pouvoir differencier
	//  les sessions (dans le cas d'importation d'une session generee depuis un autre contexte par exemple)
	//
	sprintf(IdentifieurContexteServeur,"CTXServeurSSL:%p",this);
	
	if( strlen(IdentifieurContexteServeur) < SSL_MAX_SSL_SESSION_ID_LENGTH )
	{
		if( !SSL_CTX_set_session_id_context(ContexteSSL,(const unsigned char *) IdentifieurContexteServeur,strlen(IdentifieurContexteServeur)) )
		{
			if( ModeVerbeux )
			{
				std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): SSL_CTX_set_session_id_context(): Impossible de parametrer l'identifieur des sessions crees via le contexte du serveur." << std::endl;
				AfficheErreurPileLibrairieSSL();
			}
			return false;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: Initialisation(): Chaine identifieur sessions du contexte du serveur trop longue." << std::endl;
		return false;
	}
	
	SSLInitialisee=true;			// La couche SSL est initialisee
	
	return true;
}


// Fonction de creation du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommServeurChiffreMonoClient::CreationReseau(void)
{
	// Si l'objet n'a pas ete initialise au niveau du SSL alors on le fait
	//
	if( !SSLInitialisee ) if( !Initialisation() ) return false;
	
	
	// Creation d'un point de communication pour le serveur, une socket du domaine IPv4 (PF_INET Linux),
	//  de type flux de donnees binaire echange continu avec integrite et fiabilite maximale (SOCK_STREAM)
	//  et de protocole specifique IP (/etc/protocols)
	//
	if( (IdSocket=socket(PF_INET,SOCK_STREAM,0)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Impossible d'obtenir une socket pour le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EPROTONOSUPPORT ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Type de protocole ou protocole non disponible dans ce domaine de communication." << std::endl;
			if( errno == EAFNOSUPPORT ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Famille d'adresses non supportee." << std::endl;
			if( errno == ENFILE ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Table de descripteur par processus pleine." << std::endl;
			if( errno == EMFILE ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Table des fichiers pleine." << std::endl;
			if( errno == EACCES ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Creation d'une telle socket non autorise." << std::endl;
			if( errno == ENOBUFS || errno == ENOMEM ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Pas assez de memoire pour allouer les buffers necessaires." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): socket(): Protocole demande inconnu." << std::endl;
		}
		
		return false;
	}

	// On parametre le descripteur de socket pour que l'application rende l'adresse et le port immediatement apres sa fermeture
	//  sans duree de retention par le noyau, et, que plusieurs sockets puissent s'attacher au meme port (SO_REUSEADDR=1)
	//
	int ParametreSocket=1;
	
	if( setsockopt(IdSocket,SOL_SOCKET,SO_REUSEADDR,&ParametreSocket,sizeof(int)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Impossible de parametrer le descripteur de la socket SO_REUSEADDR pour le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Le descripteur est un fichier et pas une socket." << std::endl;
			if( errno == ENOPROTOOPT ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Parametre option inconnue pour ce protocole." << std::endl;
			if( errno == EFAULT ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: CreationReseau(): setsockopt(): Mauvais pointeur passe en parametre." << std::endl;
		}
		
		return false;
	}
	
	SocketCree=true;
	
	return true;
}


// Fonction d'attachement du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommServeurChiffreMonoClient::AttachementReseau(void)
{
	// Si la socket n'est pas cree alors on le fait
	//
	if( !SocketCree ) if( !CreationReseau() ) return false;
	
	
	// On attache/nomme le point de communication a une adresse:port du domaine parametre (IP:PORT)
	//
	memset(&AdresseSocket,0,sizeof(struct sockaddr_in));

	AdresseSocket.sin_family=AF_INET;			// Famille de socket IP  (man ip)
	AdresseSocket.sin_addr.s_addr=htonl(AdresseIP);		// Adresse IP du point de communication
	AdresseSocket.sin_port=htons(Port);			// Le numero de port d'ecoute
	
	if( bind(IdSocket,(struct sockaddr *) &AdresseSocket,sizeof(struct sockaddr_in)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurChiffreMonoClient: ERREUR: AttachementReseau(): bind(): Impossible d'attacher la socket a l'adresse:port pour le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttachementReseau(): bind(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttachementReseau(): bind(): La socket possede deja une adresse." << std::endl;
			if( errno == EACCES ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttachementReseau(): bind(): L'adresse n'est utilisable qu'en super utilisateur." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttachementReseau(): bind(): Le descripteur est un fichier et pas une socket." << std::endl;
		}
		
		return false;
	}
	
	SocketAttachee=true;
	
	return true;
}


// Fonction d'ecoute du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommServeurChiffreMonoClient::EcouteReseau(void)
{
	// Si la socket n'est pas attachee alors on le fait
	//
	if( !SocketAttachee ) if( !AttachementReseau() ) return false;
	
	
	// On place l'application serveur en etat d'ecoute des connexions entrantes sur l'adresse:port definie. On fixe la limite de la liste des requetes de connexions entrantes en attente de traitement par l'application a ListeClientsMax.
	//
	if( listen(IdSocket,NbListeClientsMax) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Impossible d'initier l'ecoute des requetes de connexions entrantes sur le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EADDRINUSE ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Une autre socket ecoute sur la meme adresse:port." << std::endl;
			if( errno == EBADF ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Le descripteur n'est pas une socket." << std::endl;
			if( errno == EOPNOTSUPP ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: EcouteReseau(): listen(): Ce type de socket ne supporte pas listen()." << std::endl;
		}
		
		return false;
	}
	
	SocketEcoute=true;
	
	return true;
}


// Attente et acceptation d'une session de connexion
//
// CE:	-
//
// CS:	La fonction est vraie en cas de connexion, IdSocketSession != -1 si le client est autorise / Fausse en cas de probleme ;
//
int PointCommServeurChiffreMonoClient::AttenteAccepterSessionAutorisee(void)
{
	// Si le serveur n'est pas en ecoute sur IP:Port, on le lance
	//
	if( !SocketEcoute ) if( !EcouteReseau() ) return false;
	
	// Aucun client n'est connecte
	//
	ClientConnecte=false;
	
	// Initialisation des variables pour traiter le client potentiel
	//
	memset(&AdresseSocketClient,0,sizeof(struct sockaddr_in));	// Tous les octets de la structure a zero pour initialisation
	socklen_t TailleStructAdresseSocketClient;			// Nombre d'octets de la structure de donnees de l'adresse du client
	TailleStructAdresseSocketClient=sizeof(struct sockaddr_in);	// obligatoire : Le nombre d'octets reserves

	// On extrait et traite la premiere entree de la liste des requetes de connexions entrantes en attente
	//
	if( (IdSocketSession=accept(IdSocket,(struct sockaddr *) &AdresseSocketClient,&TailleStructAdresseSocketClient)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Impossible de recuperer la premiere entree de la liste des requetes de connexions entrantes sur le serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EAGAIN || errno == EWOULDBLOCK ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La socket est en mode non bloquante mais il n'y a aucune requete de connexion dans la liste." << std::endl;
			if( errno == EBADF ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Le descripteur n'est pas une socket." << std::endl;
			if( errno == EOPNOTSUPP ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La socket de reference n'est pas une SOCK_STREAM." << std::endl;
			if( errno == EINTR ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Appel systeme interrompu par l'arrivee d'un signal avant qu'une connexion valide ne survienne." << std::endl;
			if( errno == ECONNABORTED ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Une connexion a ete abandonnee." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La socket n'est pas en attente de connexion." << std::endl;
			if( errno == EMFILE ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): La limite de la table des descripteur est atteinte." << std::endl;
			if( errno == EFAULT ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): L'adresse n'est pas dans l'espace d'adressage accessible en ecriture." << std::endl;
			if( errno == ENOBUFS || errno == ENOMEM ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Pas assez de memoire disponible." << std::endl;
			if( errno == EPROTO ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Erreur de protocole." << std::endl;
			if( errno == EPERM ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): accept(): Connexion interdite par le firewall." << std::endl;
		}
		
		return false;
	}
	else
	{
		// Une requete de connexion valide d'un client est a traiter
		//
		if( TailleStructAdresseSocketClient == sizeof(struct sockaddr_in) )
		{
			if( ModeVerbeux )
			{
				char Chaine[TAILLE_MAXI_CHAINE_COUT_NET];
				
				sprintf(Chaine,"PointCommServeurChiffreMonoClient: Requete de connexion entrante du client %s(0x%X):%d",inet_ntoa(AdresseSocketClient.sin_addr),ntohl(AdresseSocketClient.sin_addr.s_addr),AdresseSocketClient.sin_port);
				
				std::cerr << Chaine << std::endl;
			}
		}
		else
		{
			if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: AttenteAccepterSessionAutorisee(): Code a reecrire pour traiter et afficher l'adresse du client apres accept()." << std::endl;
			
			return false;
		}

		// Comme le serveur est mono-client on filtre l'adresse du client deja a ce niveau sans initier le SSL
		//
		if( ntohl(AdresseSocketClient.sin_addr.s_addr) == AdresseIPClientAutorise )
		{
			// Le client est autorise
			//
			return true;
		}
		else
		{
			// Ce client n'est pas autorise, on ferme la requete de connexion et on en attend une autre
			//
			if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: Ce client n'est pas autorise, connexion refusee." << std::endl;
			
			close(IdSocketSession);
			IdSocketSession=-1;
			
			ClientConnecte=false;
			
			return true;
		}
	}
	
	ClientConnecte=false;
	
	return false;
}


// Fonction pour savoir si une session a ete acceptee et autorisee
//
// CE:	-
//
// CS:	La fonction est vraie si la session est autorisee
//
int PointCommServeurChiffreMonoClient::SessionAccepteeAutorisee(void)
{
	if( (IdSocketSession != -1) && (ntohl(AdresseSocketClient.sin_addr.s_addr) == AdresseIPClientAutorise) ) return true;
	
	return false; 
}


// Fonction de negociation d'une connexion SSL avec un client autorise
//
// CE:	Le client courant IdSocketSession doit etre autorise ;
//
// CS:	La fonction est fausse si la negociation n'aboutie pas a une connexion SSL ;
//
int PointCommServeurChiffreMonoClient::NegociationConnexionSSL(void)
{
	int retour=true;		// Valeur retournee par la fonction
	int RetourES_SSL;		// Valeur retournee par les fonctions E/S SSL
	int TentativesSSL_accept=0;	// Nombre de tentatives de lecture d'initiation TLS/SSL
	
	// Parametrage de la socket en mode non bloquant pour gerer un timeout sur SSL_accept()
	//
	SocketSessionNonBloquante();
	
	// On cree une socket SSL dans un objet BIO a partir de la socket (normale) de session courante
	//
	// BIO_NOCLOSE pour que BIO_free ne ferme pas et ne detruise pas automatiquement la socket
	//
	if( (SocketSessionSSL_BIO=BIO_new_socket(IdSocketSession,BIO_NOCLOSE)) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_new_socket(): Impossible de creer une socket SSL dans un objet BIO a partir de la socket (normale) de session courante." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		close(IdSocketSession);
		IdSocketSession=-1;
		return false;
	}
	
	// Creation de la structure SSL de connexion a partir du contexte du serveur
	//
	if( (StructSSLServeur=SSL_new(ContexteSSL)) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): SSL_new(): Impossible de creer la structure SSL de connexion a partir du contexte du serveur." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		close(IdSocketSession);
		IdSocketSession=-1;
		if( SocketSessionSSL_BIO != NULL )
		{
			if( !BIO_free(SocketSessionSSL_BIO) )
			{
				if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_free(): Impossible de liberer SocketSSL_BIO." << std::endl;
			}
			SocketSessionSSL_BIO=NULL;
		}
		return false;
	}
	
	int NegociationInitiee=false;		// Etat de la negociation TLS/SSL
	
	// On prepare la structure de connexion SSL a etre en mode serveur
	//
	SSL_set_accept_state(StructSSLServeur);
	
	// On connecte l'objet BIO a la structure de connexion SSL pour l'operation de negociation (OBLIGATOIRE pour SSL_accept())
	//
	// !!! StructSSLServeur ne devra plus etre libere par SSL_free() mais par BIO_free(SocketSessionSSL_BIO) !!!
	//
	SSL_set_bio(StructSSLServeur,SocketSessionSSL_BIO,SocketSessionSSL_BIO);
	
	// On attend que le client initie la negociation TLS/SSL
	//
	do
	{
		struct timeval timeout_sock_put;	// Timeout socket pour l'emission
		struct timeval timeout_sock_get;	// Timeout socket pour la reception
		
		ClientConnecte=false;			// Pour l'instant on considere que aucun client n'est connecte en SSL !
		
		switch( (RetourES_SSL=SSL_accept(StructSSLServeur)) )
		{
			case 0:
				// La negociation TLS/SSL n'a pas aboutie mais l'arret est controle par le protocole
				//
				if( ModeVerbeux )
				{
					std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): SSL_accept(): Erreur lors de la negociation TLS/SSL." << std::endl;
				
					AfficheErreurES_SSL(StructSSLServeur,RetourES_SSL);
				}
				
				//SSL_free(StructSSLServeur);	Il est libere par BIO_free(SocketSessionSSL_BIO)
				//StructSSLServeur=NULL;
				
				if( !BIO_free(SocketSessionSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_free(): Impossible de liberer SocketSessionSSL_BIO." << std::endl;
				SocketSessionSSL_BIO=NULL;
				
				close(IdSocketSession);
				IdSocketSession=-1;
				
				retour=false;
				break;
			
			case 1:
				// La negociation est inititiee
				//
				NegociationInitiee=true;
				
				// La socket de session repasse dans le mode bloquant pour la suite des echanges
				//
				SocketSessionBloquante();
				
				// On fixe un timeout pour l'emission
				//
				timeout_sock_put.tv_sec=TimeoutSocketPut;
				timeout_sock_put.tv_usec=0;
				
				if( setsockopt(IdSocketSession,SOL_SOCKET,SO_SNDTIMEO,(void *) &timeout_sock_put,sizeof(struct timeval)) == -1 )
				{
					if( ModeVerbeux )
					{
						char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_SSL];
					
						sprintf(ChaineErreur,"PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): setsockopt(): Impossible de parametrer SO_SNDTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
						std::cerr << ChaineErreur << std::endl;
					
						if( errno == EBADF ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
						if( errno == ENOPROTOOPT ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
					}
				}
				
				// On fixe un timeout pour la reception
				//
				timeout_sock_get.tv_sec=TimeoutSocketGet;
				timeout_sock_get.tv_usec=0;
				
				if( setsockopt(IdSocketSession,SOL_SOCKET,SO_RCVTIMEO,(void *) &timeout_sock_get,sizeof(struct timeval)) == -1 )
				{
					if( ModeVerbeux )
					{
						char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_SSL];
					
						sprintf(ChaineErreur,"PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): setsockopt(): Impossible de parametrer SO_RCVTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
						std::cerr << ChaineErreur << std::endl;
					
						if( errno == EBADF ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
						if( errno == ENOPROTOOPT ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
					}
				}
				
				// La negociation est reussie entre le serveur et le client selon le contexte defini
				//
				if( ModeVerbeux )
				{
					char Chaine[TAILLE_MAXI_CHAINE_COUT_SSL];
					
					sprintf(Chaine,"PointCommServeurChiffreMonoClient: Connexion SSL acceptee avec le client %s(0x%X):%d",inet_ntoa(AdresseSocketClient.sin_addr),ntohl(AdresseSocketClient.sin_addr.s_addr),AdresseSocketClient.sin_port);
					
					std::cerr << Chaine << std::endl;
				}
				
				// On cree un objet BIO de type SSL avec la methode BIO_f_ssl() qui permet de le specialiser
				//
				if( (ConnexionSSL=BIO_new(BIO_f_ssl())) == NULL )
				{
					if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_new(): Impossible de creer l'objet de connexion BIO de type SSL." << std::endl;
					
					//SSL_free(StructSSLServeur);	Il est libere par BIO_free(SocketSessionSSL_BIO)
					//StructSSLServeur=NULL;
				
					if( !BIO_free(SocketSessionSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_free(): Impossible de liberer SocketSessionSSL_BIO." << std::endl;
					SocketSessionSSL_BIO=NULL;
				
					close(IdSocketSession);
					IdSocketSession=-1;
				}
				else
				{
					// On attache la structure de connexion SSL a l'objet BIO SSL
					//
					BIO_set_ssl(ConnexionSSL,StructSSLServeur,BIO_CLOSE);
					
					// On cree un objet BIO avec buffer avec la methode BIO_f_buffer() qui permet de le specialiser,
					//  le tableau est de DEFAULT_BUFFER_SIZE octets par defaut
					//
					if( (ConnexionBuffSSL=BIO_new(BIO_f_buffer())) == NULL )
					{
						if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_new(): Impossible de creer l'objet de connexion BIO de type buffer." << std::endl;
						
						if( !BIO_free(ConnexionSSL) ) fprintf(stderr,"PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_free(): Impossible de liberer ConnexionSSL.\n");
						ConnexionSSL=NULL;
						
						//SSL_free(StructSSLServeur);	Il est libere par BIO_free(SocketSessionSSL_BIO)
						//StructSSLServeur=NULL;
				
						if( !BIO_free(SocketSessionSSL_BIO) ) fprintf(stderr,"PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_free(): Impossible de liberer SocketSSL_BIO.\n");
						SocketSessionSSL_BIO=NULL;
						
						close(IdSocketSession);
						IdSocketSession=-1;
					}
					else
					{
						// On chaine l'objet BIO SSL avec l'objet BIO buffer
						// La chaine d'objets est donc :
						// Serveur:ConnexionBuff->ConnexionSSL->StructSSLServeur->IdSocketSession -> Client
						//
						BIO_push(ConnexionBuffSSL,ConnexionSSL);
						
						if( ParamSIGPIPE )
						{
							// On parametre le nouveau gestionnaire du signal SIGPIPE
							// SIGPIPE est declanche lorsqu'un processus tente d'ecrire dans tube ferme
							//
							NouvGestSignalSIGPIPE.sa_handler=FnHandlerSIGPIPE;
							
							if( sigaction(SIGPIPE,&NouvGestSignalSIGPIPE,&AncienGestSignalSIGPIPE) < 0 )
							{
								if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): sigaction(): Impossible de parametrer un gestionnaire du signal SIGPIPE." << std::endl;
							}
							// kill(getpid(),SIGPIPE); pour tester
						}
						
						// Le chiffreur utilise pour cette connexion
						//						
						Chiffreur=SSL_get_current_cipher(StructSSLServeur);
						
						// C'est bon, une connexion SSL est etablie
						//
						ClientConnecte=true;
						
						retour=true;
						
						break;
					}
				}
				break;
				
			case -1:
				// SSL_accept() est en mode non bloquant et son appel retourne SSL_ERROR_WANT_READ
				//  car la negociation SSL/TLS n'a pas demarree
				//
				TentativesSSL_accept++;
				
				if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: La negociation TLS/SSL n'est pas encore initiee par le client n=" << TentativesSSL_accept << "." << std::endl;
				
				// On va patienter avant de relancer un appel SSL_accept()
				//
				sleep(1);
				
				break;
		}
		
	} while( TentativesSSL_accept < TimeoutSSLAccept && !NegociationInitiee );
	
	// Si aucune negociation TLS/SSL n'a ete inititiee et que le timeout est depasse
	//
	if( TentativesSSL_accept == TimeoutSSLAccept && !NegociationInitiee )
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): Aucune negociation TLS/SSL n'a ete initiee par le client." << std::endl;
		
		if( !BIO_free(SocketSessionSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: NegociationConnexionSSL(): BIO_free(): Impossible de liberer SocketSSL_BIO." << std::endl;
		SocketSessionSSL_BIO=NULL;
		
		close(IdSocketSession);
		IdSocketSession=-1;
		
		retour=false;
	}
	
	return retour;
}


// Pour savoir si un client est actuellement connecte ?
//
// CE:	-
//
// CS:	Vrai si un client est connecte ;
//
int PointCommServeurChiffreMonoClient::UnClientEstConnecte(void)
{
	return ClientConnecte;
}


// Fermeture de la connexion SSL courante
//
// CE:	-
//
// CS:	-
//
void PointCommServeurChiffreMonoClient::FermetureConnexionSSL(void)
{
	int RetourES_SSL;		// Valeur retournee par les fonctions E/S SSL
	int NombreShutdown=0;		// Nombre de tentatives possibles de fermeture de la connexion SSL
	
	// On passe la connexion SSL en mode de fermeture en toute quietude
	//  la notification de fermeture n'est pas envoye au paire pour prevenir le crash de SSL_shutdown()
	//  en cas de perte de connexion ou disparition du paire
	//
	SSL_set_quiet_shutdown(StructSSLServeur,1);
	
	// On ferme la connexion SSL
	//
	while( (RetourES_SSL=SSL_shutdown(StructSSLServeur)) != 1 && NombreShutdown < 4 )
	{
		NombreShutdown++;
		
		if( RetourES_SSL == 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): SSL_shutdown(): Tentative " << NombreShutdown << ": La fermeture de la socket SSL n'est pas terminee." << std::endl;
			
			// On signale au client que la socket ne sera plus utilisee pour l'ecriture ce qui evite de boucler plusieurs fois
			//
			if( shutdown(IdSocketSession,SHUT_WR) < 0 )
			{
				if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): shutdown(): Impossible de signaler au client que la socket ne sera plus utilisee pour l'ecriture." << std::endl;
			}
		}
		else
		{
			if( ModeVerbeux )
			{
				std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): SSL_shutdown(): Impossible de fermer la connexion SSL, erreur fatale au niveau du protocole ou erreur fatale de connexion." << std::endl;
			
				AfficheErreurES_SSL(StructSSLServeur,RetourES_SSL);
			}
		}
	}
	
	if( RetourES_SSL == 1 )
	{
		if( ModeVerbeux )
		{
			char Chaine[TAILLE_MAXI_CHAINE_COUT_SSL];
			
			sprintf(Chaine,"PointCommServeurChiffreMonoClient: Connexion SSL fermee avec le client %s(0x%X):%d",inet_ntoa(AdresseSocketClient.sin_addr),ntohl(AdresseSocketClient.sin_addr.s_addr),AdresseSocketClient.sin_port);
			
			std::cerr << Chaine << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): SSL_shutdown(): Impossible de fermer la connexion SSL apres plusieurs tentatives." << std::endl;
	}
	
	if( ParamSIGPIPE )
	{
		// On parametre le gestionnaire de SIGPIPE initial
		//
		if( sigaction(SIGPIPE,&AncienGestSignalSIGPIPE,NULL) < 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): sigaction(): Impossible de parametrer un gestionnaire du signal SIGPIPE." << std::endl;
		}
	}
	
	// Liberation de toute la chaine BIO
	//
	if( !BIO_free(ConnexionBuffSSL) ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): BIO_free(): Impossible de liberer ConnexionBuffSSL." << std::endl;
	ConnexionBuffSSL=NULL;
	
	if( !BIO_free(ConnexionSSL) ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): BIO_free(): Impossible de liberer ConnexionSSL." << std::endl;
	ConnexionSSL=NULL;
	
	//SSL_free(StructSSLServeur);	Il est libere par BIO_free(SocketSessionSSL_BIO) !!!
	
	if( !BIO_free(SocketSessionSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: FermetureConnexionSSL(): BIO_free(): Impossible de liberer SocketSSL_BIO." << std::endl;
	SocketSessionSSL_BIO=NULL;
	
	close(IdSocketSession);
	IdSocketSession=-1;
	
	Chiffreur=NULL;
	
	// Aucun client n'est connecte
	//
	ClientConnecte=false;
}


// Fonction d'emission d'une chaine par un objet BIO de type buffer et traitement du code de retour
//
// CE:	On passe un pointeur vers la chaine a envoyer ;
//
// CS:	La fonction retourne la valeur retournee par BIO_puts()
//
int PointCommServeurChiffreMonoClient::EnvoyerChaineBIO(const char *chaine)
{
	int errno_avant=errno;		// Sauvegarde de errno
	int RetourPuts;				// Valeur retournee par BIO_puts
	
	// Emission de la chaine sur l'objet BIO bufferise
	//
	RetourPuts=BIO_puts(ConnexionBuffSSL,chaine);
	
	if( RetourPuts <= 0 || errno != errno_avant )
	{
		if( RetourPuts == 0 ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: EnvoyerChaineBIO(): BIO_puts(): Connexion abandonnee par le client." << std::endl;
		
		if( RetourPuts < 0 ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR TIMEOUT: EnvoyerChaineBIO(): BIO_puts(): Duree de non activite depassee, la connexion est consideree comme perdue." << std::endl;
		
		if( errno != errno_avant )
		{
			RetourPuts=-1;		// Pour detecter l'erreur simplement avec le code retour
			
			if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: EnvoyerChaineBIO(): BIO_puts(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: S->C " << RetourPuts <<": " << chaine << std::endl;
	}
	
	// ATTENTION : Bien mettre le BIO_flush() apres le code de traitement des erreurs de BIO_puts() car il peut modifier errno
	//
	BIO_flush(ConnexionBuffSSL);
	
	return RetourPuts;
}


// Reception d'une chaine par le client
//
// CE:	-
//
// CS:	la fonction retourne la valeur de BIO_gets() ;
//
int PointCommServeurChiffreMonoClient::RecevoirChaineBIO(char *chaine)
{
	int RetourGets=BIO_gets(ConnexionBuffSSL,chaine,TAILLE_MAXI_CHAINE_BIO);
	
	if( RetourGets == 0 ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: RecevoirChaineBIO(): BIO_gets(): Connexion abandonnee par le client." << std::endl;
	
	if( RetourGets == -1 ) if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR TIMEOUT: RecevoirChaineBIO(): BIO_gets(): Duree de non activite depassee, la connexion est consideree comme perdue." << std::endl;
	
	if( ModeVerbeux && RetourGets > 0 ) std::cerr << "PointCommServeurChiffreMonoClient: S<-C " << RetourGets <<": " << chaine << std::endl;
	
	return RetourGets;
}


// Fonction retournant un pointeur vers le chiffreur de la connexion SSL courante
//
// CE:	-
//
// CS:	La fonction retourne le pointeur ;
//
const SSL_CIPHER *PointCommServeurChiffreMonoClient::DescripteurChiffreurConnexionSSL(void)
{
	return Chiffreur;
}


// Passage de la socket de session courante en mode bloquant
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
//
int PointCommServeurChiffreMonoClient::SocketSessionBloquante(void)
{
	if( IdSocketSession != -1 )
	{
		// Parametrage de la socket en mode bloquant pour attente sans consommation de CPU
		//
		int AttributsSocketSession=fcntl(IdSocketSession,F_GETFL);
		
		if( fcntl(IdSocketSession,F_SETFL,AttributsSocketSession & (~O_NONBLOCK)) == -1 ) return false; else return true;
	}
	
	return false;
}


// Passage de la socket de session courante en mode NON bloquant
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
//
int PointCommServeurChiffreMonoClient::SocketSessionNonBloquante(void)
{
	if( IdSocketSession != -1 )
	{
		// Parametrage de la socket en mode non bloquant pour gerer un timeout
		//
		int AttributsSocketSession=fcntl(IdSocketSession,F_GETFL);
		
		if( fcntl(IdSocketSession,F_SETFL,AttributsSocketSession | O_NONBLOCK) == -1 ) return false; else return true;
	}
	
	return false;
}


// Pour savoir si des donnees sont disponibles en lecture sur la socket de session ?
//
// CE:	On passe le timeout en ms de cette interrogration. La fonction est bloquante en attente si timeout > 0.
//	 non bloquante et immediate si timeout=0 ou infinie si timeout < 0 (INFTIM) ;
//
// CS:	La fonction est vraie si une donnee est disponible en lecture sur la socket
//	 == 0 (fausse) si une donnee n'est pas disponible apres timeout millisecondes
//	 == -1 si une erreur s'est produite lors de cette interrogation ;
// 
int PointCommServeurChiffreMonoClient::PossibleLireDonneesSocketSession(int timeout)
{
	struct pollfd Evenement;	// Structure de donnees pour interroger un descripteur de socket
	
	Evenement.fd=IdSocketSession;
	Evenement.events=POLLIN;	// Lecture possible sur le descripteur ?
	Evenement.revents=0;		// Reponse
	
	int retour=poll(&Evenement,1,timeout);

	switch( retour )
	{
		case 1:
			// Il y a une reponse a l'interrogation
			//
			if( Evenement.revents & POLLIN ) return true; else return false;
			
		case 0:		// timeout
		case -1:	// erreur
			return retour;
			
		default:
			// Impossible logiquement donc erreur
			//
			if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: PossibleLireDonneesSocketSession(): poll(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			return -1;
	}
}


// Pour savoir si on peut ecrire des donnees sur la socket de session ?
//
// CE:	On passe le timeout en ms de cette interrogration. La fonction est bloquante en attente si timeout > 0.
//	 non bloquante et immediate si timeout=0 ou infinie si timeout < 0 (INFTIM) ;
//
// CS:	La fonction est vraie si on peut ecrire sur la socket
//	 == 0 (fausse) si on ne peut pas ecrire apres timeout millisecondes
//	 == -1 si une erreur s'est produite lors de cette interrogation ;
// 
int PointCommServeurChiffreMonoClient::PossibleEcrireDonneesSocketSession(int timeout)
{
	struct pollfd Evenement;	// Structure de donnees pour interroger un descripteur de socket
	
	Evenement.fd=IdSocketSession;
	Evenement.events=POLLOUT;	// Ecriture possible sur le descripteur ?
	Evenement.revents=0;		// Reponse
	
	int retour=poll(&Evenement,1,timeout);

	switch( retour )
	{
		case 1:
			// Il y a une reponse a l'interrogation
			//
			if( Evenement.revents & POLLOUT ) return true; else return false;
			
		case 0:		// timeout
		case -1:	// erreur
			return retour;
			
		default:
			// Impossible logiquement donc erreur
			//
			if( ModeVerbeux ) std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: PossibleEcrireDonneesSocketSession(): poll(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			return -1;
	}
}


// FONCTION D'AFFICHAGE DES ERREURS PRODUITES SUR LA PILE DE LA LIBRAIRIE SSL
//
//	CE:	-
//
//	CS:	-
//
void PointCommServeurChiffreMonoClient::AfficheErreurPileLibrairieSSL(void)
{
	unsigned long numero;				// Numero de l'erreur de la librairie SSL
	char Chaine[TAILLE_MAXI_CHAINE_ERREUR_SSL];	// Chaine de composition des erreurs
	
	while( (numero=ERR_get_error()) != 0 )
	{
		ERR_error_string_n(numero,Chaine,TAILLE_MAXI_CHAINE_ERREUR_SSL);

		std::cerr << "PointCommServeurChiffreMonoClient: ERREUR: SSL : " << Chaine << std::endl;
	}
}


// FONCTION D'AFFICHAGE DES ERREURS PRODUITES PAR LES OPERATIONS E/S TLS/SSL
//
// CE:	On passe un pointeur sur la structure de la connexion SSL ;
//
//	On passe la valeur retournee par la fonction d'e/s TLS/SSL ;
//
// CS:	-
//
void PointCommServeurChiffreMonoClient::AfficheErreurES_SSL(const SSL *structure,int retour)
{
	switch( SSL_get_error(structure,retour) )
	{
		case SSL_ERROR_NONE:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_NONE: Aucune erreur d'e/s." << std::endl;
			break;
		case SSL_ERROR_ZERO_RETURN:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_ZERO_RETURN: La connexion TLS/SSL a ete fermee par une alerte du protocole." << std::endl;
			break;
		case SSL_ERROR_WANT_READ:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_WANT_READ: L'operation de lecture n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_WRITE:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_WANT_WRITE: L'operation d'ecriture n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_CONNECT:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_WANT_CONNECT: L'operation de connexion n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_ACCEPT:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_WANT_ACCEPT: L'operation de negociation SSL n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_WANT_X509_LOOKUP: L'operation n'a pu se realiser car la fonction de retour SSL_CTX_set_client_cert_cb() doit etre appelee une nouvelle fois." << std::endl;
			break;
		case SSL_ERROR_SYSCALL:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_SYSCALL: Erreurs d'appel systeme d'entrees/sorties ." << std::endl;
			break;
		case SSL_ERROR_SSL:
			std::cerr << "PointCommServeurChiffreMonoClient: ERREUR SSL_ERROR_SSL: Erreur dans la librairie SSL, sans doute une erreur de protocole." << std::endl;
			break;
	}
}


//--------------------------------------------------------------------------------------------------------------------------------------


// Constructeur d'un point de communication client non chiffre
//
// CE:	On passe true pour etre en mode verbeux, false pour aucun message ;
//
//	On passe l'adresse IP du serveur en valeur host (0x________) ;
//
//	On passe le port du service en valeur host (0x____) ;
//
//	On passe le timeout en secondes pour la tentative d'ecriture de donnees dans la socket ;
//
//	On passe le timeout en secondes pour la tentative de lecture de donnees dans la socket ;
//
PointCommClientNonChiffre::PointCommClientNonChiffre(int pverbeux,uint32_t pAdresse,uint16_t pPort,int pTimeoutSocketPut,int pTimeoutSocketGet) : PointComm(pverbeux,pTimeoutSocketPut,pTimeoutSocketGet)
{
	// Initialisation des variables
	//
	ObjetInitialise=false;
	ConnecteServeur=false;
	
	// L'adresse IP du serveur
	//
	AdresseIP=pAdresse;
	
	// Le port IP du service
	//
	Port=pPort;
}


// Destructeur d'un point de communication client non chiffre
//
PointCommClientNonChiffre::~PointCommClientNonChiffre()
{
	if( IdSocket != -1 ) close(IdSocket);
}


// Fonction d'initialisation du point de communication client non chiffre
//
int PointCommClientNonChiffre::Initialisation(void)
{
	ObjetInitialise=true;
	
	return true;
}


// Fonction de creation du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommClientNonChiffre::CreationReseau(void)
{
	// Si l'objet n'a pas ete initialise alors on le fait
	//
	if( !ObjetInitialise ) if( !Initialisation() ) return false;
	
	
	// Creation d'un point de communication pour le client, une socket du domaine IPv4 (PF_INET Linux),
	//  de type flux de donnees binaire echange continu avec integrite et fiabilite maximale (SOCK_STREAM)
	//  et de protocole specifique IP (/etc/protocols)
	//
	if( (IdSocket=socket(PF_INET,SOCK_STREAM,0)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Impossible d'obtenir une socket pour le client, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EPROTONOSUPPORT ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Type de protocole ou protocole non disponible dans ce domaine de communication." << std::endl;
			if( errno == EAFNOSUPPORT ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Famille d'adresses non supportee." << std::endl;
			if( errno == ENFILE ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Table de descripteur par processus pleine." << std::endl;
			if( errno == EMFILE ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Table des fichiers pleine." << std::endl;
			if( errno == EACCES ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Creation d'une telle socket non autorise." << std::endl;
			if( errno == ENOBUFS || errno == ENOMEM ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Pas assez de memoire pour allouer les buffers necessaires." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): socket(): Protocole demande inconnu." << std::endl;
		}
		
		return false;
	}
	
	struct timeval timeout_sock_put;	// Timeout socket pour l'emission
	struct timeval timeout_sock_get;	// Timeout socket pour la reception
	
	// On fixe un timeout pour l'emission
	//
	timeout_sock_put.tv_sec=TimeoutSocketPut;
	timeout_sock_put.tv_usec=0;
	
	if( setsockopt(IdSocket,SOL_SOCKET,SO_SNDTIMEO,(void *) &timeout_sock_put,sizeof(struct timeval)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommClientNonChiffre: ERREUR: CreationReseau(): setsockopt(): Impossible de parametrer SO_SNDTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOPROTOOPT ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
		}
	}
	
	// On fixe un timeout pour la reception
	//
	timeout_sock_get.tv_sec=TimeoutSocketGet;
	timeout_sock_get.tv_usec=0;
	
	if( setsockopt(IdSocket,SOL_SOCKET,SO_RCVTIMEO,(void *) &timeout_sock_get,sizeof(struct timeval)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommClientNonChiffre: ERREUR: CreationReseau(): setsockopt(): Impossible de parametrer SO_RCVTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOPROTOOPT ) std::cerr << "PointCommClientNonChiffre: ERREUR: CreationReseau(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
		}
	}
	
	SocketCree=true;
	
	return true;
}


// Fonction de connexion au serveur
//
// CE:	-
//
// CS:	La fonction est vraie si le client s'est connecte au serveur
//
int PointCommClientNonChiffre::Connecter(void)
{
	// Si la socket n'est pas cree alors on le fait
	//
	if( !SocketCree ) if( !CreationReseau() ) return false;
	
	// Pour l'instant on n'est pas connecte au serveur
	//
	ConnecteServeur=false;
	
	// Preparation de la connexion
	//
	memset(&AdresseSocket,0,sizeof(struct sockaddr_in));
	
	AdresseSocket.sin_family=AF_INET;			// Famille de socket IP  (man ip)
	AdresseSocket.sin_addr.s_addr=htonl(AdresseIP);		// Adresse IP du point de communication
	AdresseSocket.sin_port=htons(Port);			// Le numero de port
	
	// Connexion au serveur
	//
	if( connect(IdSocket,(struct sockaddr *) &AdresseSocket,sizeof(struct sockaddr_in)) != 0 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommClientNonChiffre: ERREUR: Connecter(): connect(): Echec de la connexion au serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): Mauvais descripteur." << std::endl;
			if( errno == EFAULT ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): La structure d'adresse pointe en dehors de l'espace d'adressage." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): Le descripteur ne correspond pas a une socket." << std::endl;
			if( errno == EISCONN ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): La socket est deja connectee." << std::endl;
			if( errno == ECONNREFUSED ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): La connexion est refusee par le serveur." << std::endl;
			if( errno == ETIMEDOUT ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): Depassement du delai maximum pendant la connexion." << std::endl;
			if( errno == ENETUNREACH ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): Le reseau est inaccessible." << std::endl;
			if( errno == EADDRINUSE ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): L'adresse est deja utilisee." << std::endl;
			if( errno == EINPROGRESS ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): La socket est non bloquante et la connexion ne peut pas etre etablie immediatement." << std::endl;
			if( errno == EALREADY ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): La socket est non bloquante et une tentative de connexion precedente ne s'est pas encore terminee." << std::endl;
			if( errno == EAGAIN ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): Pas de port local disponible, ou pas assez de place dans les tables de routage." << std::endl;
			if( errno == EAFNOSUPPORT ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): L'adresse transmise n'appartient pas a la famille indiquee dans son champ." << std::endl;
			if( errno == EACCES || errno == EPERM ) std::cerr << "PointCommClientNonChiffre: ERREUR: Connecter(): connect(): L'utilisateur a tente de connecter une adresse broadcast sans avoir active l'attribut broadcast, ou la demande de connexion a echouee a cause des regles d'un firewall local." << std::endl;
		}
		
		return false;
	}
	
	// Information de connexion
	//
	if( ModeVerbeux )
	{
		char Chaine[TAILLE_MAXI_CHAINE_COUT_NET];
		
		sprintf(Chaine,"PointCommClientNonChiffre: Connexion acceptee par le serveur %s(0x%X):%d",inet_ntoa(AdresseSocket.sin_addr),ntohl(AdresseSocket.sin_addr.s_addr),AdresseSocket.sin_port);
		
		std::cerr << Chaine << std::endl;
	}
	
	// On est connecte a un serveur
	//
	ConnecteServeur=true;
	
	return true;
}


// Fonction de fermeture de la connexion avec le serveur
//
// CE:	-
//
// CS:	-
//
void PointCommClientNonChiffre::Fermer(void)
{
	// On signale au serveur que la socket ne sera plus utilisee
	//
	if( shutdown(IdSocket,SHUT_RDWR) < 0 )
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientNonChiffre: ERREUR: Fermer(): shutdown(): Impossible de signaler au serveur que la socket ne sera plus utilisee." << std::endl;
	}
	
	// Fermeture de la socket
	//
	int retour=close(IdSocket);
	
	// Si la fermeture s'est bien passee
	//
	if( !retour )
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientNonChiffre: Connexion fermee avec le serveur." << std::endl;
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientNonChiffre: ERREUR: Fermer(): close(): Impossible de fermer la connexion." << std::endl;
	}
	
	// Le client n'est plus connecte au serveur
	//
	ConnecteServeur=false;
	
	// Descripteur de la socket initialise
	//
	IdSocket=-1;
	SocketCree=false;
}


// Pour savoir si on est connecte au serveur
//
// CE: -
//
// CS:	La fonction est vraie si on est connecte au serveur
//
int PointCommClientNonChiffre::Connecte(void)
{
	return ConnecteServeur;
}


// Fonction d'emission d'un caractere hors bande
//
// ATTENTION: Pour que le caractere hors bande soit correctement lu vers le destinataire, il ne doit pas etre le seul dans le tampon
//	       de reception du destinataire. C'est la raison pour laquelle, le caractere hors bande est en fait le dernier d'une
//	       petite chaine de caractere envoyee comme "caractere hors bande", sinon, s'il est seul dans le tampon de reception
//	       du destinataire, alors celui-ci ne le detecte pas comme lisible, ce qui peut conduire au bloquage de l'algorithme.
// CE:	-
//
// CS:	La fonction est vraie si l'emission a reussie
//
int PointCommClientNonChiffre::EcrireCaractereHorsBande(void)
{
	// Si le descripteur de la socket de session est coherent
	//
	if( IdSocket != -1 )
	{
		if( send(IdSocket,CARACTERE_HORS_BANDE,strlen(CARACTERE_HORS_BANDE),MSG_OOB) <= 0 ) return false; else return true;
	}
	
	return false;
}


// Fonction de lecture jusqu'au caractere hors bande
//
// CE:	ATTENTION : La socket doit etre en mode d'inclusion du caractere hors bande = SO_OOBINLINE ;
//
// CS:	La fonction est vraie si l'operation de lecture a reussie ;
//
int PointCommClientNonChiffre::LireJusquAuCaractereHorsBande(void)
{
	// Si le descripteur de la socket est coherent
	//
	if( IdSocket != -1 )
	{
		int CaractereHorsBande=0;		// Indicateur de caractere hors bande vrai ou faux
		char buffer[TAILLE_BUFFER_SOCKET];	// Buffer de stockage en reception perdue
		
		// CaractereHorsBande est vraie si le prochain caractere a lire est un caractere hors bande
		//
		if( ioctl(IdSocket,SIOCATMARK,&CaractereHorsBande) < 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommClientNonChiffre: ERREUR: LireJusquAuCaractereHorsBande(): ioctl(): Test prochain caractere hors bande ? : errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
		
		// Tant que le prochain caractere a lire n'est pas le caractere hors bande
		//
		while( !CaractereHorsBande )
		{
			// On lit les donnees en pure perte jusqu'au caractere hors bande
			//
			if( recv(IdSocket,buffer,TAILLE_BUFFER_SOCKET,0) <= 0 )
			{
				// Erreur de lecture ou timeout
				//
				if( ModeVerbeux ) std::cerr << "PointCommClientNonChiffre: ERREUR: LireJusquAuCaractereHorsBande(): recv() lecture jusqu'au caractere hors bande: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
				
				return false;
			}
			
			if( ioctl(IdSocket,SIOCATMARK,&CaractereHorsBande) < 0 )
			{
				if( ModeVerbeux ) std::cerr << "PointCommClientNonChiffre: ERREUR: LireJusquAuCaractereHorsBande(): ioctl(): Test prochain caractere hors bande ? : errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			}
		}
		
		// Le prochain caractere a lire est le caractere hors bande, on le lit (buffer[0] contient le caractere)
		//
		if( recv(IdSocket,buffer,1,0) <= 0 )
		{
			if( ModeVerbeux )
			{
				std::cerr << "PointCommClientNonChiffre: ERREUR: LireJusquAuCaractereHorsBande(): recv() MSG_OOB: erreur de lecture du caractere hors bande: errno=" << errno << " : " << strerror(errno) << "." << std::endl;
			}
			
			return false;
		}
		
		return true;
	}
	
	return false;
}


//--------------------------------------------------------------------------------------------------------------------------------------


// Constructeur d'un point de communication client chiffre
//
// CE:	On passe true pour etre en mode verbeux, false pour aucun message ;
//
//	On passe l'adresse IP du serveur en valeur host (0x________) ;
//
//	On passe le port d'attachement en valeur host (0x____) ;
//
//	On passe le timeout en secondes pour la tentative d'ecriture de donnees dans la socket ;
//
//	On passe le timeout en secondes pour la tentative de lecture de donnees dans la socket ;
//
//	On passe true si on veut parametrer la redirection de SIGPIPE (ecriture dans tube ferme) ou false dans le cas contraire ;
//
//	On passe un pointeur sur la fonction C de handler du signal SIGPIPE ;
//
//	On passe un pointeur sur une chaine de char qui contient le mot de passe pour acceder a la cle privee du client SSL
//	 Ce mot de passe ne doit pas contenir plus de TAILLE_MAX_MDP_CLE_PRIVEE-1 caracteres ;
//
//	On passe un pointeur sur char vers un buffer de stockage du mot de passe pour acceder a la cle privee du client SSL
//	 Ce buffer doit etre reserve avec TAILLE_MAX_MDP_CLE_PRIVEE elements ;
//
//	On passe un pointeur sur la fonction C appelee par la librairie SSL lors de la demande du mot de passe pour acceder a la cle
//	 privee du client SSL stockee dans un fichier PEM ;
//
//	On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM du certificat
//	 de l'autorite de certification CA qui a signe les certificats du client ;
//
//	On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM du certificat du client ;
//
//	On passe un pointeur sur une chaine de char qui contient le chemin complet du fichier PEM de la cle privee du client ;
//
// CS:	-
//
PointCommClientChiffre::PointCommClientChiffre(int pverbeux,uint32_t pAdresse,uint16_t pPort,int pTimeoutSocketPut,int pTimeoutSocketGet,int pParamSIGPIPE,void (*pFnHandlerSIGPIPE)(int),const char *MdpClePriveeClient,char *BuffStockMdpClePriveeClient,int (*pFnMotDePasseClePriveeChiffree)(char*, int, int, void*),const char *pCheminCertificatCA,const char *pCheminCertificatClient,const char *pCheminClePriveeClient) : PointComm(pverbeux,pTimeoutSocketPut,pTimeoutSocketGet)
{
	// Initialisation des variables
	//
	IdSocket=-1;			// Socket de la communication serveur
	ContexteSSL=NULL;		// Le contexte SSL de l'objet sera a creer
	StructSSLClient=NULL;		// La structure de connexion SSL du client
	SocketSSL_BIO=NULL;		// Pointeur vers socket de session SSL dans un objet BIO
	ConnexionSSL=NULL;		// Pointeur vers un objet BIO de la connexion SSL
	ConnexionBuffSSL=NULL;		// Pointeur vers un objet BIO de la connexion SSL en mode buffer ligne
	SSLInitialisee=false;		// La couche SSL n'est pas initialisee pour l'instant
	Chiffreur=NULL;			// Pointeur sur la description du chiffreur
	
	// Initialisation du contexte SSL qui contient les cles, les certificats a utiliser pour des connexions
	//
	SSL_load_error_strings();	// Chargement des chaines d'erreurs de la librairie
	
	SSL_library_init();		// Initialisation de la librairie OpenSSL : Chargement des differents algorithmes de chiffrement...
	
	MethodeVersionSSL=SSLv23_method();	// Methode du protocole SSL a utiliser : il existe aussi SSLv23_server_method()
	
	// L'adresse IP d'attachement
	//
	AdresseIP=pAdresse;
	
	// Le port IP d'attachement
	//
	Port=pPort;
	
	// Recopie du mot de passe dans le buffer de stockage passe au constructeur
	//
	if( strlen(MdpClePriveeClient) < (TAILLE_MAX_MDP_CLE_PRIVEE-1) ) strcpy(BuffStockMdpClePriveeClient,MdpClePriveeClient); else *BuffStockMdpClePriveeClient=0;
	
	// Pointeur sur la fonction C appelee lors de l'acces a un fichier PEM chiffre par un mot de passe (typiquement pour l'acces
	//  a la cle privee, qui est conservee chiffree par la passphrase PEM, ou mot de passe)
	//
	FnMotDePasseClePriveeChiffree=pFnMotDePasseClePriveeChiffree;
	
	// Si on doit ou non parametrer le signal SIGPIPE
	//
	ParamSIGPIPE=pParamSIGPIPE;
	
	// Pointeur sur la fonction C de handler du signal SIGPIPE
	//
	FnHandlerSIGPIPE=pFnHandlerSIGPIPE;
	
	// On stocke le nom du fichier du certificat de l'autorite de certification CA
	//
	if( strlen(pCheminCertificatCA) < (TAILLE_MAXI_CHAINE_FICHIER_SSL-1) ) strcpy(CertificatCA,pCheminCertificatCA); else *CertificatCA=0;
	
	// On stocke le nom du fichier du certificat du client
	//
	if( strlen(pCheminCertificatClient) < (TAILLE_MAXI_CHAINE_FICHIER_SSL-1) ) strcpy(CertificatClient,pCheminCertificatClient); else *CertificatClient=0;
	
	// On stocke le nom du fichier de la cle privee du client
	//
	if( strlen(pCheminClePriveeClient) < (TAILLE_MAXI_CHAINE_FICHIER_SSL-1) ) strcpy(ClePriveeClient,pCheminClePriveeClient); else *ClePriveeClient=0;
}


// Destructeur d'un point de communication serveur chiffre
//
PointCommClientChiffre::~PointCommClientChiffre()
{
	if( ConnexionBuffSSL != NULL ) BIO_free(ConnexionBuffSSL);
	if( ConnexionSSL != NULL ) BIO_free(ConnexionSSL);
	if( SocketSSL_BIO != NULL ) BIO_free(SocketSSL_BIO);
	if( IdSocket != -1 ) close(IdSocket);
//	if( StructSSLClient != NULL && SocketSSL_BIO == NULL ) SSL_free(StructSSLClient);	// Il est libere par BIO_free(SocketSSL_BIO) dans le cas d'une connexion realisee SINON CRASH DE L'APPLICATION
	if( ContexteSSL != NULL ) SSL_CTX_free(ContexteSSL);
	ERR_free_strings();
}


// Fonction d'initialisation du point de communication client chiffre
//
int PointCommClientChiffre::Initialisation(void)
{
	// Creation d'un objet de contexte SSL selon une methode de version du protocole
	//
	if( (ContexteSSL=SSL_CTX_new(MethodeVersionSSL)) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Initialisation(): SSL_CTX_new() : impossible de creer le contexte SSL." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// Enregistrement dans le contexte SSL du client de la fonction appelee lors d'un acces a un fichier PEM chiffre
	//  par un mot de passe (typiquement pour l'acces a la cle privee, qui est conservee chiffre par la passphrase PEM,
	//  ou mot de passe)
	//
	SSL_CTX_set_default_passwd_cb(ContexteSSL,FnMotDePasseClePriveeChiffree);
	
	// Chargement du certificat de l'autorite de certification de confiance (le CA) dans le contexte SSL pour le client
	//
	if( !SSL_CTX_load_verify_locations(ContexteSSL,CertificatCA,"/etc/ssl/certs") )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Initialisation(): SSL_CTX_load_verify_locations(): impossible de charger le certificat de l'autorite de confiance pour les certificats de ce client." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// Chargement du certificat et donc de la cle publique du client dans le contexte SSL pour le client
	//
	if( SSL_CTX_use_certificate_file(ContexteSSL,CertificatClient,SSL_FILETYPE_PEM) != 1 )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Initialisation(): SSL_CTX_use_certificate_file(): impossible de charger le certificat (cle publique) du client." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}

	// Chargement de la cle privee du client dans le contexte SSL pour le client
	//
	if( SSL_CTX_use_PrivateKey_file(ContexteSSL,ClePriveeClient,SSL_FILETYPE_PEM) != 1 )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Initialisation(): SSL_CTX_use_PrivateKey_file(): impossible de charger la cle privee du client." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// On verifie la conformite de la cle privee chargee
	//
	if( !SSL_CTX_check_private_key(ContexteSSL) )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Initialisation(): SSL_CTX_check_private_key(): impossible de verifier la conformite de la cle privee du client chargee dans le contexte." << std::endl;
			AfficheErreurPileLibrairieSSL();
		}
		return false;
	}
	
	// Le contexte est cree et parametre, il faut maintenant lui donner un identifieur caracteristique pour pouvoir differencier
	//  les sessions (dans le cas d'importation d'une session generee depuis un autre contexte par exemple)
	//
	sprintf(IdentifieurContexteClient,"CTXClientSSL:%p",this);
	
	if( strlen(IdentifieurContexteClient) < SSL_MAX_SSL_SESSION_ID_LENGTH )
	{
		if( !SSL_CTX_set_session_id_context(ContexteSSL,(const unsigned char *) IdentifieurContexteClient,strlen(IdentifieurContexteClient)) )
		{
			if( ModeVerbeux )
			{
				std::cerr << "PointCommClientChiffre: ERREUR: Initialisation(): SSL_CTX_set_session_id_context(): Impossible de parametrer l'identifieur des sessions crees via le contexte du client." << std::endl;
				AfficheErreurPileLibrairieSSL();
			}
			return false;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Initialisation(): Chaine identifieur sessions du contexte du client trop longue." << std::endl;
		return false;
	}
	
	SSLInitialisee=true;			// La couche SSL est initialisee
	
	return true;
}


// Fonction de creation du point de communication au niveau de la pile reseau
//
// CE:	-
//
// CS:	La fonction est vraie en cas de reussite ;
//
int PointCommClientChiffre::CreationReseau(void)
{
	// Si l'objet n'a pas ete initialise au niveau du SSL alors on le fait
	//
	if( !SSLInitialisee ) if( !Initialisation() ) return false;
	
	
	// Creation d'un point de communication pour le client, une socket du domaine IPv4 (PF_INET Linux),
	//  de type flux de donnees binaire echange continu avec integrite et fiabilite maximale (SOCK_STREAM)
	//  et de protocole specifique IP (/etc/protocols)
	//
	if( (IdSocket=socket(PF_INET,SOCK_STREAM,0)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Impossible d'obtenir une socket pour le client, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EPROTONOSUPPORT ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Type de protocole ou protocole non disponible dans ce domaine de communication." << std::endl;
			if( errno == EAFNOSUPPORT ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Famille d'adresses non supportee." << std::endl;
			if( errno == ENFILE ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Table de descripteur par processus pleine." << std::endl;
			if( errno == EMFILE ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Table des fichiers pleine." << std::endl;
			if( errno == EACCES ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Creation d'une telle socket non autorise." << std::endl;
			if( errno == ENOBUFS || errno == ENOMEM ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Pas assez de memoire pour allouer les buffers necessaires." << std::endl;
			if( errno == EINVAL ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): socket(): Protocole demande inconnu." << std::endl;
		}
		
		return false;
	}
	
	// On parametre le descripteur de socket pour que l'application rende l'adresse et le port immediatement apres sa fermeture
	//  sans duree de retention par le noyau, et, que plusieurs sockets puissent s'attacher au meme port (SO_REUSEADDR=1)
	//
	int ParametreSocket=1;
	
	if( setsockopt(IdSocket,SOL_SOCKET,SO_REUSEADDR,&ParametreSocket,sizeof(int)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommClientChiffre: ERREUR: CreationReseau(): setsockopt(): Impossible de parametrer le descripteur de la socket SO_REUSEADDR pour le client, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): setsockopt(): Le descripteur est un fichier et pas une socket." << std::endl;
			if( errno == ENOPROTOOPT ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): setsockopt(): Parametre option inconnue pour ce protocole." << std::endl;
			if( errno == EFAULT ) std::cerr << "PointCommClientChiffre: ERREUR: CreationReseau(): setsockopt(): Mauvais pointeur passe en parametre." << std::endl;
		}
		
		return false;
	}
	
	SocketCree=true;
	
	return true;
}


// Fonction de connexion au serveur chiffre
//
// CE:	-
//
// CS:	La fonction est vraie si le client s'est connecte au serveur chiffre
//
int PointCommClientChiffre::Connecter(void)
{
	int retour_ssl_connect;			// Valeur de retour de SSL_connect()
	struct timeval timeout_sock_put;	// Timeout socket pour l'emission
	struct timeval timeout_sock_get;	// Timeout socket pour la reception
	
	// Si la socket n'est pas cree alors on le fait
	//
	if( !SocketCree ) if( !CreationReseau() ) return false;
	
	// Pour l'instant on n'est pas connecte au serveur
	//
	ConnecteServeur=false;
	
	// Preparation de la connexion
	//
	memset(&AdresseSocket,0,sizeof(struct sockaddr_in));
	
	AdresseSocket.sin_family=AF_INET;			// Famille de socket IP  (man ip)
	AdresseSocket.sin_addr.s_addr=htonl(AdresseIP);		// Adresse IP du point de communication
	AdresseSocket.sin_port=htons(Port);			// Le numero de port
	
	// Connexion au serveur
	//
	if( connect(IdSocket,(struct sockaddr *) &AdresseSocket,sizeof(struct sockaddr_in)) != 0 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_NET];
		
			sprintf(ChaineErreur,"PointCommClientChiffre: ERREUR: Connecter(): connect(): Echec de la connexion au serveur, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): Mauvais descripteur." << std::endl;
			if( errno == EFAULT ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): La structure d'adresse pointe en dehors de l'espace d'adressage." << std::endl;
			if( errno == ENOTSOCK ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): Le descripteur ne correspond pas a une socket." << std::endl;
			if( errno == EISCONN ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): La socket est deja connectee." << std::endl;
			if( errno == ECONNREFUSED ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): La connexion est refusee par le serveur." << std::endl;
			if( errno == ETIMEDOUT ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): Depassement du delai maximum pendant la connexion." << std::endl;
			if( errno == ENETUNREACH ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): Le reseau est inaccessible." << std::endl;
			if( errno == EADDRINUSE ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): L'adresse est deja utilisee." << std::endl;
			if( errno == EINPROGRESS ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): La socket est non bloquante et la connexion ne peut pas etre etablie immediatement." << std::endl;
			if( errno == EALREADY ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): La socket est non bloquante et une tentative de connexion precedente ne s'est pas encore terminee." << std::endl;
			if( errno == EAGAIN ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): Pas de port local disponible, ou pas assez de place dans les tables de routage." << std::endl;
			if( errno == EAFNOSUPPORT ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): L'adresse transmise n'appartient pas a la famille indiquee dans son champ." << std::endl;
			if( errno == EACCES || errno == EPERM ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): connect(): L'utilisateur a tente de connecter une adresse broadcast sans avoir active l'attribut broadcast, ou la demande de connexion a echouee a cause des regles d'un firewall local." << std::endl;
		}
		
		return false;
	}
	
	// On fixe un timeout pour l'emission
	//
	timeout_sock_put.tv_sec=TimeoutSocketPut;
	timeout_sock_put.tv_usec=0;
	
	if( setsockopt(IdSocket,SOL_SOCKET,SO_SNDTIMEO,(void *) &timeout_sock_put,sizeof(struct timeval)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_SSL];
		
			sprintf(ChaineErreur,"PointCommClientChiffre: ERREUR: Connecter(): setsockopt(): Impossible de parametrer SO_SNDTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOPROTOOPT ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
		}
	}
	
	// On fixe un timeout pour la reception
	//
	timeout_sock_get.tv_sec=TimeoutSocketGet;
	timeout_sock_get.tv_usec=0;
	
	if( setsockopt(IdSocket,SOL_SOCKET,SO_RCVTIMEO,(void *) &timeout_sock_get,sizeof(struct timeval)) == -1 )
	{
		if( ModeVerbeux )
		{
			char ChaineErreur[TAILLE_MAXI_CHAINE_ERREUR_SSL];
		
			sprintf(ChaineErreur,"PointCommClientChiffre: ERREUR: Connecter(): setsockopt(): Impossible de parametrer SO_RCVTIMEO sur ce systeme, numero d'erreur %d:%s.",errno,strerror(errno));
			std::cerr << ChaineErreur << std::endl;
		
			if( errno == EBADF ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): setsockopt(): Le descripteur de socket n'est pas valide." << std::endl;
			if( errno == ENOPROTOOPT ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): setsockopt(): L'option est inconnue pour ce protocole." << std::endl;
		}
	}
	
	// Information de connexion
	//
	if( ModeVerbeux )
	{
		char Chaine[TAILLE_MAXI_CHAINE_COUT_NET];
		
		sprintf(Chaine,"PointCommClientChiffre: Connexion acceptee par le serveur %s(0x%X):%d",inet_ntoa(AdresseSocket.sin_addr),ntohl(AdresseSocket.sin_addr.s_addr),AdresseSocket.sin_port);
		
		std::cerr << Chaine << std::endl;
	}
	
	
	// Creation d'une structure de connexion SSL pour le client
	//
	if( (StructSSLClient=SSL_new(ContexteSSL)) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): SSL_new(): Impossible de creer une structure de connexion SSL au serveur avec le contexte du client." << std::endl;
		
			AfficheErreurPileLibrairieSSL();
		}
		
		close(IdSocket);
		IdSocket=-1;
		
		return false;
	}
	
	// On prepare la structure de connexion SSL a etre en mode client
	//
	SSL_set_connect_state(StructSSLClient);
	
	// La connexion TCP est realisee, on cree une socket SSL dans un objet BIO a partir de la socket (normale) courante
	//
	// BIO_NOCLOSE pour que BIO_free ne ferme pas et ne detruise pas automatiquement la socket
	//
	if( (SocketSSL_BIO=BIO_new_socket(IdSocket,BIO_NOCLOSE)) == NULL )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): BIO_new_socket(): Impossible de creer une socket SSL dans un objet BIO a partir de la socket (normale) courante." << std::endl;
		
			AfficheErreurPileLibrairieSSL();
		}
		
		close(IdSocket);
		IdSocket=-1;
		
		return false;
	}
	
	// On connecte l'objet BIO a la structure de connexion SSL
	//
	// !!! StructSSLClient ne devra plus etre libere par SSL_free() mais par BIO_free(SocketSSL_BIO) !!!
	//
	SSL_set_bio(StructSSLClient,SocketSSL_BIO,SocketSSL_BIO);
	
	
	// Initiation de la connexion SSL avec le serveur (le canal de communication doit etre assigne et parametre dans un objet BIO)
	//
	if( (retour_ssl_connect=SSL_connect(StructSSLClient)) <= 0 )
	{
		if( ModeVerbeux )
		{
			std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): SSL_connect(): La negociation TLS/SSL a echouee avec le serveur." << std::endl;
			AfficheErreurES_SSL(StructSSLClient,retour_ssl_connect);
		}
		
		if( !BIO_free(SocketSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): BIO_free(): Impossible de liberer SocketSSL_BIO." << std::endl;
		SocketSSL_BIO=NULL;
		
		close(IdSocket);
		IdSocket=-1;
		
		return false;
	}
	
	// On cree un objet BIO de type SSL avec la methode BIO_f_ssl() qui permet de le specialiser (ce n'est pas une socket mais
	//  un objet de communication)
	//
	if( (ConnexionSSL=BIO_new(BIO_f_ssl())) == NULL )
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): BIO_new(): Impossible de creer l'objet de connexion BIO de type SSL." << std::endl;
		
		//SSL_free(StructSSLClient);	Il est libere par BIO_free(SocketSSL_BIO)
		//StructSSLClient=NULL;
		
		if( !BIO_free(SocketSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): BIO_free(): Impossible de liberer SocketSSL_BIO." << std::endl;
		SocketSSL_BIO=NULL;
		
		close(IdSocket);
		IdSocket=-1;
		
		return false;
	}
	
	// On attache la structure de connexion SSL a l'objet BIO SSL
	//
	BIO_set_ssl(ConnexionSSL,StructSSLClient,BIO_CLOSE);
	
	// On cree un objet BIO de communication avec buffer avec la methode BIO_f_buffer() qui permet de le specialiser,
	//  le tableau est de DEFAULT_BUFFER_SIZE octets par defaut
	//
	if( (ConnexionBuffSSL=BIO_new(BIO_f_buffer())) == NULL )
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): BIO_new(): Impossible de creer l'objet de connexion BIO de type buffer." << std::endl;
		
		if( !BIO_free(ConnexionSSL) ) fprintf(stderr,"PointCommClientChiffre: ERREUR: Connecter(): BIO_free(): Impossible de liberer ConnexionSSL.\n");
		ConnexionSSL=NULL;
		
		//SSL_free(StructSSLClient);	Il est libere par BIO_free(SocketSSL_BIO)
		//StructSSLClient=NULL;
		
		if( !BIO_free(SocketSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): BIO_free(): Impossible de liberer SocketSSL_BIO." << std::endl;
		SocketSSL_BIO=NULL;
		
		close(IdSocket);
		IdSocket=-1;
		
		return false;
	}
	
	// On chaine l'objet BIO SSL avec l'objet BIO buffer
	// La chaine d'objets est donc :
	// Client:ConnexionBuff->ConnexionSSL->StructSSLClient->IdSocket -> Serveur
	//
	BIO_push(ConnexionBuffSSL,ConnexionSSL);
	
	if( ParamSIGPIPE )
	{
		// On parametre le nouveau gestionnaire du signal SIGPIPE
		// SIGPIPE est declanche lorsqu'un processus tente d'ecrire dans tube ferme
		//
		NouvGestSignalSIGPIPE.sa_handler=FnHandlerSIGPIPE;
		
		if( sigaction(SIGPIPE,&NouvGestSignalSIGPIPE,&AncienGestSignalSIGPIPE) < 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Connecter(): sigaction(): Impossible de parametrer un gestionnaire du signal SIGPIPE." << std::endl;
		}
		// kill(getpid(),SIGPIPE); pour tester
	}
	
	// Le chiffreur utilise pour cette connexion
	//
	Chiffreur=SSL_get_current_cipher(StructSSLClient);
	
	// La negociation est reussie entre le client et le serveur selon le contexte defini
	//
	if( ModeVerbeux )
	{
		char Chaine[TAILLE_MAXI_CHAINE_COUT_SSL];
	
		sprintf(Chaine,"PointCommClientChiffre: Connexion SSL acceptee avec le serveur %s(0x%X):%d",inet_ntoa(AdresseSocket.sin_addr),ntohl(AdresseSocket.sin_addr.s_addr),AdresseSocket.sin_port);
	
		std::cerr << Chaine << std::endl;
	}
	
	// On est connecte a un serveur chiffre
	//
	ConnecteServeur=true;
	
	return true;
}


// Fonction de fermeture de la connexion avec le serveur
//
// CE:	-
//
// CS:	-
//
void PointCommClientChiffre::Fermer(void)
{
	int RetourES_SSL;		// Valeur retournee par les fonctions E/S SSL
	int NombreShutdown=0;		// Nombre de tentatives possibles de fermeture de la connexion SSL
	
	// On passe la connexion SSL en mode de fermeture en toute quietude
	//  la notification de fermeture n'est pas envoye au paire pour prevenir le crash de SSL_shutdown()
	//  en cas de perte de connexion ou disparition du paire
	//
	SSL_set_quiet_shutdown(StructSSLClient,1);
	
	// On ferme la connexion SSL
	//
	while( (RetourES_SSL=SSL_shutdown(StructSSLClient)) != 1 && NombreShutdown < 4 )
	{
		NombreShutdown++;
		
		if( RetourES_SSL == 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): SSL_shutdown(): Tentative " << NombreShutdown << ": La fermeture de la socket SSL n'est pas terminee." << std::endl;
			
			// On signale au client que la socket ne sera plus utilisee pour l'ecriture ce qui evite de boucler plusieurs fois
			//
			if( shutdown(IdSocket,SHUT_WR) < 0 )
			{
				if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): shutdown(): Impossible de signaler au serveur que la socket ne sera plus utilisee pour l'ecriture." << std::endl;
			}
		}
		else
		{
			if( ModeVerbeux )
			{
				std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): SSL_shutdown(): Impossible de fermer la connexion SSL, erreur fatale au niveau du protocole ou erreur fatale de connexion." << std::endl;
			
				AfficheErreurES_SSL(StructSSLClient,RetourES_SSL);
			}
		}
	}
	
	if( RetourES_SSL == 1 )
	{
		if( ModeVerbeux )
		{
			char Chaine[TAILLE_MAXI_CHAINE_COUT_SSL];
			
			sprintf(Chaine,"PointCommClientChiffre: Connexion SSL fermee avec le serveur %s(0x%X):%d",inet_ntoa(AdresseSocket.sin_addr),ntohl(AdresseSocket.sin_addr.s_addr),AdresseSocket.sin_port);
			
			std::cerr << Chaine << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): SSL_shutdown(): Impossible de fermer la connexion SSL apres plusieurs tentatives." << std::endl;
	}
	
	if( ParamSIGPIPE )
	{
		// On parametre le gestionnaire de SIGPIPE initial
		//
		if( sigaction(SIGPIPE,&AncienGestSignalSIGPIPE,NULL) < 0 )
		{
			if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): sigaction(): Impossible de parametrer un gestionnaire du signal SIGPIPE." << std::endl;
		}
	}
	
	// Liberation de toute la chaine BIO
	//
	if( !BIO_free(ConnexionBuffSSL) ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): BIO_free(): Impossible de liberer ConnexionBuffSSL." << std::endl;
	ConnexionBuffSSL=NULL;
	
	if( !BIO_free(ConnexionSSL) ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): BIO_free(): Impossible de liberer ConnexionSSL." << std::endl;
	ConnexionSSL=NULL;
	
	//SSL_free(StructSSLClient);	Il est libere par BIO_free(SocketSSL_BIO) !!!
	
	if( !BIO_free(SocketSSL_BIO) ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: Fermer(): BIO_free(): Impossible de liberer SocketSSL_BIO." << std::endl;
	SocketSSL_BIO=NULL;
	
	close(IdSocket);
	IdSocket=-1;
	SocketCree=false;
	
	Chiffreur=NULL;
	
	// Le client n'est plus connecte au serveur
	//
	ConnecteServeur=false;
}


// Pour savoir si on est connecte au serveur chiffre
//
// CE: -
//
// CS:	La fonction est vraie si on est connecte au serveur chiffre
//
int PointCommClientChiffre::Connecte(void)
{
	return ConnecteServeur;
}


// Fonction d'emission d'une chaine par un objet BIO de type buffer et traitement de son code de retour
//
// CE:	On passe un pointeur vers la chaine a envoyer ;
//
// CS:	La fonction retourne la valeur retournee par BIO_puts()
//
int PointCommClientChiffre::EnvoyerChaineBIO(const char *chaine)
{
	int errno_avant=errno;		// Sauvegarde de errno
	int RetourPuts;				// Valeur retournee par BIO_puts
	
	// Emission de la chaine sur l'objet BIO bufferise
	//
	RetourPuts=BIO_puts(ConnexionBuffSSL,chaine);
	
	if( RetourPuts <= 0 || errno != errno_avant )
	{
		if( RetourPuts == 0 ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: EnvoyerChaineBIO(): BIO_puts(): Connexion abandonnee par le client." << std::endl;
		
		if( RetourPuts < 0 ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR TIMEOUT: EnvoyerChaineBIO(): BIO_puts(): Duree de non activite depassee, la connexion est consideree comme perdue." << std::endl;
		
		if( errno != errno_avant )
		{
			RetourPuts=-1;		// Pour detecter l'erreur simplement avec le code retour
			
			if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: EnvoyerChaineBIO(): BIO_puts(): errno=" << errno << " : " << strerror(errno) << "." << std::endl;
		}
	}
	else
	{
		if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: C->S " << RetourPuts <<": " << chaine << std::endl;
	}

	// ATTENTION : Bien mettre le BIO_flush() apres de code de traitement des erreurs de BIO_puts() car il peut modifier errno
	//	
	BIO_flush(ConnexionBuffSSL);
	
	return RetourPuts;
}


// Reception d'une chaine via la connexion SSL et l'objet BIO type buffer
//
// CE:	-
//
// CS:	la fonction retourne la valeur de BIO_gets() ;
//
int PointCommClientChiffre::RecevoirChaineBIO(char *chaine)
{
	int RetourGets=BIO_gets(ConnexionBuffSSL,chaine,TAILLE_MAXI_CHAINE_BIO);
	
	if( RetourGets == 0 ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR: RecevoirChaineBIO(): BIO_gets(): Connexion abandonnee par le client." << std::endl;
	
	if( RetourGets == -1 ) if( ModeVerbeux ) std::cerr << "PointCommClientChiffre: ERREUR TIMEOUT: RecevoirChaineBIO(): BIO_gets(): Duree de non activite depassee, la connexion est consideree comme perdue." << std::endl;
	
	if( ModeVerbeux && RetourGets > 0 ) std::cerr << "PointCommClientChiffre: C<-S " << RetourGets <<": " << chaine << std::endl;
	
	return RetourGets;
}


// Fonction retournant un pointeur vers le chiffreur de la connexion SSL courante
//
// CE:	-
//
// CS:	La fonction retourne le pointeur ;
//
const SSL_CIPHER *PointCommClientChiffre::DescripteurChiffreurConnexionSSL(void)
{
	return Chiffreur;
}


// FONCTION D'AFFICHAGE DES ERREURS PRODUITES SUR LA PILE DE LA LIBRAIRIE SSL
//
//	CE:	-
//
//	CS:	-
//
void PointCommClientChiffre::AfficheErreurPileLibrairieSSL(void)
{
	unsigned long numero;				// Numero de l'erreur de la librairie SSL
	char Chaine[TAILLE_MAXI_CHAINE_ERREUR_SSL];	// Chaine de composition des erreurs
	
	while( (numero=ERR_get_error()) != 0 )
	{
		ERR_error_string_n(numero,Chaine,TAILLE_MAXI_CHAINE_ERREUR_SSL);

		std::cerr << "PointCommClientChiffre: ERREUR: SSL : " << Chaine << std::endl;
	}
}


// FONCTION D'AFFICHAGE DES ERREURS PRODUITES PAR LES OPERATIONS E/S TLS/SSL
//
// CE:	On passe un pointeur sur la structure de la connexion SSL ;
//
//	On passe la valeur retournee par la fonction d'e/s TLS/SSL ;
//
// CS:	-
//
void PointCommClientChiffre::AfficheErreurES_SSL(SSL *structure,int retour)
{
	switch( SSL_get_error(structure,retour) )
	{
		case SSL_ERROR_NONE:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_NONE: Aucune erreur d'e/s." << std::endl;
			break;
		case SSL_ERROR_ZERO_RETURN:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_ZERO_RETURN: La connexion TLS/SSL a ete fermee par une alerte du protocole." << std::endl;
			break;
		case SSL_ERROR_WANT_READ:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_WANT_READ: L'operation de lecture n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_WRITE:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_WANT_WRITE: L'operation d'ecriture n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_CONNECT:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_WANT_CONNECT: L'operation de connexion n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_ACCEPT:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_WANT_ACCEPT: L'operation de negociation SSL n'a pu se realiser." << std::endl;
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_WANT_X509_LOOKUP: L'operation n'a pu se realiser car la fonction de retour SSL_CTX_set_client_cert_cb() doit etre appelee une nouvelle fois." << std::endl;
			break;
		case SSL_ERROR_SYSCALL:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_SYSCALL: Erreurs d'appel systeme d'entrees/sorties ." << std::endl;
			break;
		case SSL_ERROR_SSL:
			std::cerr << "PointCommClientChiffre: ERREUR SSL_ERROR_SSL: Erreur dans la librairie SSL, sans doute une erreur de protocole." << std::endl;
			break;
	}
}

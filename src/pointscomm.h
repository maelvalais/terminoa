//
// HEADER DU MODULE DES CLASSES DE POINTS DE COMMUNICATION
//
// (C)David.Romeuf@univ-lyon1.fr 16/03/2006 par David Romeuf
//


#ifndef _POINTSCOMM_H_
#define _POINTSCOMM_H_

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


// Inclusion des entetes C qui ne sont pas fournit par le systeme
//
extern "C"
{
}


#define TAILLE_MAXI_CHAINE_FICHIER_SSL		1024
#define TAILLE_MAXI_CHAINE_VARIABLE_SSL		1024
#define TAILLE_MAX_MDP_CLE_PRIVEE			1024
#define TAILLE_MAXI_CHAINE_ERREUR_SSL		1024
#define TAILLE_MAXI_CHAINE_COUT_SSL			1024
#define TAILLE_MAXI_CHAINE_BIO				1024

#define TAILLE_MAXI_CHAINE_NET				1024
#define TAILLE_MAXI_CHAINE_ERREUR_NET		1024
#define TAILLE_MAXI_CHAINE_COUT_NET			1024

#define CARACTERE_HORS_BANDE		"--@"	// ATTENTION : @ au hasard, -- avant pour le preceder de caracteres, eviter bloquage recv()
#define TAILLE_BUFFER_SOCKET		2048

#define TAILLE_BUFFER_ENVOYER_FICHIER	1024
#define TAILLE_BUFFER_RECEVOIR_FICHIER	1024
#define TAILLE_IDENTITE_FICHIER		26		// Nombre de caracteres de l'identite d'un fichier


// Classe de base d'un point de communication
//
class PointComm
{
private:

protected:
	int IdSocket;										// Identifieur du point de communication
	uint32_t AdresseIP;									// Adresse IP d'attachement
	uint16_t Port;										// Port IP d'attachement
	int TimeoutSocketPut;								// Timeout en secondes pour la tentative d'ecriture de donnees dans la socket
	int TimeoutSocketGet;								// Timeout en secondes pour la tentative de lecture de donnees dans la socket
	struct sockaddr_in AdresseSocket;					// Structure decrivant l'adresse:port de la socket pour le domaine AF_INET
	
	int SocketCree;										// Indicateur si la socket est cree
	
	// Drapeau pour mode verbeux
	//
	int ModeVerbeux;

public:
	PointComm(int pverbeux,int pTimeoutSocketPut,int pTimeoutSocketGet);		// Constructeur
	virtual ~PointComm();														// Destructeur
	
	virtual int Initialisation(void)=0;					// Fonction virtuelle pure imposant la redefinition dans la classe enfant
	
	int ObjetModeVerbeux(void);							// Pour savoir si on est en mode verbeux ou pas
	
	int SocketBloquante(void);							// Passage de la socket en mode bloquant
	int SocketNonBloquante(void);						// Passage de la socket en mode NON bloquant
	
	int PossibleLireDonneesSocket(int timeout);			// Pour savoir si des donnees sont disponibles en lecture sur la socket sous x ms ?
	int PossibleEcrireDonneesSocket(int timeout);		// Pour savoir si on peut ecrire des donnees sur la socket sous x ms ?
	
	int EnvoyerChaineSocket(const char *chaine);						// Envoyer une chaine de caractere sur la socket
	long EnvoyerFichierSocket(const char *nom,const char *id_fichier);	// Envoyer un fichier sur la socket
	int EnvoyerDonneesSocket(void *buf,int nb);							// Envoyer des donnees
	int LireDonneesSocket(void *buf,int max);			// Recevoir des donnees et les stocker
	int LireNbDonneesSocket(void *buf,int nb);			// Recevoir nb octets et les stocker
	int LireLigneSocket(char *buf,int max);				// Recevoir une ligne termine par \n

	int CloseIdSocket(void);							// Fermeture de la socket IdSocket
};


// Classe de base d'un point de communication Serveur non chiffre mono client
//
class PointCommServeurNonChiffreMonoClient : public PointComm
{
private:

protected:
	int ObjetInitialise;								// Indicateur d'initialisation de l'objet
	int SocketAttachee;									// Indicateur pour savoir si la socket est bien attachee
	int SocketEcoute;									// Indicateur pour savoir si la socket est a l'ecoute
	
	int IdSocketSession;								// Descripteur de la socket de session courante
	struct sockaddr_in AdresseSocketClient;				// Adresse de la socket de session du client _in car domaine Internet
	uint32_t AdresseIPClientAutorise;					// Adresse IP du client autorise
	int NbListeClientsMax;								// Nombre de clients maximum dans la file des connexions clientes en attente
	
	int ClientConnecte;									// Indicateur si un client est connecte

public:
	// Constructeur
	PointCommServeurNonChiffreMonoClient(int pverbeux,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet);
	
	// Destructeur
	virtual ~PointCommServeurNonChiffreMonoClient();
	
	int Initialisation(void);							// Fonction d'initialisation de l'objet point de communication serveur non chiffre
	
	int CreationReseau(void);							// Creation du point de communication
	
	int AttachementReseau(void);						// Attachement du point de communication
	
	int EcouteReseau(void);								// Lancement de l'ecoute du serveur IP:port
	
	int AttenteAccepterSessionAutorisee(void);			// Attente et acceptation d'une session de connexion
	
	int SessionAccepteeAutorisee(void);					// Pour savoir si une session a ete acceptee et autorisee
	
	int EcrireCaractereHorsBandeSocketSession(void);	// Envoyer un caractere urgent hors bande
	int LireJusquAuCaractereHorsBandeSocketSession(void);	// Recevoir jusqu'au prochain caractere hors bande
	
	int EnvoyerChaineSocketSession(const char *chaine);					// Envoyer une chaine de caractere
	int EnvoyerDonneesSocketSession(void *buf,int nb);					// Envoyer des donnees
	int LireDonneesSocketSession(void *buf,int max);					// Recevoir des donnees et les stocker
	long RecevoirFichierSocketSession(const char *nom,char *identite);	// Recevoir un fichier par la socket de session
	
	int UnClientEstConnecte(void);						// Pour savoir si un client est actuellement connecte
	
	int SocketSessionBloquante(void);					// Passage de la socket de session courante en mode bloquant
	int SocketSessionNonBloquante(void);				// Passage de la socket de session courante en mode NON bloquant
	int SocketSessionHorsBandeInLine(void);				// Passage de la socket de session en mode lecture en ligne du caractere hors bande
	
	int PossibleLireDonneesSocketSession(int timeout);		// Pour savoir si des donnees sont disponibles en lecture sur la socket ?
	int PossibleEcrireDonneesSocketSession(int timeout);	// Pour savoir si on peut ecrire des donnees sur la socket ?
	
	void FermetureSession(void);						// Fermeture de la connexion courante
};


// Classe de base d'un point de communication Serveur chiffre mono client
//
class PointCommServeurChiffreMonoClient : public PointComm
{
private:
	// Chemin complet et nom du fichier PEM du certificat de l'autorite de certification CA
	//
	char CertificatCA[TAILLE_MAXI_CHAINE_FICHIER_SSL];
	
	// Chemin complet et nom du fichier PEM du certificat du serveur (cle publique)
	//
	char CertificatServeur[TAILLE_MAXI_CHAINE_FICHIER_SSL];
	
	// Chemin complet et nom du fichier PEM de la cle privee du serveur
	//
	char ClePriveeServeur[TAILLE_MAXI_CHAINE_FICHIER_SSL];
	
	// Chemin complet et nom du fichier PEM des parametres Diffie-Hellman aleatoires (pour optimiser la vitesse de reponse du serveur)
	//
	char ChemParametresDH[TAILLE_MAXI_CHAINE_FICHIER_SSL];
	
	// Chaine de la liste des chiffreurs que le serveur doit utiliser
	//
	char ListeChiffreurs[TAILLE_MAXI_CHAINE_VARIABLE_SSL];
	
	// Chaine contenant le nom du contexte du serveur
	//
	char IdentifieurContexteServeur[TAILLE_MAXI_CHAINE_VARIABLE_SSL];
	
	// Pointeur sur la fonction C appelee lors de l'acces a un fichier PEM chiffre par un mot de passe de la cle privee du serveur
	//
	int (*FnMotDePasseClePriveeChiffree)(char*, int, int, void*);
	
	// Pointeur sur la fonction C de handler du signal SIGPIPE
	//
	void (*FnHandlerSIGPIPE)(int);
	
	// Drapeau pour le parametrage ou non de SIGPIPE
	//
	int ParamSIGPIPE;
	
protected:
	int SSLInitialisee;									// Indicateur d'initialisation de la couche SSL
	int SocketAttachee;									// Indicateur pour savoir si la socket est bien attachee
	int SocketEcoute;									// Indicateur pour savoir si la socket est a l'ecoute
	
	int IdSocketSession;								// Descripteur de la socket de session courante
	struct sockaddr_in AdresseSocketClient;				// Adresse de la socket de session du client _in car domaine Internet
	uint32_t AdresseIPClientAutorise;					// Adresse IP du client autorise
	int NbListeClientsMax;								// Nombre de clients maximum dans la file des connexions clientes en attente
	
	const SSL_METHOD *MethodeVersionSSL;						// Pointeur sur la methode SSL v2 ou V3
	SSL_CTX *ContexteSSL;								// Pointeur sur le contexte SSL, objet avec cle et certificats
	BIO *ObjetBIO;										// Pointeur sur un objet BIO de la librairie OpenSSL
	DH *ParametresDH;									// Pointeur sur les parametres aleatoires Diffie-Hellman
	BIO *SocketSessionSSL_BIO;							// Pointeur vers une socket de session SSL dans un objet BIO
	SSL *StructSSLServeur;								// Structure SSL de connexion
	BIO *ConnexionSSL;									// Pointeur vers un objet BIO de type SSL de la connexion
	BIO *ConnexionBuffSSL;								// Pointeur vers un objet BIO de type SSL de la connexion en mode buffer ligne
	struct sigaction AncienGestSignalSIGPIPE;			// Ancien gestionnaire du signal SIGPIPE avant gestion de ConnexionBuffSSL
	struct sigaction NouvGestSignalSIGPIPE;				// Nouveau gestionnaire du signal SIGPIPE pour gestion de ConnexionBuffSSL
	const SSL_CIPHER *Chiffreur;								// Pointeur vers description du chiffreur de la connexion
	int TimeoutSSLAccept;								// Timeout en secondes de l'initiative de la negociation TLS/SSL
	
	int ClientConnecte;									// Indicateur si un client est connecte
	
	void AfficheErreurPileLibrairieSSL(void);			// Affichage des erreurs produites sur la pile de la librairie SSL
	void AfficheErreurES_SSL(const SSL *structure,int retour);	// Affichage des erreurs produitent par les operations E/S TLS/SSL

public:
	// Constructeur
	PointCommServeurChiffreMonoClient(int pverbeux,uint32_t pAdresse,uint16_t pPort,uint32_t pAdresseClient,int pNbLClientsMax,int pTimeoutSocketPut,int pTimeoutSocketGet,int pTimeoutNegoTLSSSL,int pParamSIGPIPE,void (*pFnHandlerSIGPIPE)(int),const char *MdpClePriveeServeur,char *BuffStockMdpClePriveeServeur,int (*pFnMotDePasseClePriveeChiffree)(char*, int, int, void*),const char *pCheminCertificatCA,const char *pCheminCertificatServeur,const char *pCheminClePriveeServeur,const char *pParametresDH,const char *pListeChiffreurs);
	
	// Destructeur
	virtual ~PointCommServeurChiffreMonoClient();
	
	int Initialisation(void);							// Fonction d'initialisation de l'objet point de communication serveur chiffre
	
	int CreationReseau(void);							// Creation du point de communication
	
	int AttachementReseau(void);						// Attachement du point de communication
	
	int EcouteReseau(void);								// Lancement de l'ecoute du serveur IP:port
	
	int AttenteAccepterSessionAutorisee(void);			// Attente et acceptation d'une session de connexion
	
	int SessionAccepteeAutorisee(void);					// Pour savoir si une session a ete acceptee et autorisee
	
	int NegociationConnexionSSL(void);					// Negociation d'une connexion SSL avec un client autorise
	
	void FermetureConnexionSSL(void);					// Fermeture de la connexion SSL courante
	
	int EnvoyerChaineBIO(const char *chaine);			// Envoyer une chaine de caractere par l'objet BIO ConnexionBuffSSL
	int RecevoirChaineBIO(char *chaine);				// Reception d'une chaine de caractere par l'objet BIO ConnexionBuffSSL
	
	int UnClientEstConnecte(void);						// Pour savoir si un client est actuellement connecte
	
	int SocketSessionBloquante(void);					// Passage de la socket de session courante en mode bloquant
	int SocketSessionNonBloquante(void);				// Passage de la socket de session courante en mode NON bloquant
	
	int PossibleLireDonneesSocketSession(int timeout);	// Pour savoir si des donnees sont disponibles en lecture sur la socket ?
	int PossibleEcrireDonneesSocketSession(int timeout);	// Pour savoir si on peut ecrire des donnees sur la socket ?
	
	const SSL_CIPHER *DescripteurChiffreurConnexionSSL(void);	// Retourne un pointeur vers le descripteur du chiffreur de la session courante
	
};


// Classe de base d'un point de communication Client non chiffre
//
class PointCommClientNonChiffre : public PointComm
{
private:

protected:
	int ObjetInitialise;								// Indicateur d'initialisation de l'objet
	
	int ConnecteServeur;								// Indicateur pour savoir si l'objet est connecte a un serveur

public:
	// Constructeur
	PointCommClientNonChiffre(int pverbeux,uint32_t pAdresse,uint16_t pPort,int pTimeoutSocketPut,int pTimeoutSocketGet);
	
	// Destructeur
	virtual ~PointCommClientNonChiffre();
	
	int Initialisation(void);							// Fonction d'initialisation de l'objet point de communication client non chiffre
	
	int CreationReseau(void);							// Creation du point de communication
	
	int Connecter(void);								// Connexion au serveur
	
	int EcrireCaractereHorsBande(void);					// Envoyer un caractere urgent hors bande
	int LireJusquAuCaractereHorsBande(void);			// Recevoir jusqu'au prochain caractere hors bande
	
	void Fermer(void);									// Fermeture de la connexion courante
	
	int Connecte(void);									// Pour savoir si on est connecte au serveur
};


// Classe de base d'un point de communication Client chiffre
//
class PointCommClientChiffre : public PointComm
{
private:
	// Chemin complet et nom du fichier PEM du certificat de l'autorite de certification CA
	//
	char CertificatCA[TAILLE_MAXI_CHAINE_FICHIER_SSL];
	
	// Chemin complet et nom du fichier PEM du certificat du client (cle publique)
	//
	char CertificatClient[TAILLE_MAXI_CHAINE_FICHIER_SSL];
	
	// Chemin complet et nom du fichier PEM de la cle privee du client
	//
	char ClePriveeClient[TAILLE_MAXI_CHAINE_FICHIER_SSL];
	
	// Chaine contenant le nom du contexte du client
	//
	char IdentifieurContexteClient[TAILLE_MAXI_CHAINE_VARIABLE_SSL];
	
	// Pointeur sur la fonction C appelee lors de l'acces a un fichier PEM chiffre par un mot de passe de la cle privee du client
	//
	int (*FnMotDePasseClePriveeChiffree)(char*, int, int, void*);
	
	// Pointeur sur la fonction C de handler du signal SIGPIPE
	//
	void (*FnHandlerSIGPIPE)(int);
	
	// Drapeau pour le parametrage ou non de SIGPIPE
	//
	int ParamSIGPIPE;
	
protected:
	int SSLInitialisee;									// Indicateur d'initialisation de la couche SSL
	int ConnecteServeur;								// Indicateur pour savoir si l'objet est connecte a un serveur
	
	const SSL_METHOD *MethodeVersionSSL;						// Pointeur sur la methode SSL v2 ou V3
	SSL_CTX *ContexteSSL;								// Pointeur sur le contexte SSL, objet avec cle et certificats
	SSL *StructSSLClient;								// Structure SSL de connexion du client
	BIO *SocketSSL_BIO;									// Pointeur vers une socket SSL dans un objet BIO
	BIO *ConnexionSSL;									// Pointeur vers un objet BIO de type SSL de la connexion
	BIO *ConnexionBuffSSL;								// Pointeur vers un objet BIO de type SSL de la connexion en mode buffer ligne
	struct sigaction AncienGestSignalSIGPIPE;			// Ancien gestionnaire du signal SIGPIPE avant gestion de ConnexionBuffSSL
	struct sigaction NouvGestSignalSIGPIPE;				// Nouveau gestionnaire du signal SIGPIPE pour gestion de ConnexionBuffSSL
	const SSL_CIPHER *Chiffreur;								// Pointeur vers description du chiffreur de la connexion
	
	void AfficheErreurPileLibrairieSSL(void);			// Affichage des erreurs produites sur la pile de la librairie SSL
	void AfficheErreurES_SSL(SSL *structure,int retour);	// Affichage des erreurs produitent par les operations E/S TLS/SSL

public:
	// Constructeur
	PointCommClientChiffre(int pverbeux,uint32_t pAdresse,uint16_t pPort,int pTimeoutSocketPut,int pTimeoutSocketGet,int pParamSIGPIPE,void (*pFnHandlerSIGPIPE)(int),const char *MdpClePriveeClient,char *BuffStockMdpClePriveeClient,int (*pFnMotDePasseClePriveeChiffree)(char*, int, int, void*),const char *pCheminCertificatCA,const char *pCheminCertificatClient,const char *pCheminClePriveeClient);
	
	// Destructeur
	virtual ~PointCommClientChiffre();
	
	int Initialisation(void);							// Fonction d'initialisation de l'objet point de communication serveur chiffre
	
	int CreationReseau(void);							// Creation du point de communication
	
	int Connecter(void);								// Connexion au serveur
	void Fermer(void);									// Fermeture de la connexion courante
	
	int EnvoyerChaineBIO(const char *chaine);			// Envoyer une chaine de caractere par l'objet BIO ConnexionBuffSSL
	int RecevoirChaineBIO(char *chaine);				// Reception d'une chaine de caractere par l'objet BIO ConnexionBuffSSL
	
	const SSL_CIPHER *DescripteurChiffreurConnexionSSL(void);	// Retourne un pointeur vers le descripteur du chiffreur de la connexion courante

	int Connecte(void);									// Pour savoir si on est connecte au serveur
};


#endif

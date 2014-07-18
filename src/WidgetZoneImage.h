/* HEADER DU MODULE DU WIDGET DE LA ZONE IMAGE

   (C)David.Romeuf@univ-lyon1.fr 2006 by David Romeuf
*/

#ifndef _WIDGETZONEIMAGE_
#define _WIDGETZONEIMAGE_

// Inclusions C++
//
#include <iostream>
#include <valarray>

// Inclusions Qt
//
#include <qcolor.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qstring.h>
#include <qwidget.h>

#define TAILLE_X_OBJZONEIMAGE_RECOMMANDEE	840	// Taille recommandee en x de l'objet
#define TAILLE_Y_OBJZONEIMAGE_RECOMMANDEE	525	// Taille recommandee en y de l'objet

#define FICHIER_IMAGE_COURANTE	"courante.fts"		// Nom du fichier FITS de l'image courante

#define NB_ELEMENTS_LUT		2048			// Nombre d'elements pour la Look Up Table
#define NB_ELEMENTS_PALETTE	2048			// Nombre d'elements pour la palette

#define MAX_ROUGE		255			// Valeur maximale de la composante rouge
#define MAX_VERT		255			// Valeur maximale de la composante verte
#define MAX_BLEU		255			// Valeur maximale de la composante bleue

#define TAUX_COMPRESSION_IMAGES_JPEG_DIFFUSEES	80


// Etats d'affichage du widget
//
enum EtatsAffichage
{
	AffichageImageLancement,
	AffichageImageCourante,
	AffichageImageDiaporama
};

// Types des echelles de l'histogramme
//
enum TypesEchelles
{
	Lineaire,
	Logarithmique
};

// Types des palettes d'affichage
//
enum TypesPalettes
{
	NoirEtBlanc,
	Halpha,
	NoirOrangeBlanc,
	NoirVertBlanc,
	NoirBleuBlanc,
	PaletteNULL
};

// Liste des zones affichables possibles de l'image courante
//
enum ListeZAPIC
{
	Z0,		// Image entiere
	Z1,		// Coin superieur gauche
	Z2,
	Z3,		// Coin superieur droit
	Z4,		// Tier superieur gauche
	Z5,
	Z6,
	Z7,		// 2/3 superieur gauche
	Z8,
	Z9,
	ZoneNULL
};


// Widget de zone d'affichage graphique
//
class ObjZoneImage : public QWidget
{
	Q_OBJECT

// Zone inaccessible sauf par l'objet lui meme
private :

// Zone protegee de l'exterieur sauf des classes qui heritent
protected:
	// Proprietes
	EtatsAffichage EtatAffichageCourant;		// Etat d'affichage courant de l'objet
	QString CheminImageLancement;			// Chemin vers l'image de lancement de l'application
	QString CheminImageCourante;			// Chemin vers l'image courante CLIMSO a representer
	QString CheminImageDiaporama;			// Chemin vers l'image courante du diaporama a representer
	QString CheminDocuments;				// Chemin vers les documents a diffuser
	QString CheminFichImgWeb;				// Chemin et nom du fichier de l'image diffusee sur le web en temps reel
	QString IdTerminOA;						// Identifieur aleatoire du TerminOA
	
	std::valarray<long> DonneesIC;			// Donnees de l'image courante dans un tableau de valeur de la STL C++
	long DimXIC;					// Dimension en x de l'image courante
	long DimYIC;					// Dimension en y de l'image courante
	long ValMinIC;					// Valeur minimale des donnees de l'image
	long ValMaxIC;					// Valeur maximale des donnees de l'image
	long NbBitsDonneesIC;				// Nombre de bits necessaires pour representer les donnees de cette image
	long SeuilBasConsigne;				// Seuil bas de visualisation en consigne
	long SeuilHautConsigne;				// Seuil haut de visualisation en consigne
	long SeuilBas;					// Seuil bas de visualisation effectif
	long aSeuilBas;					// Pour sauvegarder le seuil bas utilise lors du dernier trace
	long SeuilHaut;					// Seuil haut de visualisation effectif
	long aSeuilHaut;				// Pour sauvegarder le seuil haut utilise lors du dernier trace
	double PuiLUT;					// Puissance de la forme de la Look Up Table
	double aPuiLUT;					// Pour sauvegarder la puissance de la LUT lors du dernier trace
	QString DateHeureIC;				// Date et heure de l'image courante
	ListeZAPIC ZoneAffichee;			// Zone affichee de l'image courante
	ListeZAPIC aZoneAffichee;			// Pour sauvegarder l'ancienne zone affichee
	int ICJamaisAffichee;				// Drapeau pour signifier que l'image courante chargee n'a jamais ete affichee
	std::valarray<long> DonneesICAff;		// Donnees de l'image courante affichee sur l'ecran dans un valarray de la STL C++
	QPixmap ICPixmap;				// Pixmap pour l'affichage rapide de l'image courante via bitBlt sur ce widget
	TypesPalettes TPalette;				// Type de palette
	TypesPalettes aTPalette;			// Pour sauvegarder le type de palette
	QPen PinceauxPalette[NB_ELEMENTS_PALETTE];	// Pinceaux de la palette
	int LUT[NB_ELEMENTS_LUT];			// Nombre d'elements de la Look Up Table (indice n palette fn valeur)
	
	std::valarray<unsigned long> HistogrammeIC;	// Histogramme de l'image courante dans un tableau de valeur de la STL C++
	TypesEchelles TEchelleHistog;			// Type d'echelle de l'histogramme
	double IntervaleMembreHistogramme;		// Intervale en valeur represente par un membre de l'histogramme
	
	// Methodes
	void paintEvent(QPaintEvent *event);		// Surcharge du slot herite de QWidget
	QSize sizeHint();				// Surcharge de la fonction retournant la dimension recommandee pour le widget
	double InterpolationBiLineaire(double x,double y);	// Fonction d'interpolation bi-lineaire dans l'image source courante

	
// Zone accessible par l'exterieur
public:
	void ForceIC_NULL(void);			// Force l'image courante a l'etat NULL
	void ForceICAff_NULL(void);			// Force l'image courante affichee a l'etat NULL
	void CalculNbBitsDonneesIC(void);		// Calcul du nombre de bits necessaires pour representer les donnees de cette image
	
	void CalculerHistogrammeIC(void);		// Fonction de calcul de l'histogramme de l'image courante
	void ForceHistogrammeIC_NULL(void);		// Force l'histogramme de l'image courante a l'etat NULL
	
	// Constructeur de l'objet (jamais de type de retour)
	ObjZoneImage(QWidget *parent=0,const char *name=0,WFlags f=0);
	
	// Destructeur de l'objet (jamais de type de retour et d'argument)
	~ObjZoneImage();
	
	// Fonction de parametrage de l'etat d'affichage courant du widget
	//
	void EtatAffichage(EtatsAffichage etat);
	
	// Fonction de parametrage du chemin vers le fichier de l'image de lancement
	//
	void ImageLancement(QString chemin);
	
	// Fonction de parametrage du chemin vers le fichier de l'image courante
	//
	void ImageCourante(QString chemin);
	
	// Fonction de parametrage du chemin vers le fichier de l'image de diaporama courante
	//
	void ImageDiaporama(QString chemin);

	// Fonction de parametrage du chemin vers les documents a diffuser
	//
	void DocumentsDiffuses(QString chemin);

	// Fonction de parametrage du chemin et du nom du fichier de l'image diffusee sur le web en temps reel
	//
	void ImageDiffuseeWeb(QString fichier);

	// Fonction de parametrage de l'identifieur aleatoire du TerminOA
	//
	void IdentifieurTerminOA(QString id);
	
	// Fonction de parametrage du type d'echelle de l'histogramme
	//
	void TypeEchelleHistogramme(TypesEchelles type);
	
	// Fonction retournant le type d'echelle de l'histogramme
	//
	TypesEchelles TypeEchelleHistogramme(void);
	
	// Fonction de parametrage du type de palette utilisee
	//
	void TypePalette(TypesPalettes type);
	
	// Fonction retournant le type de palette utilisee
	//
	TypesPalettes TypePalette(void);
	
	// Fonction de parametrage de la consigne du seuil bas de visualisation
	//
	void ConsigneSeuilBas(long valeur);
	
	// Fonction retournant la consigne du seuil bas
	//
	long ConsigneSeuilBas(void);
	
	// Fonction retournant le seuil bas
	//
	long ValSeuilBas(void);
	
	// Fonction de parametrage de la consigned du seuil haut de visualisation
	//
	void ConsigneSeuilHaut(long valeur);
	
	// Fonction retournant la consigne du seuil haut
	//
	long ConsigneSeuilHaut(void);
	
	// Fonction retournant le seuil haut
	//
	long ValSeuilHaut(void);
	
	// Fonction de calcul de la LUT
	//
	void PuissanceLUT(double valeur);
	
	// Fonction retournant la valeur de la puissance de la LUT de visualisation
	//
	int PuissanceLUT(void);
	
	// Fonction de parametrage des seuils en fonction des consignes
	//
	void FixeSeuilsFnConsignes(void);
	
	// Fonction de parametrage de la zone affichee de l'image courante
	//
	void ZoneAfficheeIC(ListeZAPIC zone);
	
	// Fonction retournant la zone affichee de l'image courante
	//
	ListeZAPIC ZoneAfficheeIC(void);
	
	// Fonction de representation graphique de la zone image
	//
	void RepresenterZoneImage(void);
	
	// Fonction de chargement de l'image FITS courante
	//
	int ChargerFITSCourante(QString &CheminRepTerminal);
	
	// Fonction de calcul des donnees en intensite de l'image affichee avec les extremums optimises
	//
	void CalculerImageAffichee(void);
	
	// Fonction d'affichage de l'image
	//
	void AfficherImage(void);
	
	// Fonction de tracage de l'histogramme
	//
	void TracerHistogramme(void);
	
	// Fonction de calcul de l'indice du pinceau de la palette pour une valeur
	//
	int IndicePinceau(long v);

	// Sauvegarder le QPixmap de l'image courante au format JPEG
	//
	int SauvegarderPixmapIC_JPEG(QString nom_fichier);
};

#endif

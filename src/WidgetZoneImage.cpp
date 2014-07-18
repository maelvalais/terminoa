/* MODULE DU WIDGET DE LA ZONE IMAGE

   (C)David.Romeuf@univ-lyon1.fr 2006 by David Romeuf
*/

// Inclusions C++
//
#include <iostream>
#include <new>
#include <memory>
#include <valarray>
#include <cmath>

using namespace std;

// Inclusions Qt et KDE
//
#include <qapplication.h>
#include <qfontmetrics.h>
#include <qsize.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qbrush.h>
#include <qwmatrix.h>

// Inclusions CCfits
//
#include <CCfits>

using namespace CCfits;

// Inclusions module
//
#include "WidgetZoneImage.h"


// Constructeur de la classe ObjZoneImage
//
ObjZoneImage::ObjZoneImage(QWidget *parent,const char *name,WFlags f) : QWidget(parent,name,f)
{
	// Couleur de fond du widget
	//
	setPaletteBackgroundColor(QColor(Qt::black));
	
	// Etat d'affichage par defaut
	//
	EtatAffichage(AffichageImageLancement);
	
	// Initialisation des proprietes
	//
	CheminImageLancement="";
	CheminImageCourante="";
	CheminImageDiaporama="";
	CheminDocuments="";
	CheminFichImgWeb="";
	IdTerminOA="";
	
	ForceIC_NULL();
	ForceICAff_NULL();
	ForceHistogrammeIC_NULL();
	ICPixmap.setOptimization(QPixmap::BestOptim);
	
	SeuilBasConsigne=0;
	SeuilHautConsigne=0;
	SeuilBas=0;
	SeuilHaut=0;
	aSeuilBas=-1;
	aSeuilHaut=-1;
	aTPalette=PaletteNULL;
	aZoneAffichee=ZoneNULL;
	ICJamaisAffichee=true;
	
	TypeEchelleHistogramme(Logarithmique);
	TypePalette(NoirEtBlanc);
	PuissanceLUT(1.0);
	aPuiLUT=-1.0;
	ZoneAfficheeIC(Z0);
}


// Destructeur de la classe ObjZoneImage
//
ObjZoneImage::~ObjZoneImage()
{
}


// Force l'image courante a devenir NULL
//
void ObjZoneImage::ForceIC_NULL(void)
{
	DonneesIC.resize(0);
	DimXIC=0;
	DimYIC=0;
	ValMinIC=0;
	ValMaxIC=0;
	NbBitsDonneesIC=0;
	DateHeureIC="";
}


// Force l'image courante affichee a devenir NULL
//
void ObjZoneImage::ForceICAff_NULL(void)
{
	DonneesICAff.resize(0);
}


// Force l'histogramme de l'image courante a devenir NULL
//
void ObjZoneImage::ForceHistogrammeIC_NULL(void)
{
	HistogrammeIC.resize(0);
}


// Fonction de chargement de l'image FITS courante
//
// CE:	On passe une reference vers le chemin du repertoire du terminal
//
// CS:	La fonction est vraie si l'operation reussie, fausse dans le cas contraire
//
int ObjZoneImage::ChargerFITSCourante(QString &CheminRepTerminal)
{
	FITS::setVerboseMode(true);
	
	// Bloc de test de chargement
	//
	try
	{
		// Creation d'un objet FITS et lecture des mots cl� obligatoires de l'entete
		//
		// ImageCourante est un auto pointeur de la Standard Template Library C++ qui facilite la gestion memoire de l'objet
		//
		const string NomFichier=CheminRepTerminal+"/"+FICHIER_IMAGE_COURANTE;
		
		std::auto_ptr<FITS> ImageCourante(new FITS(NomFichier,Read,false));
		
		// Il s'agit d'un fichier simple sans extension avec un seul HDU (Header/Data Unit)
		//  donc on reference le premier HDU du fichier
		//
		PHDU &pHDUImageCourante=ImageCourante->pHDU();
	
		// Lecture de la cle DATE
		//
		string DateHeureImageCourante="";
		
		// Tentative de lecture du mot cle DATE
		//
		try
		{
			pHDUImageCourante.readKey("DATE",DateHeureImageCourante);
		}
		catch(FitsException& Exception)
		{
		}
		
		// Tentative de lecture du mot cle DATE-OBS si DATE n'est pas trouve
		//
		try
		{
			if( DateHeureImageCourante == "" ) pHDUImageCourante.readKey("DATE-OBS",DateHeureImageCourante);
		}
		catch(FitsException& Exception)
		{
		}
		
		// Quelques tests avant le chargement des donnees de l'image
		//
		if( pHDUImageCourante.axes() != 2 || !(pHDUImageCourante.bitpix() == 16 || pHDUImageCourante.bitpix() == 32) || DateHeureImageCourante == "" || (pHDUImageCourante.axis(0) != pHDUImageCourante.axis(1)) )
		{
			std::cerr << "TerminOA ERREUR: L'image courante n'est pas une image FITS supportee par TerminOA." << std::endl;
			
			std::cerr << "  BITPIX:" << pHDUImageCourante.bitpix() << " NAXIS:" << pHDUImageCourante.axes() <<  " NAXIS1:" << pHDUImageCourante.axis(0) << " NAXIS2:" << pHDUImageCourante.axis(1) << " DATE:" << DateHeureImageCourante << " BZERO:" << pHDUImageCourante.zero() << " BSCALE:" << pHDUImageCourante.scale() <<  std::endl;
			
			ForceIC_NULL();
			
			return FALSE;
		}
		
		// Lecture des donnees de l'image dans un tableau dynamique de long de la Standard Template Library C++
		//
		DonneesIC.resize(0);
		
		pHDUImageCourante.read(DonneesIC,1,pHDUImageCourante.axis(0)*pHDUImageCourante.axis(1));
		
		// Si le tableau n'a pas la bonne dimension alors tous les pixels ne sont pas charges
		//
		if( DonneesIC.size() != (unsigned long) pHDUImageCourante.axis(0)*pHDUImageCourante.axis(1) )
		{
			std::cerr << "TerminOA ERREUR: Tous les pixels de l'image courante ne peuvent etre charges." << std::endl;
			
			ForceIC_NULL();
			
			return FALSE;
		}
		
		// On passe en valeurs reelles vr=BZERO+v*BSCALE
		//
		DonneesIC*=(long) pHDUImageCourante.scale();
		DonneesIC+=(long) pHDUImageCourante.zero();
		
		// Initialisation des caracteristiques de l'image dans l'objet ZoneImage
		//
		DimXIC=pHDUImageCourante.axis(0);
		DimYIC=pHDUImageCourante.axis(1);
		DateHeureIC=DateHeureImageCourante;
		DateHeureIC=DateHeureIC.replace('T',' ');
		ValMinIC=DonneesIC.min();
		ValMaxIC=DonneesIC.max();
		
		// Si on a un minimum negatif
		//
		if( ValMinIC < 0 )
		{
			// On ajoute son offset pour devenir positif, nous preferrons travailler avec des fluxs positifs
			//
			DonneesIC+=-ValMinIC;
			
			ValMinIC=DonneesIC.min();
			ValMaxIC=DonneesIC.max();
		}
		else
		{
			// Si on peut supprimer l'offset des 15 bits alors on le supprime (cas des images en "unsigned short" codees BZERO=32768 BSCALE=1 par exemple)
			//
			if( pHDUImageCourante.zero() == 32768 && ValMinIC > 32768 )
			{
				DonneesIC-=32768;			// Suppression de l'offset des valeurs
			
				ValMinIC=DonneesIC.min();
				ValMaxIC=DonneesIC.max();
			}
		}
		
		// Calcul du nombre de bits necessaires pour coder l'image
		//
		CalculNbBitsDonneesIC();
		
		ForceHistogrammeIC_NULL();		// Nouvel histogramme
	}
	
	// Si le fichier n'est pas ouvrable et CCfits declanche une exception CantOpen
	//
	catch(CCfits::FITS::CantOpen)
	{
		std::cerr << "TerminOA ERREUR Exception CCfits: Ouverture impossible du fichier de l'image courante." << std::endl;
		
		ForceIC_NULL();
		
		return FALSE;
	}
	
	// Si une exception est declanchee par CCfits
	//
	catch(FitsException& Exception)
	{
		std::cerr << "TerminOA ERREUR: Exception CCfits: Erreur declanchee mais de nature non connue." << std::endl;
		
		ForceIC_NULL();
		
		return FALSE;
	}
	
	// L'image courante chargee n'a jamais ete affichee
	//
	ICJamaisAffichee=true;
	
	return TRUE;
}


// Fonction de parametrage de l'etat d'affichage courant du widget
//
void ObjZoneImage::EtatAffichage(EtatsAffichage etat)
{
	EtatAffichageCourant=etat;
}


// Fonction de parametrage de la zone affichee de l'image courante
//
// CE:	On passe la zone a afficher
//
void ObjZoneImage::ZoneAfficheeIC(ListeZAPIC zone)
{
	ZoneAffichee=zone;
}

// Fonction retournant la zone affichee de l'image courante
//
ListeZAPIC ObjZoneImage::ZoneAfficheeIC(void)
{
	return(ZoneAffichee);
}


// Fonction de parametrage du chemin vers le fichier de l'image de lancement
//
void ObjZoneImage::ImageLancement(QString chemin)
{
	CheminImageLancement=chemin;
}


// Fonction de parametrage du chemin vers le fichier de l'image courante
//
void ObjZoneImage::ImageCourante(QString chemin)
{
	CheminImageCourante=chemin;
}


// Fonction de parametrage du chemin vers le fichier de l'image de diaporama courante
//
void ObjZoneImage::ImageDiaporama(QString chemin)
{
	CheminImageDiaporama=chemin;
}


// Fonction de parametrage du chemin vers les documents a diffuser
//
void ObjZoneImage::DocumentsDiffuses(QString chemin)
{
	CheminDocuments=chemin;
}


// Fonction de parametrage du chemin et du nom du fichier de l'image diffusee sur le web en temps reel
//
void ObjZoneImage::ImageDiffuseeWeb(QString fichier)
{
	CheminFichImgWeb=fichier;
}


// Fonction de parametrage de l'identifieur aleatoire du TerminOA
//
void ObjZoneImage::IdentifieurTerminOA(QString id)
{
	IdTerminOA=id;
}


// Fonction de parametrage du type d'echelle de l'histogramme
//
void ObjZoneImage::TypeEchelleHistogramme(TypesEchelles type)
{
	TEchelleHistog=type;
}


// Fonction retournant le type d'echelle de l'histogramme
//
TypesEchelles ObjZoneImage::TypeEchelleHistogramme(void)
{
	return(TEchelleHistog);
}


// Fonction de parametrage du type de palette utilisee
//
void ObjZoneImage::TypePalette(TypesPalettes type)
{
	double liR,liV,liB;	// Limite inferieure en pourcentage du NB_ELEMENTS_PALETTE en dessous de laquelle la composante est nulle
	double lsR,lsV,lsB;	// Limite superieure en pourcentage du NB_ELEMENTS_PALETTE au dessus de laquelle la composante est maximale
	double lsRMliR;		// Valeur de l'intervale de la composante rouge
	double lsVMliV;		// Valeur de l'intervale de la composante verte
	double lsBMliB;		// Valeur de l'intervale de la composante bleue
	
	// Parametrage de la propriete de l'objet
	//
	TPalette=type;
	
	// Parametrage des fonctions de couleur
	//
	liR=liV=liB=lsR=lsV=lsB=(NB_ELEMENTS_PALETTE-1);
	
	switch( TPalette )
	{
		case NoirEtBlanc:
			liR=liV=liB=0;
			break;
			
		case Halpha:
			liR*=0;
			liV*=0.470;
			liB*=0.294;
			lsR*=0.666;
			lsV*=1.0;
			lsB*=1.0;
			break;
			
		case NoirOrangeBlanc:
			liR*=0;
			liV*=0.156;
			liB*=0.509;
			lsR*=0.59;
			lsV*=0.784;
			lsB*=1.0;
			break;
			
		case NoirVertBlanc:
			liR*=0.15;
			liV*=0;
			liB*=0.2;
			lsR*=0.98;
			lsV*=1.0;
			lsB*=0.95;
			break;
			
		case NoirBleuBlanc:
			liR*=0.2;
			liV*=0.15;
			liB*=0.0;
			lsR*=0.98;
			lsV*=0.95;
			lsB*=1.0;
			break;
			
		case PaletteNULL: break;
	}
	
	// Valeur des intervales
	//
	lsRMliR=lsR-liR;
	lsVMliV=lsV-liV;
	lsBMliB=lsB-liB;
	
	// Calcul des pinceaux de la palette
	//
	for( int i=0; i < NB_ELEMENTS_PALETTE; i++ )
	{
		double rouge;
		double vert;
		double bleu;
		
		// Calcul de la composante rouge
		//
		rouge=(double) i;
		
		if( rouge < liR )
		{
			rouge=0;
		}
		else
		{
			if( rouge > lsR ) rouge=MAX_ROUGE; else rouge=(rouge-liR)*MAX_ROUGE/lsRMliR;
		}
		
		// Calcul de la composante verte
		//
		vert=(double) i;
		
		if( vert < liV )
		{
			vert=0;
		}
		else
		{
			if( vert > lsV ) vert=MAX_VERT; else vert=(vert-liV)*MAX_VERT/lsVMliV;
		}
		
		// Calcul de la composante bleue
		//
		bleu=(double) i;
		
		if( bleu < liB )
		{
			bleu=0;
		}
		else
		{
			if( bleu > lsB ) bleu=MAX_BLEU; else bleu=(bleu-liB)*MAX_BLEU/lsBMliB;
		}
		
		PinceauxPalette[i].setColor(QColor((int) rouge,(int) vert,(int) bleu));
		PinceauxPalette[i].setStyle(Qt::SolidLine);
		PinceauxPalette[i].setWidth(0);
	}
}


// Fonction retournant le type de palette utilisee
//
TypesPalettes ObjZoneImage::TypePalette(void)
{
	return(TPalette);
}


// Fonction de calcul de la Look Up Table qui contient l'indice du numero de pinceau de la palette
//  en fonction de l'intensite rapportee au nombre possible
//
// CE:	On passe la valeur de la puissance x^valeur
//
// CS:	-
//
void ObjZoneImage::PuissanceLUT(double valeur)
{
	// Parametrage de la propriete de l'objet
	//
	PuiLUT=valeur;

	// Calcul du tableau de la LUT
	//	
	for( int i=0; i < NB_ELEMENTS_LUT; i++ )
	{
		double v=(double) i;					// Indice du numero de pinceau de la palette
		double fr;						// Partie fractionnaire
		
		v/=(NB_ELEMENTS_LUT-1.0);
		v=std::pow(v,PuiLUT)*(NB_ELEMENTS_PALETTE-1.0);
		
		fr=std::modf(v,&fr);
		
		if( fr > 0.5 ) v+=1.0;
		
		LUT[i]=(int) v;
	}
}


// Fonction retournant la valeur de la puissance de la LUT de visualisation
//
int ObjZoneImage::PuissanceLUT(void)
{
	double pe,fr;
	
	fr=std::modf(PuiLUT*100.0,&pe);
	
	if( fr > 0.5 ) pe+=1.0;
	
	return((int) pe);
}


// Fonction de parametrage de la consigne du seuil bas de visualisation
//
void ObjZoneImage::ConsigneSeuilBas(long valeur)
{
	SeuilBasConsigne=valeur;
}


// Fonction retournant la consigne du seuil bas de visualisation
//
long ObjZoneImage::ConsigneSeuilBas(void)
{
	return( SeuilBasConsigne );
}


// Fonction retournant le seuil bas de visualisation
//
long ObjZoneImage::ValSeuilBas(void)
{
	return( SeuilBas );
}


// Fonction de parametrage de la consigne du seuil haut de visualisation
//
void ObjZoneImage::ConsigneSeuilHaut(long valeur)
{
	SeuilHautConsigne=valeur;
}


// Fonction retournant la consigne du seuil haut de visualisation
//
long ObjZoneImage::ConsigneSeuilHaut(void)
{
	return( SeuilHautConsigne );
}


// Fonction retournant le seuil haut de visualisation
//
long ObjZoneImage::ValSeuilHaut(void)
{
	return( SeuilHaut );
}


// Fonction de parametrage des seuils en fonction des consignes
//
void ObjZoneImage::FixeSeuilsFnConsignes(void)
{
	if( SeuilBasConsigne >= ValMinIC && SeuilBasConsigne <= ValMaxIC ) SeuilBas=SeuilBasConsigne; else SeuilBas=ValMinIC;
	
	if( SeuilHautConsigne >= ValMinIC && SeuilHautConsigne <= ValMaxIC ) SeuilHaut=SeuilHautConsigne; else SeuilHaut=ValMaxIC;
}


// Fonction de calcul de l'indice du pinceau de la palette pour une valeur
//
// CE:	On passe la valeur de l'intensite du pixel ;
//
// CS:	La fonction retourne l'indice du pinceau dans la palette ;
//
int ObjZoneImage::IndicePinceau(long v)
{
	if( v <= SeuilBas )
	{
		return LUT[0];
	}
	else
	{
		if( v >= SeuilHaut )
		{
			return LUT[NB_ELEMENTS_LUT-1];
		}
		else
		{
			double i=((double) v)-((double) SeuilBas);
			i/=(double) (SeuilHaut-SeuilBas);
			i*=(NB_ELEMENTS_LUT-1);
			
			return LUT[(int) i];
		}
	}	
}


// Surcharge de la fonction de slot paintEvent heritee du QWidget
//
void ObjZoneImage::paintEvent(QPaintEvent *event)
{
	QPaintEvent *e;
	e=event;

	// Representation graphique de la zone image
	//
	if( DonneesIC.size() != 0 )
	{
		RepresenterZoneImage();
	}
}


// Surcharge de la fonction retournant la dimension recommandee du widget
//
QSize ObjZoneImage::sizeHint()
{
	QSize dimension_recommandee(TAILLE_X_OBJZONEIMAGE_RECOMMANDEE,TAILLE_Y_OBJZONEIMAGE_RECOMMANDEE);
	
	return dimension_recommandee;
}


// Fonction de calcul du nombre de bits necessaires pour representer les donnees de l'image courante
//
//	CE:	-
//
//	CS:	-

void ObjZoneImage::CalculNbBitsDonneesIC(void)
{
	// On recherche le nombre de bits necessaires pour stocker les donnees de l'image courante
	//
	for( NbBitsDonneesIC=8; NbBitsDonneesIC <= 32; NbBitsDonneesIC++ ) if( (1 << NbBitsDonneesIC) > ValMaxIC ) break;
}


// Fonction de calcul de l'histogramme de l'image courante
//
//	CE:	-
//
//	CS:	-

void ObjZoneImage::CalculerHistogrammeIC(void)
{
	// La dimension de l'histogramme est directement liee au nombre de pixels de la zone d'affichage
	//
	HistogrammeIC.resize(size().height());
	
	// Initialisation des membres du tableau a la valeur nulle
	//
	HistogrammeIC=0;
	
	// Intervale de valeur en ADU represente par un membre de l'histogramme
	//
	IntervaleMembreHistogramme=((double) (1 << NbBitsDonneesIC)) / ((double) size().height());
	
	// Calcul de l'histogramme de l'image courante
	//
	for( long i=0; i < DimXIC*DimYIC; i++ )
	{
		double n=((double) DonneesIC[i]) / IntervaleMembreHistogramme;
		
		HistogrammeIC[(long) n]++;
	}
}


// Fonction de representation graphique de la zone image
//	CE:	-
//
//	CS:	-

void ObjZoneImage::RepresenterZoneImage(void)
{
	QPainter Dessin;		// Un objet de dessin
	QPixmap Image;			// Un objet pixmap lie �la carte graphique
	
//std::cout << "Dimension :" << size().width() << " par " << size().height() << std::endl;

	// L'objet de dessin va directement dessiner sur ObjZoneImage qui herite de QWidget et donc de QPaintDevice
	//
	Dessin.begin(this);
	
	// Selon l'etat d'affichage de l'objet
	//
	switch( EtatAffichageCourant )
	{
		case AffichageImageLancement:
			// Creation de l'objet QPixmap et chargement d'un fichier
			//
			Image.load(CheminImageLancement);
			
			// Si le chargement n'a pas echoue
			//
			if( !Image.isNull() )
			{
				double ratioHZoneHPixmap;	// Ratio entre les dimensions de la zone image du widget et l'image Pixmap
				double ratioVZoneVPixmap;
				
				ratioHZoneHPixmap=((double) size().width()) / ((double) Image.size().width());
				ratioVZoneVPixmap=((double) size().height()) / ((double) Image.size().height());
				
				// Si l'une des dimensions de l'image Pixmap est superieure a la dimension de la zone image on l'adapte
				//
				if( ratioHZoneHPixmap < 1.0 || ratioVZoneVPixmap < 1.0 )
				{
					QWMatrix matrice;	// Matrice de transformation du QPixmap original
					QPixmap ImageND;	// QPixmap de nouvelles dimensions
					
					matrice.setTransformationMode(QWMatrix::Areas);
					
					if( ratioHZoneHPixmap < ratioVZoneVPixmap )
					{
						// Si la plus grande difference est horizontale
						//
						matrice.scale(ratioHZoneHPixmap,ratioHZoneHPixmap);
					}
					else
					{
						// Si la plus grande difference est verticale
						//
						matrice.scale(ratioVZoneVPixmap,ratioVZoneVPixmap);
					}
					
					// Transformation du QPixmap original
					//
					ImageND=Image.xForm(matrice);
					
					// Affichage centre du QPixmap modifie
					//
					Dessin.drawPixmap((size().width()-ImageND.size().width())/2,(size().height()-ImageND.size().height())/2,ImageND);
				}
				else
				{
					// Affichage centre du QPixmap original
					//
					Dessin.drawPixmap((size().width()-Image.size().width())/2,(size().height()-Image.size().height())/2,Image);
				}
			}
			
			break;
			
		case AffichageImageCourante:
			// On decide des seuils
			//
			if( SeuilBas == 0 && SeuilHaut == 0  && SeuilBasConsigne == 0 && SeuilHautConsigne == 0 )
			{
				// Aucune consigne
				//
				SeuilBasConsigne=SeuilBas=ValMinIC;
				SeuilHautConsigne=SeuilHaut=ValMaxIC;
			}
			
			// On utilise les consignes si elles sont applicables
			//
			FixeSeuilsFnConsignes();
			
			// On affiche l'histogramme si la forme de la zone le permet
			//
			if( size().width() > size().height() ) TracerHistogramme();
			
			// On affiche l'image
			//
			AfficherImage();

			{
				// Sauvegarder la representation courante dans le repertoire des documents diffuses
				//
				QString NomFichier=CheminDocuments+"/ImageCouranteCLIMSO-PicDuMidi-";

				switch( ZoneAffichee )
				{
					case Z0:	NomFichier+="Z0"; break;
					case Z1:	NomFichier+="Z1"; break;
					case Z2:	NomFichier+="Z2"; break;
					case Z3:	NomFichier+="Z3"; break;
					case Z4:	NomFichier+="Z4"; break;
					case Z5:	NomFichier+="Z5"; break;
					case Z6:	NomFichier+="Z6"; break;
					case Z7:	NomFichier+="Z7"; break;
					case Z8:	NomFichier+="Z8"; break;
					case Z9:	NomFichier+="Z9"; break;
					default:
						break;
				}

				NomFichier+="-Borne-"+IdTerminOA+".jpg";

				if( ZoneAffichee == Z0 )
				{
					// Dans le cas de la zone Z0, on sauvegardera l'image complete de la fenetre
					//
					QPixmap FenetreComplete=QPixmap::grabWindow(winId(),0,0,-1,-1);

					if( !FenetreComplete.save(NomFichier,"JPEG",TAUX_COMPRESSION_IMAGES_JPEG_DIFFUSEES) )
					{
						std::cerr << "TerminOA: ERREUR: Impossible de sauvegarder l'image "+NomFichier << std::endl;
					}

					// On enregistre aussi sous la forme du fichier diffuse sur le web en temps reel
					//
					if( !FenetreComplete.save(CheminFichImgWeb,"JPEG",TAUX_COMPRESSION_IMAGES_JPEG_DIFFUSEES) )
					{
						std::cerr << "TerminOA: ERREUR: Impossible de sauvegarder l'image "+CheminFichImgWeb << std::endl;
					}
				}
				else
				{
					if( !ICPixmap.save(NomFichier,"JPEG",TAUX_COMPRESSION_IMAGES_JPEG_DIFFUSEES) )
					{
						std::cerr << "TerminOA: ERREUR: Impossible de sauvegarder l'image "+NomFichier << std::endl;
					}
				}
			}
			break;
			
		case AffichageImageDiaporama:
			// Creation de l'objet QPixmap et chargement d'un fichier
			//
			Image.load(CheminImageDiaporama);
			
			// Le crayon est blanc
			//
			Dessin.setPen(QPen(Qt::white));
	
			// La brosse de remplissage est noire
			//
			Dessin.setBrush(QBrush(Qt::black));
			
			// Si le chargement n'a pas echoue
			//
			if( !Image.isNull() )
			{
				double ratioHZoneHPixmap;	// Ratio entre les dimensions de la zone image du widget et l'image Pixmap
				double ratioVZoneVPixmap;
				
				ratioHZoneHPixmap=((double) size().width()) / ((double) Image.size().width());
				ratioVZoneVPixmap=((double) size().height()) / ((double) Image.size().height());
				
				// Si l'une des dimensions de l'image Pixmap est superieure a la dimension de la zone image on l'adapte
				//
				if( ratioHZoneHPixmap < 1.0 || ratioVZoneVPixmap < 1.0 )
				{
					//
					// Dans cette branche l'image chargee a au moins un des cotes plus grand que l'affichage, il faut la diminuer
					//

					QWMatrix matrice;	// Matrice de transformation du QPixmap original
					QPixmap ImageND;	// QPixmap de nouvelles dimensions
					
					matrice.setTransformationMode(QWMatrix::Areas);
					
					if( ratioHZoneHPixmap < ratioVZoneVPixmap )
					{
						// Si la plus grande difference est horizontale
						//
						matrice.scale(ratioHZoneHPixmap,ratioHZoneHPixmap);
					}
					else
					{
						// Si la plus grande difference est verticale
						//
						matrice.scale(ratioVZoneVPixmap,ratioVZoneVPixmap);
					}
					
					// Transformation du QPixmap original
					//
					ImageND=Image.xForm(matrice);
					
					// Le fond de la zone image sera noir
					//
					Dessin.fillRect(QRect(Dessin.window()),QBrush(Qt::black));
					
					// Affichage centre du QPixmap modifie
					//
					Dessin.drawPixmap((size().width()-ImageND.size().width())/2,(size().height()-ImageND.size().height())/2,ImageND);
				}
				else
				{
					//
					// Dans cette branche l'image chargee est plus petite que l'image a afficher
					//

					QWMatrix matrice;	// Matrice de transformation du QPixmap original
					QPixmap ImageND;	// QPixmap de nouvelles dimensions
					
					matrice.setTransformationMode(QWMatrix::Areas);
					
					if( ratioHZoneHPixmap < ratioVZoneVPixmap )
					{
						// Si la plus grande difference est horizontale
						//
						matrice.scale(ratioHZoneHPixmap,ratioHZoneHPixmap);
					}
					else
					{
						// Si la plus grande difference est verticale
						//
						matrice.scale(ratioVZoneVPixmap,ratioVZoneVPixmap);
					}
					
					// Transformation du QPixmap original
					//
					ImageND=Image.xForm(matrice);
					
					// Le fond de la zone image sera noir
					//
					Dessin.fillRect(QRect(Dessin.window()),QBrush(Qt::black));
					
					// Affichage centre du QPixmap original
					//
					Dessin.drawPixmap((size().width()-ImageND.size().width())/2,(size().height()-ImageND.size().height())/2,ImageND);
				}
			}
			break;
			
		default:
			std::cerr << "ObjZoneImage: ERREUR: Cas d'enumeration d'etat d'affichage impossible pour cet objet." << std::endl;
			break;
	}
}


// Fonction de tracage de l'histogramme
//
void ObjZoneImage::TracerHistogramme(void)
{
	QPainter Dessin(this);					// Un objet de dessin sur ce widget
	QFontMetrics InfosFontCourante=Dessin.fontMetrics();	// Mesures des fonts courantes dans le QPainter
	QPen PinceauBlanc(Qt::white);				// Un pinceau blanc
	QPen PinceauNoir(Qt::black);				// Un pinceau noir
	QPen PinceauGradInt(QColor(128,128,128));		// Un pinceau pour la graduation de l'axe de l'intensite
	QPen PinceauGradPop(QColor(50,50,50));			// Un pinceau pour la graduation de l'axe de la population
	QString Chaine;						// Chaine de composition
	int txzoneHistog=size().width()-size().height();	// Dimension de la zone d'affichage de l'histogramme + commentaires
	int txhistog=txzoneHistog-InfosFontCourante.height();	// Dimension en pixels de la zone reservee a l'histogramme
	double coef=0;						// Coefficient de conversion max histogramme -> nb pixels
	
	PinceauBlanc.setStyle(Qt::SolidLine);
	PinceauBlanc.setWidth(0);
	PinceauNoir.setStyle(Qt::SolidLine);
	PinceauNoir.setWidth(0);
	PinceauGradInt.setStyle(Qt::SolidLine);
	PinceauGradInt.setWidth(0);
	PinceauGradPop.setStyle(Qt::SolidLine);
	PinceauGradPop.setWidth(0);
	
	// On efface la zone de l'histogramme
	//
	Dessin.fillRect(size().width()-txzoneHistog,0,txzoneHistog,size().height(),QBrush(Qt::black));
	
	// On affiche les commentaires
	//
	Dessin.setPen(PinceauBlanc);
	
	Chaine=DateHeureIC+QString(" - (%1,%2)^%3").arg(ValSeuilBas()).arg(ValSeuilHaut()).arg(((double) PuissanceLUT())/100.0,4,'f',2);
	
	Dessin.save();
	Dessin.translate(size().width()-txzoneHistog+3,5);
	Dessin.rotate(90);
	Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
	Dessin.restore();

	if( 2*InfosFontCourante.width(Chaine) < size().height() )
	{
		Dessin.save();
		Dessin.translate(size().width()-txzoneHistog+InfosFontCourante.height()-4,size().height()-5);
		Dessin.rotate(-90);
		Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
		Dessin.restore();
	}
	
	// Calcul de l'histogramme si besoin
	//
	if( HistogrammeIC.size() != (unsigned long) size().height() ) CalculerHistogrammeIC();
	
	// On ne trace pas l'histogramme si il n'y a pas au moins x pixels disponibles
	//
	if( txhistog < 20 ) return ;
	
	// Coefficient de conversion valeur histogramme vers nombre de pixels
	//
	switch( TEchelleHistog )
	{
		case Lineaire:
			if( HistogrammeIC.max() != 0 ) coef=((double) txhistog)/((double) HistogrammeIC.max()); else coef=0.0;
			break;
			
		case Logarithmique:
			coef=((double) txhistog)/std::log(((double) HistogrammeIC.max()+1));
			break;
	}
	
	// Les intervales pour tracer
	//
	int IntervaleGradInt=(int) HistogrammeIC.size()/7;	// Intervale des graduations sur l'intensite
	
	long MaxPop=HistogrammeIC.max();			// Population maximale de la graduation
	long EchPop=10;
	while( EchPop < MaxPop ) EchPop*=10;			// Recherche de la decade au dessus de MaxPop
	double InterEchPop=((double) EchPop)/100.0;		// La decade que l'on va tracer
	
	// On trace la graduation de la population de l'histogramme
	//
	int Compteur=0;						// Compteur pour affichage valeur
	Dessin.setPen(PinceauGradPop);
	
	for( double i=0; i < (double) MaxPop; i+=InterEchPop )
	{
		double v=0;		// Variable de travail
		
		switch( TEchelleHistog )
		{
			case Lineaire:
				v=i*coef;
				break;
				
			case Logarithmique:
				v=std::log(i+1.0)*coef;
				break;
		}
		
		int x=size().width()-1-((int) v);	// Abscisse de la population correspondante
		
		Dessin.drawLine(x,0,x,size().height());
		
		switch( TEchelleHistog )
		{
			case Lineaire:
				if( Compteur == 0 )
				{
					Chaine=QString("%1").arg((long) i);
					
					Dessin.save();
					Dessin.translate(x-1,IntervaleGradInt/2+InfosFontCourante.width(Chaine)/2);
					Dessin.rotate(-90);
					Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
					Dessin.restore();
					
					Dessin.save();
					Dessin.translate(x-1,size().height()-3*IntervaleGradInt-IntervaleGradInt/2+InfosFontCourante.width(Chaine)/2);
					Dessin.rotate(-90);
					Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
					Dessin.restore();
					
					Dessin.save();
					Dessin.translate(x-1,size().height()-IntervaleGradInt/2+InfosFontCourante.width(Chaine)/2);
					Dessin.rotate(-90);
					Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
					Dessin.restore();
					
					Compteur=1;
				}
				else
				{
					Compteur=0;
				}
				break;
				
			case Logarithmique:
				if( Compteur < 3 )
				{
					Chaine=QString("%1").arg((long) i);
					
					Dessin.save();
					Dessin.translate(x+InfosFontCourante.height()-2,IntervaleGradInt/2+InfosFontCourante.width(Chaine)/2);
					Dessin.rotate(-90);
					Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
					Dessin.restore();
					
					Dessin.save();
					Dessin.translate(x+InfosFontCourante.height()-2,size().height()-3*IntervaleGradInt-IntervaleGradInt/2+InfosFontCourante.width(Chaine)/2);
					Dessin.rotate(-90);
					Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
					Dessin.restore();
					
					Dessin.save();
					Dessin.translate(x+InfosFontCourante.height()-2,size().height()-IntervaleGradInt/2+InfosFontCourante.width(Chaine)/2);
					Dessin.rotate(-90);
					Dessin.drawText(0,0,Chaine,-1,QPainter::LTR);
					Dessin.restore();
					
					Compteur++;
				}
				break;
		}
	}
	
	// On trace la graduation de l'intensite de l'histogramme (le fond)
	//
	Dessin.setPen(PinceauGradInt);
	
	for( int i=0; i < (int) HistogrammeIC.size()-IntervaleGradInt/2; i+=IntervaleGradInt )
	{
		int y=size().height()-1-i;		// Ordonnee de l'intervale de l'histogramme
		
		double valeur=((double) i)*IntervaleMembreHistogramme;
			
		Dessin.drawLine(size().width()-txhistog,y,size().width(),y);
		
		Chaine=QString("%1").arg((long) valeur);
		
		Dessin.drawText(size().width()-txhistog+2,y-2,Chaine,-1,QPainter::LTR);
	}
	Chaine=QString("%1").arg((long) (1 << NbBitsDonneesIC)-1);
	Dessin.drawText(size().width()-txhistog+2,InfosFontCourante.height(),Chaine,-1,QPainter::LTR);
	
	// On trace selon les parametres configures
	//
	int ax=-1;
	int ay=-1;
	
	for( int i=0; i < (int) HistogrammeIC.size(); i++ )
	{
		double v=0;		// Variable de travail
		
		switch( TEchelleHistog )
		{
			case Lineaire:
				v=((double) HistogrammeIC[i])*coef;
				break;
				
			case Logarithmique:
				v=std::log(((double) HistogrammeIC[i]+1))*coef;
				break;
		}
		
		int x=size().width()-1-((int) v);
		int y=size().height()-1-i;
		
		// On trace l'intensite courante
		//
		v=(double) i;
		v*=IntervaleMembreHistogramme;
		
		Dessin.setPen(PinceauxPalette[IndicePinceau((long) v)]);
		Dessin.drawLine(x,y,size().width(),y);
		
		// Si il s'agit du premier point trace
		//
		Dessin.setPen(PinceauBlanc);			// Le crayon est blanc
		
		if( ax == -1 )
		{
			Dessin.drawPoint(x,y);
		}
		else
		{
			// On trace de l'ancien au nouveau
			//
			Dessin.drawLine(ax,ay,x,y);
		}
		
		// L'ancien point sera le courant
		//
		ax=x;
		ay=y;
	}
	
	Dessin.end();
}


// Fonction de calcul des donnees en intensite de l'image affichee avec les extremums optimises
//
void ObjZoneImage::CalculerImageAffichee(void)
{
	QPen Pinceau(QColor(15,15,25));		// Un pinceau pour la couleur de progression du calcul
	Pinceau.setStyle(Qt::SolidLine);
	Pinceau.setWidth(0);
	
	QPainter Dessin(this);				// Un objet de dessin sur ce widget
	Dessin.setPen(Pinceau);				// Le crayon utilise pour la progression du calcul
	
	// Initialisation des variables pour la transformation
	//
	double x0s;				// Coordonnees et dimension du carre correspondant dans les donnees source de l'image
	double y0s;
	double txs;
	double tys;
	double x0d=0;				// Coordonnees et dimension du carre affiche dans le widget
	double y0d=0;
	long hauteur_zone=size().height();	// Hauteur du widget de la zone image
	double txd=(double) hauteur_zone;
	double tyd=(double) hauteur_zone;
	double CoefSversD;			// Coefficient de transformation de source vers destination
	double x;				// Variable de travail
	double y;
	double fr;				// Partie fractionnaire 
	
	// Abscisse origine dans l'image source
	//
	switch( ZoneAffichee )
	{
		case Z0:
		case Z1:
		case Z4:
		case Z7:
			x0s=0.0;
			break;
			
		case Z2:
		case Z5:
		case Z8:
			x0s=((double) DimXIC)/3.0;
			break;
	
		case Z3:
		case Z6:
		case Z9:
			x0s=2.0*((double) DimXIC)/3.0;
			break;
			
		case ZoneNULL:	break;
	}
	
	// Ordonnee origine dans l'image source
	//
	txs=tys=((double) DimYIC)/3.0;
	
	switch( ZoneAffichee )
	{
		case Z0:
			y0s=0;
			txs=tys=(double) DimYIC;
			break;
			
		case Z1:
		case Z2:
		case Z3:
			y0s=2.0*((double) DimYIC)/3.0;
			break;
			
		case Z4:
		case Z5:
		case Z6:
			y0s=((double) DimYIC)/3.0;
			break;
	
		case Z7:
		case Z8:
		case Z9:
			y0s=0;
			break;
			
		case ZoneNULL:	break;
	}
	
	// On commence toujours sur un pixel entier
	//
	fr=std::modf(x0s,&x0s);
	fr=std::modf(y0s,&y0s);
	
	// La dimension est toujours entiere
	//
	fr=std::modf(txs,&txs);
	tys=txs;
	
	// Coefficient de transformation de Source vers Destination (Affichee)
	//
	CoefSversD=tyd/tys;
	
	// L'image affichee sera toujours carree avec la hauteur du widget pour dimension
	//
	DonneesICAff.resize(hauteur_zone*hauteur_zone);
	
	// Tous les pixels sont initialises a une valeur negative pour pouvoir realiser le test de l'optimisation
	//
	DonneesICAff=-1;
	
//std::cout << "s:" << x0s << " " << y0s << " " << txs << " " << tys << std::endl;
//std::cout << "d:" << x0d << " " << y0d << " " << txd << " " << tyd << std::endl;
	
	// Si il y a plus de pixels dans l'image source que dans l'image affichee de destination
	//
	if( CoefSversD < 1.0 )
	{
		// Pour chaque pixel de l'image source on calcul le pixel de l'image affichee de destination
		//
		for( y=y0s; y < y0s+tys; y+=1.0 )
		{
			long offsetlignes=(long) (y*DimXIC);
			double yd;		// Ordonnee de (y) dans l'image destination
			
			yd=y0d+(y-y0s)*CoefSversD;
			std::modf(yd,&yd);
			
			// yd indique directement la progression du calcul
			//
			Dessin.drawLine(0,hauteur_zone-1-((int) yd),hauteur_zone-1,hauteur_zone-1-((int) yd));
			
			for( x=x0s; x < x0s+txs; x+=1.0 )
			{
				double xd;	// Abscisse de (x) dans l'image destination
				long indiced;	// Offset dans les tableaux de donnees
				long indices;
				
				// Coordonnees de (x,y) dans l'image destination affichee
				//
				xd=x0d+(x-x0s)*CoefSversD;
				std::modf(xd,&xd);
				
				// Indice du pixel dans le tableau de donnees de l'image source
				//
				indices=offsetlignes+(long) x;
				
				// Indice du pixel dans le tableau de donnees de l'image destination affichee
				//
				if( xd >= txd )
				{
					xd=txd-1;
					std::cout << "ObjZoneImage::CalculerImageAffichee(): xd >= txd." << std::endl;
				}
				if( yd >= tyd )
				{
					yd=tyd-1;
					std::cout << "ObjZoneImage::CalculerImageAffichee(): yd >= tyd." << std::endl;
				}
				indiced=(long) (xd+yd*txd);
				
				// Si le pixel n'a jamais ete calcule
				//
				if( DonneesICAff[indiced] < 0 )
				{
					DonneesICAff[indiced]=DonneesIC[indices];
				}
				else
				{
					if( DonneesIC[indices] < DonneesICAff[indiced] ) DonneesICAff[indiced]=DonneesIC[indices];
					if( DonneesIC[indices] > DonneesICAff[indiced] ) DonneesICAff[indiced]=DonneesIC[indices];
				}
			}
		}
	}
	else
	{
		// Pour chaque pixel de l'image destination affichee on calcul le pixel interpole dans l'image source
		//
		for( y=y0d; y < y0d+tyd; y+=1.0 )
		{
			long offsetligned=(long) (y*txd);
			
			// y indique directement la progression du calcul
			//
			Dessin.drawLine(0,hauteur_zone-1-((int) y),hauteur_zone-1,hauteur_zone-1-((int) y));
			
			for( x=x0d; x < x0d+txd; x+=1.0 )
			{
				double xs;	// Coordonnees de (x,y) dans l'image source
				double ys;
				long indiced;	// Offset dans les tableaux de donnees
				long i;		// Intensite du pixel par interpolation bi-lineaire
				
				// Coordonnees de (x,y) dans l'image source
				//
				xs=x0s+(x-x0d)/CoefSversD;
				ys=y0s+(y-y0d)/CoefSversD;
				
				// Intensite du pixel par interpolation
				//
				i=(long) InterpolationBiLineaire(xs,ys);
				
				// Indice du pixel dans le tableau de donnees de l'image destination affichee
				//
				indiced=offsetligned+(long) x;
				
				// Si le pixel n'a jamais ete calcule
				//
				if( DonneesICAff[indiced] < 0 )
				{
					DonneesICAff[indiced]=i;
				}
				else
				{
					if( i < DonneesICAff[indiced] ) DonneesICAff[indiced]=i;
					if( i > DonneesICAff[indiced] ) DonneesICAff[indiced]=i;
				}
			}
		}
	}
}


// Fonction d'interpolation bi-lineaire dans l'image source courante
//

double ObjZoneImage::InterpolationBiLineaire(double x,double y)
{
	double t;		// Constante pour le calcul
	double u;
	double un_m_t;
	double un_m_u;
	double px;		// Partie entiere de x
	double py;		// Partie entiere de y
	double valeur;		// Valeur interpolee
	long ptp;		// Offset sur pixel haut-gauche
	
	// Test si les coordonnees sont sur l'image
	//
	if( x < 0.0 || x >= ((double) DimXIC) || y < 0.0 || y >= ((double) DimYIC) )
	{
		return 0.0;
	}
	
	t=std::modf(x,&px);	// Partie fractionnaire
	u=std::modf(y,&py);
	
	un_m_t=1.0-t;		// 1-t pour vitesse
	un_m_u=1.0-u;		// 1-u pour vitesse
	
	// Cas ou x == dimxi-1
	//
	if( ((long) px) == (DimXIC-1) )
	{
		px-=1.0;
		t=1.0;
		un_m_t=0.0;
	}
	
	// Cas ou y == dimyi-1
	//
	if( ((long) py) == (DimYIC-1) )
	{
		py-=1.0;
		u=1.0;
		un_m_u=0.0;
	}
	
	// Offset sur pixel en haut a gauche du carre a interpoler
	//
	ptp=((long) py)*DimXIC;
	ptp+=(long) px;
	
	// Interpolation Bi-Lineaire
	//
	valeur=((double) DonneesIC[ptp])*un_m_t*un_m_u;
	valeur+=((double) DonneesIC[ptp+1])*t*un_m_u;
	valeur+=((double) DonneesIC[ptp+1+DimXIC])*t*u;
	valeur+=((double) DonneesIC[ptp+DimXIC])*un_m_t*u;
	
	return valeur;
}


// Fonction d'affichage de l'image
//
void ObjZoneImage::AfficherImage(void)
{
	int tracer=false;
	long hauteur_zone=size().height();	// Hauteur du widget de la zone image
	
	// Si on doit recalculer les donnees de l'image affichee avec l'algorithme du contraste optimise
	//
	if( (DonneesICAff.size() != (unsigned long)(hauteur_zone*hauteur_zone)) || (aZoneAffichee != ZoneAffichee) || ICJamaisAffichee )
	{
		CalculerImageAffichee();
		
		tracer=true;
	}
	
	// Si on a besoin de reconstruire/recalculer le Pixmap de l'image courante en dehors de l'ecran
	//
	if( ICPixmap.width() != hauteur_zone )
	{
		// On redimensionne le QPixmap a la dimension de la zone image
		//
		ICPixmap.resize(hauteur_zone,hauteur_zone);
		
		tracer=true;
	}
	
	if( ICJamaisAffichee ) tracer=true;
	
	// Si on doit tracer dans le QPixmap de l'image courante pour une raison
	//
	if( tracer || (aSeuilBas != SeuilBas) || (aSeuilHaut != SeuilHaut) || (aPuiLUT != PuiLUT) || (aTPalette != TPalette) || (aZoneAffichee != ZoneAffichee) )
	{
		double CoefPente;	// Coefficient de transformation valeur vers indice dans la LUT
		
		// Un objet de dessin sur le QPixmap
		//
		QPainter DessinICP(&ICPixmap);
		
		// Calcul du coefficient de transformation de l'intensite du pixel vers l'indice dans la LUT
		//
		CoefPente=(NB_ELEMENTS_LUT-1)/((double) (SeuilHaut-SeuilBas));
		
		// On trace chaque pixel dans le Pixmap hors ecran
		//
		long ypp=hauteur_zone-1;
		
		for( long y=0; y < (long) hauteur_zone; y++ )
		{
			long offsetligne=y*hauteur_zone;
			
			for( long x=0; x < (long) hauteur_zone; x++ )
			{
				long v=DonneesICAff[x+offsetligne];	// Valeur du pixel a afficher
				
				if( v < SeuilBas )
				{
					// Pixel avec le pinceau du premier indice de la LUT
					//
					DessinICP.setPen(PinceauxPalette[LUT[0]]);
					DessinICP.drawPoint(x,ypp-y);
				}
				else
				{
					if( v > SeuilHaut )
					{
						// Pixel avec le pinceau du dernier indice de la LUT
						//
						DessinICP.setPen(PinceauxPalette[LUT[NB_ELEMENTS_LUT-1]]);
						DessinICP.drawPoint(x,ypp-y);
					}
					else
					{
						// Indice dans la LUT correspondante
						//
						double indiceLUT=(double) v;
						indiceLUT-=(double) SeuilBas;
						indiceLUT*=CoefPente;
						
						// Pixel avec le pinceau de l'indice de la LUT
						//
						DessinICP.setPen(PinceauxPalette[LUT[(long) indiceLUT]]);
						DessinICP.drawPoint(x,ypp-y);
					}
				}
			}
		}
		
		// Sauvegarde des parametres de representation
		//
		aSeuilBas=SeuilBas;
		aSeuilHaut=SeuilHaut;
		aPuiLUT=PuiLUT;
		aTPalette=TPalette;
		aZoneAffichee=ZoneAffichee;
		
		// On a terminer de dessiner
		//
		DessinICP.end();
	}
	
	// On affiche directement du QPixmap vers l'objet ObjZoneImage sur l'ecran (QWidget) (ils heritent tous les deux d'un QPaintDevice)
	//
	bitBlt(this,0,0,&ICPixmap,0,0,hauteur_zone,hauteur_zone,Qt::CopyROP,FALSE);
	
	// L'image courante chargee vient d'etre affichee au moins une fois
	//
	ICJamaisAffichee=false;
}


// Sauvegarder le QPixmap de l'image courante au format JPEG
//
// CE:	On passe le nom et chemin complet du fichier de sauvegarde ;
//
// CS:	La fonction est vraie si la sauvegarde s'est bien deroulee ;
//
int ObjZoneImage::SauvegarderPixmapIC_JPEG(QString nom_fichier)
{
	return ICPixmap.save(nom_fichier,"JPEG",90);
}

